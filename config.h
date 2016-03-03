#ifndef CONFIG_H
#define CONFIG_H

#include <libubox/uloop.h>
#include <mosquitto.h>
#ifdef WITH_YKW
#include <ykw.h>
#endif

#define FLXD_STR_MAX		64
#define FLXD_SID_MAX		12
#define FLXD_UCI_DEVICE		"system.@system[0].device"
#define FLXD_UCI_SID_TPL	"flukso.%d.id"
#define FLXD_ULOOP_TIMEOUT	1000 /* ms */

#define MQTT_ID_TPL		"flxd-p%d"
#define MQTT_ID_LEN		16

struct mqtt {
	char *host;
	int port;
	int keepalive;
	char id[MQTT_ID_LEN];
	bool clean_session;
	int qos;
	int retain;
};

struct config {
	int verbosity;
	char *me;
	char device[FLXD_STR_MAX];
	char sid[FLXD_SID_MAX][FLXD_STR_MAX];
	struct uci_context *uci_ctx;
	struct uloop_fd flx_ufd;
	struct uloop_timeout timeout;
	struct mqtt mqtt;
	struct mosquitto *mosq;
#ifdef WITH_YKW
	struct ykw_ctx *ykw;
#endif
};

extern struct config conf;

#endif
