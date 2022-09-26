//
// Copyright 2022 NanoMQ Team, Inc. <jaylin@emqx.io>
//
// This software is supplied under the terms of the MIT License, a
// copy of which should be located in the distribution where this
// file was obtained (LICENSE.txt).  A copy of the license may also be
// found online at https://opensource.org/licenses/MIT.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "include/webhook_inproc.h"
#include "nanomq.h"
#include "nng/nng.h"
#include "nng/protocol/pipeline0/pull.h"
#include "nng/protocol/pipeline0/push.h"
#include "nng/supplemental/http/http.h"
#include "nng/supplemental/nanolib/conf.h"
#include "nng/supplemental/util/platform.h"
#include "nng/supplemental/nanolib/log.h"

#define NANO_LMQ_INIT_CAP 16

// The server keeps a list of work items, sorted by expiration time,
// so that we can use this to set the timeout to the correct value for
// use in poll.
struct hook_work {
	enum { HOOK_INIT, HOOK_RECV, HOOK_WAIT, HOOK_SEND } state;
	nng_aio *      aio;
	nng_msg *      msg;
	nng_thread *   thread;
	nng_mtx *      mtx;
	nng_lmq *      lmq;
	nng_socket     sock;
	conf_web_hook *conf;
	uint32_t       id;
	bool           busy;
};

static void webhook_cb(void *arg);

static nng_thread *inproc_thr;

static void
fatal(uint32_t id, const char *func, int rv)
{
	fprintf(stderr, "[%d] %s: %s\n", id, func, nng_strerror(rv));
	// exit(1);
}

static void
send_msg(conf_web_hook *conf, nng_msg *msg)
{
	nng_http_client *client = NULL;
	nng_http_conn *  conn   = NULL;
	nng_url *        url    = NULL;
	nng_aio *        aio    = NULL;
	nng_http_req *   req    = NULL;
	nng_http_res *   res    = NULL;
	int              rv;

	if (((rv = nng_url_parse(&url, conf->url)) != 0) ||
	    ((rv = nng_http_client_alloc(&client, url)) != 0) ||
	    ((rv = nng_http_req_alloc(&req, url)) != 0) ||
	    ((rv = nng_http_res_alloc(&res)) != 0) ||
	    ((rv = nng_aio_alloc(&aio, NULL, NULL)) != 0)) {
		log_error("init failed: %s\n", nng_strerror(rv));
		goto out;
	}

	// Start connection process...
	nng_aio_set_timeout(aio, 1000);
	nng_http_client_connect(client, aio);

	// Wait for it to finish.
	// TODO It could cause some problems.
	nng_aio_wait(aio);
	if ((rv = nng_aio_result(aio)) != 0) {
		log_error("Connect failed: %s", nng_strerror(rv));
		nng_aio_finish_sync(aio, rv);
		goto out;
	}

	// Get the connection, at the 0th output.
	conn = nng_aio_get_output(aio, 0);

	// Request is already set up with URL, and for GET via HTTP/1.1.
	// The Host: header is already set up too.
	// set_data(req, conf_req, params);
	// Send the request, and wait for that to finish.
	for (size_t i = 0; i < conf->header_count; i++) {
		nng_http_req_add_header(
		    req, conf->headers[i]->key, conf->headers[i]->value);
	}

	nng_http_req_set_method(req, "POST");
	nng_http_req_set_data(req, nng_msg_body(msg), nng_msg_len(msg));
	nng_http_conn_write_req(conn, req, aio);
	nng_aio_set_timeout(aio, 1000);
	// TODO It could cause some problems.
	nng_aio_wait(aio);

	if ((rv = nng_aio_result(aio)) != 0) {
		log_error("Write req failed: %s", nng_strerror(rv));
		nng_aio_finish_sync(aio, rv);
		goto out;
	}

out:
	if (url) {
		nng_url_free(url);
	}
	if (conn) {
		nng_http_conn_close(conn);
	}
	if (client) {
		nng_http_client_free(client);
	}
	if (req) {
		nng_http_req_free(req);
	}
	if (res) {
		nng_http_res_free(res);
	}
	if (aio) {
		nng_aio_free(aio);
	}
}

