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

#ifndef FLX_H
#define FLX_H

#include <libubox/uloop.h>

#define FLX_DEV "/dev/ttyS0"
#define FLX_BUFFER_SIZE 1024
#define FLX_BUFFER_PEEK_SIZE 4
#define FLX_PROTO_SYNC 0xaa
#define FLX_MAX_TYPES 10

enum flx_type {
	FLX_TYPE_PING,
	FLX_TYPE_PONG,
	FLX_TYPE_TIME_STAMP,
	FLX_TYPE_TIME_STEP,
	FLX_TYPE_TIME_SLEW,
	FLX_TYPE_DEBUG,
	FLX_TYPE_DEBUG_VOLTAGE1,
	FLX_TYPE_DEBUG_CURRENT1,
	FLX_TYPE_DEBUG_CURRENT2,
	FLX_TYPE_DEBUG_CURRENT3
};

enum flx_buffer_state {
	FLX_BUFFER_STATE_SYNC1,
	FLX_BUFFER_STATE_SYNC2,
	FLX_BUFFER_STATE_HEAD
};

struct buffer_s {
	size_t head;
	size_t tail;
	enum flx_buffer_state state;
    /* unsigned qualifier needed for comparison with FLX_PROTO_SYNC */
	unsigned char data[FLX_BUFFER_SIZE];
};

void flx_rx(struct uloop_fd *ufd, unsigned int events);
int flx_tx(unsigned char type, unsigned char *data, size_t len);

#endif
