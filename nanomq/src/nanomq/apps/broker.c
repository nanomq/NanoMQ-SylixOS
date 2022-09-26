//
// Copyright 2022 NanoMQ Team, Inc. <jaylin@emqx.io>
//
// This software is supplied under the terms of the MIT License, a
// copy of which should be located in the distribution where this
// file was obtained (LICENSE.txt).  A copy of the license may also be
// found online at https://opensource.org/licenses/MIT.
//
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#if !defined(NANO_PLATFORM_WINDOWS)
#include <signal.h>
#endif

#include <nng/mqtt/mqtt_client.h>
#include "nng/supplemental/tls/tls.h"
#include "nng/supplemental/util/options.h"
#include "nng/supplemental/util/platform.h"
#include "nng/protocol/pipeline0/pull.h"
#include "nng/protocol/pipeline0/push.h"
#include "nng/protocol/reqrep0/rep.h"
#include "nng/protocol/mqtt/mqtt_parser.h"
#include "nng/protocol/mqtt/nmq_mqtt.h"
#include "nng/supplemental/nanolib/conf.h"
#include "nng/supplemental/nanolib/env.h"
#include "nng/supplemental/nanolib/file.h"
#include "nng/supplemental/nanolib/hash_table.h"
#include "nng/supplemental/nanolib/mqtt_db.h"
#include "nng/supplemental/nanolib/log.h"

#include "bridge.h"
#include "mqtt_api.h"
#include "nanomq.h"
#include "process.h"
#include "pub_handler.h"
#include "sub_handler.h"
#include "unsub_handler.h"
#include "web_server.h"
#include "rest_api.h"
#include "webhook_post.h"
#include "webhook_inproc.h"
#include "nanomq.h"

// #if defined(SUPP_RULE_ENGINE)
// 	#include <foundationdb/fdb_c.h>
// 	#include <foundationdb/fdb_c_options.g.h>
// #endif
#if defined(SUPP_AWS_BRIDGE)
	#include "include/aws_bridge.h"
#endif
// Parallel is the maximum number of outstanding requests we can handle.
// This is *NOT* the number of threads in use, but instead represents
// outstanding work items.  Select a small number to reduce memory size.
// (Each one of these can be thought of as a request-reply loop.)  Note
// that you will probably run into limitations on the number of open file
// descriptors if you set this too high. (If not for that limit, this could
// be set in the thousands, each context consumes a couple of KB.)
#ifndef PARALLEL
#define PARALLEL 32
#endif

enum options {
	OPT_HELP = 1,
	OPT_CONFFILE,
	OPT_PARALLEL,
	OPT_DAEMON,
	OPT_THREADS,
	OPT_MAX_THREADS,
	OPT_PROPERTY_SIZE,
	OPT_MSQ_LEN,
	OPT_QOS_DURATION,
	OPT_URL,
	OPT_HTTP_ENABLE,
	OPT_HTTP_PORT,
	OPT_TLS_CA,
	OPT_TLS_CERT,
	OPT_TLS_KEY,
	OPT_TLS_KEYPASS,
	OPT_TLS_VERIFY_PEER,
	OPT_TLS_FAIL_IF_NO_PEER_CERT,
	OPT_LOG_LEVEL,
	OPT_LOG_STDOUT,
	OPT_LOG_FILE,
	OPT_LOG_SYSLOG,
};

static nng_optspec cmd_opts[] = {
	{ .o_name = "help", .o_short = 'h', .o_val = OPT_HELP },
	{ .o_name = "conf", .o_val = OPT_CONFFILE, .o_arg = true },
	{ .o_name = "daemon", .o_short = 'd', .o_val = OPT_DAEMON },
	{ .o_name    = "tq_thread",
	    .o_short = 't',
	    .o_val   = OPT_THREADS,
	    .o_arg   = true },
	{ .o_name    = "max_tq_thread",
	    .o_short = 'T',
	    .o_val   = OPT_MAX_THREADS,
	    .o_arg   = true },
	{ .o_name    = "parallel",
	    .o_short = 'n',
	    .o_val   = OPT_PARALLEL,
	    .o_arg   = true },
	{ .o_name    = "property_size",
	    .o_short = 's',
	    .o_val   = OPT_PROPERTY_SIZE,
	    .o_arg   = true },
	{ .o_name    = "msq_len",
	    .o_short = 'S',
	    .o_val   = OPT_MSQ_LEN,
	    .o_arg   = true },
	{ .o_name    = "qos_duration",
	    .o_short = 'D',
	    .o_val   = OPT_QOS_DURATION,
	    .o_arg   = true },
	{ .o_name = "url", .o_val = OPT_URL, .o_arg = true },
	{ .o_name = "http", .o_val = OPT_HTTP_ENABLE },
	{ .o_name    = "port",
	    .o_short = 'p',
	    .o_val   = OPT_HTTP_PORT,
	    .o_arg   = true },
	{ .o_name = "cacert", .o_val = OPT_TLS_CA, .o_arg = true },
	{ .o_name    = "cert",
	    .o_short = 'E',
	    .o_val   = OPT_TLS_CERT,
	    .o_arg   = true },
	{ .o_name = "key", .o_val = OPT_TLS_KEY, .o_arg = true },
	{ .o_name = "keypass", .o_val = OPT_TLS_KEYPASS, .o_arg = true },
	{ .o_name = "verify", .o_val = OPT_TLS_VERIFY_PEER },
	{ .o_name = "fail", .o_val = OPT_TLS_FAIL_IF_NO_PEER_CERT },
	{ .o_name = "log_level", .o_val = OPT_LOG_LEVEL, .o_arg = true },
	{ .o_name = "log_stdout", .o_val = OPT_LOG_STDOUT, .o_arg = true },
	{ .o_name = "log_syslog", .o_val = OPT_LOG_SYSLOG, .o_arg = true },
	{ .o_name = "log_file", .o_val = OPT_LOG_FILE, .o_arg = true },
	{ .o_name = NULL, .o_val = 0 },
};

// The server keeps a list of work items, sorted by expiration time,
// so that we can use this to set the timeout to the correct value for
// use in poll.

#if (defined DEBUG) && (defined ASAN)
int keepRunning = 1;
void
intHandler(int dummy)
{
	keepRunning = 0;
	fprintf(stderr, "\nBroker exit(0).\n");
}
#endif

void
fatal(const char *func, int rv)
{
	fprintf(stderr, "%s: %s\n", func, nng_strerror(rv));
	exit(1);
}

