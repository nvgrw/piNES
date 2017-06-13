#include <math.h>
#include <stdio.h>

#include "apu.h"
#include "cpu.h"

#define START_CHANNELS 0x4000
#define END_CHANNELS 0x4013
#define STATUS 0x4015
#define FRAME_COUNTER 0x4017

#define PULSE1_REG1 0x4000
#define PULSE1_REG2 0x4001
#define PULSE1_REG3 0x4002
#define PULSE1_REG4 0x4003

#define PULSE2_REG1 0x4004
#define PULSE2_REG2 0x4005
#define PULSE2_REG3 0x4006
#define PULSE2_REG4 0x4007

#define TRIANGLE_REG1 0x4008
#define TRIANGLE_REG2 0x400A
#define TRIANGLE_REG3 0x400B

#define NOISE_REG1 0x400C
#define NOISE_REG2 0x400E
#define NOISE_REG3 0x400F

#define DMC_REG1 0x4010
#define DMC_REG2 0x4011
#define DMC_REG3 0x4012
#define DMC_REG4 0x4013

#define CONTROL 0x4015
#define STATUS 0x4015
#define FRAME_COUNTER 0x4017

const uint8_t APU_LENGTH_TABLE[] = {
    0x0A, 0xFE, 0x14, 0x02, 0x28, 0x04, 0x50, 0x06, 0xA0, 0x08, 0x3C,
    0x0A, 0x0E, 0x0C, 0x1A, 0x0E, 0x0C, 0x10, 0x18, 0x12, 0x30, 0x14,
    0x60, 0x16, 0xC0, 0x18, 0x48, 0x1A, 0x10, 0x1C, 0x20, 0x1E};

const uint8_t APU_DUTY_TABLE[] = {0x40, 0x60, 0x78, 0x9f};

const uint16_t APU_NOISE_TABLE[] = {4,   8,   16,  32,  64,  96,   128,  160,
                                    202, 254, 380, 508, 762, 1016, 2034, 4068};

apu_t* apu_init(void) { return calloc(1, sizeof(apu_t)); }

/* ----- ENVELOPE ----- */
static void apu_clock_specific_envelope(const apu_dve dve,
                                        apu_unit_envelope* envelope) {
  if (!envelope->start_flag) {
    if (envelope->divider == 0) {
      envelope->divider = dve.data.divider_period;
      if (envelope->decay_level_counter != 0) {
        envelope->decay_level_counter--;
      } else if (dve.data.length_counter_halt) {
        envelope->decay_level_counter = 15;
      }
    } else {
      envelope->divider--;
    }
  } else {
    envelope->start_flag = false;
    envelope->decay_level_counter = 15;
    envelope->divider = dve.data.divider_period;
  }

  if (dve.data.constant_volume_envelope) {
    envelope->last_value = 15;
  } else {
    envelope->last_value = envelope->decay_level_counter;
  }
}

static void apu_clock_envelope(apu_t* apu) {
  // Update envelopes
  {
    apu_dve dve = {.raw = mmap_cpu_read(apu->mapper, PULSE1_REG1, false)};
    apu_clock_specific_envelope(dve, &apu->pulse1_envelope);
  }
  // {
  //   apu_dve dve = {.raw = mmap_cpu_read(apu->mapper, PULSE2_REG1, false)};
  //   apu_clock_specific_envelope(dve, &apu->pulse2_envelope);
  // }
  // {
  //   apu_dve dve = {.raw = mmap_cpu_read(apu->mapper, NOISE_REG1, false)};
  //   apu_clock_specific_envelope(dve, &apu->noise_envelope);
  // }
}

/* ----- SWEEP ----- */
static void apu_clock_specific_sweep(apu_t* apu, const register_4001_4005 flags,
                                     apu_unit_sweep* unit, uint16_t reg3,
                                     uint16_t reg4) {
  register_4002_4006 par_b = {.raw = mmap_cpu_read(apu->mapper, reg3, false)};
  apu_lclt par_c = {.raw = mmap_cpu_read(apu->mapper, reg4, false)};
  uint16_t raw_period = (((uint16_t)par_c.data_period.timer_high) << 8) |
                        (uint16_t)par_b.data.timer_low;

  // if (!flags.data.enabled) {
  //   unit->last_period = raw_period;
  //   return;
  // }

  uint16_t target_period = raw_period >> flags.data.shift_count;
  if (!flags.data.negate) {
    target_period = raw_period + target_period;
  } else {
    target_period = raw_period - target_period;
  }

  if (unit->reload) {
    if (unit->divider == 0 && flags.data.enabled) {
      unit->last_period = target_period;
    }
    unit->divider = flags.data.period;
    unit->reload = false;
  } else {
    if (unit->divider != 0) {
      unit->divider--;
    } else if (flags.data.enabled) {
      unit->divider = flags.data.period;
      unit->last_period = target_period;
    }
  }
}

