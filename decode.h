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

#define DECODE_NUM_SAMPLES 32
#define DECODE_SERIES_VOLTAGE "[[%lu,%hu],["\
	"%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,"\
	"%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu],\"V\"]"
#define DECODE_SERIES_CURRENT "[[%lu,%hu],["\
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

struct voltage_s {
	unsigned int time;
	unsigned short millis;
	unsigned short adc[DECODE_NUM_SAMPLES];
};

struct current_s {
	unsigned int time;
	unsigned short millis;
	short adc[DECODE_NUM_SAMPLES];
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

static bool decode_voltage(struct buffer_s *b, struct decode_s *d)
{
	int i, len;
	char topic[FLXD_STR_MAX];
	struct voltage_s dv;

	d->dest = DECODE_DEST_MQTT;
	d->type = FLX_TYPE_VOLTAGE1;
	len = b->data[(b->tail + 1) % FLX_BUFFER_SIZE];
	for (i = 0; i < len; i++) {
		*((unsigned char *)&dv + i) = b->data[(b->tail + 2 + i) % FLX_BUFFER_SIZE];
	}
	dv.time = ltobl(dv.time);
	dv.millis = ltobs(dv.millis);
	for (i = 0; i < DECODE_NUM_SAMPLES; i++) {
		dv.adc[i] = ltobs(dv.adc[i]);
	}
	d->len = snprintf((char *)d->data,
	    DECODE_BUFFER_SIZE,
	    DECODE_SERIES_VOLTAGE,
	    (unsigned long)dv.time,
	    dv.millis,
	    dv.adc[0],
	    dv.adc[1],
	    dv.adc[2],
	    dv.adc[3],
	    dv.adc[4],
	    dv.adc[5],
	    dv.adc[6],
	    dv.adc[7],
	    dv.adc[8],
	    dv.adc[9],
	    dv.adc[10],
	    dv.adc[11],
	    dv.adc[12],
	    dv.adc[13],
	    dv.adc[14],
	    dv.adc[15],
	    dv.adc[16],
	    dv.adc[17],
	    dv.adc[18],
	    dv.adc[19],
	    dv.adc[20],
	    dv.adc[21],
	    dv.adc[22],
	    dv.adc[23],
	    dv.adc[24],
	    dv.adc[25],
	    dv.adc[26],
	    dv.adc[27],
	    dv.adc[28],
	    dv.adc[29],
	    dv.adc[30],
	    dv.adc[31]);
	snprintf(topic, FLXD_STR_MAX, DECODE_TOPIC_VOLTAGE, conf.device,
	         d->type - FLX_TYPE_VOLTAGE1 + 1);
	mosquitto_publish(conf.mosq, NULL, topic, d->len, d->data, conf.mqtt.qos,
	                  conf.mqtt.retain);
#ifdef WITH_YKW
	ykw_process_voltage(conf.ykw, dv.time, dv.millis, dv.adc, DECODE_NUM_SAMPLES);
#endif
	return true;
}

static bool decode_current(struct buffer_s *b, struct decode_s *d)
{
	int i, len;
	char topic[FLXD_STR_MAX];
	struct current_s dc;

	d->dest = DECODE_DEST_MQTT;
	d->type = b->data[b->tail]; /* FLX_TYPE_CURRENT[1|2|3] */
	len = b->data[(b->tail + 1) % FLX_BUFFER_SIZE];
	for (i = 0; i < len; i++) {
		*((unsigned char *)&dc + i) = b->data[(b->tail + 2 + i) % FLX_BUFFER_SIZE];
	}
	dc.time = ltobl(dc.time);
	dc.millis = ltobs(dc.millis);
	for (i = 0; i < DECODE_NUM_SAMPLES; i++) {
		dc.adc[i] = ltobs(dc.adc[i]);
	}
	d->len = snprintf((char *)d->data,
	    DECODE_BUFFER_SIZE,
	    DECODE_SERIES_CURRENT,
	    (unsigned long)dc.time,
	    dc.millis,
	    dc.adc[0],
	    dc.adc[1],
	    dc.adc[2],
	    dc.adc[3],
	    dc.adc[4],
	    dc.adc[5],
	    dc.adc[6],
	    dc.adc[7],
	    dc.adc[8],
	    dc.adc[9],
	    dc.adc[10],
	    dc.adc[11],
	    dc.adc[12],
	    dc.adc[13],
	    dc.adc[14],
	    dc.adc[15],
	    dc.adc[16],
	    dc.adc[17],
	    dc.adc[18],
	    dc.adc[19],
	    dc.adc[20],
	    dc.adc[21],
	    dc.adc[22],
	    dc.adc[23],
	    dc.adc[24],
	    dc.adc[25],
	    dc.adc[26],
	    dc.adc[27],
	    dc.adc[28],
	    dc.adc[29],
	    dc.adc[30],
	    dc.adc[31]);
	snprintf(topic, FLXD_STR_MAX, DECODE_TOPIC_CURRENT, conf.device,
	         d->type - FLX_TYPE_CURRENT1 + 1);
	mosquitto_publish(conf.mosq, NULL, topic, d->len, d->data, conf.mqtt.qos,
	                  conf.mqtt.retain);
#ifdef WITH_YKW
	ykw_process_current(conf.ykw, dc.time, dc.millis,
	    d->type - FLX_TYPE_CURRENT1, dc.adc, DECODE_NUM_SAMPLES);
#endif
	return true;
}


static const decode_fun decode_handler[] = {
	decode_void, /* TODO ping */
	decode_pong,
	decode_void, /* TODO time stamp */
	decode_void, /* TODO time step */
	decode_void, /* TODO time slew */
	decode_void, /* TODO debug */
	decode_voltage,
	decode_current, /* FLX_TYPE_CURRENT1 */
	decode_current, /* FLX_TYPE_CURRENT2 */
	decode_current  /* FLX_TYPE_CURRENT3 */
};

#endif
