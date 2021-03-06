/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2017 Bart Van Der Meerssche <bart@flukso.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>
#include <mosquitto.h>
#include <stdint.h>
#include "binary.h"
#include "config.h"
#include "flx.h"
#include "shift.h"

struct config conf;

static void sighandler(int sig)
{
	uloop_end();
}

static bool configure_tty(int fd)
{
	struct termios term;

	if (tcgetattr(fd, &term) == -1) {
		return false;
	}
	if (cfsetospeed(&term, B460800) == -1) {
		return false;
	}
	/* configure tty in raw mode */
	term.c_iflag &= ~(BRKINT | ICRNL | IGNBRK | IGNCR | INLCR | INPCK |
	                  ISTRIP | IXOFF | IXON | PARMRK);
	term.c_oflag &= ~OPOST;
	term.c_cflag &= ~PARENB;
	term.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
	term.c_cc[VMIN] = 1;
	term.c_cc[VTIME] = 0;
	if (tcsetattr(fd, TCSAFLUSH, &term) == -1) {
		return false;
	}
	return true;
}

static int usage(const char *progname)
{
	fprintf(stderr,
		"Usage: %s [<options>]\n"
		"Options:\n"
		"  -v:	Increase verbosity\n"
		"\n", progname);
	return 1;
}

static void timer(struct uloop_timeout *t)
{
	static unsigned int counter = 0;

	flx_tx(FLX_TYPE_PING, (unsigned char *)&counter, sizeof(counter));
	uloop_timeout_set(t, CONFIG_ULOOP_TIMEOUT);
}

static void mosq_on_connect_cb(struct mosquitto *mosq, void *obj, int rc)
{
	if (rc == 0) { /* success */
		if (conf.verbosity > 0) {
			fprintf(stdout, "[mosq] connected to broker\n");
		}
		mosquitto_subscribe(mosq, NULL, conf.topic_bridge_stat, 0);
#ifdef WITH_YKW
		mosquitto_subscribe(mosq, NULL, conf.topic_ykw_config_push, 0);
#endif
	}
}

static void mosq_on_message_cb(struct mosquitto *mosq, void *obj,
                               const struct mosquitto_message *message)
{
	if (strcmp(message->topic, conf.topic_bridge_stat) == 0) {
		if (conf.verbosity > 0) {
			fprintf(stdout, "[mosq] rx %s: %.1s\n", message->topic,
			        (char *)message->payload);
		}
		write(conf.fd_globe, message->payload, 1);
#ifdef WITH_YKW
	} else if (strcmp(message->topic, conf.topic_ykw_config_push) == 0) {
		if (conf.verbosity > 0) {
			fprintf(stdout, "[mosq] rx %s: %s\n", message->topic,
			        (char *)message->payload);
		}
		config_push_ykw(message->payload);
		ykw_set_theta(conf.ykw, conf.theta);
#endif
	}
}

static void ub_sighup(struct ubus_context *ctx, struct ubus_event_handler *ev,
                   const char *type, struct blob_attr *msg)
{
	if (conf.verbosity > 0) {
		fprintf(stdout, CONFIG_UBUS_DEBUG, CONFIG_UBUS_EV_SIGHUP);
	}
	config_init();
	config_load_all();
	shift_init();
#ifdef WITH_YKW
	ykw_set_theta(conf.ykw, conf.theta);
	ykw_set_enabled(conf.ykw, conf.enabled);
	ykw_set_masked(conf.ykw, conf.masked);
#endif
}

static void ub_shift_calc(struct ubus_context *ctx, struct ubus_event_handler *ev,
                   const char *type, struct blob_attr *msg)
{
	int rem;
	struct blob_attr *attr;
	void *data;
	int port = SHIFT_PORT_WILDCARD;

	if (conf.verbosity > 0) {
		fprintf(stdout, CONFIG_UBUS_DEBUG, CONFIG_UBUS_EV_SHIFT_CALC);
	}
	rem = blob_len(msg);
	blobmsg_for_each_attr(attr, msg, rem) {
		if (strcmp("port", blobmsg_name(attr)) == 0) {
			data = blobmsg_data(attr);
			switch(blob_id(attr)) {
			case BLOBMSG_TYPE_INT32:
				port = be32_to_cpu(*(uint32_t *)data) - 1;
				break;
			case BLOBMSG_TYPE_STRING: /* expecting "*" */
				port = SHIFT_PORT_WILDCARD;
				break;
			}
		}
	}
	shift_calculate(port);
}

