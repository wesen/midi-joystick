#include "app.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>

#include "common.h"

int main(void) {
  SET_BIT8(DDRB, PB0);
  CLEAR_BIT8(DDRB, PB4);
  SET_BIT8(PORTB, PB4);

  for (;;) {
    if (IS_BIT_SET8(PINB, PB4)) {
      _delay_ms(500);
      SET_BIT8(PORTB, PB0);
      _delay_ms(500);
      CLEAR_BIT8(PORTB, PB0);
    }
  }
}
