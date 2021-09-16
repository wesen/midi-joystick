#include <inttypes.h>
#include <avr/io.h>

volatile uint8_t data[64];

typedef struct foo_s {
  uint8_t num1;
  uint8_t num2;
  uint8_t num3;
} foo_t;

typedef struct foo2_s {
  uint8_t num1;
  foo_t foo[8];
} foo2_t;

foo2_t foo2s[16] = { {0} };
foo_t foos[16];

#if 0
foo_t *get_foo2_foo(uint8_t i) {
  return foo2s[i].foo;
}
#endif

extern foo_t *get_foo2_foo(uint8_t i) __attribute__((naked));
extern foo2_t *get_foo2(uint8_t i) __attribute__((naked)); 


/* 146 in C */
uint8_t f3(uint8_t val) {
  uint8_t i, j;
  // register declaration makes no difference (aber trotzdem in 26, dann rueber kopiert
  //  foo2_t *foo2_ptr = foo2s;
  for (i = 0; i < 16; i++) {
    register foo2_t *foo2_ptr asm("r30") = 0;
    foo2_ptr = get_foo2(i);
#if 0
    asm volatile("mov r25, %0"    "\n\t"
		 "rcall get_foo2" "\n\t"
		 : "=r" (i) : : "r30", "r31");
#endif

    // register declaration makes no difference
    foo_t *foo_ptr   = foo2_ptr[i].foo;
    //    foo_t *foo_ptr = get_foo2_foo(i);

    //    foo2s[i].num1 = val;
    
    /* 128 bytes with foo2_ptr */
    /* 120 bytes with foo2_ptr and foo_ptr */
    //    foo_t *foo_ptr = foo2s->foo;

    foo2_ptr[i].num1 = val;
    
    for (j = 0; j < 8; j++) {
#if 1
      foo_ptr[j].num1 = val;
      foo_ptr[j].num2 = val;
      foo_ptr[j].num3 = val;
#endif
#if 0
      // bigger 
      foo2_ptr->foo[j].num1 = val;
      foo2_ptr->foo[j].num2 = val;
      foo2_ptr->foo[j].num3 = val;
      foo2_ptr->num1 = val;
      foo2_ptr++;
#endif

#if 0
      foo2_ptr[i].foo[j].num1 = val;
      foo2_ptr[i].foo[j].num2 = val;
      foo2_ptr[i].foo[j].num3 = val;
      foo2_ptr[i].num1 = val;
#endif

#if 0
      foo2s[i].foo[j].num1 = val;
      foo2s[i].foo[j].num2 = val;
      foo2s[i].foo[j].num3 = val;
#endif
    }
  }
  return 0;
}

uint8_t f2(uint8_t val) {
  uint8_t i;
  //  register foo_t *foo_ptr asm("r30");
  foo_t *foo_ptr;
  
  foo_ptr = foos;
  for (i = 0; i < 16; i++) {
    foo_ptr[i].num1 = val;
    foo_ptr[i].num2 = val;
    foo_ptr[i].num3 = val;
  }

  return foos[0].num1;
}

/* wieso data nicht in Z ausserhalb des loops */

uint8_t f1(void) {
  uint8_t i;
  register uint8_t *ptr asm("r30") = (uint8_t *)data;
  for (i = 0; i < 64; i++) {
    *ptr++ = i;
  }
  return i;
}

int main(void) {
  volatile uint8_t foo;
  asm volatile("rcall f1" "\n\t"
	       "mov %0, r24" "\n\t"
	       : "=r" (foo));
  asm volatile("mov r24, %0" "\n\t"
	       "rcall f2" "\n\t"
	       : : "r" (foo));
  
  for (;;)
    ;

  return 0;
}
