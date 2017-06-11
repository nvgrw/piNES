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

apu* apu_init(void) { return calloc(1, sizeof(apu)); }

uint8_t apu_clock_envelope(const apu_dve* dve, apu_envelope* envelope) {
  if (!envelope->start_flag) {
    if (envelope->divider == 0) {
      envelope->divider = dve->data.divider_period;
      if (envelope->decay_level_counter != 0) {
        envelope->decay_level_counter--;
      } else if (dve->data.length_counter_halt) {
        envelope->decay_level_counter = 15;
      }
    } else {
      envelope->divider--;
    }
  } else {
    envelope->start_flag = false;
    envelope->decay_level_counter = 15;
    envelope->divider = dve->data.divider_period;
  }

  if (dve->data.constant_volume_envelope) {
    return 15;
  } else {
    return envelope->decay_level_counter;
  }
}

void apu_pulse1_calculate(apu* apu) {
  // Components of the pulse channel
  // Envelope
  // Sweep
  // Length counter
  //
  // The envelope is the way that a sound's parameter changes over time. The NES
  // emvelope generates a decreasing saw envelope with optional looping, or a
  // constant volume.

  apu_dve par_a = {.raw = mmap_cpu_read(apu->mapper, PULSE1_REG1)};
  apu_lclt par_b = {.raw = mmap_cpu_read(apu->mapper, PULSE1_REG4)};

  uint8_t volume = apu_clock_envelope(&par_a, &apu->pulse1_envelope);
}

void apu_pulse2_calculate(apu* apu) {
  apu_dve par_a = {.raw = mmap_cpu_read(apu->mapper, PULSE2_REG1)};
  apu_lclt par_b = {.raw = mmap_cpu_read(apu->mapper, PULSE2_REG4)};

  uint8_t volume = apu_clock_envelope(&par_a, &apu->pulse2_envelope);
}

void apu_triangle_calculate(apu* apu) {}

void apu_noise_calculate(apu* apu) {
  apu_dve par_a = {.raw = mmap_cpu_read(apu->mapper, NOISE_REG1)};
  apu_lclt par_b = {.raw = mmap_cpu_read(apu->mapper, NOISE_REG3)};

  uint8_t volume = apu_clock_envelope(&par_a, &apu->noise_envelope);
}

void apu_dmc_calculate(apu* apuc) {}

uint8_t apu_pulses_output(apu* apu) {
  // This formula is a linear approximation
  return 0.00752 * (apu->last_pulse1 + apu->last_pulse2);
}

uint8_t apu_tnd_output(apu* apu) {
  return 0.00851 * apu->last_triangle + 0.00494 * apu->last_noise +
         0.00335 * apu->last_dmc;
}

void apu_mem_write(apu* apu, uint16_t address, uint8_t val) {}

void apu_write_to_buffer(apu* apu, uint8_t value) {
  apu->buffer[apu->buffer_cursor] = value;
  apu->buffer_cursor++;
  apu->buffer_cursor = apu->buffer_cursor % AUDIO_BUFFER_SIZE;
}

void apu_sequencer_cycle(apu* apu) {
  // Get the current mode
  register_4017_frame_counter fc = {
      .raw = mmap_cpu_read(apu->mapper, FRAME_COUNTER)};
  register_4015_status status = {.raw = mmap_cpu_read(apu->mapper, STATUS)};
  if (fc.data.mode == 0) {
    // Counter mode 0
    if (apu->sequencer_elapsed_apu_cycles >= 14915) {
      apu_pulse1_calculate(apu);
      apu_pulse2_calculate(apu);
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
      apu_pulse1_calculate(apu);
      apu_pulse2_calculate(apu);
    } else if (apu->sequencer_elapsed_apu_cycles >= 7457) {
      apu_pulse1_calculate(apu);
      apu_pulse2_calculate(apu);
    } else if (apu->sequencer_elapsed_apu_cycles >= 3729) {
      apu_pulse1_calculate(apu);
      apu_pulse2_calculate(apu);
    }
  } else if (fc.data.mode == 1) {
    // Counter mode 1
    if (apu->sequencer_elapsed_apu_cycles >= 18641) {
      apu_pulse1_calculate(apu);
      apu_pulse2_calculate(apu);
      apu->sequencer_elapsed_apu_cycles = 0;
    } else if (apu->sequencer_elapsed_apu_cycles >= 14915) {
    } else if (apu->sequencer_elapsed_apu_cycles >= 11186) {
      apu_pulse1_calculate(apu);
      apu_pulse2_calculate(apu);
    } else if (apu->sequencer_elapsed_apu_cycles >= 7457) {
      apu_pulse1_calculate(apu);
      apu_pulse2_calculate(apu);
    } else if (apu->sequencer_elapsed_apu_cycles >= 3729) {
      apu_pulse1_calculate(apu);
      apu_pulse2_calculate(apu);
    }
  }

  apu->sequencer_elapsed_apu_cycles += 2;
}

void apu_cycle(apu* apu, void* context,
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

  // Create output
  uint8_t val = apu_pulses_output(apu) + apu_tnd_output(apu);

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
