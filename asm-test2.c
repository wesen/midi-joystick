#include <avr/io.h>

__attribute__ ((noinline)) uint8_t f1(uint8_t i, uint8_t j) {
  //  return i + j;
  uint8_t ret;
  asm volatile("add %0, %1\n\t"
	       "mov %2, %0\n\t"
	       : "=r" (i), "=r" (j) : "r" (ret));
  return ret;
}

void blorg(void) {
  uint8_t i = f1(1, 3);
  PORTB = i;
}