static void
thread_cb(void *arg)
{
	struct hook_work *w   = arg;
	nng_lmq *         lmq = w->lmq;
	nng_msg *         msg = NULL;
	int               rv;
	while (true) {
		if (!nng_lmq_empty(lmq)) {
			nng_mtx_lock(w->mtx);
			rv = nng_lmq_get(lmq, &msg);
			nng_mtx_unlock(w->mtx);
			if (0 == rv) {
				send_msg(w->conf, msg);
				nng_msg_free(msg);
			}
		} else {
			// try to reduce lmq cap
			size_t lmq_len = nng_lmq_len(w->lmq);
			if (lmq_len > (NANO_LMQ_INIT_CAP * 2)) {
				size_t lmq_cap = nng_lmq_cap(w->lmq);
				if (lmq_cap > (lmq_len * 2)) {
					nng_mtx_lock(w->mtx);
					nng_lmq_resize(w->lmq, lmq_cap / 2);
					nng_mtx_unlock(w->mtx);
				}
			}
			nng_msleep(10);
		}
	}
}

static void
webhook_cb(void *arg)
{
	struct hook_work *work = arg;
	int               rv;
	switch (work->state) {
	case HOOK_INIT:
		work->state = HOOK_RECV;
		nng_recv_aio(work->sock, work->aio);
		break;

	case HOOK_RECV:
		if ((rv = nng_aio_result(work->aio)) != 0) {
			fatal(work->id, "nng_recv_aio", rv);
		}
		work->msg = nng_aio_get_msg(work->aio);
		nng_mtx_lock(work->mtx);
		if (nng_lmq_full(work->lmq)) {
			size_t lmq_cap = nng_lmq_cap(work->lmq);
			if ((rv = nng_lmq_resize(
			         work->lmq, lmq_cap + (lmq_cap / 2))) != 0) {
				fatal(work->id, "nng_lmq_resize", rv);
			}
		}
		nng_lmq_put(work->lmq, work->msg);
		nng_mtx_unlock(work->mtx);
		work->msg   = NULL;
		work->state = HOOK_RECV;
		nng_recv_aio(work->sock, work->aio);
		break;

	default:
		fatal(work->id, "bad state!", NNG_ESTATE);
		break;
	}
}

static struct hook_work *
alloc_work(nng_socket sock, conf_web_hook *conf)
{
	struct hook_work *w;
	int               rv;

	if ((w = nng_alloc(sizeof(*w))) == NULL) {
		fatal(w->id, "nng_alloc", NNG_ENOMEM);
	}
	if ((rv = nng_aio_alloc(&w->aio, webhook_cb, w)) != 0) {
		fatal(w->id, "nng_aio_alloc", rv);
	}
	if ((rv = nng_mtx_alloc(&w->mtx)) != 0) {
		fatal(w->id, "nng_mtx_alloc", rv);
	}
	if ((rv = nng_lmq_alloc(&w->lmq, NANO_LMQ_INIT_CAP) != 0)) {
		fatal(w->id, "nng_lmq_alloc", rv);
	}
	if ((rv = nng_thread_create(&w->thread, thread_cb, w)) != 0) {
		fatal(w->id, "nng_thread_create", rv);
	}

	w->conf  = conf;
	w->sock  = sock;
	w->state = HOOK_INIT;
	w->busy  = false;
	return (w);
}

// The server runs forever.
void
webhook_thr(void *arg)
{
	conf *            conf = arg;
	nng_socket        sock;
	struct hook_work **works =
	    nng_zalloc(conf->web_hook.pool_size * sizeof(struct hook_work *));

	int    rv;
	size_t i;

	/*  Create the socket. */
	rv = nng_pull0_open(&sock);
	if (rv != 0) {
		fatal(0, "nng_rep0_open", rv);
	}

	for (i = 0; i < conf->web_hook.pool_size; i++) {
		works[i]     = alloc_work(sock, &conf->web_hook);
		works[i]->id = i;
	}

	if ((rv = nng_listen(sock, WEB_HOOK_INPROC_URL, NULL, 0)) != 0) {
		fatal(0, "nng_listen", rv);
	}

	for (i = 0; i < conf->web_hook.pool_size; i++) {
		webhook_cb(works[i]);
	}

	for (;;) {
		nng_msleep(3600000); // neither pause() nor sleep() portable
	}

	for (i = 0; i < conf->web_hook.pool_size; i++) {
		nng_free(works[i], sizeof(struct hook_work));
	}
	nng_free(works, conf->web_hook.pool_size * sizeof(struct hook_work *));
}

int
start_webhook_service(conf *conf)
{
	int rv = nng_thread_create(&inproc_thr, webhook_thr, conf);
	if (rv != 0) {
		fatal(0, "nng_thread_create", rv);
	}
	nng_msleep(500);
	return rv;
}

int
stop_webhook_service(void)
{
	nng_thread_destroy(inproc_thr);
	return 0;
}
