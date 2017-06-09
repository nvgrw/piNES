#include "apu.h"
#include <math.h>
#include <stdio.h>

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

uint8_t apu_pulse0_output(apu* apu) { return -1; }

uint8_t apu_pulse1_output(apu* apu) { return -1; }

// These formulae are linear approximations
uint8_t apu_pulses_output(apu* apu) {
  uint8_t pulse0 = apu_pulse0_output(apu);
  uint8_t pulse1 = apu_pulse1_output(apu);
  return 0.00752 * (pulse0 + pulse1);
}

uint8_t apu_triangle_output(apu* apu) { return -1; }
uint8_t apu_noise_output(apu* apu) { return -1; }
uint8_t apu_dmc_output(apu* apuc) { return -1; }

uint8_t apu_tnd_output(apu* apu) {
  uint8_t triangle = apu_triangle_output(apu);
  uint8_t noise = apu_noise_output(apu);
  uint8_t dmc = apu_dmc_output(apu);
  return 0.00851 * triangle + 0.00494 * noise + 0.00335 * dmc;
}

void apu_mem_write(apu* apu, uint16_t address, uint8_t val) {}

void apu_write_to_buffer(apu* apu, uint8_t value) {
  apu->buffer[apu->buffer_cursor] = value;
  apu->buffer_cursor = (apu->buffer_cursor + 1) % AUDIO_BUFFER_SIZE;
}

void apu_cycle(apu* apu) {
  // uint8_t pulse = apu_pulses_output(apu);
  // uint8_t tnd = apu_tnd_output(apu);

  uint8_t val = 128 + (uint8_t)(127 * sin(apu->cntr));
  apu_write_to_buffer(apu, val);
  apu->cntr += 0.01f;
  if (apu->cntr >= 1.0) apu->cntr = -1.0;
}
