#include "app.h"
#include "common.h"

#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/boot.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <avr/eeprom.h>

#include "uart.h"
#include "midi.h"

int main(void);

#define _B1024

#ifdef _B128
  #define APP_PAGES ((2*4096 / SPM_PAGESIZE)- (2*128 / SPM_PAGESIZE )) 
  #define APP_END APP_PAGES * SPM_PAGESIZE 
#endif
#ifdef _B256
  #define APP_PAGES ((2*4096 / SPM_PAGESIZE)- (2*256 / SPM_PAGESIZE )) 
  #define APP_END APP_PAGES * SPM_PAGESIZE 
#endif
#ifdef _B512
  #define APP_PAGES ((2*4096 / SPM_PAGESIZE)- (2*512 / SPM_PAGESIZE )) 
  #define APP_END APP_PAGES * SPM_PAGESIZE 
#endif
#ifdef _B1024
  #define APP_PAGES ((2*4096 / SPM_PAGESIZE)- (2*1024 / SPM_PAGESIZE )) 
  #define APP_END APP_PAGES * SPM_PAGESIZE 
#endif  
#ifdef _B2048
  #error "_B2048 not suppoted on this device"
#endif   

void (*jump_to_app)(void) = 0x0000;

uint8_t check_firmware_checksum(void) {
  uint16_t len = eeprom_read_word(FIRMWARE_LENGTH_ADDR);
  uint16_t firm_checksum = eeprom_read_word(FIRMWARE_CHECKSUM_ADDR);
  uint16_t i;
  uint16_t checksum = 0;
  
  for (i = 0; i < len; i++) {
    checksum += pgm_read_byte(i);
  }

  if ((checksum & 0x3FFF) == firm_checksum)
    return 1;
  else
    return 0;
}

void write_firmware_checksum(uint16_t len, uint16_t checksum) {
  eeprom_write_word(FIRMWARE_LENGTH_ADDR, len);
  eeprom_write_word(FIRMWARE_CHECKSUM_ADDR, checksum);
  eeprom_write_word(START_MAIN_APP_ADDR, 1);
}

uint8_t bl_uart_getc(void) {
  while (!UART_CHECK_RX())
    ;
  return UART_READ_CHAR();
}

void bl_uart_putc(uint8_t c) {
  while (!UART_CHECK_EMPTY_BUFFER())
    ;
  UART_WRITE_CHAR(c);
}

uint8_t ack_msg[6] = {
  0xf0, 0x00, 0x13, 0x37, CMD_DATA_BLOCK_ACK, 0xf7
};

uint8_t nak_msg[6] = {
  0xf0, 0x00, 0x13, 0x37, CMD_DATA_BLOCK_NAK, 0xf7
};

void midi_sysex_send_ack(void) {
  uint8_t i;
  for (i = 0; i < sizeof(ack_msg); i++)
    bl_uart_putc(ack_msg[i]);
}  

void midi_sysex_send_nak(void) {
  uint8_t i;
  for (i = 0; i < sizeof(nak_msg); i++)
    bl_uart_putc(nak_msg[i]);
}  

uint8_t jump_to_main_program(void) {
  if (check_firmware_checksum()) {
    jump_to_app();
    return 1;
  } else {
    return 0;
  }
}

uint8_t block_cnt = 0;
uint8_t sysex_data[64];

uint8_t data[100];
uint8_t sysex_cnt;
uint8_t in_sysex= 0;

uint16_t make_word(uint8_t idx, uint8_t cnt) {
  uint8_t ptr = idx + cnt - 1;
  uint32_t ret = 0;
  for (; ptr >= idx; ptr--) {
    ret <<= 7;
    ret |= data[ptr];
  }
  return ret;
}

uint8_t write_checksum(void) {
  uint16_t len = make_word(4, 3);
  uint16_t checksum = data[7] | (data[8] << 7);
  write_firmware_checksum(len, checksum);
  return 1;
}

