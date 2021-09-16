#include "app.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>

#include "common.h"
#include "hardware.h"
#include "uart.h"
#include "midi-small.h"
#include "sr165.h"
#include "monomachine.h"

void joystick_routine(void);

button_t buttons[NUM_BUTTONS];

uint8_t base_channel = 0;
uint8_t current_channel = 0;
uint8_t tmp_channels[6] = { 0 };
uint8_t mutes[6] = { 0 };
uint8_t allup = 1;
uint8_t tmpled = 0;

typedef enum monojoystick_mode_e {
  normal_mode = 0,
  mute_mode = 1,
  randomizer_mode = 2,
  configuration_mode = 3
} monojoystick_mode_t;

monojoystick_mode_t current_mode = normal_mode;


#ifdef MIDI_CLOCK_ENABLE
midi_clock_t midi_clock;
#endif

/* clock handling */
volatile uint16_t timer2_slowclock = 0;

uint16_t clock_diff(uint16_t old_clock, uint16_t new_clock) {
  if (new_clock >= old_clock)
    return new_clock - old_clock;
  else
    return new_clock + (65535 - old_clock);
}

void midi_cc_callback(uint8_t msg[3]) {
  uint8_t track = 0;
  uint8_t param = 0;
  elektron_cc_to_param(msg[0] & 0xF, msg[1], &track, &param);

  if (param != 255) {
    if (BUTTON_DOWN(SELECT_BUTTON)) {
      uint8_t i;
      for (i = 0; i < 6; i++) {
	elektron_send_param(i, param, msg[2]);
      }
    } else {
      uint8_t i;
      for (i = 0; i < 6; i++) {
	if (tmp_channels[i]) {
	  elektron_send_param(i, param, msg[2]);
	}
      }
    }
  }
}

ISR(TIMER2_OVF_vect) {
  timer2_slowclock++;
}

void adc_init(void) {
  DDRC = 0x00;
  CLEAR_BIT(PORTC, PC0);
  CLEAR_BIT(PORTC, PC1);
  ADCSRA = _BV(ADPS0) | _BV(ADPS1);
  ADCSRA |= _BV(ADEN);
  ADMUX = _BV(ADLAR); // channel 0
  //  ADCSRA |= _BV(ADSC) | _BV(ADFR); // start first conversion, free running
}

/** Read an ADC value. **/
uint8_t read_adc(uint8_t adc) {
  ADMUX = _BV(ADLAR) | adc;
  ADCSRA |= _BV(ADSC);
  loop_until_bit_is_set(ADCSRA, ADIF);
  loop_until_bit_is_clear(ADCSRA, ADSC);
  return ADCH;
}

