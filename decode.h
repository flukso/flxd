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

#define DECODE_TOPIC_SAR "/device/%s/flx/sar/%d"
#define DECODE_TOPIC_SDADC "/device/%s/flx/sdadc/%d"
#define DECODE_TOPIC_VOLTAGE "/device/%s/flx/voltage/%d"
#define DECODE_TOPIC_CURRENT "/device/%s/flx/current/%d"
#define DECODE_TOPIC_TIME "/device/%s/flx/time"
#define DECODE_TOPIC_COUNTER "/sensor/%s/counter"
#define DECODE_TOPIC_GAUGE "/sensor/%s/gauge"

#define DECODE_NUM_SAMPLES 32
#define DECODE_SAR "[[%d,%d],["\
	"%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,"\
	"%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu],\"\"]"
#define DECODE_SDADC "[[%d,%d],["\
	"%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,"\
	"%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd],\"\"]"
#define DECODE_VOLTAGE "[[%d,%d],["\
	"%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,"\
	"%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld],\"mV\"]"
#define DECODE_CURRENT "[[%d,%d],["\
	"%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,"\
	"%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld],\"mA\"]"
#define DECODE_TIME "{flm:{time:[%d,%d],update:%s},flx:{time:[%d,%d],update:%s}}"
#define DECODE_COUNTER "[%d, %u, \"%s\"]"
#define DECODE_COUNTER_FRAC "[%d, %u.%03u, \"%s\"]"
#define DECODE_GAUGE "[%d, %d, \"%s\"]"
#define DECODE_GAUGE_FRAC "[%d, %d.%03u, \"%s\"]"
#define DECODE_11BIT_FRAC_MASK 0x000007FFUL
#define DECODE_20BIT_INTEG_MASK 0x7FFFF800UL
#define DECODE_SIGN_MASK 0x80000000UL

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

enum decode_ct_params {
	DECODE_CT_PARAM_PPLUS,
	DECODE_CT_PARAM_PMINUS,
	DECODE_CT_PARAM_Q1,
	DECODE_CT_PARAM_Q2,
	DECODE_CT_PARAM_Q3,
	DECODE_CT_PARAM_Q4,
	DECODE_CT_PARAM_VRMS,
	DECODE_CT_PARAM_IRMS,
	DECODE_CT_PARAM_PF,
	DECODE_CT_PARAM_VTHD,
	DECODE_CT_PARAM_ITHD,
	DECODE_CT_PARAM_ALPHA,
	DECODE_MAX_CT_PARAMS
};

const char *decode_ct_counter_unit[DECODE_CT_PARAM_Q4 + 1] = {
	"Wh",
	"Wh",
	"VARh",
	"VARh",
	"VARh",
	"VARh"
};

const char *decode_ct_gauge_unit[DECODE_MAX_CT_PARAMS] = {
	"W",
	"W",
	"VAR",
	"VAR",
	"VAR",
	"VAR",
	"V",
	"A",
	"",
	"",
	"",
	"°"
};

const char *decode_pulse_counter_unit[] = {
	"Wh",
	"L",
	"L",
	""
};

const char *decode_pulse_gauge_unit[] = {
	"W",
	"L/day",
	"L/day",
	""
};

const float decode_pulse_gauge_factor[] = {
	3.6e3f,
	24 * 3.6e3f,
	24 * 3.6e3f,
	1.0f
};

struct ct_data_s {
	uint32_t time;
	uint16_t millis;
	uint8_t port;
	uint8_t padding;
	uint32_t counter_integ[DECODE_CT_PARAM_Q4 + 1];
	uint16_t counter_frac[DECODE_CT_PARAM_Q4 + 1];
	int32_t gauge[DECODE_MAX_CT_PARAMS];
};

struct pulse_data_s {
	uint32_t time;
	uint16_t millis;
	uint8_t port;
	uint8_t padding;
	int32_t gauge; /* q20.11 */
	uint32_t counter_integ;
	uint16_t counter_millis;
};

struct sar_s {
	uint32_t time;
	uint16_t millis;
	uint16_t adc[DECODE_NUM_SAMPLES];
};

struct sdadc_s {
	uint32_t time;
	uint16_t millis;
	uint8_t index;
	uint8_t padding;
	int16_t adc[DECODE_NUM_SAMPLES];
};

struct voltage_s {
	uint32_t time;
	uint16_t millis;
	uint16_t padding;
	int32_t rms;
	int32_t sample[DECODE_NUM_SAMPLES];
};

