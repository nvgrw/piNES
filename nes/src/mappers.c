#include "mappers.h"
#include <stdio.h>

#define NROM256_SIZE 0x8000
#define NROM128_SIZE 0x4000

void mapper000_init(mapper* mapper) {
  size_t size = rom_get_prg_rom_size(mapper);
  if (size == NROM256_SIZE) {
    // NROM256 does not mirror (default).
    mapper->mapped.prg_rom1 = mapper->memory->prg_rom;
    mapper->mapped.prg_rom2 = mapper->memory->prg_rom + NROM128_SIZE;
  } else if (size == NROM128_SIZE) {
    // ROM 2 mirrors ROM 1
    mapper->mapped.prg_rom2 = mapper->mapped.prg_rom1;
  }
}

void mapper000_cpu_write(mapper* mapper, uint16_t adress, uint8_t val) {}

uint8_t mapper000_cpu_read(mapper* mapper, uint16_t address) { return 0; }

void mapper000_ppu_write(mapper* mapper, uint16_t adress, uint8_t val) {}

uint8_t mapper000_ppu_read(mapper* mapper, uint16_t address) { return 0; }

const mapper_special MAPPERS[NUM_MAPPERS] = {{.mapper_init = &mapper000_init,
                                              .cpu_write = &mapper000_cpu_write,
                                              .cpu_read = &mapper000_cpu_read,
                                              .ppu_write = &mapper000_ppu_write,
                                              .ppu_read = &mapper000_ppu_read}};
