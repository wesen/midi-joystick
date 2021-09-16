#include <avr/io.h>
#include "common.h"
#include "pattern.h"
#include "midi_clock.h"

#include "i2c_eeprom.h"

pattern_steps_t current_steps = {
  0, // num
  {
    16, // cv1_length
    {
#if 0
      { 1, 0 },
      { 1, 10 },
      { 1, 20 },
      { 1, 30 },
      { 1, 40 },
      { 1, 50 },
      { 1, 60 },
      { 1, 70 },
#endif
      
      { 0, 0 },
      { 0, 90 },
      { 0, 0 },
      { 0, 80 },
      { 0, 0 },
      { 0, 70 },
      { 0, 0 },
      { 0, 50 },

      { 1, 70 },
      { 1, 60 },
      { 1, 50 },
      { 1, 40 },
      { 1, 30 },
      { 1, 20 },
      { 1, 10 },
      { 1, 00 },
    },
    0, 0, 0, 0, 0,
    0,
  },

  {
    16, // cv2_length
    {
      { 0, 0 },
      { 0, 10 },
      { 0, 20 },
      { 0, 30 },
      { 0, 40 },
      { 0, 50 },
      { 0, 60 },
      { 0, 70 },
      
      { 0, 0 },
      { 0, 100 },
      { 0, 0 },
      { 0, 100 },
      { 0, 0 },
      { 0, 100 },
      { 0, 0 },
      { 0, 100 },
    },
    0, 0, 0, 0, 0,
    0
  },
};

#define PATTERN_EEPROM_SIZE 40

uint8_t pattern_read(uint8_t num) {
  uint16_t address = num * PATTERN_EEPROM_SIZE;
  uint8_t num2;
  if (!i2c_eeprom_read_byte(address, &num2) || (num != num2))
    return 0;
  
  if (!i2c_eeprom_read_byte(address + 1, &current_steps.cv1.len))
    return 0;
  if (!i2c_eeprom_read_bytes(address + 2, (uint8_t *)&current_steps.cv1.steps, 16))
    return 0;
  if (!i2c_eeprom_read_byte(address + 18, &current_steps.cv2.len))
    return 0;
  if (!i2c_eeprom_read_bytes(address + 19, (uint8_t *)&current_steps.cv2.steps, 16))
    return 0;
  return 1;
}

uint8_t pattern_write(uint8_t num) {
  uint16_t address = num * PATTERN_EEPROM_SIZE;
  if (!i2c_eeprom_write_byte(address, num))
    return 0;
  
  if (!i2c_eeprom_write_byte(address + 1, current_steps.cv1.len))
    return 0;
  if (!i2c_eeprom_write_bytes(address + 2, (uint8_t *)&current_steps.cv1.steps, 16))
    return 0;
  if (!i2c_eeprom_write_byte(address + 18, current_steps.cv2.len))
    return 0;
  if (!i2c_eeprom_write_bytes(address + 19, (uint8_t *)&current_steps.cv2.steps, 16))
    return 0;
  
  return 1;
}

void pattern_init(pattern_t *pattern, volatile uint16_t *addr) {
  pattern_reset(pattern);
  pattern->addr = addr;
}

void pattern_steps_init(void) {
  pattern_init(&current_steps.cv1, &OCR1B);
  pattern_init(&current_steps.cv2, &OCR1A);
}

void pattern_reset(pattern_t *pattern) {
  pattern->curstep = -1;
  pattern->nextstep = 0;
  pattern->inc = 0;
  pattern->curvalue = 0;
}

void pattern_inc_step(pattern_t *pattern) {
  pattern->curvalue += pattern->inc;

  if (pattern->inc < 0) {
    if (pattern->curvalue < pattern->nextvalue)
      pattern->curvalue = pattern->nextvalue;
  } else if (pattern->inc > 0) {
    if (pattern->curvalue > pattern->nextvalue)
      pattern->curvalue = pattern->nextvalue;
  }

  if (pattern->addr) {
    *(pattern->addr) = (pattern->curvalue >> 8) * 2;
  }
}

void pattern_step(pattern_t *pattern, uint16_t count) {
  pattern->curstep = INC_STEP(pattern, pattern->curstep);
  pattern->nextstep = INC_STEP(pattern, pattern->curstep);
  pattern->curvalue = (uint16_t)pattern->steps[pattern->curstep].value << 8;
  pattern->nextvalue = (uint16_t)pattern->steps[pattern->nextstep].value << 8;
  if (pattern->addr) {
    *(pattern->addr) = (pattern->curvalue >> 8) * 2;
  }

  if (pattern->steps[pattern->curstep].ramp) {
    pattern->inc = ((int32_t)pattern->nextvalue - (int32_t)pattern->curvalue) / count;
  } else {
    pattern->inc = 0;
  }
}