static inline bool
bridge_handler(nano_work *work)
{
	nng_msg     *smsg;
	bool        rv = false;
	property *props = NULL;

	if (work->proto_ver == MQTT_PROTOCOL_VERSION_v5) {
		mqtt_property_dup(
		    &props, work->pub_packet->var_header.publish.properties);
	}

	smsg = bridge_publish_msg(
	    work->pub_packet->var_header.publish.topic_name.body,
	    work->pub_packet->payload.data, work->pub_packet->payload.len,
	    work->pub_packet->fixed_header.dup,
	    work->pub_packet->fixed_header.qos,
	    work->pub_packet->fixed_header.retain, props);

	for (size_t t = 0; t < work->config->bridge.count; t++) {
		conf_bridge_node *node = work->config->bridge.nodes[t];
		if (node->enable) {
			for (size_t i = 0; i < node->forwards_count; i++) {
				if (topic_filter(node->forwards[i],
				        work->pub_packet->var_header.publish
				            .topic_name.body)) {
					work->state = SEND;
					nng_msg_clone(smsg);
					nng_aio_set_msg(work->aio, smsg);
					nng_socket *socket = node->sock;

					// what if send qos msg failed?
					// nanosdk deal with fail send
					// and cnng_sendmsglose the pipe
					if (nng_aio_busy(work->bridge_aio)) {
						log_warn("bridging aio busy! msg lost!");
						nng_msg_free(smsg);
					} else {
						nng_aio_set_timeout(work->bridge_aio,
						3000);
						nng_aio_set_msg(
						    work->bridge_aio, smsg);
						nng_send_aio(
						    *socket, work->bridge_aio);
					}
					rv = true;
				}
			}
		}
	}
	if (!rv) {
		work->proto_ver == MQTT_PROTOCOL_VERSION_v5
		    ? nng_mqttv5_msg_encode(smsg)
		    : nng_mqtt_msg_encode(smsg);
	}

	nng_msg_free(smsg);
	return rv;
}

