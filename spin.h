#ifndef SPIN_H
#define SPIN_H

#define SPIN_100K_CYCLES	(100 * 1000)

static void spin(int cycles) {
	int i;

	for (i = 0; i < cycles; i++) {
		__asm__("NOP");
	}
}
#endif
