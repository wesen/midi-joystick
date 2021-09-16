#include "app.h"

#include <avr/pgmspace.h>

#include "common.h"

void my_memcpy(void *dst, void *src, uint8_t cnt) {
  while (cnt--) {
    *((uint8_t *)dst++) = *((uint8_t *)src++);
  }
}

void my_memcpy_p(void *dst, PGM_P src, uint8_t cnt) {
  while (cnt--) {
    *((uint8_t *)dst++) = pgm_read_byte(src);
    src++;
  }
}


void my_strncpy(void *dst, char *src, uint8_t cnt) {
  while (cnt-- && *src) {
    *((uint8_t *)dst++) = *((uint8_t *)src++);
  }
}

void my_strncpy_fill(void *dst, char *src, uint8_t cnt) {
  while (cnt && *src) {
    *((uint8_t *)dst++) = *((uint8_t *)src++);
    cnt--;
  }
  while (cnt--) {
    *((uint8_t *)dst++) = ' ';
  }
}

void my_strncpy_p(void *dst, PGM_P src, uint8_t cnt) {
  while (cnt--) {
    char byte = pgm_read_byte(src);
    if (byte == 0)
      break;
    *((uint8_t *)dst++) = byte;
    src++;
  }
}

void my_strncpy_p_fill(void *dst, PGM_P src, uint8_t cnt) {
  while (cnt) {
    char byte = pgm_read_byte(src);
    if (byte == 0)
      break;
    *((uint8_t *)dst++) = byte;
    src++;
    cnt--;
  }
  while (cnt--) {
    *((uint8_t *)dst++) = ' ';
  }
}

void my_memclr(void *dst, uint8_t cnt) {
  while (cnt--)
    *((uint8_t *)dst++) = 0;
}

void my_str16cpy_fill(void *dst, char *src) {
  my_strncpy_fill(dst, src, 16);
}

void my_str16cpy_p_fill(void *dst, PGM_P src) {
  my_strncpy_p_fill(dst, src, 16);
}

void my_str16cpy_p(void *dst, PGM_P src) {
  my_strncpy_p(dst, src, 16);
}
