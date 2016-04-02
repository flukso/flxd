/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Bart Van Der Meerssche <bart@flukso.net>
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
#include <unistd.h>
#include <termios.h>
#include <uci.h>
#include <libubox/uloop.h>
#include <libubus.h>
#include <mosquitto.h>
#include "config.h"
#include "flx.h"

struct config conf;

static bool configure_tty(int fd)
{
	struct termios term;

	if (tcgetattr(fd, &term) == -1) {
		return false;
	}
	if (cfsetospeed(&term, B921600) == -1) {
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

static bool load_uci_config(struct uci_context *ctx, char *param, char *value)
{
	struct uci_ptr ptr;
	char str[FLXD_STR_MAX];

	strncpy(str, param, FLXD_STR_MAX);
	if (uci_lookup_ptr(ctx, &ptr, str, true) != UCI_OK) {
		uci_perror(ctx, param);
		return false;
	}
	if (!(ptr.flags & UCI_LOOKUP_COMPLETE)) {
		ctx->err = UCI_ERR_NOTFOUND;
		uci_perror(ctx, param);
		return false;
	}
	strncpy(value, ptr.o->v.string, FLXD_STR_MAX);
	if (conf.verbosity > 0) {
		fprintf(stdout, "%s=%s\n", param, value);
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
	uloop_timeout_set(t, FLXD_ULOOP_TIMEOUT);
}

static void sighup(struct ubus_context *ctx, struct ubus_event_handler *ev,
                   const char *type, struct blob_attr *msg)
{
	/* TODO reload dynamic config settings */
	fprintf(stdout, "Received flukso.sighup ubus event.\n");
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
	.ubus_ev_sighup = {
		.cb = sighup
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
	int i, opt, rc = 0;
	char param[FLXD_STR_MAX];

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

	conf.uci_ctx = uci_alloc_context();
	if (!conf.uci_ctx) {
		rc = 1;
		goto oom;
	}
	rc = 2;
	if (!load_uci_config(conf.uci_ctx, FLXD_UCI_DEVICE, conf.device))
		goto finish;
	for (i = 0; i < FLXD_SID_MAX; i++) {
		snprintf(param, FLXD_STR_MAX, FLXD_UCI_SID_TPL, i + 1);
		if (!load_uci_config(conf.uci_ctx, param, conf.sid[i]))
			goto finish;
	}
	rc = 0;

	conf.flx_ufd.fd = open(FLX_DEV, O_RDWR);
	if (conf.flx_ufd.fd < 0) {
		perror(FLX_DEV);
		rc = 3;
		goto finish;
	}
	if (!configure_tty(conf.flx_ufd.fd)) {
		fprintf(stderr, "%s: Failed to configure tty params\n", FLX_DEV);
		rc = 4;
		goto finish;
	}

	conf.ubus_ctx = ubus_connect(NULL);
	if (!conf.ubus_ctx) {
		fprintf(stderr, "Failed to connect to ubus\n");
		rc = 5;
		goto finish;
	}

#ifdef WITH_YKW
	conf.ykw = ykw_new(YKW_DEFAULT_THETA);
	if (conf.ykw == NULL) {
		rc = 6;
		goto oom;
	}
#endif

	mosquitto_lib_init();
	snprintf(conf.mqtt.id, MQTT_ID_LEN, MQTT_ID_TPL, getpid());
	conf.mosq = mosquitto_new(conf.mqtt.id, conf.mqtt.clean_session, &conf);
	if (!conf.mosq) {
		switch (errno) {
		case ENOMEM:
			rc = 7;
			goto oom;
		case EINVAL:
			fprintf(stderr, "mosq_new: Invalid id and/or clean_session.\n");
			rc = 8;
			goto finish;
		}
	}
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
	uloop_timeout_set(&conf.timeout, FLXD_ULOOP_TIMEOUT);
	ubus_add_uloop(conf.ubus_ctx);
	ubus_register_event_handler(conf.ubus_ctx, &conf.ubus_ev_sighup,
	                            FLXD_UBUS_EV_SIGHUP);
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
	ykw_free(conf.ykw);
	ubus_free(conf.ubus_ctx);
	close(conf.flx_ufd.fd);
	uci_free_context(conf.uci_ctx);
	return rc;
}

