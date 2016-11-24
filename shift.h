#ifndef SHIFT_H
#define SHIFT_H

#define SHIFT_TOPIC			"/device/%s/debug/flx/shift"
#define SHIFT_DATA_TPL		"[%d, [%u, %u, %u], \"\"]"
#define SHIFT_UCI			"flx"
#define SHIFT_UCI_SET_TPL	"flx.%d.shift=%u"
#define SHIFT_SORT_DEBUG	"[shift] sequence={%d,%d,%d}\n"
#define SHIFT_DEBUG			"[shift] alpha[%d]=%d shift=%d\n"

void shift_init(void);
void shift_push_params(int port, int32_t alpha, int32_t irms);
void shift_calculate(void);

#endif