uint8_t write_block(void) {
  
  uint8_t checksum = 0;
  uint8_t i;
  for (i = 3; i < sysex_cnt - 1; i++) {
    checksum ^= data[i];
  }

  uint8_t length = data[4];
  uint16_t sysex_address = make_word(5, 4);

  if (sysex_address >= APP_END) {
    return 0;
  }
  
  uint8_t cnt = 0, recvd = 0;
  uint8_t bits = 0;
  for (cnt = 0; cnt < (sysex_cnt - 9); cnt++) {
    if ((cnt % 8) == 0) {
      bits = data[9 + cnt];
    } else {
      sysex_data[recvd++] = data[9 + cnt] | ((bits & 1) << 7);
      bits >>= 1;
    }
    if (recvd >= length)
      break;
  }

  uint8_t check = data[sysex_cnt - 1];
  checksum &= 0x7f;

  if ((checksum != check) || (recvd != 64)) {
    return 0;
  }

  uint8_t sreg = SREG;
  cli();
  boot_page_erase(sysex_address);
  boot_spm_busy_wait();
  uint16_t address = sysex_address;
  for (i = 0; i < 64; i+=2) {
    uint16_t tmp = sysex_data[i] | (sysex_data[i + 1] << 8);
    boot_page_fill(address, tmp);
    address += 2;
  }
  boot_page_write(sysex_address);
  boot_spm_busy_wait();
  boot_rww_enable();
  SREG = sreg;
  
  return 1;
}

void handle_sysex(void) {
  uint8_t res = 0;
  if (sysex_cnt < 4) {
    return;
  } else {
    if (data[3] == CMD_START_BOOTLOADER) {
      res = 1;
    } else if (data[3] == CMD_MAIN_PROGRAM) {
      res = jump_to_main_program();
    } else if (data[3] == CMD_BOOT_DATA_BLOCK) {
      res = write_block();
    } else if (data[3] == CMD_FIRMWARE_CHECKSUM && sysex_cnt == 9) {
      res = write_checksum();
    }

    if (res)
      midi_sysex_send_ack();
    else
      midi_sysex_send_nak();
  }
}

void handle_midi(uint8_t c) {
  if (c == 0xF0) {
    if (in_sysex == 1) {
      handle_sysex();
    }
    sysex_cnt = 0;
    in_sysex = 1;
    return;
  } else if (c == 0xF7) {
    if (in_sysex == 1) {
      handle_sysex();
    }
    in_sysex = 0;
    return;
  } else if (MIDI_IS_STATUS_BYTE(c)) {
    in_sysex = 0;
    return;
  }
  
  if (in_sysex) {
    data[sysex_cnt++] = c;
    if (sysex_cnt >= 3) {
      if (data[0] != SYSEX_VENDOR_1 ||
	  data[1] != SYSEX_VENDOR_2 ||
	  data[2] != SYSEX_VENDOR_3) {
	in_sysex = 0;
      }
    }
    if (sysex_cnt > 100) {
      /* discard too long message */
      in_sysex = 0;
    }
  }
}

int main() {
  wdt_disable();

  /* move interrupts to bootloader section */
  GICR = _BV(IVCE) | _BV(IVSEL);

  SET_BIT8(PORTB, PB4);
  SET_BIT8(DDRB, PB0);

  /* init uart */
  UBRRH = (UART_BAUDRATE_REG >> 8);
  UBRRL = (UART_BAUDRATE_REG & 0xFF);

  /** 8 bit, no parity **/
  UCSRC = _BV(UCSZ0) | _BV(UCSZ1) | _BV(URSEL);
  UCSRB = _BV(RXEN) | _BV(TXEN);

  midi_sysex_send_ack();
    
  if (eeprom_read_word(START_MAIN_APP_ADDR) == 1 && IS_BIT_SET8(PINB, PB4)) {
    bl_uart_putc(0x90);
    bl_uart_putc(0x1);
    bl_uart_putc(0x1);
    jump_to_main_program();
  }

  midi_sysex_send_ack();

  uint8_t button = 0;
  for (;;) {
    if (!IS_BIT_SET8(PINB, PB4)) {
      button = 1;
    } else {
      if (button) {
	jump_to_main_program();
      }
    }

    if (UART_CHECK_RX()) {
      uint8_t c = bl_uart_getc();
      handle_midi(c);
    }
  }
}
