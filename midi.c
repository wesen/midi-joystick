#include "midi.h"
#include "midi_clock.h"

#include <avr/io.h>

void midi_in_stm_init(midi_in_stm_t *stm) {
  stm->last_status = 0;
  stm->running_status = 0;
  stm->in_state = midi_ignore_message;
  sysex_in_stm_init(&stm->sysex_stm);
}

const midi_parse_t midi_parse[] = {
  { MIDI_NOTE_OFF,         midi_wait_byte_2,     midi_note_off_callback },
  { MIDI_NOTE_ON,          midi_wait_byte_2,     midi_note_on_callback },
  { MIDI_AFTER_TOUCH,      midi_wait_byte_2,     midi_at_callback }, /* ???? */
  { MIDI_CONTROL_CHANGE,   midi_wait_byte_2,     midi_cc_callback },
  { MIDI_PROGRAM_CHANGE,   midi_wait_byte_1,     midi_prgchg_callback },
  { MIDI_CHANNEL_PRESSURE, midi_wait_byte_1,     NULL },
  { MIDI_PITCH_WHEEL,      midi_wait_byte_2,     NULL },
  /* special handling for SYSEX */
  { MIDI_MTC_QUARTER_FRAME, midi_wait_byte_1,    NULL },
  { MIDI_SONG_POSITION_PTR, midi_wait_byte_2,    NULL },
  { MIDI_SONG_SELECT,       midi_wait_byte_1,    NULL },
  { MIDI_TUNE_REQUEST,      midi_wait_status,    NULL },
  { 0, 0, 0}
};

void handle_midi_rx(midi_in_stm_t *stm, uint8_t byte) {
 again:
  if (MIDI_IS_REALTIME_STATUS_BYTE(byte)) {
    /* handle realtime message */
#ifdef MIDI_CLOCK_ENABLE
    switch (byte) {
    case MIDI_CLOCK:
      midi_clock_handle_midi_clock(&midi_clock);
      break;
      
    case MIDI_START:
      midi_clock_handle_midi_start(&midi_clock);
      break;
      
    case MIDI_STOP:
      midi_clock_handle_midi_stop(&midi_clock);
      break;
    }
#endif
    return;
  }

  switch (stm->in_state) {
  case midi_ignore_message:
    if (MIDI_IS_STATUS_BYTE(byte)) {
      stm->in_state = midi_wait_status;
      goto again;
    } else {
      /* ignore */
    }
    break;

    /* handle sysex message */
  case midi_wait_sysex:
    if (MIDI_IS_STATUS_BYTE(byte) && (byte != MIDI_SYSEX_END)) {
      /* status byte interrupts sysex request */
      stm->in_state = midi_wait_status;
      midi_abort_sysex(stm);
      goto again;
    }

    //    stm->msg[stm->in_msg_len] = byte;
    midi_byte_sysex(stm, byte);
    break;

  case midi_wait_status:
    {
      /* special handling for sysex */
      if (byte == MIDI_SYSEX_START) {
	stm->in_state = midi_wait_sysex;
	midi_start_sysex(stm);
	stm->running_status = 0;
	stm->last_status = 0;
	return;
      }

      if (MIDI_IS_STATUS_BYTE(byte)) {
	stm->last_status = byte;
	stm->running_status = 0;
      } else {
	if (stm->last_status == 0)
	    break;
	stm->running_status = 1;
      }

      uint8_t status = stm->last_status;
      if (MIDI_IS_VOICE_STATUS_BYTE(status)) {
	status = MIDI_VOICE_TYPE_NIBBLE(status);
      }

      stm->callback = NULL;
      int i;
      for (i = 0; midi_parse[i].midi_status != 0; i++) {
	if (midi_parse[i].midi_status == status) {
	  stm->in_state = midi_parse[i].next_state;
	  stm->msg[0] = stm->last_status;
	  stm->in_msg_len = 1;
	  stm->callback = midi_parse[i].callback;

	  break;
	}
      }

      if (midi_parse[i].midi_status == 0) {
	stm->in_state = midi_ignore_message;
	return;
      }
      if (stm->running_status)
	goto again;
    }
    break;

  case midi_wait_byte_1:
    stm->msg[stm->in_msg_len++] = byte;
    if (stm->callback != NULL) {
      stm->callback(stm->msg);
    }
    stm->in_state = midi_wait_status;
    break;

  case midi_wait_byte_2:
    stm->msg[stm->in_msg_len++] = byte;
    stm->in_state = midi_wait_byte_1;
    break;
  }
}

