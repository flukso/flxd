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

#ifndef ENCODE_H
#define ENCODE_H

#define ENCODE_BUFFER_SIZE 128
#define ENCODE_SYNC_TL_LEN 4
#define ENCODE_FLETCHER16_LEN 2

struct encode_s {
	unsigned char type;
	const unsigned char *data;
	size_t len;
};

static inline unsigned short encode_fletcher16(unsigned char *data, size_t bytes)
{
	unsigned short sum1 = 0xff, sum2 = 0xff;

	while (bytes) {
		size_t tlen = bytes > 20 ? 20 : bytes;
		bytes -= tlen;
		do {
			sum2 += sum1 += *data++;
		} while (--tlen);
		sum1 = (sum1 & 0xff) + (sum1 >> 8);
		sum2 = (sum2 & 0xff) + (sum2 >> 8);
	}
	/* Second reduction step to reduce sums to 8 bits */
	sum1 = (sum1 & 0xff) + (sum1 >> 8);
	sum2 = (sum2 & 0xff) + (sum2 >> 8);
	return sum2 << 8 | sum1;
}

static inline void encode_handler(struct encode_s *e, unsigned char *telegram)
{
	*(telegram + 0) = 0xaa;
	*(telegram + 1) = 0xaa;
	*(telegram + 2) = e->type;
	*(telegram + 3) = e->len;
	for (int i = 0; i < e->len; i++) {
		*(telegram + ENCODE_SYNC_TL_LEN + i) = *(e->data + i);
	}
	unsigned short check = encode_fletcher16(telegram + 2, e->len + 2);
	*(telegram + 4 + e->len) = (check >> 8) & 0xff;
	*(telegram + 5 + e->len) = check & 0xff;
}

#endif
