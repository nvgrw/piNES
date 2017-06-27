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

#include <math.h>
#include <stdio.h>

#include "apu.h"
#include "cpu.h"

apu_t* apu_init(void) {
  apu_t* apu = calloc(1, sizeof(apu_t));
  apu->sample_skips = 0.0;
  apu->buffer_cursor = 0;
  apu->is_even_cycle = false;

  // Set up shift register
  apu->channel_noise.shift_register = 1;

  // Link timer period to sweep
  apu->channel_pulse1.sweep.c_timer_period =
      &apu->channel_pulse1.timer.c_timer_period;
  apu->channel_pulse2.sweep.c_timer_period =
      &apu->channel_pulse2.timer.c_timer_period;

  // Calculate the mixer lookup tables
  // Formulae from here:
  // https://wiki.nesdev.com/w/index.php/APU_Mixer#Lookup_Table
  for (int i = 0; i < LU_PULSE_SIZE; i++) {
    apu->lookup_pulse_table[i] = 95.52 / (8128.0 / i + 100);
  }

  for (int i = 0; i < LU_TND_SIZE; i++) {
    apu->lookup_tnd_table[i] = 163.67 / (24329.0 / i + 100);
  }

  return apu;
}

#define AR(type) type r = {.raw = val}

// TODO: Move to region.h, these are NTSC-specific periods
static const int TIMER_PERIODS[] = {4,   8,   16,  32,  64,  96,   128,  160,
                                    202, 254, 380, 508, 762, 1016, 2034, 4068};

static void apu_frame_counter_clock_half_frame(apu_t* apu);

static void apu_frame_counter_clock_quarter_frame(apu_t* apu);

