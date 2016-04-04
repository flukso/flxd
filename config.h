#ifndef CONFIG_H
#define CONFIG_H

#include <uci.h>
#include <libubox/uloop.h>
#include <libubus.h>
#include <mosquitto.h>
#ifdef WITH_YKW
#include <ykw.h>
#endif

#define FLXD_MAX_PORTS			7
#define FLXD_MAX_PORT_PARAMS	5
#define FLXD_STR_MAX			64
#define FLXD_SID_MAX			36
#define FLXD_UCI_EXTENDED		true
#define FLXD_UCI_DEVICE			"system.@system[0].device"
#define FLXD_UCI_SID_TPL		"flukso.%d.id"
#define FLXD_ULOOP_TIMEOUT		1000 /* ms */
#define FLXD_UBUS_EV_SIGHUP		"flukso.sighup"

#define MQTT_ID_TPL				"flxd-p%d"
#define MQTT_ID_LEN				16

#define ltobs(A) ((((uint16_t)(A) & 0xff00) >> 8) | \
	              (((uint16_t)(A) & 0x00ff) << 8))
#define ltobl(A) ((((uint32_t)(A) & 0xff000000) >> 24) | \
	              (((uint32_t)(A) & 0x00ff0000) >> 8)  | \
	              (((uint32_t)(A) & 0x0000ff00) << 8)  | \
	              (((uint32_t)(A) & 0x000000ff) << 24))

struct mqtt {
	char *host;
	int port;
	int keepalive;
	char id[MQTT_ID_LEN];
	bool clean_session;
	int qos;
	int retain;
};

struct port {
	uint32_t constant;
	uint16_t fraction;
	uint8_t current;
	uint8_t shift;
	uint8_t enable;
	uint8_t padding1;
	uint16_t padding2;
};

struct config {
	int verbosity;
	char *me;
	char device[FLXD_STR_MAX];
	char sid[FLXD_SID_MAX][FLXD_STR_MAX];
	struct uci_context *uci_ctx;
	struct uloop_fd flx_ufd;
	struct uloop_timeout timeout;
	struct ubus_context *ubus_ctx;
	struct ubus_event_handler ubus_ev_sighup;
	struct mqtt mqtt;
	struct mosquitto *mosq;
	struct port port[FLXD_MAX_PORTS];
#ifdef WITH_YKW
	struct ykw_ctx *ykw;
#endif
};

extern struct config conf;

bool config_init(void);
bool config_load_all(void);

#endif
