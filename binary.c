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

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "binary.h"

const uint8_t bin2hex[] = {
	'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'
};

void hexlify(uint8_t *bin, uint8_t *hex, size_t len)
{
	size_t i;

	for (i = 0; i < len; i++) {
		hex[2 * i] = bin2hex[(bin[i] & 0xf0) >> 4];
		hex[2 * i + 1] = bin2hex[(bin[i] & 0x0f)];
	}
}

bool unhexlify(uint8_t *hex, uint8_t *bin, size_t len)
{
	size_t i;
	uint8_t c;

	for (i = 0; i < len / 2; i++) {
		c = hex[2 * i];
		if (c >= '0' && c <= '9') {
			bin[i] = (c - '0') << 4;
		} else if (c >= 'a' && c <= 'f') {
			bin[i] = (c - 'a' + 10) << 4;
		} else if (c >= 'A' && c <= 'F') {
			bin[i] = (c - 'A' + 10) << 4;
		} else {
			return false;
		}

		c = hex[2 * i + 1];
		if (c >= '0' && c <= '9') {
			bin[i] += c - '0';
		} else if (c >= 'a' && c <= 'f') {
			bin[i] += c - 'a' + 10;
		} else if (c >= 'A' && c <= 'F') {
			bin[i] += c - 'A' + 10;
		} else {
			return false;
		}
	}
	return true;
}

