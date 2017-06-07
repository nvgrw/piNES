#include "mappers.h"

void mapper000_init(mapper* mapper) {}

void mapper000_cpu_write(mapper* mapper, uint16_t adress, uint8_t val) {}

uint8_t mapper000_cpu_read(mapper* mapper, uint16_t address) { return 0; }

void mapper000_ppu_write(mapper* mapper, uint16_t adress, uint8_t val) {}

uint8_t mapper000_ppu_read(mapper* mapper, uint16_t address) { return 0; }

const mapper_special MAPPERS[NUM_MAPPERS] = {{.mapper_init = &mapper000_init,
                                              .cpu_write = &mapper000_cpu_write,
                                              .cpu_read = &mapper000_cpu_read,
                                              .ppu_write = &mapper000_ppu_write,
                                              .ppu_read = &mapper000_ppu_read}};