static void ub_kube_ctrl(struct ubus_context *ctx, struct ubus_event_handler *ev,
                   const char *type, struct blob_attr *msg)
{
	int rem;
	struct blob_attr *attr;
	void *data;

	if (conf.verbosity > 0) {
		fprintf(stdout, CONFIG_UBUS_DEBUG, CONFIG_UBUS_EV_KUBE_CTRL);
	}
	rem = blob_len(msg);
	blobmsg_for_each_attr(attr, msg, rem) {
		if (strcmp("group", blobmsg_name(attr)) == 0) {
			data = blobmsg_data(attr);
			if (blob_id(attr) != BLOBMSG_TYPE_INT32) {
				fprintf(stderr, "[kube] 'group' data type is not uint32_t\n");
			} else {
				conf.kube.group = be32_to_cpu(*(uint32_t *)data);
				config_push_kube();
			}
		}
	}
}

static void ub_kube_packet_tx(struct ubus_context *ctx, struct ubus_event_handler *ev,
                   const char *type, struct blob_attr *msg)
{
	int len, rem;
	struct blob_attr *attr;
	void *hex;
	uint8_t bin[FLX_KUBE_MAX_PACKET_SIZE + 1] = { 0 };

	if (conf.verbosity > 0) {
		fprintf(stdout, CONFIG_UBUS_DEBUG, CONFIG_UBUS_EV_KUBE_PKT_TX);
	}
	rem = blob_len(msg);
	blobmsg_for_each_attr(attr, msg, rem) {
		if (strcmp("hex", blobmsg_name(attr)) == 0) {
			hex = blobmsg_data(attr);
			len = blobmsg_len(attr) - 1; /* pinch off the null termination */
			if (blob_id(attr) != BLOBMSG_TYPE_STRING) {
				fprintf(stderr, "[kube] 'hex' data type is not string\n");
			} else if (len > FLX_KUBE_MAX_PACKET_SIZE * 2) {
				fprintf(stderr, "[kube] tx packet exceeds max size\n");
			} else {
				if (unhexlify(hex, bin, len)) {
					flx_tx(FLX_TYPE_KUBE_PACKET, bin, len / 2);
				}
			}
		}
	}
}

struct config conf = {
	.me = "flxd",
	.verbosity = 0,
	.flx_ufd = {
		.cb = flx_rx
	},
	.timeout = {
		.cb = timer
	},
	.uci_ctx = NULL,
	.ubus_ev_sighup = {
		.cb = ub_sighup
	},
	.ubus_ev_shift_calc = {
		.cb = ub_shift_calc
	},
	.ubus_ev_kube_ctrl = {
		.cb = ub_kube_ctrl
	},
	.ubus_ev_kube_packet_tx = {
		.cb = ub_kube_packet_tx
	},
	.mqtt = {
		.host = "localhost",
		.port = 1883,
		.keepalive = 60,
		.clean_session = true,
		.qos = 0,
		.retain = 0
	}
};

