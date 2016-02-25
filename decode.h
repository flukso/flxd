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

#ifndef DECODE_H
#define DECODE_H

#define DECODE_BUFFER_SIZE 1024

#define DECODE_TOPIC_VOLTAGE "/device/%s/debug/flx/voltage/%d"
#define DECODE_TOPIC_CURRENT "/device/%s/debug/flx/current/%d"

#define DECODE_DEBUG_SAMPLES 32
#define DECODE_DEBUG_SERIES_VOLTAGE "[[%lu,%hu],["\
	"%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,"\
	"%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu],\"V\"]"
#define DECODE_DEBUG_SERIES_CURRENT "[[%lu,%hu],["\
	"%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,"\
	"%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd],\"A\"]"

#define ltobs(A) ((((uint16_t)(A) & 0xff00) >> 8) | \
	              (((uint16_t)(A) & 0x00ff) << 8))
#define ltobl(A) ((((uint32_t)(A) & 0xff000000) >> 24) | \
	              (((uint32_t)(A) & 0x00ff0000) >> 8)  | \
	              (((uint32_t)(A) & 0x0000ff00) << 8)  | \
	              (((uint32_t)(A) & 0x000000ff) << 24))

enum decode_dest {
	DECODE_DEST_DAEMON,
	DECODE_DEST_FLX,
	DECODE_DEST_UBUS,
	DECODE_DEST_MQTT
};

struct decode_s {
	enum decode_dest dest;
	unsigned char type;
	unsigned char data[DECODE_BUFFER_SIZE];
	size_t len;
};

struct debug_voltage_s {
	unsigned int time;
	unsigned short seq;
	unsigned short adc[DECODE_DEBUG_SAMPLES];
};

struct debug_current_s {
	unsigned int time;
	unsigned short seq;
	short adc[DECODE_DEBUG_SAMPLES];
};

typedef bool (*decode_fun)(struct buffer_s *, struct decode_s *);

static bool decode_void(struct buffer_s *b, struct decode_s *d)
{
	return false;
}

static bool decode_pong(struct buffer_s *b, struct decode_s *d)
{
	int i;
	d->dest = DECODE_DEST_DAEMON;
	d->type = FLX_TYPE_PONG;
	d->len = b->data[(b->tail + 1) % FLX_BUFFER_SIZE];
	for (i = 0; i < d->len; i++) {
		d->data[i] = b->data[(b->tail + 2 + i) % FLX_BUFFER_SIZE];
	}
	return true;
}

static bool decode_debug_voltage(struct buffer_s *b, struct decode_s *d)
{
	int i, len;
	char topic[FLXD_STR_MAX];
	struct debug_voltage_s dv;

	d->dest = DECODE_DEST_MQTT;
	d->type = FLX_TYPE_DEBUG_VOLTAGE1;
	len = b->data[(b->tail + 1) % FLX_BUFFER_SIZE];
	for (i = 0; i < len; i++) {
		*((unsigned char *)&dv + i) = b->data[(b->tail + 2 + i) % FLX_BUFFER_SIZE];
	}
	d->len = snprintf((char *)d->data,
	    DECODE_BUFFER_SIZE,
	    DECODE_DEBUG_SERIES_VOLTAGE,
	    (unsigned long)ltobl(dv.time),
	    ltobs(dv.seq),
	    ltobs(dv.adc[0]),
	    ltobs(dv.adc[1]),
	    ltobs(dv.adc[2]),
	    ltobs(dv.adc[3]),
	    ltobs(dv.adc[4]),
	    ltobs(dv.adc[5]),
	    ltobs(dv.adc[6]),
	    ltobs(dv.adc[7]),
	    ltobs(dv.adc[8]),
	    ltobs(dv.adc[9]),
	    ltobs(dv.adc[10]),
	    ltobs(dv.adc[11]),
	    ltobs(dv.adc[12]),
	    ltobs(dv.adc[13]),
	    ltobs(dv.adc[14]),
	    ltobs(dv.adc[15]),
	    ltobs(dv.adc[16]),
	    ltobs(dv.adc[17]),
	    ltobs(dv.adc[18]),
	    ltobs(dv.adc[19]),
	    ltobs(dv.adc[20]),
	    ltobs(dv.adc[21]),
	    ltobs(dv.adc[22]),
	    ltobs(dv.adc[23]),
	    ltobs(dv.adc[24]),
	    ltobs(dv.adc[25]),
	    ltobs(dv.adc[26]),
	    ltobs(dv.adc[27]),
	    ltobs(dv.adc[28]),
	    ltobs(dv.adc[29]),
	    ltobs(dv.adc[30]),
	    ltobs(dv.adc[31]));
	snprintf(topic, FLXD_STR_MAX, DECODE_TOPIC_VOLTAGE, conf.device,
	         d->type - FLX_TYPE_DEBUG_VOLTAGE1 + 1);
	mosquitto_publish(conf.mosq, NULL, topic, d->len, d->data, conf.mqtt.qos,
	                  conf.mqtt.retain);
	return true;
}

