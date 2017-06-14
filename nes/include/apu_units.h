#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  bool start_flag;
  uint8_t divider;
  uint8_t decay_level_counter;  // This is the output volume

  // Last register contents
  uint8_t c_volume_envelope;
  bool c_env_loop_flag;
} apu_unit_envelope_t;

typedef struct {
  uint8_t divider;
  bool reload_flag;
  uint16_t* c_timer_period;

  // Last register contents
  bool c_enabled;
  uint8_t c_divider_period;
  bool c_negate;
  uint8_t c_shift_count;
} apu_unit_sweep_t;

typedef struct {
  uint16_t divider;

  // Last register contents
  uint16_t c_timer_period;
} apu_unit_timer_t;

typedef struct {
  uint8_t length_counter;

  // Last register contents
  bool c_length_counter_hold;
  uint8_t c_length_counter_load;
} apu_unit_length_counter_t;

void apu_unit_envelope_clock(apu_unit_envelope_t* unit);

void apu_unit_sweep_sweep(apu_unit_sweep_t* unit, bool ones_complement);
void apu_unit_sweep_clock(apu_unit_sweep_t* unit, bool ones_complement);

typedef void apu_timer_context_t;
typedef void (*apu_timer_clock_t)(apu_timer_context_t* context);

void apu_unit_timer_clock(apu_unit_timer_t* unit, apu_timer_context_t* context,
                          apu_timer_clock_t on_clock);

void apu_unit_length_counter_onenable(apu_unit_length_counter_t* unit);
void apu_unit_length_counter_ondisable(apu_unit_length_counter_t* unit);
void apu_unit_length_counter_clock(apu_unit_length_counter_t* unit);