struct current_s {
	uint32_t time;
	uint16_t millis;
	uint8_t index;
	uint8_t padding;
	int32_t rms;
	int32_t sample[DECODE_NUM_SAMPLES];
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

static bool decode_ping(struct buffer_s *b, struct decode_s *d)
{
	/* we only get pinged when no port config is present */
	config_push();
	return false;
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
	char topic[CONFIG_STR_MAX];
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
	snprintf(topic, CONFIG_STR_MAX, DECODE_TOPIC_TIME, conf.device);
	mosquitto_publish(conf.mosq, NULL, topic, d->len, d->data, conf.mqtt.qos,
	                  conf.mqtt.retain);
	return true;
}

static bool decode_voltage(struct buffer_s *b, struct decode_s *d)
{
	int i;
	char topic[CONFIG_STR_MAX];
	struct voltage_s v;

	d->dest = DECODE_DEST_MQTT;
	d->type = FLX_TYPE_VOLTAGE;
	decode_memcpy(b, (unsigned char *)&v);
	v.time = ltobl(v.time);
	v.millis = ltobs(v.millis);
	v.rms = ltobl(v.rms);
	for (i = 0; i < DECODE_NUM_SAMPLES; i++) {
		v.sample[i] = ltobl(v.sample[i]);
	}
	d->len = snprintf((char *)d->data,
	    DECODE_BUFFER_SIZE,
	    DECODE_VOLTAGE,
	    v.time,
	    v.millis,
	    (long)v.sample[0],
	    (long)v.sample[1],
	    (long)v.sample[2],
	    (long)v.sample[3],
	    (long)v.sample[4],
	    (long)v.sample[5],
	    (long)v.sample[6],
	    (long)v.sample[7],
	    (long)v.sample[8],
	    (long)v.sample[9],
	    (long)v.sample[10],
	    (long)v.sample[11],
	    (long)v.sample[12],
	    (long)v.sample[13],
	    (long)v.sample[14],
	    (long)v.sample[15],
	    (long)v.sample[16],
	    (long)v.sample[17],
	    (long)v.sample[18],
	    (long)v.sample[19],
	    (long)v.sample[20],
	    (long)v.sample[21],
	    (long)v.sample[22],
	    (long)v.sample[23],
	    (long)v.sample[24],
	    (long)v.sample[25],
	    (long)v.sample[26],
	    (long)v.sample[27],
	    (long)v.sample[28],
	    (long)v.sample[29],
	    (long)v.sample[30],
	    (long)v.sample[31]);
	snprintf(topic, CONFIG_STR_MAX, DECODE_TOPIC_VOLTAGE, conf.device, 1);
	mosquitto_publish(conf.mosq, NULL, topic, d->len, d->data, conf.mqtt.qos,
	                  conf.mqtt.retain);
#ifdef WITH_YKW
	if (ykw_process_voltage(conf.ykw, v.time, v.millis, v.rms, (long *)v.sample,
	                       DECODE_NUM_SAMPLES)) {
		snprintf(topic, CONFIG_STR_MAX, YKW_TOPIC_EVENT, conf.device);
		mosquitto_publish(conf.mosq, NULL, topic, conf.ykw->db.fill,
		                  conf.ykw->db.buffer, conf.mqtt.qos + 1,
		                  conf.mqtt.retain);
	}
#endif
	return true;
}

static bool decode_current(struct buffer_s *b, struct decode_s *d)
{
	int i;
	char topic[CONFIG_STR_MAX];
	struct current_s c;

	d->dest = DECODE_DEST_MQTT;
	d->type = FLX_TYPE_CURRENT;
	decode_memcpy(b, (unsigned char *)&c);
	c.time = ltobl(c.time);
	c.millis = ltobs(c.millis);
	c.rms = ltobl(c.rms);
	for (i = 0; i < DECODE_NUM_SAMPLES; i++) {
		c.sample[i] = ltobl(c.sample[i]);
	}
	d->len = snprintf((char *)d->data,
	    DECODE_BUFFER_SIZE,
	    DECODE_CURRENT,
	    c.time,
	    c.millis,
	    (long)c.sample[0],
	    (long)c.sample[1],
	    (long)c.sample[2],
	    (long)c.sample[3],
	    (long)c.sample[4],
	    (long)c.sample[5],
	    (long)c.sample[6],
	    (long)c.sample[7],
	    (long)c.sample[8],
	    (long)c.sample[9],
	    (long)c.sample[10],
	    (long)c.sample[11],
	    (long)c.sample[12],
	    (long)c.sample[13],
	    (long)c.sample[14],
	    (long)c.sample[15],
	    (long)c.sample[16],
	    (long)c.sample[17],
	    (long)c.sample[18],
	    (long)c.sample[19],
	    (long)c.sample[20],
	    (long)c.sample[21],
	    (long)c.sample[22],
	    (long)c.sample[23],
	    (long)c.sample[24],
	    (long)c.sample[25],
	    (long)c.sample[26],
	    (long)c.sample[27],
	    (long)c.sample[28],
	    (long)c.sample[29],
	    (long)c.sample[30],
	    (long)c.sample[31]);
	snprintf(topic, CONFIG_STR_MAX, DECODE_TOPIC_CURRENT, conf.device,
	         c.index + 1);
	mosquitto_publish(conf.mosq, NULL, topic, d->len, d->data, conf.mqtt.qos,
	                  conf.mqtt.retain);
#ifdef WITH_YKW
	ykw_process_current(conf.ykw, c.time, c.millis, c.index, c.rms,
	                    (long *)c.sample, DECODE_NUM_SAMPLES);
#endif
	return true;
}

static void decode_pub_counter(char *sid, uint32_t time, uint32_t counter,
                               uint16_t frac, const char *unit)
{
	int len;
	char topic[CONFIG_STR_MAX];
	char data[CONFIG_STR_MAX];

	snprintf(topic, CONFIG_STR_MAX, DECODE_TOPIC_COUNTER, sid);
	if (frac == 0) {
		len = snprintf(data, CONFIG_STR_MAX, DECODE_COUNTER, time, counter, unit);
	} else {
		len = snprintf(data, CONFIG_STR_MAX, DECODE_COUNTER_FRAC, time,
		               counter, frac, unit);
	}
	mosquitto_publish(conf.mosq, NULL, topic, len, data, conf.mqtt.qos,
	                  conf.mqtt.retain);
}

static void decode_pub_gauge(char *sid, uint32_t time, int32_t gauge,
                             uint16_t frac, const char *unit)
{
	int len;
	char topic[CONFIG_STR_MAX];
	char data[CONFIG_STR_MAX];

	snprintf(topic, CONFIG_STR_MAX, DECODE_TOPIC_GAUGE, sid);
	if (frac == 0) {
		len = snprintf(data, CONFIG_STR_MAX, DECODE_GAUGE, time, gauge, unit);
	} else {
		len = snprintf(data, CONFIG_STR_MAX, DECODE_GAUGE_FRAC, time,
		               gauge, frac, unit);
	}
	mosquitto_publish(conf.mosq, NULL, topic, len, data, conf.mqtt.qos,
	                  conf.mqtt.retain);
}

/* fractional to decimal conversion */
static uint16_t ftod(uint16_t frac, uint8_t width)
{
	return (uint16_t)((frac * 125) >> (width - 3));
}

static bool decode_ct_data(struct buffer_s *b, struct decode_s *d)
{
	int i, offset;
	int32_t integer;
	uint16_t decimal;
	struct ct_data_s ct;

	decode_memcpy(b, (unsigned char *)&ct);
	offset = ct.port * DECODE_MAX_CT_PARAMS;
	ct.time = ltobl(ct.time) - 1;
	ct.millis = ltobs(ct.millis);
	for (i = 0; i <= DECODE_CT_PARAM_Q4; i++) {
		decode_pub_counter(conf.sensor[offset + i].id,
		                   ct.time,
		                   ltobl(ct.counter_integ[i]),
		                   ftod(ltobs(ct.counter_frac[i]), 16),
		                   decode_ct_counter_unit[i]);
	}
	for (i = 0; i < DECODE_MAX_CT_PARAMS; i++) {
		ct.gauge[i] = ltobl(ct.gauge[i]);
		integer = ct.gauge[i] >> 11; /* ASR */
		decimal = ftod(ct.gauge[i] & DECODE_11BIT_FRAC_MASK, 11);
		decode_pub_gauge(conf.sensor[offset + i].id,
		                 ct.time,
		                 integer,
		                 decimal,
		                 decode_ct_gauge_unit[i]);
	}
	shift_push_alpha(ct.port, ct.gauge[DECODE_CT_PARAM_ALPHA]);
	return false;
}

static bool decode_pulse_data(struct buffer_s *b, struct decode_s *d)
{
	int offset, sensor;
	struct pulse_data_s pulse;
	float gauge_integ, gauge_frac;

	decode_memcpy(b, (unsigned char*)&pulse);
	offset = CONFIG_MAX_ANALOG_PORTS * DECODE_MAX_CT_PARAMS;
	sensor = offset + pulse.port - CONFIG_MAX_ANALOG_PORTS;
	pulse.time = ltobl(pulse.time);
	pulse.millis = ltobs(pulse.millis);
	decode_pub_counter(conf.sensor[sensor].id,
	                   pulse.time,
	                   ltobl(pulse.counter_integ),
	                   ltobs(pulse.counter_millis),
	                   decode_pulse_counter_unit[conf.sensor[sensor].type]);
	pulse.gauge = ltobl(pulse.gauge);
	if (pulse.gauge == 0) {
		return false;
	}
	gauge_frac = modff((float)pulse.gauge / 2048.0f *
	             decode_pulse_gauge_factor[conf.sensor[sensor].type], &gauge_integ);
	decode_pub_gauge(conf.sensor[sensor].id,
	                 pulse.time,
	                 (int32_t)gauge_integ,
	                 (uint16_t)fabsf(gauge_frac * 1000.0f),
	                 decode_pulse_gauge_unit[conf.sensor[sensor].type]);
	return false;
}

static bool decode_debug_sar(struct buffer_s *b, struct decode_s *d)
{
	int i;
	char topic[CONFIG_STR_MAX];
	struct sar_s sar;

	d->dest = DECODE_DEST_MQTT;
	d->type = FLX_TYPE_SAR;
	decode_memcpy(b, (unsigned char *)&sar);
	sar.time = ltobl(sar.time);
	sar.millis = ltobs(sar.millis);
	for (i = 0; i < DECODE_NUM_SAMPLES; i++) {
		sar.adc[i] = ltobs(sar.adc[i]);
	}
	d->len = snprintf((char *)d->data,
	    DECODE_BUFFER_SIZE,
	    DECODE_SAR,
	    sar.time,
	    sar.millis,
	    sar.adc[0],
	    sar.adc[1],
	    sar.adc[2],
	    sar.adc[3],
	    sar.adc[4],
	    sar.adc[5],
	    sar.adc[6],
	    sar.adc[7],
	    sar.adc[8],
	    sar.adc[9],
	    sar.adc[10],
	    sar.adc[11],
	    sar.adc[12],
	    sar.adc[13],
	    sar.adc[14],
	    sar.adc[15],
	    sar.adc[16],
	    sar.adc[17],
	    sar.adc[18],
	    sar.adc[19],
	    sar.adc[20],
	    sar.adc[21],
	    sar.adc[22],
	    sar.adc[23],
	    sar.adc[24],
	    sar.adc[25],
	    sar.adc[26],
	    sar.adc[27],
	    sar.adc[28],
	    sar.adc[29],
	    sar.adc[30],
	    sar.adc[31]);
	snprintf(topic, CONFIG_STR_MAX, DECODE_TOPIC_SAR, conf.device, 1);
	mosquitto_publish(conf.mosq, NULL, topic, d->len, d->data, conf.mqtt.qos,
	                  conf.mqtt.retain);
	return true;
}

static bool decode_debug_sdadc(struct buffer_s *b, struct decode_s *d)
{
	int i;
	char topic[CONFIG_STR_MAX];
	struct sdadc_s sdadc;

	d->dest = DECODE_DEST_MQTT;
	d->type = FLX_TYPE_SDADC;
	decode_memcpy(b, (unsigned char *)&sdadc);
	sdadc.time = ltobl(sdadc.time);
	sdadc.millis = ltobs(sdadc.millis);
	for (i = 0; i < DECODE_NUM_SAMPLES; i++) {
		sdadc.adc[i] = ltobs(sdadc.adc[i]);
	}
	d->len = snprintf((char *)d->data,
	    DECODE_BUFFER_SIZE,
	    DECODE_SDADC,
	    sdadc.time,
	    sdadc.millis,
	    sdadc.adc[0],
	    sdadc.adc[1],
	    sdadc.adc[2],
	    sdadc.adc[3],
	    sdadc.adc[4],
	    sdadc.adc[5],
	    sdadc.adc[6],
	    sdadc.adc[7],
	    sdadc.adc[8],
	    sdadc.adc[9],
	    sdadc.adc[10],
	    sdadc.adc[11],
	    sdadc.adc[12],
	    sdadc.adc[13],
	    sdadc.adc[14],
	    sdadc.adc[15],
	    sdadc.adc[16],
	    sdadc.adc[17],
	    sdadc.adc[18],
	    sdadc.adc[19],
	    sdadc.adc[20],
	    sdadc.adc[21],
	    sdadc.adc[22],
	    sdadc.adc[23],
	    sdadc.adc[24],
	    sdadc.adc[25],
	    sdadc.adc[26],
	    sdadc.adc[27],
	    sdadc.adc[28],
	    sdadc.adc[29],
	    sdadc.adc[30],
	    sdadc.adc[31]);
	snprintf(topic, CONFIG_STR_MAX, DECODE_TOPIC_SDADC, conf.device,
	         sdadc.index + 1);
	mosquitto_publish(conf.mosq, NULL, topic, d->len, d->data, conf.mqtt.qos,
	                  conf.mqtt.retain);
	return true;
}


static const decode_fun decode_handler[] = {
	decode_ping,
	decode_pong,
	decode_void, /* port config */
	decode_time_stamp,
	decode_void, /* time step */
	decode_void, /* time slew */
	decode_voltage,
	decode_current,
	decode_ct_data,
	decode_pulse_data,
	decode_void, /* TODO debug */
	decode_debug_sar,
	decode_debug_sdadc,
};

#endif