/* ----- PULSE CALCULATION ----- */
static void apu_pulse1_calculate(apu_t* apu) {
  apu_dve par_a = {.raw = mmap_cpu_read(apu->mapper, PULSE1_REG1, false)};
  // register_4002_4006 par_b = {.raw = mmap_cpu_read(apu->mapper,
  // PULSE1_REG3, false)}; apu_lclt par_c = {.raw = mmap_cpu_read(apu->mapper,
  // PULSE1_REG4, false)};

  //   uint16_t raw_period = (((uint16_t)par_c.data_period.timer_high) << 8) |
  //                         (uint16_t)par_b.data.timer_low;

  if (apu->pulse1_channel.sequence_counter == 0) {
    // The waveform generator is clocked every time the counter goes to zero
    // Either use sweep unit period or raw period, depending on mute settings.
    // apu->pulse1_channel.sequence_counter = raw_period;
    apu->pulse1_channel.sequence_counter = apu->pulse1_sweep.last_period;
    apu->pulse1_channel.initial_sequence_counter =
        apu->pulse1_sweep.last_period;
  } else {
    apu->pulse1_channel.sequence_counter--;
  }
  // TODO: Implement duty cycle sequences
  // & pulse 2

  const double b = apu->pulse1_channel.initial_sequence_counter;
  if (b < 0.001) {
    apu->last_pulse1 = 0;
    return;
  }

  const double a = apu->pulse1_channel.sequence_counter;
  const uint8_t progress = ~(uint8_t)(8 * a / b) & 0x7;

  if ((APU_DUTY_TABLE[par_a.data_period.duty_cycle] & (1 << progress)) != 0) {
    apu->last_pulse1 = apu->pulse1_envelope.last_value;
  } else {
    apu->last_pulse1 = 0;
  }
}

// static void apu_pulse2_calculate(apu* apu) {}

// static void apu_triangle_calculate(apu* apu) {}

// static void apu_noise_calculate(apu* apu) {}

// static void apu_dmc_calculate(apu* apuc) {}

// static uint8_t apu_pulses_output(apu* apu) {
//   // This formula is a linear approximation
//   return 0.00752 * (apu->last_pulse1 + apu->last_pulse2);
// }

// static uint8_t apu_tnd_output(apu* apu) {
//   return 0.00851 * apu->last_triangle + 0.00494 * apu->last_noise +
//          0.00335 * apu->last_dmc;
// }

void apu_mem_write(apu_t* apu, uint16_t address, uint8_t val) {
  if (PULSE1_REG4 == address) {
    apu->pulse1_envelope.start_flag = true;
  }

  if (PULSE2_REG4 == address) {
    apu->pulse2_envelope.start_flag = true;
  }

  if (NOISE_REG3 == address) {
    apu->noise_envelope.start_flag = true;
  }

  if (PULSE1_REG2 == address) {
    apu->pulse1_sweep.reload = true;
  }

  if (PULSE2_REG2 == address) {
    apu->pulse2_sweep.reload = true;
  }
}

static void apu_write_to_buffer(apu_t* apu, uint8_t value) {
  apu->buffer[apu->buffer_cursor] = value;
  apu->buffer_cursor++;
  apu->buffer_cursor = apu->buffer_cursor % AUDIO_BUFFER_SIZE;
}