void
server_cb(void *arg)
{
	nano_work     *work = arg;
	nng_msg       *msg  = NULL;
	nng_msg       *smsg = NULL;
	int            rv;

	mqtt_msg_info *msg_info;

	switch (work->state) {
	case INIT:
		log_debug("INIT ^^^^ ctx%d ^^^^\n", work->ctx.id);
		work->state = RECV;
		if (work->proto == PROTO_MQTT_BROKER) {
			nng_ctx_recv(work->ctx, work->aio);
		} else {
			nng_ctx_recv(work->extra_ctx, work->aio);
		}
		break;
	case RECV:
		log_debug("RECV  ^^^^ ctx%d ^^^^\n", work->ctx.id);
		if ((rv = nng_aio_result(work->aio)) != 0) {
			log_warn("RECV nng aio result error: %d", rv);
			work->state = RECV;
			if (work->proto == PROTO_MQTT_BROKER) {
				nng_ctx_recv(work->ctx, work->aio);
				break;
			} else {
				log_info("bridge connection closed with reason %d\n", rv);
				if (rv != NNG_ECONNSHUT) {
					nng_ctx_recv(work->extra_ctx, work->aio);
					break;
				}
			}
		}
		if ((msg = nng_aio_get_msg(work->aio)) == NULL) {
			fatal("RECV NULL MSG", rv);
		}
		if (work->proto == PROTO_MQTT_BRIDGE) {
			uint8_t type;
			type = nng_msg_get_type(msg);
			if (type == CMD_CONNACK) {
				log_info("bridge client is connected!");
				nng_msg_set_cmd_type(msg, type);
			} else if (type != CMD_PUBLISH) {
				// only accept publish msg from upstream
				work->state = RECV;
				nng_msg_free(msg);
				nng_ctx_recv(work->extra_ctx, work->aio);
				break;
			} else { 
				nng_msg_set_cmd_type(msg, type);
				// clone conn_param every single time
				conn_param_clone(nng_msg_get_conn_param(msg));
			}
		} else if (work->proto == PROTO_HTTP_SERVER ||
		    work->proto == PROTO_AWS_BRIDGE) {
			nng_msg *decode_msg = NULL;
			if (decode_common_mqtt_msg(&decode_msg, msg) != 0 ||
			    nng_msg_get_type(decode_msg) != CMD_PUBLISH) {
				conn_param_free(nng_msg_get_conn_param(decode_msg));
				work->state = RECV;
				nng_ctx_recv(work->extra_ctx, work->aio);
				break;
			}
			msg = decode_msg;
			nng_msg_set_cmd_type(msg, CMD_PUBLISH);
			// alloc conn_param every single time
		}
		work->msg       = msg;
		work->pid       = nng_msg_get_pipe(work->msg);
		work->cparam    = nng_msg_get_conn_param(work->msg);
		work->proto_ver = conn_param_get_protover(work->cparam);
		work->flag      = nng_msg_cmd_type(msg);

		if (work->flag == CMD_SUBSCRIBE) {
			smsg = work->msg;
			work->msg_ret = NULL;

			if ((work->sub_pkt = nng_alloc(
			         sizeof(packet_subscribe))) == NULL)
				log_error("nng_alloc");
			memset(work->sub_pkt, '\0', sizeof(packet_subscribe));

			if ((rv = decode_sub_msg(work)) != 0 ||
			    (rv = sub_ctx_handle(work)) != 0) {
				work->code = rv;
				log_error("sub_handler: [%d]", rv);
			}

			// TODO not all codes needs to close the pipe
			if (work->code != SUCCESS) {
				if (work->msg_ret)
					cvector_free(work->msg_ret);
				if (work->sub_pkt)
					sub_pkt_free(work->sub_pkt);
				// free conn_param due to clone in protocol layer
				conn_param_free(work->cparam);

				work->state = CLOSE;
				nng_aio_finish(work->aio, 0);
				// TODO break or return?
				break;
			}

			// TODO Error handling
			if (0 != (rv = encode_suback_msg(smsg, work)))
				log_error("error in encode suback: [%d]", rv);

			sub_pkt_free(work->sub_pkt);
			// handle retain (Retain flag handled in npipe)
			work->msg = NULL;
			if (work->msg_ret) {
				log_debug("retain msg [%p] size [%ld] \n",
				    work->msg_ret,
				    cvector_size(work->msg_ret));
				for (int i = 0;
				     i < cvector_size(work->msg_ret) &&
				     check_msg_exp(work->msg_ret[i],
				         nng_msg_get_proto_data(
				             work->msg_ret[i]));
				     i++) {
					nng_msg *m = work->msg_ret[i];
					nng_msg_clone(m);
					work->msg = m;
					nng_aio_set_msg(work->aio, work->msg);
					nng_msg_set_pipe(work->msg, work->pid);
					nng_ctx_send(work->ctx, work->aio);
				}
				cvector_free(work->msg_ret);
			}
			nng_msg_set_cmd_type(smsg, CMD_SUBACK);
			nng_msg_set_pipe(smsg, work->pid);
			nng_aio_set_msg(work->aio, smsg);
			work->msg   = NULL;
			work->state = SEND;
			nng_ctx_send(work->ctx, work->aio);
			smsg = NULL;
			nng_aio_finish(work->aio, 0);
			// free conn_param in SEND state
			break;
		} else if (work->flag == CMD_UNSUBSCRIBE) {
			work->pid = nng_msg_get_pipe(work->msg);
			smsg = work->msg;
			if ((work->unsub_pkt = nng_alloc(
			         sizeof(packet_unsubscribe))) == NULL)
				log_error("nng_alloc");

			if ((rv = decode_unsub_msg(work)) != 0 ||
			    (rv = unsub_ctx_handle(work)) != 0) {
				log_error("unsub_handler [%d]", rv);
			}

			if (0 != (rv = encode_unsuback_msg(smsg, work)))
				log_error("in unsuback [%d]", rv);

			// free unsub_pkt
			unsub_pkt_free(work->unsub_pkt);

			work->pid.id = 0;
			nng_msg_set_pipe(work->msg, work->pid);
			nng_aio_set_msg(work->aio, work->msg);
			work->msg   = NULL;
			work->state = SEND;
			nng_ctx_send(work->ctx, work->aio);
			smsg = NULL;
			nng_aio_finish(work->aio, 0);
			//free conn_param in SEND state
			break;
		} else if (work->flag == CMD_PUBLISH) {
			// Set V4/V5 flag for publish msg
			if (work->proto_ver == 5) {
				nng_msg_set_cmd_type(msg, CMD_PUBLISH_V5);
			} else {
				nng_msg_set_cmd_type(msg, CMD_PUBLISH);
			}
			work->code = handle_pub(work, work->pipe_ct, work->proto_ver);
			if (work->proto == PROTO_HTTP_SERVER ||
			    work->proto == PROTO_AWS_BRIDGE) {
				nng_msg *rep_msg;
				// TODO carry code with msg
				nng_msg_alloc(&rep_msg, 0);
				nng_aio_set_msg(work->aio, rep_msg);
				if (work->code == SUCCESS)
					work->state = WAIT;
				else
					work->state = SEND;
				nng_ctx_send(work->extra_ctx, work->aio);
				break;
			}
			if (work->code != SUCCESS) {
				//what if extra ctx brings a wrong msg?
				if (work->proto != PROTO_MQTT_BROKER) {
					work->state = SEND;
					nng_aio_finish(work->aio, 0);
					// break or return?
					break;
				}
				work->state = CLOSE;
				free_pub_packet(work->pub_packet);
				work->pub_packet = NULL;
				cvector_free(work->pipe_ct->msg_infos);
				// free conn_param due to
				// clone in protocol layer
				conn_param_free(work->cparam);
				nng_aio_finish(work->aio, 0);
				// break or return?
				break;
			}
		} else if (work->flag == CMD_CONNACK) {
			nng_msg_set_pipe(work->msg, work->pid);
			// clone for sending connect event notification
			nng_msg_clone(work->msg);
			nng_aio_set_msg(work->aio, work->msg);
			nng_ctx_send(work->ctx, work->aio); // send connack

			uint8_t *body        = nng_msg_body(work->msg);
			uint8_t  reason_code = *(body + 1);
			smsg = nano_msg_notify_connect(work->cparam, reason_code);
			webhook_entry(work, reason_code);
			// Set V4/V5 flag for publish notify msg
			nng_msg_set_cmd_type(smsg, CMD_PUBLISH);
			work->flag = CMD_PUBLISH;
			nng_msg_free(work->msg);
			work->msg = smsg;
			handle_pub(work, work->pipe_ct, MQTT_PROTOCOL_VERSION_v311);
			// remember to free conn_param in WAIT 
			// due to clone in protocol layer
		} else if (work->flag == CMD_DISCONNECT_EV) {
			// v4 as default, or send V5 notify msg?
			webhook_entry(work, 0);
			nng_msg_set_cmd_type(msg, CMD_PUBLISH);
			work->flag = CMD_PUBLISH;
			handle_pub(work, work->pipe_ct, MQTT_PROTOCOL_VERSION_v311);
			// TODO set reason code
			// uint8_t *payload = nng_msg_payload_ptr(work->msg);
			// uint8_t reason_code = *(payload+16);
			// free client ctx
			if (dbhash_check_id(work->pid.id)) {
				destroy_sub_client(work->pid.id, work->db);
			}
			if (conn_param_get_will_flag(work->cparam) == 0 ||
			    !conn_param_get_will_topic(work->cparam) ||
			    !conn_param_get_will_msg(work->cparam)) {
				// no will msg - free the cp
				conn_param_free(work->cparam);
			} else {
				// set to END to send will msg
				work->state = END;
				// leave cp for will msg
				nng_aio_finish(work->aio, 0);
				break;
			}
		}
		work->state = WAIT;
		nng_aio_finish(work->aio, 0);
		break;
	case WAIT:
		// do not access to cparam
		log_debug("WAIT ^^^^ ctx%d ^^^^", work->ctx.id);
		if (nng_msg_get_type(work->msg) == CMD_PUBLISH) {
			if ((rv = nng_aio_result(work->aio)) != 0) {
				log_error("WAIT nng aio result error: %d", rv);
				fatal("WAIT nng_ctx_recv/send", rv);
			}
			smsg      = work->msg; // reuse the same msg
			cvector(mqtt_msg_info) msg_infos;
			msg_infos = work->pipe_ct->msg_infos;

			log_debug("total pipes: %ld", cvector_size(msg_infos));
			if (cvector_size(msg_infos))
				if (encode_pub_message(smsg, work, PUBLISH))
					for (int i = 0; i < cvector_size(msg_infos) && rv== 0; ++i) {
						msg_info = &msg_infos[i];
						nng_msg_clone(smsg);
						work->pid.id = msg_info->pipe;
						nng_msg_set_pipe(smsg, work->pid);
						work->msg = smsg;
						nng_aio_set_msg(work->aio, work->msg);
						nng_ctx_send(work->ctx, work->aio);
					}
			work->msg = smsg;

			// bridge logic first
			if (work->config->bridge_mode) {
				bridge_handler(work);
#if defined(SUPP_AWS_BRIDGE)
				aws_bridge_forward(work);
#endif
			}
			//check webhook & rule engine
			conf_web_hook *hook_conf   = &(work->config->web_hook);
			uint8_t rule_opt = RULE_ENG_OFF;
#if defined(SUPP_RULE_ENGINE)
			rule_opt = work->config->rule_eng.option;
#endif
			if (hook_conf->enable || rule_opt != RULE_ENG_OFF) {
				work->state = SEND;
				nng_aio_finish(work->aio, 0);
				break;
			}
			nng_msg_free(work->msg);
			smsg = NULL;
			work->msg = NULL;
			// free conn_param due to clone in protocol layer
			conn_param_free(work->cparam);
			free_pub_packet(work->pub_packet);
			work->pub_packet = NULL;
			cvector_free(msg_infos);
			work->pipe_ct->msg_infos = NULL;
			init_pipe_content(work->pipe_ct);
			work->state = RECV;
			if (work->proto != PROTO_MQTT_BROKER) {
				nng_ctx_recv(work->extra_ctx, work->aio);
			} else {
				nng_ctx_recv(work->ctx, work->aio);
			}
		} else if (nng_msg_cmd_type(work->msg) == CMD_PUBACK ||
		    nng_msg_cmd_type(work->msg) == CMD_PUBREL ||
		    nng_msg_cmd_type(work->msg) == CMD_PUBCOMP) {
			nng_msg_free(work->msg);
			work->msg   = NULL;
			work->state = RECV;
			nng_ctx_recv(work->ctx, work->aio);
			break;
		} else {
			log_debug("broker has nothing to do");
			if (work->msg != NULL)
				nng_msg_free(work->msg);
			work->msg   = NULL;
			work->state = RECV;
			nng_ctx_recv(work->ctx, work->aio);
			break;
		}
		break;
	case SEND:
		log_debug("SEND ^^^^ ctx%d ^^^^", work->ctx.id);
#if defined(SUPP_RULE_ENGINE)
		if (work->flag == CMD_PUBLISH && work->config->rule_eng.option != RULE_ENG_OFF) {
			rule_engine_insert_sql(work);
		}
#endif
		// webhook here
		webhook_entry(work, 0);

		if (NULL != work->msg) {
			nng_msg_free(work->msg);
			work->msg = NULL;
		}
		if ((rv = nng_aio_result(work->aio)) != 0) {
			fatal("SEND nng_ctx_send", rv);
		}
		if (work->pub_packet != NULL) {
			free_pub_packet(work->pub_packet);
			work->pub_packet = NULL;
		}
		if (work->pipe_ct->msg_infos != NULL) {
			cvector_free(work->pipe_ct->msg_infos);
			work->pipe_ct->msg_infos = NULL;
			init_pipe_content(work->pipe_ct);
		}
		// free conn_param due to clone in protocol layer
		conn_param_free(work->cparam);
		work->state = RECV;
		work->flag  = 0;
		if (work->proto == PROTO_MQTT_BROKER) {
			nng_ctx_recv(work->ctx, work->aio);
		} else{
			nng_ctx_recv(work->extra_ctx, work->aio);
		}
		break;
	case END:
		log_debug("END ^^^^ ctx%d ^^^^ ", work->ctx.id);
		if (nng_msg_get_type(work->msg) == CMD_PUBLISH) {
			if ((rv = nng_aio_result(work->aio)) != 0) {
				log_error("WAIT nng aio result error: %d", rv);
				fatal("WAIT nng_ctx_recv/send", rv);
			}
			smsg      = work->msg; // reuse the same msg
			work->msg = NULL;

			cvector(mqtt_msg_info) msg_infos;
			msg_infos = work->pipe_ct->msg_infos;

			log_debug("total pipes: %ld", cvector_size(msg_infos));
			//TODO encode abstract msg only
			if (cvector_size(msg_infos))
				if (encode_pub_message(smsg, work, PUBLISH))
					for (int i=0; i<cvector_size(msg_infos); ++i) {
						msg_info = &msg_infos[i];
						nng_msg_clone(smsg);
						work->pid.id = msg_info->pipe;
						nng_msg_set_pipe(smsg, work->pid);
						work->msg = smsg;
						nng_aio_set_msg(work->aio, work->msg);
						nng_ctx_send(work->ctx, work->aio);
					}
			webhook_entry(work, 0);
			nng_msg_free(smsg);
			smsg = NULL;
			work->msg = NULL;
			free_pub_packet(work->pub_packet);
			work->pub_packet = NULL;
			cvector_free(work->pipe_ct->msg_infos);
			work->pipe_ct->msg_infos = NULL;
			init_pipe_content(work->pipe_ct);

			// processing will msg
			if (conn_param_get_will_flag(work->cparam)) {
				msg = nano_pubmsg_composer(&msg,
				    conn_param_get_will_retain(work->cparam),
				    conn_param_get_will_qos(work->cparam),
				    (mqtt_string *) conn_param_get_will_msg(
				        work->cparam),
				    (mqtt_string *) conn_param_get_will_topic(
				        work->cparam),
				    conn_param_get_protover(work->cparam),
					nng_clock());
				work->msg = msg;
				work->flag = CMD_PUBLISH;
				// Set V4/V5 flag for publish msg
				if (conn_param_get_protover(work->cparam) == 5) {
					property *will_property =
					    conn_param_get_will_property(
					        work->cparam);
					nng_msg_set_cmd_type(
					    msg, CMD_PUBLISH_V5);
					handle_pub(work, work->pipe_ct,
					    MQTT_PROTOCOL_VERSION_v5);
					work->pub_packet->var_header.publish
					    .properties = property_pub_by_will(
					    will_property);
					work->pub_packet->var_header.publish
					    .prop_len = get_properties_len(
					    work->pub_packet->var_header
					        .publish.properties);
				} else {
					nng_msg_set_cmd_type(msg, CMD_PUBLISH);
					handle_pub(work, work->pipe_ct,
					    MQTT_PROTOCOL_VERSION_v311);
				}
				work->state = WAIT;
				nng_aio_finish(work->aio, 0);
			} else {
				if (work->msg != NULL)
					nng_msg_free(work->msg);
				work->msg = NULL;
				work->state = RECV;
				if (work->proto == PROTO_MQTT_BROKER) {
					nng_ctx_recv(work->ctx, work->aio);
				} else {
					nng_ctx_recv(work->extra_ctx, work->aio);
				}
			}
		}
		conn_param_free(work->cparam);
		break;
	case CLOSE:
		log_debug(" CLOSE ^^^^ ctx%d ^^^^", work->ctx.id);
		smsg = nano_dismsg_composer(work->code, NULL, NULL, NULL);
		nng_msg_free(work->msg);
		work->msg = smsg;
		// compose a disconnect msg
		nng_msg_set_pipe(work->msg, work->pid);
		// clone for sending connect event notification
		nng_aio_set_msg(work->aio, work->msg);
		nng_ctx_send(work->ctx, work->aio); // send connack

		// clear reason code
		work->code = SUCCESS;
		work->state = RECV;
		nng_ctx_recv(work->ctx, work->aio);
		break;
	default:
		fatal("bad state!", NNG_ESTATE);
		break;
	}
}

