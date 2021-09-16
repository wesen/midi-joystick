#include "app.h"

#include <util/delay.h>

#include "common.h"
#include "midi-small.h"
#include "uart.h"
#include "monomachine.h"

void handle_elektron_sysex(uint8_t c);
void start_elektron_sysex(void);
void end_elektron_sysex(void);
void end_ruinwesen_sysex(void);

typedef struct midi_size_s {
  uint8_t midi_status;
  uint8_t size;
} midi_size_t;

const midi_size_t midi_sizes[] = {
  { MIDI_NOTE_OFF, 2 },
  { MIDI_NOTE_ON, 2 },
  { MIDI_AFTER_TOUCH, 2 },
  { MIDI_CONTROL_CHANGE, 2 },
  { MIDI_PROGRAM_CHANGE, 1 },
  { MIDI_CHANNEL_PRESSURE, 1},
  { MIDI_PITCH_WHEEL, 1 }
};

uint8_t merging = 0;
uint8_t merge_status = 0;
uint8_t merge_to_go = 0;
uint16_t last_clock = 0;
uint8_t merge_enabled = 0;

uint8_t midi_recv = 0;
// uint8_t midi_msg_type = 0;
uint8_t midi_msg_cnt = 0;
uint8_t midi_msg[3];

uint16_t sysex_cnt;
uint8_t in_sysex= 0;
uint8_t sysex_discard = 0;

uint8_t sysex_buflen = 0;
uint8_t sysex_buf_record = 0;
uint8_t sysex_buf[128];

uint8_t elektron_sysex = 0;
uint8_t ruinwesen_sysex = 0;

void start_sysex(void) {
  sysex_discard = 0;
  sysex_buflen = 0;
  sysex_buf_record = 0;
  elektron_sysex = 0;
  ruinwesen_sysex = 0;
}

void end_sysex(void) {
  if (elektron_sysex) {
    end_elektron_sysex();
  }
  if (ruinwesen_sysex) {
    end_ruinwesen_sysex();
  }
}

void sysex_record(void) {
  sysex_buf_record = 1;
  sysex_buflen = 0;
}

void handle_ruinwesen_sysex(uint8_t c) {
}

void handle_sysex(uint8_t c) {
  if (sysex_discard)
    return;

  if (sysex_buf_record) {
    sysex_buf[sysex_buflen++] = c;
  }

  if (sysex_cnt == 2) {
    sysex_buf_record = 0;
    if ((sysex_buf[0] == elektron_sysex_hdr[0]) &&
	(sysex_buf[1] == elektron_sysex_hdr[1]) &&
	(sysex_buf[2] == elektron_sysex_hdr[2])) {
      start_elektron_sysex();
      elektron_sysex = 1;
    } else if ((sysex_buf[0] == SYSEX_VENDOR_1)  &&
	       (sysex_buf[1] == SYSEX_VENDOR_2) &&
	       (sysex_buf[2] == SYSEX_VENDOR_3)) {
      ruinwesen_sysex = 1;
      sysex_record();
    } else {
      sysex_discard = 1;
    }

    return;
  }

  if (elektron_sysex) {
    handle_elektron_sysex(c);
  }
  if (ruinwesen_sysex) {
    handle_ruinwesen_sysex(c);
  }
    
}

void handle_midi_rx2(uint8_t byte) {
  if (merge_enabled) {
    if (MIDI_IS_REALTIME_STATUS_BYTE(byte)) {
      uart_putc(byte);
      return;
    }
  } else {
    if (MIDI_IS_REALTIME_STATUS_BYTE(byte))
      return;
  }

  if (MIDI_IS_STATUS_BYTE(byte)) {
    midi_recv = 0;
    midi_msg[0] = byte;

    if (byte == 0xF0) {
      if (in_sysex) {
	end_sysex();
      }
      start_sysex();
      sysex_record();
      in_sysex = 1;
      sysex_cnt = 0;

      if (merge_enabled) {
	merging = 1;
	uart_putc(byte);
      }

      /* ignore sysex for now */
      return;
    } else if (merge_enabled) {
      uint8_t i;
      for (i = 0; i < countof(midi_sizes); i++) {
	if (byte == midi_sizes[i].midi_status) {
	  merging = 1;
	  uart_putc(byte);
	  merge_status = byte;
	  merge_to_go = midi_sizes[i].size;
	  break;
	}
      }
    }

    if (byte == 0xF7) {
      if (merge_enabled) {
	merging = 0;
	uart_putc(byte);
      }
      
    if (in_sysex) {
      end_sysex();
    }

    in_sysex = 0;

      
      return;
    }

    if (byte == MIDI_CONTROL_CHANGE) {
      midi_recv = midi_msg_cnt = 1;
      return;
    }
  } else {
    if (merge_enabled) {
      if (merging) {
	uart_putc(byte);
	if (!in_sysex) {
	  if (--merge_to_go == 0)
	    merging = 0;
	}
      }
    }
    
    if (in_sysex) {
      handle_sysex(byte);
      sysex_cnt++;
      return;
    }
    
  again:
    if (midi_recv) {
      midi_msg[midi_msg_cnt++] = byte;

      // XXX running status
      if (midi_msg_cnt == 3) {
	midi_recv = 0;
	midi_msg_cnt = 0;
	midi_cc_callback(midi_msg);
      }
    } else if (MIDI_VOICE_TYPE_NIBBLE(midi_msg[0]) == MIDI_CONTROL_CHANGE) {
      /* cheapo running status for control change */
      midi_recv = midi_msg_cnt = 1;
      goto again;
    }
  }
}