void pwm_init(void) {
  //  TCCR1A = _BV(WGM10); 
  //  TCCR1B |= _BV(CS10) | _BV(WGM12);
  TCCR2 = _BV(WGM20) | _BV(WGM21) | _BV(CS22);
  TIMSK |= _BV(TOIE2);
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

void end_ruinwesen_sysex(void) {
  if (sysex_buflen >= 1) {
    if (sysex_buf[0] == CMD_START_BOOTLOADER) {
      uart_flush();
      uint8_t i;
      for (i = 0; i < 20; i++) {
	_delay_ms(10);
      }
      cli();
      eeprom_write_word(START_MAIN_APP_ADDR, 0);
      wdt_enable(WDTO_15MS);
      for (;;)
	;
    }
  }
}

uint8_t old_x = 0;
uint8_t old_y = 0;

uint16_t last_x = 0;
uint16_t last_y = 0;

#define JOYSTICK_DELAY 5

void put_joystick_x(uint16_t val, uint8_t channel) {
  uart_putc(0xE0 | (base_channel + channel));
  uart_putc(val & 0x7F);
  uart_putc((val >> 7) & 0x7F);
}

void put_joystick_y(uint8_t y, uint8_t channel) {
  uart_putc(0xB0 | (channel + base_channel));
  if (y >= 128) {
    uart_putc(1);
    uart_putc(y - 128);
    
  } else {
    uart_putc(2);
    uart_putc(127 - y);
  }
}

void put_default_joystick(uint8_t channel) {
  put_joystick_x(0x2000, channel);
  put_joystick_y(128, channel);
}

void handle_joystick(void) {
  //  uint8_t x = read_adc(0);
  uint8_t x = -(read_adc(0) + 1);
  uint8_t y = -(read_adc(1) + 1);
  // delay shiz
  uint8_t tmp = SREG;
  uint16_t sum = (2 * x + old_x) / 3;
  cli();
  if ((ABS((int16_t)sum - (int16_t)old_x)) &&
      (clock_diff(last_x, timer2_slowclock) > JOYSTICK_DELAY)) {
    old_x = sum;
    
    uint16_t val = x << 6;
    if (allup) {
      put_joystick_x(val, current_channel);
    } else {
      uint8_t i;
      for (i = 0; i < 6; i++) {
	if (tmp_channels[i])
	  put_joystick_x(val, i);
      }
    }
    last_x = timer2_slowclock;
    old_x = sum;
  }
  sum = (2 * y + old_y) / 3;

  if (1 && (ABS((int16_t)sum - (int16_t)old_y)) &&
      (clock_diff(last_y, timer2_slowclock) > JOYSTICK_DELAY)) {
    if (allup) {
      put_joystick_y(y, current_channel);
    } else {
      uint8_t i;
      for (i = 0; i < 6; i++) {
	if (tmp_channels[i])
	  put_joystick_y(y, i);
      }
    }
    last_y = timer2_slowclock;
    old_y = y;
  }
  sei();
  //    old_y = y;
}  

#define SET_LED(i) (CLEAR_BIT(PORTD, i+2))
#define CLEAR_LED(i) (SET_BIT(PORTD, i+2))

typedef struct debounce_s {
  uint16_t stamp;
  uint8_t status;
} debounce_t;

debounce_t debounce[NUM_BUTTONS];
#define DEBOUNCE_TIME 1

uint8_t dbounce(uint8_t but, debounce_t *bounce, uint8_t num) {
  uint16_t time;
  uint8_t i;
  uint8_t dbut = 0;

  cli();
  time = timer2_slowclock;
  sei();

  for (i = 0; i < num; i++) {
    if (IS_BIT_SET(but, i) != bounce[i].status) {
      if (clock_diff(bounce[i].stamp, time) > 100) {
	bounce[i].stamp = time;
	bounce[i].status = IS_BIT_SET(but, i);
      }
    }
    if (bounce[i].status) {
      SET_BIT(dbut, i);
    } else {
      CLEAR_BIT(dbut, i);
    }
  }
  return dbut;
}
  

/** Main routine for midi-link. **/
int main(void) {
  /** Disable watchdog. **/
  wdt_disable();

  /* move interrupts to bootloader section */
  GICR = _BV(IVCE);
  GICR = 0;

  DDRB |= _BV(PB0);
  DDRD |= _BV(PD2) | _BV(PD3) | _BV(PD4) | _BV(PD5) | _BV(PD6) | _BV(PD7);
  CLEAR_BIT(PORTB, PB0);

  SET_BIT(PORTC, PC3);
  SET_BIT(PORTC, PC2);

  base_channel = eeprom_read_byte(BASE_CHANNEL_ADDR);
  if (base_channel >= 16) {
    base_channel = 0;
    merge_enabled = 0;
    eeprom_write_byte(BASE_CHANNEL_ADDR, base_channel);
    eeprom_write_byte(MERGE_ENABLED_ADDR, merge_enabled);
  }
  merge_enabled = eeprom_read_byte(MERGE_ENABLED_ADDR);
  
  SET_LED(current_channel);
  
  adc_init();
  pwm_init();
  sr165_init();
  uart_init();

  /** Enable interrupts. **/
  sei();

  for (;;) {
    if (!(merging && merge_enabled)) {
      uint8_t but = sr165_read();
      uint8_t i;
      
      handle_buttons(dbounce(but, debounce, 8), buttons, 8);

      // PC2 = joystick, PC3 = mute
      but = (IS_BIT_SET(PINC, PC2) << 1) | (IS_BIT_SET(PINC, PC3));
      handle_buttons(dbounce(but, debounce + 8, 2), buttons + 8, 2);
      
      if ((current_mode == normal_mode) || (current_mode == mute_mode)) {
	handle_joystick();
      }
      
      joystick_routine();
      mono_routine();
      clear_buttons();
    }
    
    while (uart_avail()) {
      uint8_t c = uart_getc();
      handle_midi_rx2(c);
    }
  }
}

#define SET_LED_TMP(i) (CLEAR_BIT(tmpled, i+2))
#define CLEAR_LED_TMP(i) (SET_BIT(tmpled, i+2))

void joystick_normal(void) {

  if ((BUTTON_PRESSED(MUTE_BUTTON) && BUTTON_DOWN(SELECT_BUTTON) && BUTTON_DOWN(SHIFT_BUTTON)) ||
      (BUTTON_DOWN(MUTE_BUTTON) && BUTTON_PRESSED(SELECT_BUTTON) && BUTTON_DOWN(SHIFT_BUTTON)) ||
      (BUTTON_DOWN(MUTE_BUTTON) && BUTTON_DOWN(SELECT_BUTTON) && BUTTON_PRESSED(SHIFT_BUTTON))) {
    current_mode = configuration_mode;
    return;
  }

  if ((BUTTON_PRESSED(MUTE_BUTTON) && BUTTON_DOWN(SHIFT_BUTTON)) ||
      (BUTTON_DOWN(MUTE_BUTTON) && BUTTON_PRESSED(SHIFT_BUTTON))) {
    current_mode = mute_mode;
    return;
  }

  if (BUTTON_DOWN(MUTE_BUTTON) && !BUTTON_DOWN(SELECT_BUTTON)) {
    joystick_mute();
    return;
  }
  
  uint8_t i;
  allup = 1;
  if (!BUTTON_DOWN(SELECT_BUTTON) && !BUTTON_DOWN(SHIFT_BUTTON)) {
    for (i = 0; i < 6; i++) {
      tmp_channels[i] = 0;
      if (BUTTON_DOWN(i)) {
	allup = 0;
	tmp_channels[i] = 1;
      }
    }
  } else {
    allup = 1;
    for (i = 0; i < 6; i++) {
      tmp_channels[i] = 0;
      if (BUTTON_PRESSED(i)) {
	if (BUTTON_DOWN(SELECT_BUTTON)) {
	  current_channel = i;
	} else if (BUTTON_DOWN(SHIFT_BUTTON)) {
	  reload_track = i;
	  mono_status = reload_track_get_current_kit;
	}
      }
    }
  }

  if (BUTTON_DOWN(MUTE_BUTTON) && BUTTON_DOWN(SELECT_BUTTON)) {
    allup = 0;
    for (i = 0; i < 6; i++)
      tmp_channels[i] = 1;
  }

  for (i = 0; i < 6; i++) {
    if (allup) {
      if (i == current_channel)
	SET_LED_TMP(i);
      else
	CLEAR_LED_TMP(i);
    } else {
      if (tmp_channels[i])
	SET_LED_TMP(i);
      else
	CLEAR_LED_TMP(i);
    }
  }

  if ((BUTTON_DOWN(SHIFT_BUTTON) && BUTTON_PRESSED(SELECT_BUTTON)) ||
      (BUTTON_PRESSED(SHIFT_BUTTON) && BUTTON_DOWN(SELECT_BUTTON))) {
    mono_status = reload_kit_get_current_kit;
  }

}

void joystick_mute(void) {
  allup = 1;
  if ((BUTTON_PRESSED(SHIFT_BUTTON) && BUTTON_DOWN(MUTE_BUTTON)) ||
      (BUTTON_DOWN(MUTE_BUTTON) && BUTTON_PRESSED(SHIFT_BUTTON))) {
    current_mode = normal_mode;
    return;
  }

  uint8_t i;
  allup = 1;

  if ((current_mode == normal_mode) ||
      (!BUTTON_DOWN(MUTE_BUTTON) && !BUTTON_DOWN(SELECT_BUTTON) && !BUTTON_DOWN(SHIFT_BUTTON))) {
    for (i = 0; i < 6; i++) {
      if (BUTTON_PRESSED(i)) {
	mutes[i] = !mutes[i];
	if (!BUTTON_DOWN(SELECT_BUTTON)) {
	  uart_putc(0xB0 | (i + base_channel));
	  uart_putc(0x03);
	  uart_putc(mutes[i]);
	}
      }
      
      if (mutes[i])
	CLEAR_LED_TMP(i);
      else
	SET_LED_TMP(i);
    }
  } else {
    if (BUTTON_DOWN(MUTE_BUTTON)) {
      for (i = 0; i < 6; i++) {
	tmp_channels[i] = 0;
	if (BUTTON_DOWN(i)) {
	  allup = 0;
	  tmp_channels[i] = 1;
	}
      }
    } else {
      allup = 1;
      for (i = 0; i < 6; i++) {
	tmp_channels[i] = 0;
	if (BUTTON_PRESSED(i)) {
	  if (BUTTON_DOWN(SELECT_BUTTON)) {
	    current_channel = i;
	  } else if (BUTTON_DOWN(SHIFT_BUTTON)) {
	    reload_track = i;
	    mono_status = reload_track_get_current_kit;
	  }
	}
      }
    }

    if (BUTTON_DOWN(MUTE_BUTTON) && BUTTON_DOWN(SELECT_BUTTON)) {
      allup = 0;
      for (i = 0; i < 6; i++)
	tmp_channels[i] = 1;
    }
    
    for (i = 0; i < 6; i++) {
      if (allup) {
	if (i == current_channel)
	  SET_LED_TMP(i);
	else
	  CLEAR_LED_TMP(i);
      } else {
	if (tmp_channels[i])
	  SET_LED_TMP(i);
	else
	  CLEAR_LED_TMP(i);
      }
    }
  }

  if ((BUTTON_DOWN(SHIFT_BUTTON) && BUTTON_PRESSED(SELECT_BUTTON)) ||
      (BUTTON_PRESSED(SHIFT_BUTTON) && BUTTON_DOWN(SELECT_BUTTON))) {
    mono_status = reload_kit_get_current_kit;
  }
}

void joystick_randomizer(void) {
  if (BUTTON_PRESSED(SHIFT_BUTTON)) {
    current_mode = normal_mode;
    return;
  }
}

void joystick_configuration(void) {
  uint8_t i;
  static uint16_t last_time;
  uint16_t time;
  cli();
  time = timer2_slowclock;
  sei();
  if (clock_diff(last_time, time) > 300) {
    TOGGLE_BIT(PORTB, PB0);
    last_time = time;
  }
  CLEAR_LED_TMP(4);
  if (BUTTON_PRESSED(5)) {
    merge_enabled = !merge_enabled;
  }
  
  if (merge_enabled) {
    SET_LED_TMP(5);
  } else {
    CLEAR_LED_TMP(5);
  }

  for (i = 0; i < 4; i++) {
    if (BUTTON_PRESSED(i)) {
      TOGGLE_BIT(base_channel, 3 - i);
    }
    if (IS_BIT_SET(base_channel, 3 - i)) {
      SET_LED_TMP(i);
    } else {
      CLEAR_LED_TMP(i);
    }
  }
  if (BUTTON_PRESSED(SHIFT_BUTTON)) {
    CLEAR_BIT(PORTB, PB0);
    eeprom_write_byte(BASE_CHANNEL_ADDR, base_channel);
    eeprom_write_byte(MERGE_ENABLED_ADDR, merge_enabled);
    current_mode = normal_mode;
    return;
  }
}

void joystick_routine(void) {
  tmpled = 0;
  
  switch (current_mode) {
  case normal_mode:
    joystick_normal();
    break;

  case mute_mode:
    joystick_mute();
    break;

  case randomizer_mode:
    joystick_randomizer();
    break;

  case configuration_mode:
    joystick_configuration();
    break;
  }

  cli();
  PORTD = (PORTD & 3) | (tmpled & 0xFC);
  sei();
  
}