void bridge_send_cb(void *arg)
{
	nano_work *work = arg;

}

struct work *
alloc_work(nng_socket sock)
{
	struct work *w;
	int          rv;

	if ((w = nng_alloc(sizeof(*w))) == NULL) {
		fatal("nng_alloc", NNG_ENOMEM);
	}
	if ((rv = nng_aio_alloc(&w->aio, server_cb, w)) != 0) {
		fatal("nng_aio_alloc", rv);
	}
	if ((rv = nng_aio_alloc(&w->bridge_aio, bridge_send_cb, w)) != 0) {
		fatal("bridge_aio nng_aio_alloc", rv);
	}
	if ((rv = nng_ctx_open(&w->ctx, sock)) != 0) {
		fatal("nng_ctx_open", rv);
	}

	w->pipe_ct = nng_alloc(sizeof(struct pipe_content));
	init_pipe_content(w->pipe_ct);
	w->pub_packet = NULL;

	w->state = INIT;
	return (w);
}

nano_work *
proto_work_init(nng_socket sock,nng_socket inproc_sock, nng_socket bridge_sock, uint8_t proto,
    dbtree *db_tree, dbtree *db_tree_ret, conf *config)
{
	int        rv;
	nano_work *w;
	w         = alloc_work(sock);
	w->db     = db_tree;
	w->db_ret = db_tree_ret;
	w->proto  = proto;
	w->config = config;
	w->code   = SUCCESS;

	// only create ctx for extra ctx that are required to receive msg
	if (config->http_server.enable && proto == PROTO_HTTP_SERVER) {
		if ((rv = nng_ctx_open(&w->extra_ctx, inproc_sock)) != 0) {
			fatal("nng_ctx_open", rv);
		}
	} else if (config->bridge_mode) {
		if (proto == PROTO_MQTT_BRIDGE) {
			if ((rv = nng_ctx_open(&w->extra_ctx, bridge_sock)) !=
			    0) {
				fatal("nng_ctx_open", rv);
			}
		} else if (proto == PROTO_AWS_BRIDGE) {
			if ((rv = nng_ctx_open(&w->extra_ctx, inproc_sock)) !=
			    0) {
				fatal("nng_ctx_open", rv);
			}
		}
	}

	if(config->web_hook.enable) {
		if ((rv = nng_push0_open(&w->webhook_sock)) != 0) {
			fatal("nng_socket", rv);
		}
		if ((rv = nng_dial(w->webhook_sock, WEB_HOOK_INPROC_URL, NULL,
		         0)) != 0) {
			fatal("nng_dial", rv);
		}
	}

	return w;
}