static void apu_sequencer_cycle(apu_t* apu) {
  // Frame counter (drives envelope)
  register_4017_frame_counter fc = {
      .raw = mmap_cpu_read(apu->mapper, FRAME_COUNTER, false)};
  register_4015_status status = {.raw =
                                     mmap_cpu_read(apu->mapper, STATUS, false)};
  if (fc.data.mode == 0) {
    // Counter mode 0
    if (apu->sequencer_elapsed_apu_cycles >= 14915) {
      register_4001_4005 flags_pulse1 = {
          .raw = mmap_cpu_read(apu->mapper, PULSE1_REG2, false)};
      apu_clock_specific_sweep(apu, flags_pulse1, &apu->pulse1_sweep,
                               PULSE1_REG3, PULSE1_REG4);
      // register_4001_4005 flags_pulse2 = {
      //     .raw = mmap_cpu_read(apu->mapper, PULSE2_REG2, false)};
      // apu_clock_specific_sweep(apu, flags_pulse2, &apu->pulse2_sweep,
      //                          PULSE2_REG3, PULSE2_REG4);
      apu->sequencer_elapsed_apu_cycles = 0;
    } else if (apu->sequencer_elapsed_apu_cycles >= 14914) {
      if (!fc.data.irq_inhibit) {
        // Set status frame interrupt flag to 1 & issue IRQ
        status.data.frame_interrupt = 1;
        mmap_cpu_write(apu->mapper, STATUS, status.raw);
        cpu_interrupt(apu->mapper->cpu, INTRT_IRQ);
      } else if (status.data.frame_interrupt) {
        // Setting the inhibit flag will unset the interrupt flag
        // TODO: So does reading $4015
        status.data.frame_interrupt = 0;
        mmap_cpu_write(apu->mapper, STATUS, status.raw);
      }
    } else if (apu->sequencer_elapsed_apu_cycles >= 11186) {
      apu_clock_envelope(apu);
    } else if (apu->sequencer_elapsed_apu_cycles >= 7457) {
      apu_clock_envelope(apu);
      register_4001_4005 flags_pulse1 = {
          .raw = mmap_cpu_read(apu->mapper, PULSE1_REG2, false)};
      apu_clock_specific_sweep(apu, flags_pulse1, &apu->pulse1_sweep,
                               PULSE1_REG3, PULSE1_REG4);
      // register_4001_4005 flags_pulse2 = {
      //     .raw = mmap_cpu_read(apu->mapper, PULSE2_REG2, false)};
      // apu_clock_specific_sweep(apu, flags_pulse2, &apu->pulse2_sweep,
      //                          PULSE2_REG3, PULSE2_REG4);
    } else if (apu->sequencer_elapsed_apu_cycles >= 3729) {
      apu_clock_envelope(apu);
    }
  } else if (fc.data.mode == 1) {
    // Counter mode 1
    if (apu->sequencer_elapsed_apu_cycles >= 18641) {
      apu_clock_envelope(apu);
      register_4001_4005 flags_pulse1 = {
          .raw = mmap_cpu_read(apu->mapper, PULSE1_REG2, false)};
      apu_clock_specific_sweep(apu, flags_pulse1, &apu->pulse1_sweep,
                               PULSE1_REG3, PULSE1_REG4);
      // register_4001_4005 flags_pulse2 = {
      //     .raw = mmap_cpu_read(apu->mapper, PULSE2_REG2, false)};
      // apu_clock_specific_sweep(apu, flags_pulse2, &apu->pulse2_sweep,
      //                          PULSE2_REG3, PULSE2_REG4);
      apu->sequencer_elapsed_apu_cycles = 0;
    } else if (apu->sequencer_elapsed_apu_cycles >= 14915) {
    } else if (apu->sequencer_elapsed_apu_cycles >= 11186) {
      apu_clock_envelope(apu);
    } else if (apu->sequencer_elapsed_apu_cycles >= 7457) {
      apu_clock_envelope(apu);
      register_4001_4005 flags_pulse1 = {
          .raw = mmap_cpu_read(apu->mapper, PULSE1_REG2, false)};
      apu_clock_specific_sweep(apu, flags_pulse1, &apu->pulse1_sweep,
                               PULSE1_REG3, PULSE1_REG4);
      // register_4001_4005 flags_pulse2 = {
      //     .raw = mmap_cpu_read(apu->mapper, PULSE2_REG2, false)};
      // apu_clock_specific_sweep(apu, flags_pulse2, &apu->pulse2_sweep,
      //                          PULSE2_REG3, PULSE2_REG4);
    } else if (apu->sequencer_elapsed_apu_cycles >= 3729) {
      apu_clock_envelope(apu);
    }
  }

  apu->sequencer_elapsed_apu_cycles += 2;
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
    apu_sequencer_cycle(apu);
  }
  apu->is_even_cycle = !apu->is_even_cycle;

  apu_pulse1_calculate(apu);
  uint8_t val = apu->last_pulse1;

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
