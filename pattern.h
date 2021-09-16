#ifndef PATTERN_H__
#define PATTERN_H__

#include "common.h"

typedef struct step_s {
  unsigned char ramp : 1;
  unsigned char value : 7;
} step_t;

#define INC_STEP(pat, step) ((step + 1) % (pat)->len)
// (pat)->len)
typedef struct pattern_s {
  uint8_t len;
  step_t steps[16];
  uint8_t curstep;
  uint8_t nextstep;
  int32_t curvalue;
  int32_t nextvalue;
  int32_t inc;
  volatile uint16_t *addr;
} pattern_t;

typedef struct pattern_steps_s {
  uint8_t num;
  pattern_t cv1;
  pattern_t cv2;
} PACKED pattern_steps_t;

extern pattern_steps_t current_steps;

void pattern_steps_init(void);
void pattern_init(pattern_t *pattern, volatile uint16_t *addr);
void pattern_reset(pattern_t *pattern);
void pattern_inc_step(pattern_t *pattern);
void pattern_step(pattern_t *pattern, uint16_t count);

uint8_t pattern_read(uint8_t num);
uint8_t pattern_write(uint8_t num);

#endif /* PATTERN_H__ */
