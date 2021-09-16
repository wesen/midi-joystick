#include "app.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>

#include "common.h"
#include "midi-kontrol.h"
#include "lcd.h"
#include "uart.h"
#include "midi-small.h"
#include "i2c.h"
#include "i2c_eeprom.h"
#include "sr165.h"
#include "midi-kontrol.h"
#include "kontrol-sysex.h"

enum { UP, DOWN };

#define OC1A PB1
#define OC1B PB2
#define OC2 PB3
#define DDROC DDRB
#define TIMER1_TOP 1023
#define TIMER2_TOP 255


/* clock handling */
volatile uint16_t timer2_clock = 0;
volatile uint16_t timer2_slowclock = 0;

void midi_cc_callback(uint8_t msg[3]) {
}

uint8_t ack_msg[6] = {
  0xf0, 0x00, 0x13, 0x37, CMD_DATA_BLOCK_ACK, 0xf7
};

void midi_main_sysex_send_ack(void) {
  uint8_t i;
  for (i = 0; i < sizeof(ack_msg); i++)
    uart_putc(ack_msg[i]);
}

void (*bootloader_start)(void) = (void(*)(void))0x1800;

void midi_sysex_callback(void) {
  if (sysex_cnt < 4)
    return;
  if ((sysex_data[0] == SYSEX_VENDOR_1) &&
      (sysex_data[1] == SYSEX_VENDOR_2) &&
      (sysex_data[2] == SYSEX_VENDOR_3)) {
    if (sysex_data[3] == CMD_START_BOOTLOADER) {
      lcd_line1();
      lcd_puts_p_fill(pstr_loading_boot, 16);
      lcd_line2();
      lcd_clear_line();
      cli();
      eeprom_write_word(START_MAIN_APP_ADDR, 0);
      wdt_enable(WDTO_15MS);
      for (;;)
	;
    } else if (sysex_data[3] == CMD_SEND_PAGE) {
      receive_page_sysex();
    } else if (sysex_data[3] == CMD_SAVE_PAGE) {
      save_page_sysex();
    } else if (sysex_data[3] == CMD_GET_PATCH && sysex_data[4] < 8) {
      send_patch_sysex(sysex_data[4]);
    }
  }
}

/** Main routine for midi-link. **/
int main(void) {
  /** Disable watchdog. **/
  wdt_disable();

  /* move interrupts to bootloader section */
  GICR = _BV(IVCE);
  GICR = 0;

  DDRB |= _BV(PB0);
  DDRC |= _BV(PC3);

  lcd_init();
  i2c_init();
  sr165_init();
  uart_init();
  //  pwm_init();

  factory_reset();

  /** Enable interrupts. **/
  sei();

  lcd_line1();
  lcd_puts("FACTORY RESET");
  lcd_line2();
  lcd_puts("PLEASE RELOAD");
  
  for (;;) {
    while (uart_avail()) {
      uint8_t c = uart_getc();
      handle_midi_rx2(c);
    }
  }
}