static bool decode_debug_current(struct buffer_s *b, struct decode_s *d)
{
	int i, len;
	char topic[FLXD_STR_MAX];
	struct debug_current_s dc;

	d->dest = DECODE_DEST_MQTT;
	d->type = b->data[b->tail]; /* FLX_TYPE_DEBUG_CURRENT[1|2|3] */
	len = b->data[(b->tail + 1) % FLX_BUFFER_SIZE];
	for (i = 0; i < len; i++) {
		*((unsigned char *)&dc + i) = b->data[(b->tail + 2 + i) % FLX_BUFFER_SIZE];
	}
	d->len = snprintf((char *)d->data,
	    DECODE_BUFFER_SIZE,
	    DECODE_DEBUG_SERIES_CURRENT,
	    (unsigned long)ltobl(dc.time),
	    ltobs(dc.seq),
	    ltobs(dc.adc[0]),
	    ltobs(dc.adc[1]),
	    ltobs(dc.adc[2]),
	    ltobs(dc.adc[3]),
	    ltobs(dc.adc[4]),
	    ltobs(dc.adc[5]),
	    ltobs(dc.adc[6]),
	    ltobs(dc.adc[7]),
	    ltobs(dc.adc[8]),
	    ltobs(dc.adc[9]),
	    ltobs(dc.adc[10]),
	    ltobs(dc.adc[11]),
	    ltobs(dc.adc[12]),
	    ltobs(dc.adc[13]),
	    ltobs(dc.adc[14]),
	    ltobs(dc.adc[15]),
	    ltobs(dc.adc[16]),
	    ltobs(dc.adc[17]),
	    ltobs(dc.adc[18]),
	    ltobs(dc.adc[19]),
	    ltobs(dc.adc[20]),
	    ltobs(dc.adc[21]),
	    ltobs(dc.adc[22]),
	    ltobs(dc.adc[23]),
	    ltobs(dc.adc[24]),
	    ltobs(dc.adc[25]),
	    ltobs(dc.adc[26]),
	    ltobs(dc.adc[27]),
	    ltobs(dc.adc[28]),
	    ltobs(dc.adc[29]),
	    ltobs(dc.adc[30]),
	    ltobs(dc.adc[31]));
	snprintf(topic, FLXD_STR_MAX, DECODE_TOPIC_CURRENT, conf.device,
	         d->type - FLX_TYPE_DEBUG_CURRENT1 + 1);
	mosquitto_publish(conf.mosq, NULL, topic, d->len, d->data, conf.mqtt.qos,
	                  conf.mqtt.retain);
	return true;
}


static const decode_fun decode_handler[] = {
	decode_void, /* TODO ping */
	decode_pong,
	decode_void, /* TODO time stamp */
	decode_void, /* TODO time step */
	decode_void, /* TODO time slew */
	decode_void, /* TODO debug */
	decode_debug_voltage,
	decode_debug_current, /* FLX_TYPE_DEBUG_CURRENT1 */
	decode_debug_current, /* FLX_TYPE_DEBUG_CURRENT2 */
	decode_debug_current  /* FLX_TYPE_DEBUG_CURRENT3 */
};

#endif
