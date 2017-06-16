#pragma once

#include <stdint.h>

#include "rom.h"

#define NUM_MAPPERS 5

typedef struct mapper_special {
  void (*mapper_init)(struct mapper_special* self, mapper* mapper);
  void (*mapper_deinit)(struct mapper_special* self, mapper* mapper);
  void (*cpu_write)(struct mapper_special* self, mapper* mapper,
                    uint16_t address, uint8_t val);
  uint8_t (*cpu_read)(struct mapper_special* self, mapper* mapper,
                      uint16_t address);
  void (*ppu_write)(struct mapper_special* self, mapper* mapper,
                    uint16_t address, uint8_t val);
  uint8_t (*ppu_read)(struct mapper_special* self, mapper* mapper,
                      uint16_t address);
  bool present;
  void* data;
} mapper_special;

extern mapper_special MAPPERS[NUM_MAPPERS];
