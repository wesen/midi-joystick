#include <stdio.h>
#include <unistd.h>

typedef unsigned char uint8_t;

/* max speed 12 hz */
/* slowest speed 0.12 hz */
/* speed from 1023 = 12 hz -> 0 = 0.12 hz */
unsigned int speed = 10;

/* y=log(1/((1024-x)*2)) */
/*  0.12 + ( x^2 * 11.88 / 1024^2) */

/* 0.12 + (log(1/((1024-x)*2)) - log(1/2048)) * ((12.0 - 0.12) / (log(1/2) - log(1/2048))) */

float updatefreq = 12.0;
float freq = 0.5;

#define BALKEN

int main(void) {
  int i, curstep = -1, nextstep  = 0;
  int countdown = updatefreq / freq;
  int inc = 0;
  int curvalue = 0;
  int cnt = 0;
  
  for (;;) {
    if (--cnt <= 0) {
      curstep = INC_STEP(curstep);
      nextstep = INC_STEP(curstep);
      cnt = countdown;
      curvalue = pattern[curstep].value;
      if (pattern[curstep].ramp) {
	inc = (pattern[nextstep].value - curvalue) / countdown;
      } else {
	inc = 0;
      }
    }
#ifdef BALKEN
    printf("\r                                                                        \r");
    int j;
    for (j = 0; j < curvalue / 3; j++)
      printf("*");
    fflush(stdout);
#else
    printf("new step %d, ramp %d, curvalue %d, inc %d, countdown %d\n", curstep, pattern[curstep].ramp, curvalue, inc, countdown);
#endif
    //    printf("curvalue %d\n", curvalue);
    curvalue += inc;
    usleep(1000000.0 / updatefreq);
  }
  return 0;
}
