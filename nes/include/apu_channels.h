#pragma once

#include "apu_units.h"

typedef struct {
  apu_unit_envelope_t envelope;
  apu_unit_sweep_t sweep;
  apu_unit_timer_t timer;
  apu_unit_length_counter_t length_counter;

  // 8-step sequencer
  // ??
} apu_channel_pulse_t;

typedef struct {
  apu_unit_timer_t timer;
  apu_unit_length_counter_t length_counter;

  uint8_t linear_counter;
  bool linear_counter_reload_flag;
  bool control_flag;

  // Sequencer??
} apu_channel_triangle_t;

typedef struct {
  apu_unit_envelope_t envelope;
  // LFSR??
  apu_unit_length_counter_t length_counter;
} apu_channel_noise_t;

typedef struct {
  // Memory reader??
  bool interrupt_flag;
  // Sample buffer??
  apu_unit_timer_t timer;
  uint8_t output_level;
} apu_channel_dmc_t;
