#include "app.h"
#include <util/delay.h>
#include "midi_clock.h"

extern uint16_t timer2_read_clock(void);
extern uint16_t timer2_clock;
extern uint16_t clock_diff(uint16_t, uint16_t);

midi_start_callback_t midi_start_callback = NULL;

uint16_t pll_x = 220;

void midi_clock_init(midi_clock_t *midiclock) {
  midiclock->active = 0;
  //  midiclock->clock_interval = 0;
  midiclock->clock_counter = 0;
  midiclock->last_clock = 0;
  midiclock->div96th_counter = 0;
  midiclock->indiv96th_counter = 0;
  midiclock->clock_diff_idx = 0;
  uint8_t i;
  for (i = 0; i < CLOCK_DIFF_COUNT; i++)
    midiclock->clock_diffs[i] = 0;
}

void midi_clock_handle_midi_clock(midi_clock_t *midiclock) {
  uint16_t cur_clock = timer2_read_clock();
  uint16_t diff = clock_diff(midiclock->last_clock, cur_clock);
  midiclock->last_interval = diff;
  midiclock->last_clock = cur_clock;
  midiclock->indiv96th_counter++;


  if (midiclock->active == MIDI_CLOCK_STARTING) {
    midiclock->active = MIDI_CLOCK_STARTED;
  } else {
    if (midiclock->indiv96th_counter == 2) {
      midiclock->clock_interval = diff;
    } else {
      midiclock->clock_interval =
	(((uint32_t)midiclock->clock_interval * (uint32_t)pll_x)
	 + (uint32_t)(256 - pll_x) * (uint32_t)diff) >> 8;
    }
  }

#if 0
  midiclock->clock_diffs[midiclock->clock_diff_idx] = diff;
  midiclock->clock_diff_idx = (midiclock->clock_diff_idx + 1) % CLOCK_DIFF_COUNT;

  uint16_t sum =0;
  uint8_t i;
  for (i = 0; i < CLOCK_DIFF_COUNT; i++)
    sum += midiclock->clock_diffs[i];
  midiclock->clock_interval = sum / CLOCK_DIFF_COUNT;
#endif

}

void midi_clock_handle_midi_start(midi_clock_t *midiclock) {
  midi_clock_init(midiclock);
  if (midi_start_callback != NULL)
    midi_start_callback();
  midiclock->active = MIDI_CLOCK_STARTING;
  midiclock->clock_counter = 0;
}

void midi_clock_handle_midi_stop(midi_clock_t *midiclock) {
  midiclock->active = MIDI_CLOCK_STOPPED;
}

#define PHASE_FACTOR 15
uint8_t midi_clock_handle_timer_int(midi_clock_t *midiclock) {
  if (midiclock->active != MIDI_CLOCK_STARTED)
    return 0;
  if (midiclock->clock_counter == 0) {
    midiclock->clock_counter = midiclock->clock_interval;
    midiclock->div96th_counter++;

    uint16_t cur_clock = timer2_clock;
    uint16_t diff = clock_diff(midiclock->last_clock, cur_clock);
#if 1

    if ((midiclock->div96th_counter < midiclock->indiv96th_counter) ||
	(midiclock->div96th_counter > (midiclock->indiv96th_counter + 1))){
      midiclock->div96th_counter = midiclock->indiv96th_counter;
    }

    if (midiclock->div96th_counter <= midiclock->indiv96th_counter) {
      //      midiclock->clock_counter -= ((uint32_t)diff * ((uint32_t)(1 << 8) - pll_x)) >> 8;
      midiclock->clock_counter -= ((uint32_t)diff * PHASE_FACTOR)  >> 8;
    } else {
      //      midiclock->clock_counter += ((uint32_t)(midiclock->clock_counter - diff) * ((uint32_t)(1 << 8) - pll_x)) >> 8;
      if (midiclock->clock_counter > diff)
	midiclock->clock_counter += ((uint32_t)(midiclock->clock_counter - diff) * PHASE_FACTOR) >> 8;
    }
#endif
    return 1;
  } else {
    midiclock->clock_counter--;
    return 0;
  }
}

