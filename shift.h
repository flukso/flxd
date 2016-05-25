#ifndef SHIFT_H
#define SHIFT_H

#define SHIFT_TOPIC			"/device/%s/debug/flx/shift"
#define SHIFT_DATA_TPL		"[%d, [%u, %u, %u], \"\"]"
#define SHIFT_UCI			"flx"
#define SHIFT_UCI_SET_TPL	"flx.%d.shift=%u"
#define SHIFT_DEBUG			"[shift] alpha[%d]=%d a=%d shift=%d\n"

void shift_push_alpha(int port, int32_t alpha);
void shift_calculate(void);

#endif

