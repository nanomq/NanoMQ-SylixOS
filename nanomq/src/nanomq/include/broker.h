#ifndef NANOMQ_BROKER_H
#define NANOMQ_BROKER_H

#define HTTP_CTX_NUM 4

#include "nng/supplemental/nanolib/conf.h"
#include "nng/supplemental/nanolib/nanolib.h"
#include "nng/nng.h"
#include "nng/protocol/mqtt/mqtt.h"
#include "nng/supplemental/util/platform.h"
#include "nng/mqtt/packet.h"
#include "hashmap.h"

#define PROTO_MQTT_BROKER 0x00
#define PROTO_MQTT_BRIDGE 0x01
#define PROTO_AWS_BRIDGE 0x02
#define PROTO_HTTP_SERVER 0x03

#define STATISTICS

typedef struct work nano_work;
struct work {
	enum {
		INIT,
		RECV,
		WAIT,
		SEND, // Actions after sending msg
		HOOK, // Rule Engine
		END,  // Clear state and cache before disconnect
		CLOSE // sending disconnect packet and err code
	} state;
	// 0x00 mqtt_broker
	// 0x01 mqtt_bridge
	uint8_t proto;
	// MQTT version cache
	uint8_t     proto_ver;
	uint8_t     flag; // flag for webhook & rule_engine
	nng_aio *   aio;
	nng_aio *   bridge_aio; // aio for sending bridging msg (merge as one aio only?)
	nng_msg *   msg;
	nng_msg **  msg_ret;
	nng_ctx     ctx;        // ctx for mqtt broker
	nng_ctx     extra_ctx; //  ctx for bridging/http post
	nng_pipe    pid;
	dbtree *    db;
	dbtree *    db_ret;
	conf *      config;
	reason_code code; // MQTT reason code

	nng_socket webhook_sock;

	struct pipe_content *     pipe_ct;
	conn_param *              cparam;
	struct pub_packet_struct *pub_packet;
	packet_subscribe *        sub_pkt;
	packet_unsubscribe *      unsub_pkt;
};

struct client_ctx {
	nng_pipe pid;
#ifdef STATISTICS
	nng_atomic_u64 *recv_cnt;
#endif
	conn_param *cparam;
	uint32_t    prop_len;
	property   *properties;
	uint8_t     proto_ver;
};

typedef struct client_ctx client_ctx;

int broker_start(int argc, char **argv);
int broker_stop(int argc, char **argv);
int broker_restart(int argc, char **argv);
int broker_dflt(int argc, char **argv);
void bridge_send_cb(void *arg);

#ifdef STATISTICS
uint64_t nanomq_get_message_in(void);
uint64_t nanomq_get_message_out(void);
uint64_t nanomq_get_message_drop(void);
#endif
dbtree *get_broker_db(void);
struct hashmap_s *get_hashmap(void);
int rule_engine_insert_sql(nano_work *work);

#endif
