#pragma once

#include <stdint.h>

#include "rom.h"

#define NUM_MAPPERS 1

typedef struct {
  void (*mapper_init)(mapper* mapper);
  void (*cpu_write)(mapper* mapper, uint16_t address, uint8_t val);
  uint8_t (*cpu_read)(mapper* mapper, uint16_t address);
  void (*ppu_write)(mapper* mapper, uint16_t address, uint8_t val);
  uint8_t (*ppu_read)(mapper* mapper, uint16_t address);
} mapper_special;

extern const mapper_special MAPPERS[NUM_MAPPERS];
