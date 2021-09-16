
#include "app.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>

#include "common.h"
#include "lcd.h"
#include "midi-kontrol.h"

uint8_t menu_mode = 0;
uint8_t menu_page = 0;
uint8_t menu_page2 = 0;
uint8_t menu_edit_name = 0;
uint8_t menu_edited_name = 0;
uint8_t menu_edit_macro = 0;
void *menu_page_data;

typedef void (*handle_menu_callback_t)(void);
handle_menu_callback_t handle_menu_counters = NULL;

void handle_edit_menu_name(void);
void handle_edit_midi_page(void);
void handle_edit_midi_page2(void);
void handle_edit_md_page(void);
void handle_edit_macro_page(void);
void handle_page_menu_page(void);

void set_menu_edit_name(uint8_t name) {
  menu_edited_name = name;
  menu_edit_name = 1;
  set_flash_string_p(pstr_edit_name, pstr_page);
  flash[10] = name + 1 + '0';
  flash2[5] = menu_page + 1 + '0';
  handle_menu_counters = handle_edit_menu_name;
}

void set_menu_edit_macro(void) {
  if (!menu_edit_macro) {
    menu_edit_macro = 1;
    set_flash_string_p(pstr_edit_macro, pstr_page);
    flash2[5] = menu_page + 1 + '0';
    handle_menu_counters = handle_edit_macro_page;
  } else {
    set_menu_page(menu_page);
  }
}

void set_menu_page(uint8_t page) {
  menu_page = page;
  menu_edit_name = 0;
  line1_changed = 1;
  menu_mode = 1;
  menu_edit_macro = 0;

  if (menu_page < 4) {
    uint8_t type = get_encoder_page_type(menu_page);
    set_flash_string_p(pstr_edit_page, long_type_names[type]);
    flash[10] = menu_page + 1 + '0';

    if (type == midi_page_type) {
      if (menu_page2) {
	handle_menu_counters = handle_edit_midi_page2;
	flash[12] = '(';
	flash[13] = '2';
	flash[14] = ')';
      } else {
	handle_menu_counters = handle_edit_midi_page;
      }
    } else {
      handle_menu_counters = handle_edit_md_page;
    }
  } else {
    my_str16cpy_p(line1, pstr_pg1_pg2_pg3_pg4);
    set_flash_string_p(pstr_menu, pstr_page_types);
    handle_menu_counters = handle_page_menu_page;
  }
}

void handle_edit_macro_page(void) {
  std_page_t * const std_page = menu_page_data;
  uint8_t i;
  for (i = 0; i < 4; i++) {
    U_LIMIT(std_page->macro[i][0], ENCODER_NORMAL(i) +
	    MIDI_ACCEL * ENCODER_BUTTON(i),
	    0, 127);
    set_line1_value(i, std_page->macro[i][0]);
    U_LIMIT(std_page->macro[i][1], ENCODER_SHIFT(i) +
	    MIDI_ACCEL * ENCODER_BUTTON_SHIFT(i),
	    0, 127);
    set_line2_value(i, std_page->macro[i][1]);
  }
}

void handle_edit_menu_name(void) {
  uint8_t i;
  midi_page_t * const midi_page  = menu_page_data;
  for (i = 0; i < 4; i++) {
    U_LIMIT_CHAR(midi_page->names[menu_edited_name][i], ENCODER_NORMAL(i));
  }
  my_str16cpy_p_fill(line1, pstr_edit_name);
  line1[10] = menu_edited_name + 1 + '0';
  clear_line(line2);
  set_line2_string(0, midi_page->names[menu_edited_name]);
}

void handle_edit_midi_page(void) {
  uint8_t i;
  midi_page_t * const midi_page = menu_page_data;
  
  for (i = 0; i < 4; i++) {
    U_LIMIT(midi_page->ccs[i], ENCODER_NORMAL(i), 0, 127);
    my_memcpy(line1, midi_page->names, 16);
    set_line2_value(i, midi_page->ccs[i]);
  }
}

void handle_edit_midi_page2(void) {
  uint8_t i;
  midi_page_t * const midi_page = menu_page_data;
  
  for (i = 0; i < 4; i++) {
    my_memcpy(line1, midi_page->names, 16);
    U_LIMIT(midi_page->channel[i], ENCODER_NORMAL(i), 0, 15);
    set_line2_value(i, midi_page->channel[i] + 1);
  }
}

void handle_edit_md_page(void) {
  uint8_t i;
  md_page_t * const data = menu_page_data;
  uint8_t type = get_encoder_page_type(menu_page);
  uint8_t *params = data->params;
  for (i = 0; i < 4; i++) {
    U_LIMIT(params[i], ENCODER_NORMAL(i), 0, 7);
    //    U_LIMIT(data->defaults[i], ENCODER_SHIFT(i), 0, 127);
    //    set_line2_value(i, data->defaults[i]);
  }
  clear_line(line2);
  show_md_page_names(type - 1, params);
}

void handle_page_menu_page(void) {
  uint8_t i;
  for (i = 0; i < 4; i++) {
    uint8_t oldval;
    encoder_page_t *epage = get_encoder_page(i);
    oldval = epage->type;
    U_LIMIT(epage->type, ENCODER_NORMAL(i), 0, 4);
    set_line2_p_string(i, type_names[epage->type]);
    if (oldval != epage->type) {
      read_page_defaults(i);
    }
  }
}

