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
#define DECODE_TS_THRESHOLD 1234567890

#define DECODE_TOPIC_VOLTAGE "/device/%s/debug/flx/voltage/%d"
#define DECODE_TOPIC_CURRENT "/device/%s/debug/flx/current/%d"
#define DECODE_TOPIC_TIME "/device/%s/debug/flx/time"

#define DECODE_NUM_SAMPLES 32
#define DECODE_VOLTAGE "[[%d,%d],["\
	"%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,"\
	"%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu],\"V\"]"
#define DECODE_CURRENT "[[%d,%d],["\
	"%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,"\
	"%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd],\"A\"]"
#define DECODE_TIME "{flm:{time:[%d,%d],update:%s},flx:{time:[%d,%d],update:%s}}"

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

static int decode_memcpy(struct buffer_s *b, unsigned char *sink)
{
	int i, len;

	len = b->data[(b->tail + 1) % FLX_BUFFER_SIZE];
	for (i = 0; i < len; i++) {
		*((unsigned char *)sink + i) = b->data[(b->tail + 2 + i) %
		                               FLX_BUFFER_SIZE];
	}
	return len;
}

static bool decode_pong(struct buffer_s *b, struct decode_s *d)
{
	d->dest = DECODE_DEST_DAEMON;
	d->type = FLX_TYPE_PONG;
	d->len = decode_memcpy(b, d->data);
	return true;
}

static time_t time_delta(struct timeval *t1, struct timeval *t2)
{
	struct timeval delta;

	delta.tv_sec = t1->tv_sec - t2->tv_sec;
	delta.tv_usec = t1->tv_usec - t2->tv_usec;
	if (delta.tv_usec < 0) {
		delta.tv_sec--;
		delta.tv_usec += 1000000;
	}
	return delta.tv_sec >= 0 ? delta.tv_sec : delta.tv_sec + 1;
}

static bool time_threshold(struct timeval *t)
{
	return t->tv_sec > DECODE_TS_THRESHOLD ? true : false;
}

static bool decode_time_stamp(struct buffer_s *b, struct decode_s *d)
{
	char topic[FLXD_STR_MAX];
	time_t t = 0;
	struct timeval t_flm, t_flx = {0, 0};
	char* flm_update = "false";
	char* flx_update = "false";

	if (gettimeofday(&t_flm, NULL) != 0)
		return false;
	d->len = decode_memcpy(b, (unsigned char *)&t_flx.tv_sec);
	t_flx.tv_sec = ltobl(t_flx.tv_sec);
	if (!time_threshold(&t_flm) && time_threshold(&t_flx)) {
		settimeofday(&t_flx, NULL);
		flm_update = "true";
	} else if (time_threshold(&t_flm) && abs(time_delta(&t_flx, &t_flm)) > 0) {
		t = ltobl(t_flm.tv_sec);
		flx_tx(FLX_TYPE_TIME_STEP, (unsigned char *)&t, sizeof(t));
		flx_update = "true";
	}
	d->dest = DECODE_DEST_MQTT;
	d->type = FLX_TYPE_TIME_STAMP;
	d->len = snprintf((char *)d->data,
	    DECODE_BUFFER_SIZE,
	    DECODE_TIME,
	    (int)t_flm.tv_sec,
	    (int)t_flm.tv_usec,
	    flm_update,
	    (int)t_flx.tv_sec,
	    (int)t_flx.tv_usec,
	    flx_update);
	snprintf(topic, FLXD_STR_MAX, DECODE_TOPIC_TIME, conf.device);
	mosquitto_publish(conf.mosq, NULL, topic, d->len, d->data, conf.mqtt.qos,
	                  conf.mqtt.retain);
	return true;
}

