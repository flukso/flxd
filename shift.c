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

#include <sys/time.h>
#include "config.h"
#include "shift.h"

int32_t alpha[CONFIG_MAX_ANALOG_PORTS];

static int32_t shift_q20dot11_to_int(int32_t x)
{
	return x >= 0 ? (x + (1 << 10)) >> 11 : (x - (1 << 10)) >> 11;
}

void shift_push_alpha(int port, int32_t a)
{
	alpha[port] = shift_q20dot11_to_int(a);
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

void shift_calculate(void)
{
	int i;
	int32_t a;

	for (i = 0; i < CONFIG_MAX_ANALOG_PORTS; i++) {
		a = alpha[i] + 30;
		if (a < 0) {
			a += 360;
		}
		conf.port[i].shift = a / 60;
	}
	shift_uci_commit();	
	config_push();
	shift_pub();
}

