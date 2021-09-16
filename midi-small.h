#ifndef MIDI_SMALL_H__
#define MIDI_SMALL_H__

#include "midi-common.h"

extern uint8_t merging;
extern uint8_t merge_enabled;

extern uint16_t sysex_cnt;

void midi_sysex_callback(void);
void handle_midi_rx2(uint8_t byte);

extern uint8_t sysex_buflen;
extern uint8_t sysex_buf_record;
extern uint8_t sysex_buf[128];

#endif /* MIDI_SMALL_H__ */
