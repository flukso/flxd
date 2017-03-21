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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>
#include <mosquitto.h>
#include "binary.h"
#include "spin.h"
#include "config.h"
#include "shift.h"
#include "flx.h"
#include "decode.h"
#include "encode.h"

const char* state2string[] = {
	"sync1", "sync2", "head"
};

static inline void flx_buffer_peek(struct buffer_s *b, unsigned char *peek)
{
	int i;
	for (i = 0; i < FLX_BUFFER_PEEK_SIZE; i++) {
		*(peek + 2 * i) =
		    bin2hex[(b->data[(b->tail + i) % FLX_BUFFER_SIZE] & 0xf0) >> 4];
		*(peek + 2 * i + 1) =
		    bin2hex[b->data[(b->tail + i) % FLX_BUFFER_SIZE] & 0x0f];
	}
	*(peek + FLX_BUFFER_PEEK_SIZE * 2) = 0;
}

static void flx_buffer_dbg(struct buffer_s *b)
{
	unsigned char peek[FLX_BUFFER_PEEK_SIZE * 2 + 1];
	flx_buffer_peek(b, peek);
	fprintf(stdout,
	    "buffer[size:%d, fill:%d, head:%d, tail:%d, state:%s, peek:%s]\n",
	    sizeof(b->data),
		(FLX_BUFFER_SIZE + b->head - b->tail) % FLX_BUFFER_SIZE,
	    b->head,
	    b->tail,
		state2string[b->state],
		peek);
}

static void flx_buffer_advance_head(struct buffer_s *b, size_t n)
{
	b->head = (b->head + n) % FLX_BUFFER_SIZE;
}

static void flx_buffer_advance_tail(struct buffer_s *b, size_t n)
{
	b->tail = (b->tail + n) % FLX_BUFFER_SIZE;
}

static size_t flx_buffer_fill(struct buffer_s *b)
{
	return (FLX_BUFFER_SIZE + b->head - b->tail) % FLX_BUFFER_SIZE;
}

static size_t flx_buffer_max_read(struct buffer_s *b)
{
	size_t phy_free, log_free;

	/* we don't want head == tail since that would indicate an empty buffer */
	log_free = FLX_BUFFER_SIZE - flx_buffer_fill(b) - 1;
	phy_free = FLX_BUFFER_SIZE - b->head;
	return (log_free < phy_free) ? log_free : phy_free;
}

static int flx_buffer_is_empty(struct buffer_s *b)
{
	return b->head == b->tail;
}

static inline int flx_check_fletcher16(struct buffer_s *b)
{
	size_t pos = b->tail;
	size_t bytes = b->data[(b->tail + 1) % FLX_BUFFER_SIZE] + 2;
	unsigned short sum1 = 0xff, sum2 = 0xff;

	while (bytes) {
		size_t tlen = bytes > 20 ? 20 : bytes;
		bytes -= tlen;
		do {
			sum2 += sum1 += b->data[pos++];
			pos %= FLX_BUFFER_SIZE;
		} while (--tlen);
		sum1 = (sum1 & 0xff) + (sum1 >> 8);
		sum2 = (sum2 & 0xff) + (sum2 >> 8);
	}
	/* Second reduction step to reduce sums to 8 bits */
	sum1 = (sum1 & 0xff) + (sum1 >> 8);
	sum2 = (sum2 & 0xff) + (sum2 >> 8);

	if (b->data[pos] == sum2 && b->data[(pos + 1) % FLX_BUFFER_SIZE] == sum1) {
		return 1;
	}
	return 0;
}

static void flx_decode(struct buffer_s *b)
{
	struct decode_s d;
	unsigned char type = b->data[b->tail];
	if (type >= FLX_MAX_TYPES || !decode_handler[type](b, &d)) {
		return;
	}
	if (conf.verbosity > 1) {
		d.data[d.len] = '\0';
		fprintf(stdout, "[dest:%d] [type:%d] [len:%d] %s\n",
		        d.dest, d.type, d.len, d.data);
	}
}

static void flx_pop(struct buffer_s *b)
{
	size_t packet_size;

	while (!flx_buffer_is_empty(b)) {
		switch (b->state) {
		case FLX_BUFFER_STATE_SYNC1:
			if (b->data[b->tail] == FLX_PROTO_SYNC) {
				b->state = FLX_BUFFER_STATE_SYNC2;
			}
			flx_buffer_advance_tail(b, 1);
			break;
		case FLX_BUFFER_STATE_SYNC2:
			if (b->data[b->tail] == FLX_PROTO_SYNC) {
				b->state = FLX_BUFFER_STATE_HEAD;
			} else {
				b->state = FLX_BUFFER_STATE_SYNC1;
			}
			flx_buffer_advance_tail(b, 1);
			break;
		case FLX_BUFFER_STATE_HEAD:
			if (flx_buffer_fill(b) < 4) {
				return;
			}
			packet_size = b->data[(b->tail + 1) % FLX_BUFFER_SIZE] + 4;
			if (flx_buffer_fill(b) < packet_size) {
				return;
			}
			if (flx_check_fletcher16(b)) {
				flx_decode(b);
			} else if (conf.verbosity > 0) {
				fprintf(stdout, "[flx] fletcher16 checksum error\n");
			}
			b->state = FLX_BUFFER_STATE_SYNC1;
			flx_buffer_advance_tail(b, packet_size);
			break;
		}
	}
}

void flx_rx(struct uloop_fd *ufd, unsigned int events)
{
	ssize_t bytes_read;
	static struct buffer_s b = {
		.head = 0,
		.tail = 0,
		.state = FLX_BUFFER_STATE_SYNC1
	};

	bytes_read = read(ufd->fd, &b.data[b.head], flx_buffer_max_read(&b));	
	flx_buffer_advance_head(&b, bytes_read);
	if (conf.verbosity > 2) {
		flx_buffer_dbg(&b);
	}
	flx_pop(&b);
}

int flx_tx(unsigned char type, unsigned char *data, size_t len)
{
	unsigned char telegram[ENCODE_BUFFER_SIZE];
	struct encode_s e = (struct encode_s) {
		.type = type,
		.data = data,
		.len = len
	};

	if (e.len > ENCODE_BUFFER_SIZE - ENCODE_SYNC_TL_LEN -
	            ENCODE_FLETCHER16_LEN) {
		return -2;
	}
	encode_handler(&e, telegram);
	return write(conf.flx_ufd.fd, telegram, len + ENCODE_SYNC_TL_LEN +
	      ENCODE_FLETCHER16_LEN);
}

