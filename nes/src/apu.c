#include "apu.h"
#include <stdio.h>

#define START_CHANNELS 0x4000
#define END_CHANNELS 0x4013
#define STATUS 0x4015
#define FRAME_COUNTER 0x4017

const uint8_t APU_LENGTH_TABLE[] = {
    0x0A, 0xFE, 0x14, 0x02, 0x28, 0x04, 0x50, 0x06, 0xA0, 0x08, 0x3C,
    0x0A, 0x0E, 0x0C, 0x1A, 0x0E, 0x0C, 0x10, 0x18, 0x12, 0x30, 0x14,
    0x60, 0x16, 0xC0, 0x18, 0x48, 0x1A, 0x10, 0x1C, 0x20, 0x1E};

const uint8_t APU_DUTY_TABLE[] = {0x40, 0x60, 0x78, 0x9f};

const uint16_t APU_NOISE_TABLE[] = {4,   8,   16,  32,  64,  96,   128,  160,
                                    202, 254, 380, 508, 762, 1016, 2034, 4068};

bool apu_mem_is_valid(uint16_t address) {
  uint16_t translated_address = apu_mem_translate(address);
  return (START_CHANNELS <= translated_address &&
          translated_address <= END_CHANNELS) ||
         translated_address == STATUS || translated_address == FRAME_COUNTER;
}

uint16_t apu_mem_translate(uint16_t address) { return address; }

uint8_t apu_pulse0_output(apu* apu) { return -1; }

uint8_t apu_pulse1_output(apu* apu) { return -1; }

/* this formulae are linear approximations */

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

uint8_t apu_output(apu* apu) {
  uint8_t pulse = apu_pulses_output(apu);
  uint8_t tnd = apu_tnd_output(apu);
  return pulse + tnd;
}

apu* apu_init(void) { return (apu*)calloc(1, sizeof(apu)); }
