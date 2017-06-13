#include <math.h>
#include <stdio.h>

#include "apu.h"
#include "cpu.h"

apu_t* apu_init(void) { return calloc(1, sizeof(apu_t)); }

#define AR(type) type r = {.raw = mmap_cpu_read(apu->mapper, address, false)}

void apu_mem_write(apu_t* apu, uint16_t address, uint8_t val) {
  switch (address) {
    case 0x4000: {
      AR(apu_register_4000_4004_t);
      apu->channel_pulse1.envelope.c_volume_envelope =
          r.data_period.divider_period;
      apu->channel_pulse1.envelope.c_env_loop_flag =
          r.data_period.constant_volume_envelope;
      apu->channel_pulse1.length_counter.c_length_counter_hold =
          r.data_period.length_counter_halt;
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
      apu->channel_pulse1.length_counter.c_length_counter_hold =
          r.data_period.length_counter_load;
      // TODO: Side effect-sequencer restarted at first value of the sequence
      // Envelope restarted
      apu->channel_pulse1.envelope.start_flag = true;
    } break;
    case 0x4004: {
      AR(apu_register_4000_4004_t);
      apu->channel_pulse2.envelope.c_volume_envelope =
          r.data_period.divider_period;
      apu->channel_pulse2.envelope.c_env_loop_flag =
          r.data_period.constant_volume_envelope;
      apu->channel_pulse2.length_counter.c_length_counter_hold =
          r.data_period.length_counter_halt;
    } break;
    case 0x4005: {
      AR(apu_register_4001_4005_t);
      apu->channel_pulse2.sweep.c_enabled = r.data.enabled;
      apu->channel_pulse2.sweep.c_divider_period = r.data.period;
      apu->channel_pulse2.sweep.c_negate = r.data.negate;
      apu->channel_pulse2.sweep.c_shift_count = r.data.shift_count;
      // Set sweep period pointer
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
      apu->channel_pulse2.length_counter.c_length_counter_hold =
          r.data_period.length_counter_load;
      // TODO: Side effect-sequencer restarted at first value of the sequence
      // Envelope restarted
      apu->channel_pulse2.envelope.start_flag = true;
    } break;
    case 0x4008: {
      AR(apu_register_4008_t);
      apu->channel_triangle.length_counter.c_length_counter_hold =
          r.data.control;
      // TODO: Handle length counter load flag
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
      apu->channel_triangle.linear_counter_reload_flag = true;
    } break;
    // TODO: Continue with registers
    default:
      break;
  }
}

static void apu_write_to_buffer(apu_t* apu, uint8_t value) {
  apu->buffer[apu->buffer_cursor] = value;
  apu->buffer_cursor++;
  apu->buffer_cursor = apu->buffer_cursor % AUDIO_BUFFER_SIZE;
}

void apu_cycle(apu_t* apu, void* context,
               void (*enqueue_audio)(void* context, uint8_t* buffer, int len)) {
  // This function is called at the rate of the PPU. Only do something useful
  // every 3 cycles.

  if (apu->cycle_count != 0) {
    apu->cycle_count--;
    return;
  }

  apu->cycle_count = 2;

  // Frame counter
  if (apu->is_even_cycle) {
  }
  apu->is_even_cycle = !apu->is_even_cycle;

  uint8_t val = 0;

  // Create output
  // uint8_t val = apu_pulses_output(apu) + apu_tnd_output(apu);

  // Skip samples/ downsample
  if (apu->sample_skips <= 1.0) {
    apu_write_to_buffer(apu, val);
    if (apu->buffer_cursor == 0) {
      enqueue_audio(context, apu->buffer, AUDIO_BUFFER_SIZE);
    }
    apu->sample_skips += APU_SAMPLE_RATE / APU_ACTUAL_SAMPLE_RATE;
  } else {
    apu->sample_skips -= 1.0;
  }
}
