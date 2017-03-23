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

#include <json/json.h>
#include "math.h"
#include "config.h"
#include "flx.h"
#include "spin.h"

const char* config_uci_sensor_tpl[] = {
	"flukso.%d.id",
	"flukso.%d.type",
	"flukso.%d.enable",
};

const char* config_uci_port_tpl[] = {
	"flx.%d.constant",
	"flx.%d.current",
	"flx.%d.shift",
	"flx.%d.enable"
};

bool config_init(void)
{
	if (conf.uci_ctx != NULL) {
		uci_free_context(conf.uci_ctx);
	}
	return (conf.uci_ctx = uci_alloc_context()) ? true : false;
}

static bool config_load_str(char *key, char *value)
{
	struct uci_ptr ptr;
	char str[CONFIG_STR_MAX];

	strncpy(str, key, CONFIG_STR_MAX);
	if (uci_lookup_ptr(conf.uci_ctx, &ptr, str, true) != UCI_OK) {
		uci_perror(conf.uci_ctx, key);
		return false;
	}
	if (!(ptr.flags & UCI_LOOKUP_COMPLETE)) {
		conf.uci_ctx->err = UCI_ERR_NOTFOUND;
		uci_perror(conf.uci_ctx, key);
		return false;
	}
	strncpy(value, ptr.o->v.string, CONFIG_STR_MAX);
	if (conf.verbosity > 0) {
		fprintf(stdout, "[uci] %s=%s\n", key, value);
	}
	return true;
}

static uint32_t config_load_uint(char *key, uint32_t def)
{
	char str_value[CONFIG_STR_MAX];

	if (!config_load_str(key, str_value)) {
		return def;
	}
	return strtoul(str_value, (char **)NULL, 10);
}

static double config_load_fp(char *key, double def)
{
	char str_value[CONFIG_STR_MAX];

	if (!config_load_str(key, str_value)) {
		return def;
	}
	return atof(str_value);
}

static uint8_t config_type_to_index(char *type)
{
	if (strcmp("electricity", type) == 0) {
		return CONFIG_SENSOR_TYPE_ELECTRICITY;
	} else if (strcmp("gas", type) == 0) {
		return CONFIG_SENSOR_TYPE_GAS;
	} else if (strcmp("water", type) == 0) {
		return CONFIG_SENSOR_TYPE_WATER;
	} else {
		return CONFIG_SENSOR_TYPE_OTHER;
	}
}

static bool config_load_sensor(int sensor)
{
	int i;
	char key[CONFIG_STR_MAX];
	char str_value[CONFIG_STR_MAX];

	for (i = 0; i < CONFIG_MAX_SENSOR_PARAMS; i++) {
		snprintf(key, CONFIG_STR_MAX, config_uci_sensor_tpl[i], sensor + 1);
		switch (i) {
		case 0:
			if (!config_load_str(key, conf.sensor[sensor].id)) {
				return false;
			}
			break;
		case 1:
			/* we only require a type for pulse sensors */
			if (sensor < CONFIG_MAX_SENSORS - 3) {
				break;
			}
			if (!config_load_str(key, str_value)) {
				return false;
			}
			conf.sensor[sensor].type = (uint8_t)config_type_to_index(str_value);
			break;
		case 2:
			conf.sensor[sensor].enable = (uint8_t)config_load_uint(key, 0);
			break;
		}
	}
	return true;
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
	double tmpint, tmpfrac;
	char key[CONFIG_STR_MAX];

	for (i = 0; i < CONFIG_MAX_PORT_PARAMS; i++) {
		snprintf(key, CONFIG_STR_MAX, config_uci_port_tpl[i], port + 1);
		switch (i) {
		case 0:
			tmpfrac = modf(config_load_fp(key, 0.0), &tmpint);
			conf.port[port].constant = ltobs((uint16_t)tmpint);
			conf.port[port].fraction = ltobs((uint16_t)(tmpfrac * 1000 + 0.5));
			break;
		case 1:
			conf.port[port].current = config_current_to_index(
			    config_load_uint(key, 0));
			break;
		case 2:
			conf.port[port].shift = (uint8_t)config_load_uint(key, 0);
			break;
		case 3:
			conf.port[port].enable = (uint8_t)config_load_uint(key, 0);
#ifdef WITH_YKW
			if (conf.port[port].enable) {
				conf.enabled |= (1 << port);
			}
#endif
			break;
		}
	}
}

static uint8_t config_phase_to_index(char *phase)
{
	if (strcmp("3p+n", phase) == 0) {
		return CONFIG_3PHASE_PLUS_N;
	} else if (strcmp("3p-n", phase) == 0) {
		return CONFIG_3PHASE_MINUS_N;
	} else {
		return CONFIG_1PHASE;
	}
}