static dbtree           *db        = NULL;
static dbtree           *db_ret    = NULL;
// TODO For HTTP SUB/UNSUB usage
static struct hashmap_s *cid_table = NULL;

struct hashmap_s *
get_hashmap(void)
{
	return cid_table;
}

dbtree *
get_broker_db(void)
{
	return db;
}

int
broker(conf *nanomq_conf)
{
	nng_socket sock;
	nng_socket *bridge_sock;
	nng_pipe   pipe_id;
	int        rv;
	int        i, j, count;
	// add the num of other proto
	uint64_t num_ctx = nanomq_conf->parallel;

#if defined(SUPP_RULE_ENGINE)
	conf_rule *cr = &nanomq_conf->rule_eng;
	if (cr->option & RULE_ENG_SDB) {
		nanomq_client_sqlite(cr, false);
	}

	if (cr->option & RULE_ENG_MDB) {
		nanomq_client_mysql(cr, false);
	}

#if defined(FDB_SUPPORT)
	if (cr->option & RULE_ENG_FDB) {
		// RULE_ENGINE_FDB:
		// RULE_ENGINE_SDB:
		pthread_t   netThread;
		fdb_error_t err =
		    fdb_select_api_version(FDB_API_VERSION);
		if (err) {
			log_debug("select API version error: %s",
			    fdb_get_error(err));
			exit(1);
		}
		FDBDatabase *fdb   = openDatabase(&netThread);
		nanomq_conf->rule_eng.rdb[1] = (void *) fdb;
	}
#endif

	if (cr->option & RULE_ENG_RPB) {
		for (int i = 0; i < cvector_size(cr->rules); i++) {
			if (RULE_FORWORD_REPUB == cr->rules[i].forword_type) {
				int              index = 0;
				nng_socket *sock  = (nng_socket *) nng_alloc(
				    sizeof(nng_socket));
				nano_client(sock, cr->rules[i].repub);
			}

		}
	}



#endif

	// init tree
	dbtree_create(&db);
	if (db == NULL) {
		printf("NNL_ERROR error in db create");
	}
	dbtree_create(&db_ret);
	if (db_ret == NULL) {
		printf("NNL_ERROR error in db create");
	}

	dbhash_init_cached_table();
	dbhash_init_pipe_table();
	dbhash_init_alias_table();

	/*  Create the socket. */
	nanomq_conf->db_root = db;
	sock.id              = 0;
	sock.data            = nanomq_conf;
	rv                   = nng_nmq_tcp0_open(&sock);
	if (rv != 0) {
		fatal("nng_nmq_tcp0_open", rv);
	}

	nng_socket inproc_sock;

	if (nanomq_conf->http_server.enable || nanomq_conf->bridge_mode) {
		rv = nng_rep0_open(&inproc_sock);
		if (rv != 0) {
			fatal("nng_rep0_open", rv);
		}
		// set 4 ctx for HTPP as default
		if (nanomq_conf->http_server.enable) {
			num_ctx += HTTP_CTX_NUM;
		}
	}

	if (nanomq_conf->web_hook.enable) {
		start_webhook_service(nanomq_conf);
	}
	if (nanomq_conf->bridge_mode) {
		for (size_t t = 0; t < nanomq_conf->bridge.count; t++) {
			conf_bridge_node *node = nanomq_conf->bridge.nodes[t];
			if (node->enable) {
				num_ctx += node->parallel;
				node->sock = (nng_socket *) nng_alloc(
				    sizeof(nng_socket));
				bridge_client(node->sock, nanomq_conf, node);
			}
		}

#if defined(SUPP_AWS_BRIDGE)
		for (size_t c = 0; c < nanomq_conf->aws_bridge.count; c++) {
			conf_bridge_node *node =
			    nanomq_conf->aws_bridge.nodes[c];
			if (node->enable) {
				num_ctx += node->parallel;
			}
		}
#endif
	}

	struct work **works = nng_zalloc(num_ctx * sizeof(struct work *));
	// create broker ctx
	for (i = 0; i < nanomq_conf->parallel; i++) {
		works[i] = proto_work_init(sock, inproc_sock, sock,
		    PROTO_MQTT_BROKER, db, db_ret, nanomq_conf);
	}

	// create bridge ctx
	// only create ctx when there is sub topics
	size_t tmp = nanomq_conf->parallel;
	if (nanomq_conf->bridge_mode) {
		// iterates all bridge targets
		for (size_t t = 0; t < nanomq_conf->bridge.count; t++) {
			conf_bridge_node *node = nanomq_conf->bridge.nodes[t];
			if (node->enable) {
				bridge_sock = node->sock;
				for (i = tmp; i < (tmp + node->parallel);
				     i++) {
					works[i] = proto_work_init(sock,
					    inproc_sock, *bridge_sock,
					    PROTO_MQTT_BRIDGE, db, db_ret,
					    nanomq_conf);
				}
				tmp += node->parallel;
			}
		}

#if defined(SUPP_AWS_BRIDGE)
		for (size_t t = 0; t < nanomq_conf->aws_bridge.count; t++) {
			conf_bridge_node *node =
			    nanomq_conf->aws_bridge.nodes[t];
			if (node->enable) {
				for (i = tmp; i < (tmp + node->parallel);
				     i++) {
					works[i] =
					    proto_work_init(sock, inproc_sock,
					        sock, PROTO_AWS_BRIDGE, db,
					        db_ret, nanomq_conf);
				}
				tmp += node->parallel;
				aws_bridge_client(node);
			}
		}
#endif
	}

	// create http server ctx
	if (nanomq_conf->http_server.enable) {
		for (i = tmp; i < tmp + HTTP_CTX_NUM; i++) {
			works[i] = proto_work_init(sock, inproc_sock, sock,
			    PROTO_HTTP_SERVER, db, db_ret, nanomq_conf);
		}
	}

	if ((rv = nano_listen(sock, nanomq_conf->url, NULL, 0, nanomq_conf)) != 0) {
		fatal("nng_listen", rv);
	}

	// read from command line & config file
	if (nanomq_conf->websocket.enable) {
		if ((rv = nano_listen(
		         sock, nanomq_conf->websocket.url, NULL, 0, nanomq_conf)) != 0) {
			fatal("nng_listen ws", rv);
		}
	}

	if (nanomq_conf->tls.enable) {
		nng_listener tls_listener;

		if ((rv = nng_listener_create(
		         &tls_listener, sock, nanomq_conf->tls.url)) != 0) {
			fatal("nng_listener_create tls", rv);
		}
		nng_listener_set(
		    tls_listener, NANO_CONF, nanomq_conf, sizeof(nanomq_conf));

		init_listener_tls(tls_listener, &nanomq_conf->tls);
		if ((rv = nng_listener_start(tls_listener, 0)) != 0) {
			fatal("nng_listener_start tls", rv);
		}
		// TODO websocket ssl 
		// if (nanomq_conf->websocket.enable) {
		// 	nng_listener wss_listener;
		// 	if ((rv = nng_listener_create(&wss_listener, sock,
		// 	         nanomq_conf->tls.url)) != 0) {
		// 		fatal("nng_listener_create wss", rv);
		// 	}
		// 	init_listener_tls(wss_listener, &nanomq_conf->tls);
		// 	if ((rv = nng_listener_start(wss_listener, 0)) != 0) {
		// 		fatal("nng_listener_start wss", rv);
		// 	}
		// }
	}

	if (nanomq_conf->http_server.enable || nanomq_conf->bridge_mode) {
		if ((rv = nano_listen(inproc_sock, INPROC_SERVER_URL, NULL, 0,
		         nanomq_conf)) != 0) {
			fatal("nng_listen " INPROC_SERVER_URL, rv);
		}
	}

	for (i = 0; i < num_ctx; i++) {
		server_cb(works[i]); // this starts them going (INIT state)
	}

	if (nanomq_conf->http_server.enable) {
		start_rest_server(nanomq_conf);
	}

#if (defined DEBUG) && (defined ASAN)
#if !(defined NANO_PLATFORM_WINDOWS)
	signal(SIGINT, intHandler);
#endif
	for (;;) {
		if (keepRunning == 0) {
#if defined(SUPP_RULE_ENGINE)

	#if defined(FDB_SUPPORT)
			if (nanomq_conf->rule_eng.option & RULE_ENG_FDB) {
				fdb_database_destroy(
				    nanomq_conf->rule_eng.rdb[1]);
				fdb_stop_network();
			}
	#endif
#endif
			for (size_t i = 0; i < num_ctx; i++) {
				nng_free(works[i]->pipe_ct,
				    sizeof(struct pipe_content));
				nng_aio_free(works[i]->bridge_aio);
				nng_free(works[i], sizeof(struct work));
			}
			nng_free(works, num_ctx * sizeof(struct work *));

			exit(0);
		}
		nng_msleep(6000);
	}
#else
	for (;;) {
		nng_msleep(3600000); // neither pause() nor sleep() portable
	}
#endif
}