void apu_mem_write(apu_t* apu, uint16_t address, uint8_t val) {
  switch (address) {
    case 0x4000: {
      AR(apu_register_4000_4004_t);
      apu->channel_pulse1.envelope.c_volume_envelope =
          r.data_period.divider_period;
      apu->channel_pulse1.envelope.c_constant_volume_flag =
          r.data_period.constant_volume_envelope;
      apu->channel_pulse1.envelope.c_env_loop_flag =
          r.data_period.length_counter_halt;
      apu->channel_pulse1.length_counter.c_length_counter_hold =
          r.data_period.length_counter_halt;
      apu->channel_pulse1.envelope.start_flag = true;
    } break;
    case 0x4001: {
      AR(apu_register_4001_4005_t);
      apu->channel_pulse1.sweep.c_enabled = r.data.enabled;
      apu->channel_pulse1.sweep.c_divider_period = r.data.period;
      apu->channel_pulse1.sweep.c_negate = r.data.negate;
      apu->channel_pulse1.sweep.c_shift_count = r.data.shift_count;
      // Set sweep period pointer
    } break;
    case 0x4002: {
      AR(apu_register_4002_4006_t);
      apu->channel_pulse1.timer.c_timer_period &= 0xFF00;
      apu->channel_pulse1.timer.c_timer_period |= r.data.timer_low;
    } break;
    case 0x4003: {
      AR(apu_register_4003_4007_t);
      apu->channel_pulse1.timer.c_timer_period &= 0xFF;
      apu->channel_pulse1.timer.c_timer_period |=
          ((uint16_t)r.data_period.timer_high) << 8;
      apu->channel_pulse1.length_counter.c_length_counter_load =
          r.data_period.length_counter_load;
      if (apu->previous_status.data.enable_pulse1) {
        apu_unit_length_counter_reload(&apu->channel_pulse1.length_counter);
      }
      // Sequencer restarted
      apu->channel_pulse1.current_sequence_position = 0;
      // Envelope restarted
      apu->channel_pulse1.envelope.start_flag = true;
    } break;
    case 0x4004: {
      AR(apu_register_4000_4004_t);
      apu->channel_pulse2.envelope.c_volume_envelope =
          r.data_period.divider_period;
      apu->channel_pulse2.envelope.c_constant_volume_flag =
          r.data_period.constant_volume_envelope;
      apu->channel_pulse2.envelope.c_env_loop_flag =
          r.data_period.length_counter_halt;
      apu->channel_pulse2.length_counter.c_length_counter_hold =
          r.data_period.length_counter_halt;
      apu->channel_pulse2.envelope.start_flag = true;
    } break;
    case 0x4005: {
      AR(apu_register_4001_4005_t);
      apu->channel_pulse2.sweep.c_enabled = r.data.enabled;
      apu->channel_pulse2.sweep.c_divider_period = r.data.period;
      apu->channel_pulse2.sweep.c_negate = r.data.negate;
      apu->channel_pulse2.sweep.c_shift_count = r.data.shift_count;
    } break;
    case 0x4006: {
      AR(apu_register_4002_4006_t);
      apu->channel_pulse2.timer.c_timer_period &= 0xFF00;
      apu->channel_pulse2.timer.c_timer_period |= r.data.timer_low;
    } break;
    case 0x4007: {
      AR(apu_register_4003_4007_t);
      apu->channel_pulse2.timer.c_timer_period &= 0xFF;
      apu->channel_pulse2.timer.c_timer_period |=
          ((uint16_t)r.data_period.timer_high) << 8;
      apu->channel_pulse2.length_counter.c_length_counter_load =
          r.data_period.length_counter_load;
      if (apu->previous_status.data.enable_pulse2) {
        apu_unit_length_counter_reload(&apu->channel_pulse2.length_counter);
      }
      // Sequencer restarted
      apu->channel_pulse2.current_sequence_position = 0;
      // Envelope restarted
      apu->channel_pulse2.envelope.start_flag = true;
    } break;
    case 0x4008: {
      AR(apu_register_4008_t);
      apu->channel_triangle.length_counter.c_length_counter_hold =
          r.data.control;
      apu->channel_triangle.control_flag = r.data.control;
      apu->channel_triangle.c_linear_counter_reload =
          r.data.linear_counter_reload;
    } break;
    case 0x400A: {
      AR(apu_register_400a_t);
      apu->channel_triangle.timer.c_timer_period &= 0xFF00;
      apu->channel_triangle.timer.c_timer_period |= r.data.timer_low;
    } break;
    case 0x400B: {
      AR(apu_register_400b_t);
      apu->channel_triangle.timer.c_timer_period &= 0xFF;
      apu->channel_triangle.timer.c_timer_period |=
          ((uint16_t)r.data.timer_high) << 8;
      apu->channel_triangle.length_counter.c_length_counter_load =
          r.data.length_counter_load;
      if (apu->previous_status.data.enable_triangle) {
        apu_unit_length_counter_reload(&apu->channel_triangle.length_counter);
      }
      apu->channel_triangle.linear_counter_reload_flag = true;
    } break;
    case 0x400C: {
      AR(apu_register_4000_4004_t);
      apu->channel_noise.envelope.c_constant_volume_flag =
          r.data_noise.constant_volume_envelope;
      apu->channel_noise.envelope.c_volume_envelope =
          r.data_noise.divider_period;
      apu->channel_noise.envelope.c_env_loop_flag =
          r.data_noise.length_counter_halt;
      apu->channel_noise.length_counter.c_length_counter_hold =
          r.data_noise.length_counter_halt;
    } break;
    case 0x400E: {
      AR(apu_register_400e_t);
      apu->channel_noise.timer.c_timer_period = TIMER_PERIODS[r.data.period];
      apu->channel_noise.mode_flag = r.data.mode;
    } break;
    case 0x400F: {
      AR(apu_register_4003_4007_t);
      apu->channel_noise.length_counter.c_length_counter_load =
          r.data_noise.length_counter_load;
      if (apu->previous_status.data.enable_noise) {
        apu_unit_length_counter_reload(&apu->channel_noise.length_counter);
      }
      // Envelope restarted
      apu->channel_noise.envelope.start_flag = true;
    } break;
    // TODO: DMC Channels
    case 0x4015: {
      AR(apu_register_4015_status_t);
#define ENABLED_CHECK(CHNEL)                                                   \
  do {                                                                         \
    if (!apu->previous_status.data.enable_##CHNEL && r.data.enable_##CHNEL) {  \
      apu_unit_length_counter_onenable(&apu->channel_##CHNEL.length_counter);  \
    }                                                                          \
    if (apu->previous_status.data.enable_##CHNEL && !r.data.enable_##CHNEL) {  \
      apu_unit_length_counter_ondisable(&apu->channel_##CHNEL.length_counter); \
    }                                                                          \
  } while (0);
      // TODO: Handle DMC (e.g. clearing DMC flag)
      ENABLED_CHECK(pulse1);
      ENABLED_CHECK(pulse2);
      ENABLED_CHECK(noise);
      ENABLED_CHECK(triangle);
#undef ENABLED_CHECK

      // DMC Interrupt cleared by write
      r.data.dmc_interrupt = 0;

      // Reset length counters
      if (!r.data.enable_pulse1) {
        apu->channel_pulse1.length_counter.length_counter = 0;
      }

      if (!r.data.enable_pulse2) {
        apu->channel_pulse2.length_counter.length_counter = 0;
      }

      if (!r.data.enable_noise) {
        apu->channel_noise.length_counter.length_counter = 0;
      }

      if (!r.data.enable_triangle) {
        apu->channel_triangle.length_counter.length_counter = 0;
      }

      // Store the new status
      apu->previous_status.raw = r.raw;
    } break;
    case 0x4017: {
      AR(apu_register_4017_frame_counter_t);
      apu->frame_counter.mode_flag = r.data.mode;
      apu->frame_counter.irq_inhibit_flag = r.data.irq_inhibit;
      if (r.data.irq_inhibit) {
        // Setting irq inhibit clears frame interrupt
        apu->previous_status.data.frame_interrupt = false;
      }
      apu->frame_counter.reset_queued = true;
      if (!apu->is_even_cycle) {  // An even APU cycle, as APU clocked after CPU
        apu->frame_counter.reset_queue_divider = 2;
      } else {
        apu->frame_counter.reset_queue_divider = 3;
      }

      if (r.data.mode) {
        // If mode 1 set, also clock all units.
        apu_frame_counter_clock_half_frame(apu);
        apu_frame_counter_clock_quarter_frame(apu);
      }
    } break;
    default:
      break;
  }
}

uint8_t apu_mem_read(apu_t* apu, uint16_t address) {
  if (address == 0x4015) {  // Status register
    apu_register_4015_status_t status = {.raw = 0};
#define LENGTH_COUNTER_STATUS(CHNEL) \
  status.data.enable_##CHNEL =       \
      apu->channel_##CHNEL.length_counter.length_counter > 0

    LENGTH_COUNTER_STATUS(pulse1);
    LENGTH_COUNTER_STATUS(pulse2);
    LENGTH_COUNTER_STATUS(triangle);
    LENGTH_COUNTER_STATUS(noise);

#undef LENGTH_COUNTER_STATUS

    // TODO: Handle DMC
    status.data.enable_dmc = 0;

    // Reading clears frame interrupt but returns old value
    status.data.frame_interrupt = apu->previous_status.data.frame_interrupt;
    apu->previous_status.data.frame_interrupt = 0;

    status.data.dmc_interrupt = apu->previous_status.data.dmc_interrupt;

    return status.raw;
  }

  return -1;
}

// ----- TIMER -----
static void apu_timer_clock(apu_t* apu) {
  if (apu->is_even_cycle) {
    apu_unit_timer_clock(&apu->channel_pulse1.timer, &apu->channel_pulse1,
                         apu_channel_pulse_sequencer_clock);
    apu_unit_timer_clock(&apu->channel_pulse2.timer, &apu->channel_pulse2,
                         apu_channel_pulse_sequencer_clock);
    apu_unit_timer_clock(&apu->channel_noise.timer, &apu->channel_noise,
                         apu_channel_noise_lfsr_clock);
    apu_unit_timer_clock(&apu->channel_dmc.timer, NULL, NULL);
  }

  apu_unit_timer_clock(&apu->channel_triangle.timer, &apu->channel_triangle,
                       apu_channel_triangle_sequencer_clock);
}

// ----- FRAME COUNTER -----
#define FC_SEQ_0_LEN 4
#define FC_SEQ_1_LEN 5
static const uint16_t FRAME_COUNTER_SEQUENCE_M0[FC_SEQ_0_LEN] = {3729, 7457,
                                                                 11186, 14914};
static const uint16_t FRAME_COUNTER_SEQUENCE_M1[FC_SEQ_1_LEN] = {
    3729, 7457, 11186, 14915, 18641};

static void apu_frame_counter_clock_half_frame(apu_t* apu) {
  // Length counters & sweep units
  apu_unit_length_counter_clock(&apu->channel_pulse1.length_counter);
  apu_unit_length_counter_clock(&apu->channel_pulse2.length_counter);
  apu_unit_length_counter_clock(&apu->channel_triangle.length_counter);
  apu_unit_length_counter_clock(&apu->channel_noise.length_counter);

  apu_unit_sweep_clock(&apu->channel_pulse1.sweep, true);
  apu_unit_sweep_clock(&apu->channel_pulse2.sweep, false);
}

static void apu_frame_counter_clock_quarter_frame(apu_t* apu) {
  // Envelopes & triangle's linear counter
  apu_unit_envelope_clock(&apu->channel_pulse1.envelope);
  apu_unit_envelope_clock(&apu->channel_pulse2.envelope);
  apu_unit_envelope_clock(&apu->channel_noise.envelope);

  apu_channel_triangle_linear_counter_clock(&apu->channel_triangle);
}

static void apu_frame_counter_interrupt(apu_t* apu) {
  if (!apu->frame_counter.irq_inhibit_flag) {
    apu->previous_status.data.frame_interrupt = true;
    // Issue interrupt
    cpu_interrupt(apu->mapper->cpu, INTRT_IRQ);
  }
}

static void apu_frame_counter_reset_now(apu_t* apu) {
  apu->frame_counter.cycle_index = 0;
  apu->frame_counter.cycles = 0;
  apu->frame_counter.reset_queued = false;
  apu->frame_counter.reset_queue_divider = 0;
}

static void apu_frame_counter_clock(apu_t* apu) {
  if (apu->frame_counter.mode_flag == 0) {
    if (apu->frame_counter.cycles ==
        FRAME_COUNTER_SEQUENCE_M0[apu->frame_counter.cycle_index]) {
      switch (apu->frame_counter.cycle_index) {
        case 0:
        case 2:
          apu_frame_counter_clock_quarter_frame(apu);
          break;
        case 1:
          apu_frame_counter_clock_quarter_frame(apu);
          apu_frame_counter_clock_half_frame(apu);
          break;
        case 3:
          apu_frame_counter_clock_quarter_frame(apu);
          apu_frame_counter_clock_half_frame(apu);
          // TODO: This is called one cycle too late
          apu_frame_counter_interrupt(apu);
          break;
      }
    }
  } else {
    if (apu->frame_counter.cycles ==
        FRAME_COUNTER_SEQUENCE_M1[apu->frame_counter.cycle_index]) {
      switch (apu->frame_counter.cycle_index) {
        case 0:
        case 2:
          apu_frame_counter_clock_quarter_frame(apu);
          break;
        case 1:
        case 4:
          apu_frame_counter_clock_quarter_frame(apu);
          apu_frame_counter_clock_half_frame(apu);
          break;
      }
    }
  }

  // Reset the frame counter if necessary. Statement could be merged with above.
  if (apu->frame_counter.mode_flag == 0 &&
      apu->frame_counter.cycles ==
          FRAME_COUNTER_SEQUENCE_M0[apu->frame_counter.cycle_index]) {
    apu->frame_counter.cycle_index++;
    if (apu->frame_counter.cycle_index >= FC_SEQ_0_LEN) {
      apu_frame_counter_reset_now(apu);
    }
  } else if (apu->frame_counter.mode_flag == 1 &&
             apu->frame_counter.cycles ==
                 FRAME_COUNTER_SEQUENCE_M1[apu->frame_counter.cycle_index]) {
    apu->frame_counter.cycle_index++;
    if (apu->frame_counter.cycle_index >= FC_SEQ_1_LEN) {
      apu_frame_counter_reset_now(apu);
    }
  } else {
    apu->frame_counter.cycles++;

    if (apu->frame_counter.reset_queued == true) {
      apu->frame_counter.reset_queue_divider--;
      if (apu->frame_counter.reset_queue_divider == 0) {
        apu_frame_counter_reset_now(apu);
      }
    }
  }
}

// ----- OUTPUT -----
static uint8_t apu_output_pulse(bool enabled, apu_channel_pulse_t* channel) {
  if (!enabled) return 0;

  if (channel->duty_cycle_value == 0) return 0;
  if ((*channel->sweep.c_timer_period) > 0x7FF) return 0;
  if (channel->length_counter.length_counter == 0) return 0;
  if (channel->timer.divider < 8) return 0;

  return apu_unit_envelope_output(&channel->envelope);
}

static uint8_t apu_output_noise(bool enabled, apu_channel_noise_t* channel) {
  if (!enabled) return 0;

  if ((channel->shift_register & 0x1) == 0x1) return 0;
  if (channel->length_counter.length_counter == 0) return 0;

  return apu_unit_envelope_output(&channel->envelope);
}

static uint8_t apu_output_triangle(bool enabled,
                                   apu_channel_triangle_t* channel) {
  if (!enabled) return 0;

  if (channel->linear_counter == 0) return 0;
  if (channel->length_counter.length_counter == 0) return 0;

  return channel->duty_cycle_value;
}

static uint8_t apu_output_dmc(bool enabled, apu_channel_dmc_t* channel) {
  if (!enabled) return 0;
  return 0;
}

static apu_buffer_t apu_mix(apu_t* apu) {
  uint8_t pulse1_out = apu_output_pulse(apu->previous_status.data.enable_pulse1,
                                        &apu->channel_pulse1);
  uint8_t pulse2_out = apu_output_pulse(apu->previous_status.data.enable_pulse2,
                                        &apu->channel_pulse2);
  uint8_t triangle_out = apu_output_triangle(
      apu->previous_status.data.enable_triangle, &apu->channel_triangle);
  uint8_t noise_out = apu_output_noise(apu->previous_status.data.enable_noise,
                                       &apu->channel_noise);
  uint8_t dmc_out =
      apu_output_dmc(apu->previous_status.data.enable_dmc, &apu->channel_dmc);

  double pulse_out = apu->lookup_pulse_table[pulse1_out + pulse2_out];
  double tnd_out =
      apu->lookup_tnd_table[3 * triangle_out + 2 * noise_out + dmc_out];
  return (float)(pulse_out + tnd_out);
}

// ----- REST -----
static void apu_write_to_buffer(apu_t* apu, apu_buffer_t value) {
  apu->buffer[apu->buffer_cursor] = value;
  apu->buffer_cursor++;
  apu->buffer_cursor %= AUDIO_BUFFER_SIZE;
}

void apu_cycle(apu_t* apu, void* context, apu_enqueue_audio_t enqueue_audio,
               apu_get_queue_size_t get_queue_size) {
  // Timer clock
  apu_timer_clock(apu);
  apu->is_even_cycle = !apu->is_even_cycle;

  // Frame counter
  apu_frame_counter_clock(apu);

  // Skip samples / downsample
  if (apu->sample_skips <= 1.0) {
    apu_write_to_buffer(apu, apu_mix(apu));
    if (apu->buffer_cursor == 0) {
      enqueue_audio(context, apu->buffer, AUDIO_BUFFER_SIZE);
    }

    apu->sample_skips += APU_SAMPLE_RATE / APU_ACTUAL_SAMPLE_RATE;
  } else {
    apu->sample_skips -= 1.0;
  }
}
