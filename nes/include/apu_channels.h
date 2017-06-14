#pragma once

#include "apu_units.h"

typedef struct {
  apu_unit_envelope_t envelope;
  apu_unit_sweep_t sweep;
  apu_unit_timer_t timer;
  apu_unit_length_counter_t length_counter;

  // 8-step sequencer
  uint8_t current_sequence_position;
  uint8_t duty_cycle_value;
  uint8_t c_duty_cycle;
} apu_channel_pulse_t;

typedef struct {
  apu_unit_timer_t timer;
  apu_unit_length_counter_t length_counter;

  uint8_t linear_counter;
  bool linear_counter_reload_flag;
  bool control_flag;
  uint8_t c_linear_counter_reload;

  // Sequencer
  uint8_t current_sequence_position;
  uint8_t duty_cycle_value;
} apu_channel_triangle_t;

typedef struct {
  apu_unit_envelope_t envelope;
  apu_unit_timer_t timer;
  apu_unit_length_counter_t length_counter;

  bool mode_flag;
  uint16_t shift_register;
} apu_channel_noise_t;

typedef struct {
  // Memory reader??
  bool interrupt_flag;
  // Sample buffer??
  apu_unit_timer_t timer;
  uint8_t output_level;
} apu_channel_dmc_t;

void apu_channel_pulse_sequencer_clock(apu_timer_context_t* context);

void apu_channel_triangle_sequencer_clock(apu_timer_context_t* context);

void apu_channel_triangle_linear_counter_clock(apu_channel_triangle_t* channel);

void apu_channel_noise_lfsr_clock(apu_timer_context_t* context);
