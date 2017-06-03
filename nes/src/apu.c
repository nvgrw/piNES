#include "apu.h"
#include <stdio.h>

#define START_CHANNELS 0x4000;
#define END_CHANNELS 0x4013;
#define STATUS 0x4015;
#define FRAME_COUNTER 0x4017;

bool apu_mem_is_valid(uint16_t address) {
  uint16_t translated_address = apu_mem_translate(address);
  return (START_CHANNELS <= translated_address &&
          translated_address <= END_CHANNELS) ||
         translated_address == STATUS || translated_address == FRAME_COUNTER;
}

uint16_t apu_mem_translate(uint16_t address) { return -1; }
