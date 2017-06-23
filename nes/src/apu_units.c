#include <stdio.h>
#include <stdlib.h>

#include "apu_units.h"

/* https://wiki.nesdev.com/w/index.php/APU_Envelope */
void apu_unit_envelope_clock(apu_unit_envelope_t* unit) {
  if (unit->start_flag) {
    unit->start_flag = false;
    unit->decay_level_counter = 15;
    unit->divider = unit->c_volume_envelope;
  } else if (unit->divider != 0) {
    unit->divider--;
  } else {
    unit->divider = unit->c_volume_envelope;
    if (unit->decay_level_counter != 0) {
      unit->decay_level_counter--;
    } else if (unit->c_env_loop_flag) {
      unit->decay_level_counter = 15;
    }
  }
}

u_int8_t apu_unit_envelope_output(apu_unit_envelope_t* unit) {
  if (unit->c_constant_volume_flag) {
    return unit->c_volume_envelope;
  }

  return unit->decay_level_counter;
}

/* https://wiki.nesdev.com/w/index.php/APU_Sweep */
void apu_unit_sweep_sweep(apu_unit_sweep_t* unit, bool ones_complement) {
  uint16_t change_amount = (*unit->c_timer_period) >> unit->c_shift_count;
  if (unit->c_negate) {
    (*unit->c_timer_period) -= change_amount;
    if (ones_complement) {
      (*unit->c_timer_period)--;
    }
  } else {
    (*unit->c_timer_period) += change_amount;
  }
}

void apu_unit_sweep_clock(apu_unit_sweep_t* unit, bool ones_complement) {
  if (unit->reload_flag) {
    if (unit->c_enabled && unit->divider == 0) {
      apu_unit_sweep_sweep(unit, ones_complement);
    }
    unit->divider = unit->c_divider_period;
    unit->reload_flag = false;
  } else if (unit->divider != 0) {
    unit->divider--;
  } else {
    if (unit->c_enabled) {
      apu_unit_sweep_sweep(unit, ones_complement);
    }
    unit->divider = unit->c_divider_period;
  }
}

/* https://wiki.nesdev.com/w/index.php/APU#Glossary */
void apu_unit_timer_clock(apu_unit_timer_t* unit, apu_timer_context_t* context,
                          apu_timer_clock_t on_clock) {
  if (unit->divider == 0) {
    unit->divider = unit->c_timer_period;
    if (context != NULL && on_clock != NULL) {
      on_clock(context);
    }
  } else {
    unit->divider--;
  }
}

static const int LENGTH_COUNTER_LOAD_TABLE[] = {
    10, 254, 20, 2,  40, 4,  80, 6,  160, 8,  60, 10, 14, 12, 26, 14,
    12, 16,  24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30};

void apu_unit_length_counter_reload(apu_unit_length_counter_t* unit) {
  unit->length_counter = LENGTH_COUNTER_LOAD_TABLE[unit->c_length_counter_load];
}

/* https://wiki.nesdev.com/w/index.php/APU_Length_Counter */
void apu_unit_length_counter_onenable(apu_unit_length_counter_t* unit) {
  apu_unit_length_counter_reload(unit);
}

void apu_unit_length_counter_ondisable(apu_unit_length_counter_t* unit) {
  unit->length_counter = 0;
}

void apu_unit_length_counter_clock(apu_unit_length_counter_t* unit) {
  if (unit->length_counter == 0 || unit->c_length_counter_hold) {
    return;
  }

  unit->length_counter--;
}