static void config_load_main(void)
{
	char str_value[CONFIG_STR_MAX];

	conf.main.phase = config_load_str(CONFIG_UCI_PHASE, str_value) ?
		config_phase_to_index(str_value) : CONFIG_1PHASE;
	conf.main.led = (uint8_t)config_load_uint(CONFIG_UCI_LED_MODE,
	                                          CONFIG_LED_MODE_DEFAULT);
}

#ifdef WITH_YKW
static void config_load_theta(void)
{
	int theta_watt;

	theta_watt = config_load_uint(CONFIG_UCI_THETA, YKW_DEFAULT_THETA);
	conf.theta = conf.main.phase == CONFIG_3PHASE_MINUS_N ?
	                 theta_watt * 1000 / 133 : theta_watt * 1000 / 230;
}
#endif

static void config_load_batch(void)
{
	char serial[CONFIG_STR_MAX];

	if (config_load_str(CONFIG_UCI_SERIAL, serial)) {
		serial[4] = 0;
		conf.main.batch = strtoul(&serial[2], NULL, 10);
	}
}

static uint8_t config_math_to_index(char *math)
{
	if (strcmp("p2+p1", math) == 0) {
		return CONFIG_MATH_P2_PLUS_P1;
	} else if (strcmp("p1+p2+p3", math) == 0) {
		return CONFIG_MATH_P1_PLUS_P2_PLUS_P3;
	} else {
		return CONFIG_MATH_NONE;
	}
}

static void config_load_math(void)
{
	char math[CONFIG_STR_MAX];

	conf.main.math = config_load_str(CONFIG_UCI_MATH, math) ?
		config_math_to_index(math) : CONFIG_MATH_NONE;
#ifdef WITH_YKW
	if (conf.main.math == CONFIG_MATH_P2_PLUS_P1) {
		conf.masked |= (1 << CONFIG_PORT1);
	}
#endif
}

void config_push(void)
{
	flx_tx(FLX_TYPE_PORT_CONFIG, (unsigned char *)&conf.port,
	       sizeof(struct port) * CONFIG_MAX_PORTS + sizeof(struct main));
}

void config_load_kube(void)
{
	conf.kube.group = (uint8_t)config_load_uint(CONFIG_UCI_COLLECT_GRP,
	                                            CONFIG_COLLECT_GRP_DEFAULT);
}

void config_push_kube(void)
{
	flx_tx(FLX_TYPE_KUBE_CTRL, (unsigned char *)&conf.kube,
	       sizeof(struct kube));
}

#ifdef WITH_YKW
void config_push_ykw(char *json)
{
	json_object *jobj;
	struct uci_ptr ptr;
	char str[CONFIG_STR_MAX];

	jobj = json_tokener_parse(json);
	json_object_object_foreach(json_object_object_get(jobj, "config"), key, val) {
		if (strncmp(key, YKW_PARAM_PREFIX, strlen(YKW_PARAM_PREFIX)) != 0) {
			continue;
		}
		strncpy(str, key, CONFIG_STR_MAX);
		if (uci_lookup_ptr(conf.uci_ctx, &ptr, str, true) != UCI_OK) {
			uci_perror(conf.uci_ctx, key);
			continue;
		}
		if (!(ptr.flags & UCI_LOOKUP_COMPLETE)) {
			conf.uci_ctx->err = UCI_ERR_NOTFOUND;
			uci_perror(conf.uci_ctx, key);
			continue;
		}
		ptr.value = json_object_to_json_string(val);
		if (uci_set(conf.uci_ctx, &ptr) == UCI_OK) {
			uci_commit(conf.uci_ctx, &ptr.p, false);
		}
	}
	config_load_theta();
}
#endif

bool config_load_all(void)
{
	int i;

#ifdef WITH_YKW
	conf.enabled = 0;
	conf.masked = 0;
#endif
	if (!config_load_str(CONFIG_UCI_DEVICE, conf.device))
		return false;
	for (i = 0; i < CONFIG_MAX_SENSORS; i++) {
		if (!config_load_sensor(i)) {
			return false;
		}
	}
	for (i = 0; i < CONFIG_MAX_PORTS; i++) {
		config_load_port(i);
	}
	config_load_main();
#ifdef WITH_YKW
	config_load_theta();
#endif
	config_load_batch();
	config_load_math();
	config_push();
	spin(SPIN_100K_CYCLES);
	config_load_kube();
	config_push_kube();
	return true;
}