void
print_usage(void)
{
	printf("Usage: nanomq { { start | restart [--url <url>] "
	       "[--conf <path>] [-t, --tq_thread <num>]\n                     "
	       "[-T, -max_tq_thread <num>] [-n, --parallel <num>] \n                     "
	       "[-D, --qos_duration <num>] [--http] "
	       "[-p, --port] [-d, --daemon] \n                     "
	       "[--cacert <path>] [-E, --cert <path>] "
	       "[--key <path>] \n                     "
	       "[--keypass <password>] [--verify] [--fail] }\n            "
	       "         | stop }\n\n");
	printf("Options: \n");
	printf("  --url <url>                Specify listener's url: "
	       "'nmq-tcp://host:port', \r\n                             "
	       "'tls+nmq-tcp://host:port', \r\n                             "
	       "'nmq-ws://host:port/path', \r\n                             "
	       "'nmq-wss://host:port/path'\n");
	printf("  --conf <path>              The path of a specified nanomq "
	       "configuration file \n");

	printf("  --http                     Enable http server (default: "
	       "false)\n");
	printf(
	    "  -p, --port <num>           The port of http server (default: "
	    "8081)\n");
	printf(
	    "  -t, --tq_thread <num>      The number of taskq threads used, "
	    "\r\n                             "
	    "`num` greater than 0 and less than 256\n");
	printf(
	    "  -T, --max_tq_thread <num>  The maximum number of taskq threads "
	    "used, \r\n                             "
	    "`num` greater than 0 and less than 256\n");
	printf(
	    "  -n, --parallel <num>       The maximum number of outstanding "
	    "requests we can handle\n");
	printf("  -s, --property_size <num>  The max size for a MQTT user "
	       "property\n");
	printf("  -S, --msq_len <num>        The queue length for resending "
	       "messages\n");
	printf("  -D, --qos_duration <num>   The interval of the qos timer\n");
	printf("  -d, --daemon               Run nanomq as daemon (default: "
	       "false)\n");
	printf("  --cacert                   Path to the file containing "
	       "PEM-encoded CA certificates\n");
	printf("  -E, --cert                 Path to a file containing the "
	       "user certificate\n");
	printf("  --key                      Path to the file containing the "
	       "user's private PEM-encoded key\n");
	printf("  --keypass                  String containing the user's "
	       "password. \r\n                             "
		   "Only used if the private keyfile is password-protected\n");
	printf("  --verify                   Set verify peer "
	       "certificate (default: false)\n");
	printf("  --fail                     Server will fail if the client "
	       "does not have a \r\n                             "
	       "certificate to send (default: false)\n");
	printf("  --log_level   <level>      The level of log output \n       "
	       "                      "
	       "(level: trace, debug, info, warn, error, fatal)\n             "
	       "                "
	       "(default: warn)\n");
	printf("  --log_file    <file_path>  The path of the log file \n");
	printf(
	    "  --log_stdout  <true|false> Enable/Disable console log output "
	    "(default: true)\n");

#if defined(SUPP_SYSLOG)
	printf("  --log_syslog  <true|false> Enable/Disable syslog output "
	       "(default: false)\n");
#endif

}

