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

#include "config.h"

bool config_init(void)
{
	return (conf.uci_ctx = uci_alloc_context()) ? true : false;
}

static bool config_load_param(char *param, char *value)
{
	struct uci_ptr ptr;
	char str[FLXD_STR_MAX];

	strncpy(str, param, FLXD_STR_MAX);
	if (uci_lookup_ptr(conf.uci_ctx, &ptr, str, FLXD_UCI_EXTENDED) != UCI_OK) {
		uci_perror(conf.uci_ctx, param);
		return false;
	}
	if (!(ptr.flags & UCI_LOOKUP_COMPLETE)) {
		conf.uci_ctx->err = UCI_ERR_NOTFOUND;
		uci_perror(conf.uci_ctx, param);
		return false;
	}
	strncpy(value, ptr.o->v.string, FLXD_STR_MAX);
	if (conf.verbosity > 0) {
		fprintf(stdout, "%s=%s\n", param, value);
	}
	return true;
}

bool config_load_all(void)
{
	int i;
	char param[FLXD_STR_MAX];

	if (!config_load_param(FLXD_UCI_DEVICE, conf.device))
		return false;
	for (i = 0; i < FLXD_SID_MAX; i++) {
		snprintf(param, FLXD_STR_MAX, FLXD_UCI_SID_TPL, i + 1);
		if (!config_load_param(param, conf.sid[i]))
			return false;
	}
	return true;
}

