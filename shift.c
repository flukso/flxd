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
#include <sys/time.h>
#include "config.h"
#include "shift.h"

const uint8_t map_shift_1p[] = { 0, 0, 3, 3, 3, 0 };
int32_t alpha[CONFIG_MAX_ANALOG_PORTS] = { 0 };
int32_t irms[CONFIG_MAX_ANALOG_PORTS] = { 0 };

void shift_init(void)
{
	int i;

	for (i = 0; i < CONFIG_MAX_ANALOG_PORTS; i++) {
		alpha[i] = 0;
		irms[i] = 0;
	}
}

static int32_t shift_q20dot11_to_int(int32_t x)
{
	return x >= 0 ? (x + (1 << 10)) >> 11 : (x - (1 << 10)) >> 11;
}

void shift_push_params(int port, int32_t a, int32_t i)
{
	if (port < CONFIG_MAX_ANALOG_PORTS) {
		alpha[port] = shift_q20dot11_to_int(a);
		irms[port] = i;
	}
}

static void shift_uci_commit(void)
{
	int i;
	struct uci_ptr ptr;
	char str[CONFIG_STR_MAX];

	for (i = 0; i < CONFIG_MAX_ANALOG_PORTS; i++) {
		snprintf(str, CONFIG_STR_MAX, SHIFT_UCI_SET_TPL, i + 1,
		         conf.port[i].shift);
		if (uci_lookup_ptr(conf.uci_ctx, &ptr, str, true) != UCI_OK) {
			uci_perror(conf.uci_ctx, str);
			exit(9);
		}
		uci_set(conf.uci_ctx, &ptr);
		uci_save(conf.uci_ctx, ptr.p);
	}
	if (uci_lookup_ptr(conf.uci_ctx, &ptr, SHIFT_UCI, true) != UCI_OK) {
		uci_perror(conf.uci_ctx, SHIFT_UCI);
		exit(10);
	}
	uci_commit(conf.uci_ctx, &ptr.p, false);
}

static void shift_pub(void)
{
	int len;
	char topic[CONFIG_STR_MAX];
	char data[CONFIG_STR_MAX];
	struct timeval t;

	if (gettimeofday(&t, NULL) != 0) {
		return;
	}	
	snprintf(topic, CONFIG_STR_MAX, SHIFT_TOPIC, conf.device);
	len = snprintf(data, CONFIG_STR_MAX, SHIFT_DATA_TPL, (int)t.tv_sec,
	               conf.port[0].shift, conf.port[1].shift, conf.port[2].shift);
	mosquitto_publish(conf.mosq, NULL, topic, len, data, conf.mqtt.qos,
	                  conf.mqtt.retain);
}

static int32_t shift_calculate_shift(int32_t a)
{
	a += 30;
	if (a < 0) {
		a += 360;
	}
	return a / 60;
}

static void shift_calculate_1p(void)
{
	int i;

	for (i = 0; i < CONFIG_MAX_ANALOG_PORTS; i++) {
		if (conf.port[i].enable == 0) {
			continue;
		}
		conf.port[i].shift = map_shift_1p[shift_calculate_shift(alpha[i])];
		if (conf.verbosity > 0) {
			fprintf(stdout, SHIFT_DEBUG, i, alpha[i], conf.port[i].shift);
		}
	}
}

static void swap(int *el0, int *el1)
{
	int tmp;

	tmp = *el0;
	*el0 = *el1;
	*el1 = tmp;
}

static void sort_irms_desc(int *el0, int *el1)
{
	if (irms[*el0] < irms[*el1]) {
		swap(el0, el1);
	}
}

static bool is_leading(int alpha0, int alpha1)
{
	int angle;

	angle = alpha0 - alpha1;
	if (angle < 0) {
		angle += 360;
	}
	return angle < 180 ? true : false;
}

static void shift_calculate_3p(void)
{
	int i;
	int seq[CONFIG_MAX_ANALOG_PORTS] = {0, 1, 2};

	sort_irms_desc(&seq[0], &seq[1]);
	sort_irms_desc(&seq[1], &seq[2]);
	sort_irms_desc(&seq[0], &seq[1]);
	if (conf.verbosity > 0) {
		fprintf(stdout, SHIFT_SORT_DEBUG, seq[0], seq[1], seq[2]);
	}
	conf.port[seq[0]].shift = shift_calculate_shift(alpha[seq[0]]);
	if (is_leading(alpha[seq[0]], alpha[seq[1]])) {
		conf.port[seq[1]].shift = (conf.port[seq[0]].shift + 4) % 6;
		conf.port[seq[2]].shift = (conf.port[seq[0]].shift + 2) % 6;
	} else {
		conf.port[seq[1]].shift = (conf.port[seq[0]].shift + 2) % 6;
		conf.port[seq[2]].shift = (conf.port[seq[0]].shift + 4) % 6;
	}
	if (conf.verbosity > 0) {
		for (i = 0; i < CONFIG_MAX_ANALOG_PORTS; i++) {
			fprintf(stdout, SHIFT_DEBUG, i, alpha[i], conf.port[i].shift);
		}
	}
}


void shift_calculate(void)
{
	if (conf.main.phase == CONFIG_1PHASE) {
		shift_calculate_1p();
	} else {
		shift_calculate_3p();
	}
	shift_uci_commit();	
	config_push();
	shift_pub();
}

