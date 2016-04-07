#ifndef CONFIG_H
#define CONFIG_H

#include <uci.h>
#include <libubox/uloop.h>
#include <libubus.h>
#include <mosquitto.h>
#ifdef WITH_YKW
#include <ykw.h>
#endif

#define CONFIG_MAX_PORTS			7
#define CONFIG_MAX_ANALOG_PORTS		3
#define CONFIG_MAX_PORT_PARAMS		4
#define CONFIG_STR_MAX				64
#define CONFIG_SID_MAX				36
#define CONFIG_UCI_DEVICE			"system.@system[0].device"
#define CONFIG_UCI_SID_TPL			"flukso.%d.id"
#define CONFIG_UCI_PHASE			"flx.main.phase"
#define CONFIG_UCI_LED_MODE			"flx.main.led_mode"
#define CONFIG_ULOOP_TIMEOUT		1000 /* ms */
#define CONFIG_UBUS_EV_SIGHUP		"flukso.sighup"
#define CONFIG_UBUS_EV_SHIFT_CALC 	"flx.shift.calc"
#define CONFIG_LED_MODE_DEFAULT		255

#define MQTT_ID_TPL					"flxd-p%d"
#define MQTT_ID_LEN					16

#define ltobs(A) ((((uint16_t)(A) & 0xff00) >> 8) | \
	              (((uint16_t)(A) & 0x00ff) << 8))
#define ltobl(A) ((((uint32_t)(A) & 0xff000000) >> 24) | \
	              (((uint32_t)(A) & 0x00ff0000) >> 8)  | \
	              (((uint32_t)(A) & 0x0000ff00) << 8)  | \
	              (((uint32_t)(A) & 0x000000ff) << 24))

enum {
	CONFIG_1PHASE,
	CONFIG_3PHASE_PLUS_N,
	CONFIG_3PHASE_MINUS_N
};

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
	uint16_t constant;
	uint16_t fraction;
	uint8_t current;
	uint8_t shift;
	uint8_t enable;
	uint8_t padding;
};

struct main {
	uint8_t phase;
	uint8_t led;
};

struct config {
	int verbosity;
	char *me;
	char device[CONFIG_STR_MAX];
	char sid[CONFIG_SID_MAX][CONFIG_STR_MAX];
	struct uci_context *uci_ctx;
	struct uloop_fd flx_ufd;
	struct uloop_timeout timeout;
	struct ubus_context *ubus_ctx;
	struct ubus_event_handler ubus_ev_sighup;
	struct ubus_event_handler ubus_ev_shift_calc;
	struct mqtt mqtt;
	struct mosquitto *mosq;
	struct port port[CONFIG_MAX_PORTS];
	struct main main;
#ifdef WITH_YKW
	struct ykw_ctx *ykw;
#endif
};

extern struct config conf;

bool config_init(void);
bool config_load_all(void);
void config_push(void);

#endif