static bool decode_voltage(struct buffer_s *b, struct decode_s *d)
{
	int i;
	char topic[FLXD_STR_MAX];
	struct voltage_s v;

	d->dest = DECODE_DEST_MQTT;
	d->type = FLX_TYPE_VOLTAGE1;
	decode_memcpy(b, (unsigned char *)&v);
	v.time = ltobl(v.time);
	v.millis = ltobs(v.millis);
	for (i = 0; i < DECODE_NUM_SAMPLES; i++) {
		v.adc[i] = ltobs(v.adc[i]);
	}
	d->len = snprintf((char *)d->data,
	    DECODE_BUFFER_SIZE,
	    DECODE_VOLTAGE,
	    v.time,
	    v.millis,
	    v.adc[0],
	    v.adc[1],
	    v.adc[2],
	    v.adc[3],
	    v.adc[4],
	    v.adc[5],
	    v.adc[6],
	    v.adc[7],
	    v.adc[8],
	    v.adc[9],
	    v.adc[10],
	    v.adc[11],
	    v.adc[12],
	    v.adc[13],
	    v.adc[14],
	    v.adc[15],
	    v.adc[16],
	    v.adc[17],
	    v.adc[18],
	    v.adc[19],
	    v.adc[20],
	    v.adc[21],
	    v.adc[22],
	    v.adc[23],
	    v.adc[24],
	    v.adc[25],
	    v.adc[26],
	    v.adc[27],
	    v.adc[28],
	    v.adc[29],
	    v.adc[30],
	    v.adc[31]);
	snprintf(topic, FLXD_STR_MAX, DECODE_TOPIC_VOLTAGE, conf.device,
	         d->type - FLX_TYPE_VOLTAGE1 + 1);
	mosquitto_publish(conf.mosq, NULL, topic, d->len, d->data, conf.mqtt.qos,
	                  conf.mqtt.retain);
#ifdef WITH_YKW
	ykw_process_voltage(conf.ykw, v.time, v.millis, v.adc, DECODE_NUM_SAMPLES);
#endif
	return true;
}

static bool decode_current(struct buffer_s *b, struct decode_s *d)
{
	int i;
	char topic[FLXD_STR_MAX];
	struct current_s c;

	d->dest = DECODE_DEST_MQTT;
	d->type = b->data[b->tail]; /* FLX_TYPE_CURRENT[1|2|3] */
	decode_memcpy(b, (unsigned char *)&c);
	c.time = ltobl(c.time);
	c.millis = ltobs(c.millis);
	for (i = 0; i < DECODE_NUM_SAMPLES; i++) {
		c.adc[i] = ltobs(c.adc[i]);
	}
	d->len = snprintf((char *)d->data,
	    DECODE_BUFFER_SIZE,
	    DECODE_CURRENT,
	    c.time,
	    c.millis,
	    c.adc[0],
	    c.adc[1],
	    c.adc[2],
	    c.adc[3],
	    c.adc[4],
	    c.adc[5],
	    c.adc[6],
	    c.adc[7],
	    c.adc[8],
	    c.adc[9],
	    c.adc[10],
	    c.adc[11],
	    c.adc[12],
	    c.adc[13],
	    c.adc[14],
	    c.adc[15],
	    c.adc[16],
	    c.adc[17],
	    c.adc[18],
	    c.adc[19],
	    c.adc[20],
	    c.adc[21],
	    c.adc[22],
	    c.adc[23],
	    c.adc[24],
	    c.adc[25],
	    c.adc[26],
	    c.adc[27],
	    c.adc[28],
	    c.adc[29],
	    c.adc[30],
	    c.adc[31]);
	snprintf(topic, FLXD_STR_MAX, DECODE_TOPIC_CURRENT, conf.device,
	         d->type - FLX_TYPE_CURRENT1 + 1);
	mosquitto_publish(conf.mosq, NULL, topic, d->len, d->data, conf.mqtt.qos,
	                  conf.mqtt.retain);
#ifdef WITH_YKW
	ykw_process_current(conf.ykw, c.time, c.millis,
	    d->type - FLX_TYPE_CURRENT1, c.adc, DECODE_NUM_SAMPLES);
#endif
	return true;
}


static const decode_fun decode_handler[] = {
	decode_void, /* TODO ping */
	decode_pong,
	decode_time_stamp,
	decode_void, /* time step */
	decode_void, /* time slew */
	decode_void, /* TODO debug */
	decode_voltage,
	decode_current, /* FLX_TYPE_CURRENT1 */
	decode_current, /* FLX_TYPE_CURRENT2 */
	decode_current  /* FLX_TYPE_CURRENT3 */
};

#endif
