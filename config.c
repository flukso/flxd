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

#include "config.h"

const char* flxd_uci_port_tpl[] = {
	"flx.%d.constant",
	"flx.%d.fraction",
	"flx.%d.current",
	"flx.%d.shift",
	"flx.%d.enable"
};

bool config_init(void)
{
	return (conf.uci_ctx = uci_alloc_context()) ? true : false;
}

static bool config_load_str(char *key, char *value)
{
	struct uci_ptr ptr;
	char str[FLXD_STR_MAX];

	strncpy(str, key, FLXD_STR_MAX);
	if (uci_lookup_ptr(conf.uci_ctx, &ptr, str, FLXD_UCI_EXTENDED) != UCI_OK) {
		uci_perror(conf.uci_ctx, key);
		return false;
	}
	if (!(ptr.flags & UCI_LOOKUP_COMPLETE)) {
		conf.uci_ctx->err = UCI_ERR_NOTFOUND;
		uci_perror(conf.uci_ctx, key);
		return false;
	}
	strncpy(value, ptr.o->v.string, FLXD_STR_MAX);
	if (conf.verbosity > 0) {
		fprintf(stdout, "%s=%s\n", key, value);
	}
	return true;
}

static uint32_t config_load_uint(char *key, uint32_t def)
{
	char str_value[FLXD_STR_MAX];

	if (!config_load_str(key, str_value)) {
		return def;
	}
	return strtoul(str_value, (char **)NULL, 10);
}

static uint8_t config_current_to_index(uint32_t current)
{
	uint8_t i = 0;

	switch (current) {
	case 50:
		i = 0;
		break;
	case 100:
		i = 1;
		break;
	case 250:
		i = 2;
		break;
	case 500:
		i = 3;
		break;
	}
	return i;
}

static void config_load_port(int port)
{
	int i;
	char key[FLXD_STR_MAX];

	for (i = 0; i < FLXD_MAX_PORT_PARAMS; i++) {
		snprintf(key, FLXD_STR_MAX, flxd_uci_port_tpl[i], port + 1);
		switch (i) {
		case 0:
			conf.port[port].constant = ltobl(config_load_uint(key, 0));
			break;
		case 1:
			conf.port[port].fraction = ltobs((uint16_t)config_load_uint(key, 0));
			break;
		case 2:
			conf.port[port].current = config_current_to_index(
			    config_load_uint(key, 0));
			break;
		case 3:
			conf.port[port].shift = (uint8_t)config_load_uint(key, 0);
			break;
		case 4:
			conf.port[port].enable = (uint8_t)config_load_uint(key, 0);
			break;
		}
	}
}

bool config_load_all(void)
{
	int i;
	char key[FLXD_STR_MAX];

	if (!config_load_str(FLXD_UCI_DEVICE, conf.device))
		return false;
	for (i = 0; i < FLXD_SID_MAX; i++) {
		snprintf(key, FLXD_STR_MAX, FLXD_UCI_SID_TPL, i + 1);
		if (!config_load_str(key, conf.sid[i]))
			return false;
	}
	for (i = 0; i < FLXD_MAX_PORTS; i++) {
		config_load_port(i);
	}
	return true;
}

