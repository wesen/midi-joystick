#include "app.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>

#include "common.h"

uint8_t menu_mode = 0;

kontrol_config_t current_config = {
  2, 0, 0
};