int
status_check(int *pid)
{
#ifdef NANO_PLATFORM_WINDOWS
	(void) pid;
	log_warn("Not support on Windows\n");
	return -1;
#else
	char  *data = NULL;
	size_t size = 0;

	int rc;
	if ((rc = nng_file_get(PID_PATH_NAME, (void *) &data, &size)) != 0) {
		nng_free(data, size);
		log_warn(".pid file not found or unreadable\n");
		return 1;
	} else {
		if ((data) != NULL) {
			sscanf(data, "%u", pid);
			log_info("pid read, [%u]", *pid);
			nng_free(data, size);

			if ((kill(*pid, 0)) == 0) {
				log_info("there is a running NanoMQ instance "
				          ": pid [%u]",
				    *pid);
				return 0;
			}
		}
		if (!nng_file_delete(PID_PATH_NAME)) {
			log_info(".pid file is removed");
			return 1;
		}
		log_error("unexpected error");
		return -1;
	}
#endif
}

int
store_pid()
{
	int  status;
	char pid_c[12] = "";

	sprintf(pid_c, "%d", nng_getpid());
	log_info("%s", pid_c);

	status = nng_file_put(PID_PATH_NAME, pid_c, sizeof(pid_c));
	return status;
}

void
active_conf(conf *nanomq_conf)
{
	// check if daemonlize
#ifdef NANO_PLATFORM_WINDOWS
	if (nanomq_conf->daemon) {
		log_error("Daemon mode is not supported on Windows");
		exit(EXIT_FAILURE);
	}
#else
	if (nanomq_conf->daemon == true && process_daemonize()) {
		log_error("Error occurs, cannot daemonize");
		exit(EXIT_FAILURE);
	}
#endif
	// taskq and max_taskq
	if (nanomq_conf->num_taskq_thread || nanomq_conf->max_taskq_thread) {
		nng_taskq_setter(nanomq_conf->num_taskq_thread,
		    nanomq_conf->max_taskq_thread);
	}
}

static void
predicate_url(conf *config, char *url)
{
	if (strncmp(BROKER_NMQ_TCP_URL_PREFIX, url,
	        strlen(BROKER_NMQ_TCP_URL_PREFIX)) == 0 ||
	    strncmp(BROKER_TCP_URL_PREFIX, url,
	        strlen(BROKER_TCP_URL_PREFIX)) == 0) {
		FREE_NONULL(config->url);
		config->url = nng_strdup(url);
	}
	if (strncmp(BROKER_NMQ_TCP_TLS_URL_PREFIX, url,
	        strlen(BROKER_NMQ_TCP_TLS_URL_PREFIX)) == 0) {
		FREE_NONULL(config->tls.url);
		config->tls.enable = true;
		config->tls.url    = nng_strdup(url);
	} else if (strncmp(BROKER_NMQ_WS_URL_PREFIX, url,
	               strlen(BROKER_NMQ_WS_URL_PREFIX)) == 0 ||
	    strncmp(BROKER_WS_URL_PREFIX, url, strlen(BROKER_WS_URL_PREFIX)) ==
	        0) {
		if (strncmp(BROKER_NMQ_WSS_URL_PREFIX, url,
		        strlen(BROKER_NMQ_WSS_URL_PREFIX)) == 0 ||
		    strncmp(BROKER_WSS_URL_PREFIX, url,
		        strlen(BROKER_WSS_URL_PREFIX)) == 0) {
			FREE_NONULL(config->websocket.tls_url);
			config->tls.enable        = true;
			config->websocket.tls_url = nng_strdup(url);
		} else {
			FREE_NONULL(config->websocket.url);
			config->websocket.url = nng_strdup(url);
		}
		config->websocket.enable = true;
	}
}

int
file_path_parse(int argc, char **argv, conf *config)
{
    int idx = 2;
    char **arg;
    int val = 0;
    int rv = -1;

    printf("arg: [%p]\n", arg);

	while ((rv = nng_opts_parse(argc, argv, cmd_opts, &val, arg, &idx)) ==
	    0) {
		switch (val) {
		case OPT_HELP:
			print_usage();
			exit(0);
			break;
		case OPT_CONFFILE:
			FREE_NONULL(config->conf_file);
			config->conf_file = nng_strdup(arg);
			break;

		default:
			break;
		}
	}

	switch (rv) {
	case NNG_EINVAL:
		fprintf(stderr,
		    "Option %s is invalid.\nTry 'nanomq --help' for "
		    "more information.\n",
		    argv[idx]);
		break;
	case NNG_EAMBIGUOUS:
		fprintf(stderr,
		    "Option %s is ambiguous (specify in full).\nTry 'nanomq "
		    "broker --help' for more information.\n",
		    argv[idx]);
		break;
	case NNG_ENOARG:
		fprintf(stderr,
		    "Option %s requires argument.\nTry 'nanomq --help' "
		    "for more information.\n",
		    argv[idx]);
		break;
	default:
		break;
	}

	return rv == -1;
}