int main(int argc, char **argv)
{
	int opt, rc = 0;
	struct sigaction sa;

	while ((opt = getopt(argc, argv, "hv")) != -1) {
		switch (opt) {
		case 'v':
			conf.verbosity++;
			break;
		case 'h':
		default:
			return usage(argv[0]);
		}	
	}

	conf.fd_globe = open(CONFIG_GLOBE_LED_PATH, O_WRONLY);
	if (conf.fd_globe < 0) {
		perror(CONFIG_GLOBE_LED_PATH);
		rc = 1;
		goto finish;
	}
	conf.flx_ufd.fd = open(FLX_DEV, O_RDWR);
	if (conf.flx_ufd.fd < 0) {
		perror(FLX_DEV);
		rc = 2;
		goto finish;
	}
	if (!configure_tty(conf.flx_ufd.fd)) {
		fprintf(stderr, "%s: Failed to configure tty params\n", FLX_DEV);
		rc = 3;
		goto finish;
	}

	if (!config_init()) {
		rc = 4;
		goto oom;
	}
	if (!config_load_all()) {
		rc = 5;
		goto finish;
	}

	conf.ubus_ctx = ubus_connect(NULL);
	if (!conf.ubus_ctx) {
		fprintf(stderr, "Failed to connect to ubus\n");
		rc = 6;
		goto finish;
	}

#ifdef WITH_YKW
	conf.ykw = ykw_new(conf.device, conf.theta, conf.enabled, conf.masked,
	                   conf.verbosity);
	if (conf.ykw == NULL) {
		rc = 7;
		goto oom;
	}
#endif

	mosquitto_lib_init();
	snprintf(conf.mqtt.id, CONFIG_MQTT_ID_LEN, CONFIG_MQTT_ID_TPL, getpid());
	snprintf(conf.topic_bridge_stat, CONFIG_STR_MAX, CONFIG_TOPIC_BRIDGE_STAT,
	         conf.device);
#ifdef WITH_YKW
	snprintf(conf.topic_ykw_config_push, CONFIG_STR_MAX, YKW_TOPIC_CONFIG_PUSH,
	         conf.device);
#endif
	conf.mosq = mosquitto_new(conf.mqtt.id, conf.mqtt.clean_session, &conf);
	if (!conf.mosq) {
		switch (errno) {
		case ENOMEM:
			rc = 8;
			goto oom;
		case EINVAL:
			fprintf(stderr, "mosq_new: Invalid id and/or clean_session.\n");
			rc = 9;
			goto finish;
		}
	}
	mosquitto_connect_callback_set(conf.mosq, mosq_on_connect_cb);
	mosquitto_message_callback_set(conf.mosq, mosq_on_message_cb);
	rc = mosquitto_loop_start(conf.mosq);
	switch (rc) {
	case MOSQ_ERR_INVAL:
		fprintf(stderr, "mosq_loop_start: Invalid input parameters.\n");
		goto finish;
	case MOSQ_ERR_NOT_SUPPORTED:
		fprintf(stderr, "mosq_loop_start: No threading support.\n");
		goto finish;
	};
	rc = mosquitto_connect_async(conf.mosq, conf.mqtt.host, conf.mqtt.port,
	                  conf.mqtt.keepalive);
	switch (rc) {
	case MOSQ_ERR_INVAL:
		fprintf(stderr, "mosq_connect_async: Invalid input parameters.\n");
		goto finish;
	case MOSQ_ERR_ERRNO:
		perror("mosq_connect_async");
		goto finish;
	}

	uloop_init();
	uloop_fd_add(&conf.flx_ufd, ULOOP_READ);
	uloop_timeout_set(&conf.timeout, CONFIG_ULOOP_TIMEOUT);
	ubus_add_uloop(conf.ubus_ctx);
	ubus_register_event_handler(conf.ubus_ctx, &conf.ubus_ev_sighup,
	                            CONFIG_UBUS_EV_SIGHUP);
	ubus_register_event_handler(conf.ubus_ctx, &conf.ubus_ev_shift_calc,
	                            CONFIG_UBUS_EV_SHIFT_CALC);
	ubus_register_event_handler(conf.ubus_ctx, &conf.ubus_ev_kube_ctrl,
	                            CONFIG_UBUS_EV_KUBE_CTRL);
	ubus_register_event_handler(conf.ubus_ctx, &conf.ubus_ev_kube_packet_tx,
	                            CONFIG_UBUS_EV_KUBE_PKT_TX);

	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = sighandler;
	if (sigaction(SIGTERM, &sa, NULL) == -1) {
		fprintf(stderr, "Failed to set signal handler.\n");
		rc = 10;
		goto finish;
	}

	uloop_run();
	uloop_done();
	goto finish;

oom:
	fprintf(stderr, "error: Out of memory.\n");
finish:
	mosquitto_disconnect(conf.mosq);
	mosquitto_loop_stop(conf.mosq, false);
	mosquitto_destroy(conf.mosq);
	mosquitto_lib_cleanup();
#ifdef WITH_YKW
	ykw_free(conf.ykw);
#endif
	if (conf.ubus_ctx != NULL) {
		ubus_free(conf.ubus_ctx);
	}
	flx_tx(FLX_TYPE_EXIT, NULL, 0);
	close(conf.flx_ufd.fd);
	uci_free_context(conf.uci_ctx);
	return rc;
}

