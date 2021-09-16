#include "common.h"
#include "midi-kontrol.h"
#include "midi-small.h"
#include "uart.h"
#include "sysex-data.h"

uint8_t send_data[64];
void send_page_sysex(uint8_t page) {
  uint8_t cnt = 0;
  send_data[cnt++] = 0xF0; // -1
  send_data[cnt++] = 0x00; // 0
  send_data[cnt++] = 0x13; // 1
  send_data[cnt++] = 0x37; // 2
  send_data[cnt++] = CMD_SEND_PAGE; // 3
  send_data[cnt++] = curpatch; // 4
  send_data[cnt++] = page;     // 5
  send_data[cnt++] = bkp_encoder_pages[page].type;     // 6
  cnt += data_to_sysex((uint8_t *)&bkp_encoder_pages[page].data, send_data + cnt,
		       sizeof(encoder_page_t));
  send_data[cnt++] = 0xF7;
  uart_puts(send_data, cnt);
}

void send_patch_sysex(uint8_t patch) {
  uint8_t i;
  for (i = 0; i < MAX_PAGES; i++) {
    send_page_sysex(i);
  }
}

void receive_page_sysex(void) {
  if (sysex_data[4] == curpatch) {
    uint8_t page = sysex_data[5];
    uint8_t type = sysex_data[6];
    sysex_to_data(sysex_data + 7, get_encoder_page_data(page), sysex_cnt - 7);
    get_encoder_page(page)->type = type;
    current_page->refresh = 1;
    if (page == curpage)
      set_page(page);
  }
}

void save_page_sysex(void) {
  receive_page_sysex();
  uint8_t patch = sysex_data[4];
  uint8_t page = sysex_data[5];
  uint8_t type = sysex_data[6];
  sysex_to_data(sysex_data + 7, send_data, sysex_cnt - 7);
  write_kontrol_page(patch, page, type, send_data);
}