int
broker_parse_opts(int argc, char **argv, conf *config)
{
	int   idx = 2;
	char *arg;
	int   val;
	int   rv;

	while ((rv = nng_opts_parse(argc, argv, cmd_opts, &val, &arg, &idx)) ==
	    0) {
		switch (val) {
		case OPT_PARALLEL:
			config->parallel = atoi(arg);
			break;
		case OPT_DAEMON:
			config->daemon = true;
			break;
		case OPT_THREADS:
			config->num_taskq_thread = atoi(arg);
			break;
		case OPT_MAX_THREADS:
			config->max_taskq_thread = atoi(arg);
			break;
		case OPT_PROPERTY_SIZE:
			config->property_size = atoi(arg);
			break;
		case OPT_MSQ_LEN:
			config->msq_len = atoi(arg);
			break;
		case OPT_QOS_DURATION:
			config->qos_duration = atoi(arg);
			break;
		case OPT_URL:
			predicate_url(config, arg);
			break;
		case OPT_TLS_CA:
			FREE_NONULL(config->tls.ca);
			file_load_data(arg, (void **) &config->tls.ca);
			break;
		case OPT_TLS_CERT:
			FREE_NONULL(config->tls.cert);
			file_load_data(arg, (void **) &config->tls.cert);
			break;
		case OPT_TLS_KEY:
			FREE_NONULL(config->tls.key);
			file_load_data(arg, (void **) &config->tls.key);
			break;
		case OPT_TLS_KEYPASS:
			FREE_NONULL(config->tls.key_password);
			config->tls.key_password = nng_strdup(arg);
			break;
		case OPT_TLS_VERIFY_PEER:
			config->tls.verify_peer = true;
			break;
		case OPT_TLS_FAIL_IF_NO_PEER_CERT:
			config->tls.set_fail = true;
			break;
		case OPT_HTTP_ENABLE:
			config->http_server.enable = true;
			break;
		case OPT_HTTP_PORT:
			config->http_server.port = atoi(arg);
			break;
		case OPT_LOG_LEVEL:
			config->log.level = log_level_num(arg);
			break;
		case OPT_LOG_FILE:
			FREE_NONULL(config->log.file);
			FREE_NONULL(config->log.dir);
			config->log.type |= LOG_TO_FILE;
			char *file_name = strrchr(arg, '/');
			if (file_name) {
				config->log.file = nng_strdup(file_name);
				config->log.dir =
				    nng_strndup(arg, file_name - arg);
			} else {
				config->log.file = nng_strdup(arg);
			}
			break;
		case OPT_LOG_SYSLOG:
#if defined(SUPP_SYSLOG)
			if (nng_strcasecmp("true", arg) == 0) {
				config->log.type |= LOG_TO_SYSLOG;
			} else if (nng_strcasecmp("false", arg) == 0) {
				config->log.type &= ~LOG_TO_SYSLOG;
			}
#else
			fprintf(stderr,
			    "Syslog is not supported, please make sure you "
			    "have built nanomq with option '-D ENABLE_SYSLOG=ON' "
			    ".\n");
			exit(EXIT_FAILURE);
#endif
			break;
		case OPT_LOG_STDOUT:
			if (nng_strcasecmp("true", arg) == 0) {
				config->log.type |= LOG_TO_CONSOLE;
			} else if (nng_strcasecmp("false", arg) == 0) {
				config->log.type &= ~LOG_TO_CONSOLE;
			}
			break;

		default:
			break;
		}
	}

	switch (rv) {
	case NNG_EINVAL:
		fprintf(stderr,
		    "Option %s is invalid.\nTry 'nanomq --help' for "
		    "more information.\n",
		    argv[idx]);
		break;
	case NNG_EAMBIGUOUS:
		fprintf(stderr,
		    "Option %s is ambiguous (specify in full).\nTry 'nanomq "
		    "broker --help' for more information.\n",
		    argv[idx]);
		break;
	case NNG_ENOARG:
		fprintf(stderr,
		    "Option %s requires argument.\nTry 'nanomq --help' "
		    "for more information.\n",
		    argv[idx]);
		break;
	default:
		break;
	}

	return rv == -1;
}

int
broker_start(int argc, char **argv)
{
	int i, url, temp, rc, num_ctx = 0;
	int pid = 0;

	conf *nanomq_conf;

	if (!status_check(&pid)) {
		fprintf(stderr,
		    "One NanoMQ instance is still running, a new instance "
		    "won't be started until the other one is stopped.\n");
		exit(EXIT_FAILURE);
	}

	if ((nanomq_conf = nng_zalloc(sizeof(conf))) == NULL) {
		fprintf(stderr,
		    "Cannot allocate storge for configuration, quit\n");
		exit(EXIT_FAILURE);
	}

	// Priority: config < environment variables < command opts
	conf_init(nanomq_conf);
	read_env_conf(nanomq_conf);

	if (!file_path_parse(argc, argv, nanomq_conf)) {
		conf_fini(nanomq_conf);
		fprintf(stderr, "Cannot parse command line arguments, quit\n");
		exit(EXIT_FAILURE);
	}

	conf_parse(nanomq_conf);

	if (!broker_parse_opts(argc, argv, nanomq_conf)) {
		conf_fini(nanomq_conf);
		fprintf(stderr, "Cannot parse command line arguments, quit\n");
		exit(EXIT_FAILURE);
	}

	nanomq_conf->url = nanomq_conf->url != NULL
	    ? nanomq_conf->url
	    : nng_strdup(CONF_TCP_URL_DEFAULT);

	if (nanomq_conf->tls.enable) {
		nanomq_conf->tls.url = nanomq_conf->tls.url != NULL
		    ? nanomq_conf->tls.url
		    : nng_strdup(CONF_TLS_URL_DEFAULT);
	}

	if (nanomq_conf->websocket.enable) {
		nanomq_conf->websocket.url = nanomq_conf->websocket.url != NULL
		    ? nanomq_conf->websocket.url
		    : nng_strdup(CONF_WS_URL_DEFAULT);

		if (nanomq_conf->tls.enable) {
			nanomq_conf->websocket.tls_url =
			    nanomq_conf->websocket.tls_url != NULL
			    ? nanomq_conf->websocket.tls_url
			    : nng_strdup(CONF_WSS_URL_DEFAULT);
		}
	}

	if ((rc = log_init(&nanomq_conf->log)) != 0) {
		fatal("log_init", rc);
	}

	print_conf(nanomq_conf);
	active_conf(nanomq_conf);

	if (store_pid()) {
		log_error("create \"nanomq.pid\" file failed");
	}

	rc = broker(nanomq_conf);

	exit(rc == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}

#ifndef NANO_PLATFORM_WINDOWS

int
broker_stop(int argc, char **argv)
{
	int pid = 0;

	if (argc > 2) {
		print_usage();
		exit(EXIT_FAILURE);
	}

	if (!(status_check(&pid))) {
		kill(pid, SIGTERM);
	} else {
		fprintf(stderr, "There is no running NanoMQ instance.\n");
		exit(EXIT_FAILURE);
	}
	fprintf(stderr, "NanoMQ stopped.\n");
	exit(EXIT_SUCCESS);
}

int
broker_restart(int argc, char **argv)
{
	int pid = 0;

	if (!(status_check(&pid))) {
		kill(pid, SIGTERM);
		while (!status_check(&pid)) {
			kill(pid, SIGKILL);
		}
		fprintf(stderr, "Previous NanoMQ instance stopped.\n");
	} else {
		fprintf(stderr, "There is no running NanoMQ instance.\n");
	}

	return broker_start(argc, argv);
}

#else

int
broker_restart(int argc, char **argv)
{
	log_error("Not support on Windows\n");
	exit(EXIT_SUCCESS);
}


int
broker_stop(int argc, char **argv)
{
	log_error("Not support on Windows\n");
	exit(EXIT_SUCCESS);
}

#endif

int
broker_dflt(int argc, char **argv)
{
	print_usage();
	return 0;
}
