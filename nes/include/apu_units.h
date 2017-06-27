/**
 * MIT License
 *
 * Copyright (c) 2017
 * Aurel Bily, Alexis I. Marinoiu, Andrei V. Serbanescu, Niklas Vangerow
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

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
  bool c_constant_volume_flag;
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
uint8_t apu_unit_envelope_output(apu_unit_envelope_t* unit);

void apu_unit_sweep_sweep(apu_unit_sweep_t* unit, bool ones_complement);
void apu_unit_sweep_clock(apu_unit_sweep_t* unit, bool ones_complement);

typedef void apu_timer_context_t;
typedef void (*apu_timer_clock_t)(apu_timer_context_t* context);

void apu_unit_timer_clock(apu_unit_timer_t* unit, apu_timer_context_t* context,
                          apu_timer_clock_t on_clock);

void apu_unit_length_counter_reload(apu_unit_length_counter_t* unit);

void apu_unit_length_counter_onenable(apu_unit_length_counter_t* unit);
void apu_unit_length_counter_ondisable(apu_unit_length_counter_t* unit);
void apu_unit_length_counter_clock(apu_unit_length_counter_t* unit);
