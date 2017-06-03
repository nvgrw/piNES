#include "apu.h"
#include <stdio.h>

bool apu_mem_is_valid(uint16_t address) {
  uint16_t translated_address = apu_mem_translate(address);
  return (4000 <= translated_address && translated_address <= 4013) ||
         translated_address == 4015 || translated_address == 4017;
}

uint16_t apu_mem_translate(uint16_t address) { return -1; }
