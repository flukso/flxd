#ifndef CONFIG_H
#define CONFIG_H

#include <libubox/uloop.h>

struct config {
	int verbosity;
	struct uloop_fd flx_ufd;
};

extern struct config conf;

#endif
