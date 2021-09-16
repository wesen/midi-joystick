#ifndef MIDI_CLOCK_H__
#define MIDI_CLOCK_H__

#ifndef NULL
#define NULL 0
#endif

#include <inttypes.h>

#define CLOCK_DIFF_COUNT 16
typedef struct midi_clock_s {
  uint8_t active;
  uint16_t clock_interval;
  uint16_t clock_counter;
  uint16_t last_clock;
  uint16_t last_interval;
  uint16_t clock_diffs[CLOCK_DIFF_COUNT];
  uint8_t clock_diff_idx;
  uint32_t div96th_counter;
  uint32_t indiv96th_counter;
} midi_clock_t;

extern midi_clock_t midi_clock;

#define MIDI_CLOCK_STOPPED 0
#define MIDI_CLOCK_STARTING 1
#define MIDI_CLOCK_STARTED 2

typedef void (*midi_start_callback_t)(void);

extern midi_start_callback_t midi_start_callback;

void midi_clock_init(midi_clock_t *midiclock);
void midi_clock_handle_midi_clock(midi_clock_t *midiclock);
void midi_clock_handle_midi_start(midi_clock_t *midiclock);
void midi_clock_handle_midi_stop(midi_clock_t *midiclock);
uint8_t midi_clock_handle_timer_int(midi_clock_t *midiclock);

#endif /* MIDI_CLOCK_H__ */
