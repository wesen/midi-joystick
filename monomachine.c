#include "uart.h"
#include "hardware.h"
#include "midi-common.h"
#include "monomachine.h"
#include "midi-small.h"

uint8_t reload_track = 0;

/* sysex monomachine midi engine */
uint8_t elektron_sysex_hdr[5] = {
  0x00,
  0x20,
  0x3c,
  0x03, /* monomachine ID */
  0x00 /* base channel padding */
};

extern uint8_t base_channel;
volatile mono_status_t mono_status = mono_none;
uint8_t current_kit = 0;
uint8_t current_audio_track = 0;

void elektron_send_param(uint8_t track, uint8_t param, uint8_t value) {
  uart_putc(0xB0 + track + base_channel);
  if (param < 16) {
    uart_putc(48 + param);
  } else {
    uart_putc(56 + param);
  }
  uart_putc(value);
}

void elektron_cc_to_param(uint8_t channel, uint8_t cc,
			  uint8_t *track, uint8_t *param) {
  if ((channel >= base_channel) && (channel <= (base_channel +  5))) {
    *track = channel - base_channel;
    if ((cc >= 48) && (cc < (48 + 16))) {
      *param = cc - 48;
    } else if ((cc >= 72) && (cc < 120)) {
      *param = cc - 56;
    } else {
      *param = 255;
    }
  } else {
    *param = 255;
  }
}

void elektron_send_request(uint8_t byte1, uint8_t byte2) {
  uart_putc(0xF0);
  uint8_t i;
  for (i = 0; i < sizeof(elektron_sysex_hdr); i++)
    uart_putc(elektron_sysex_hdr[i]);
  uart_putc(byte1);
  uart_putc(byte2);
  uart_putc(0xF7);
}

uint8_t msg_type;

void end_elektron_sysex(void) {
  switch (msg_type) {
  case ELEKTRON_MNM_STATUS_RESPONSE:
    if (sysex_buflen >= 2) {
      switch (sysex_buf[0]) {
      case ELEKTRON_MNM_REQ_CURRENT_KIT:
	current_kit = sysex_buf[1];
	if (mono_status == reload_kit_wait_current_kit) {
	  mono_status = reload_kit_got_current_kit;
	} else if (mono_status == reload_track_wait_current_kit) {
	  mono_status = reload_track_got_current_kit;
	}
	break;

      case ELEKTRON_MNM_REQ_CURRENT_AUDIO_TRACK:
	current_audio_track = sysex_buf[1];
	if (mono_status == reload_track_wait_current_track) {
	  mono_status = reload_track_got_current_track;
	}
	break;
      }
    }
    break;
  }
}

uint16_t unpack_8len = 0;
uint8_t  unpack_8data = 0;
uint16_t dump_len = 0;
uint8_t unpack_repeat = 0;
uint8_t unpack_data[8];

uint8_t track_cnt;
uint8_t dump_record = 0;
uint16_t dump_rec_len = 0;

void start_elektron_sysex(void) {
  unpack_8data = 0;
  unpack_8len = 0;
  dump_len = 0;
  unpack_repeat = 0;
  dump_record = 0;
  dump_rec_len = 0;
}

void start_dump_record(void) {
  dump_record = 1;
  dump_rec_len = 0;
}
void handle_kit_dump(uint8_t c) {
#if 1
  if (dump_len == (0x11 + 72 * reload_track)) {
    start_dump_record();
  }

  if (dump_len == (0x11 + 72 * reload_track + 72)) {
    dump_record = 0;
    uint8_t i;
    for (i = 0; i < (7 * 8); i++) {
      elektron_send_param(reload_track, i, sysex_buf[i]);
    }
    put_default_joystick(reload_track);
  }
#endif
  
  if (dump_len == 0) {
    start_dump_record();
  }

  if (dump_len == 10) {
    dump_record = 0;
    sysex_buf[dump_rec_len] = 0;
  }

  if (dump_record) {
    sysex_buf[dump_rec_len++] = c;
  }

  dump_len++;
}

void handle_elektron_sysex(uint8_t c) {
  if ((sysex_cnt < sizeof(elektron_sysex_hdr)) &&
      (c != elektron_sysex_hdr[sysex_cnt])) {
    sysex_discard = 1;
    return;
  } else if (sysex_cnt == sizeof(elektron_sysex_hdr)) {
    msg_type = c;
    switch (c) {
    case ELEKTRON_MNM_STATUS_RESPONSE:
      sysex_record();
      break;
    }
  } else if (sysex_cnt == 9 && msg_type == ELEKTRON_MNM_KIT_DUMP) {
    unpack_8data = 1;
    unpack_8len = 0;
  }

  if (unpack_8data) {
    unpack_data[unpack_8len++] = c;
    if (unpack_8len == 8) {
      uint8_t i;
      for (i = 0; i < 7; i++) {
	unpack_data[i+1] |= ((unpack_data[0] >> (6-i)) & 1) << 7;
	if (unpack_data[i+1] & 0x80) {
	  unpack_repeat = unpack_data[i+1] & 0x7F;
	} else {
	  if (unpack_repeat) {
	    uint8_t j;
	    for (j = 0; j < unpack_repeat; j++) {
	      handle_kit_dump(unpack_data[i+1]);
	    }
	    unpack_repeat = 0;
	  } else {
	    handle_kit_dump(unpack_data[i+1]);
	  }
	}
      }
      unpack_8len = 0;
    }
  }
  
}

void do_buttons(void) {
  if (BUTTON_PRESSED(0)) {
    mono_status = reload_kit_get_current_kit;
  }
  if (BUTTON_PRESSED(1)) {
    mono_status = reload_track_get_current_kit;
  }
}

void mono_routine(void) {
  switch (mono_status) {
  case reload_kit_get_current_kit:
    elektron_send_request(ELEKTRON_MNM_STATUS_REQUEST,
			  ELEKTRON_MNM_REQ_CURRENT_KIT);
    mono_status = reload_kit_wait_current_kit;
    break;

  case reload_kit_got_current_kit:
    elektron_send_request(ELEKTRON_MNM_LOAD_KIT, current_kit);
    {
      uint8_t i;
      for (i = 0; i < 6; i++) {
	put_default_joystick(i);
      }
    }
    mono_status = mono_none;
    break;

  case reload_track_get_current_kit:
    elektron_send_request(ELEKTRON_MNM_STATUS_REQUEST,
			  ELEKTRON_MNM_REQ_CURRENT_KIT);
    mono_status = reload_track_wait_current_kit;
    break;

  case reload_track_got_current_kit:
    elektron_send_request(ELEKTRON_MNM_REQUEST_KIT,
			  current_kit);
    mono_status = reload_track_wait_track_data;
    break;

  case reload_track_got_track_data:
    break;

  default:
    break;
  }

}
