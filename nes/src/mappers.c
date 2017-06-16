#include "mappers.h"
#include <stdio.h>

#define NROM256_SIZE 0x8000
#define NROM128_SIZE 0x4000

// Mapper 0
static void mapper000_init(mapper_special* self, mapper* mapper) {
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

static void mapper000_deinit(mapper_special* self, mapper* mapper) {}

static void mapper000_cpu_write(mapper_special* self, mapper* mapper,
                                uint16_t address, uint8_t val) {}

static uint8_t mapper000_cpu_read(mapper_special* self, mapper* mapper,
                                  uint16_t address) {
  return 0;
}

static void mapper000_ppu_write(mapper_special* self, mapper* mapper,
                                uint16_t address, uint8_t val) {}

static uint8_t mapper000_ppu_read(mapper_special* self, mapper* mapper,
                                  uint16_t address) {
  return 0;
}

// MAPPER 4
#define BANK_SIZE 0x2000

typedef union {
  struct {
    uint8_t sel : 3;
    uint8_t : 3;
    uint8_t prg_bank_mode : 1;
    uint8_t chr_a12_inversion : 1;
  } data;
  uint8_t raw;
} mapper004_bank_select_t;

typedef union {
  struct {
    uint8_t half : 1;
    uint8_t bank : 5;
    uint8_t : 2;
  } data;
  uint8_t raw;
} mapper004_bank_data_t;

typedef union {
  struct {
    uint8_t is_horizontal : 1;
    uint8_t : 7;
  } data;
  uint8_t raw;
} mapper004_mirroring_t;

typedef union {
  struct {
    uint8_t : 6;
    uint8_t deny_writes : 1;
    uint8_t enable_prg_ram : 1;
  } data;
  uint8_t raw;
} mapper004_prg_ram_protect_t;

typedef struct {
  mapper004_bank_select_t bank_select;
  mapper004_bank_data_t bank_data;
  mapper004_mirroring_t mirroring;
  mapper004_prg_ram_protect_t ram_protect;
  uint8_t irq_latch;
  bool irq_reload;
  bool irq_enable;
} mapper004_t;

static void mapper004_init(mapper_special* self, mapper* mapper) {
  /**
   * PRG & CHR ROM and RAM are freed for us.
   * CHR ROM: 8K + 8K + 16K fixed.
   */
  mapper->memory->prg_ram = calloc(1, sizeof(uint8_t) * 0x2000);
  mapper->mapped.prg_rom1 = NULL;  // TODO: To handle 8K + 8K --> IF
                                   // consecutive, may be able to cheat here.
  // Maps to second to last and last bank (fixed)
  size_t prg_rom_size = rom_get_prg_rom_size(mapper);
  mapper->mapped.prg_rom2 =
      mapper->memory->prg_rom + prg_rom_size - 2 * BANK_SIZE;

  mapper004_t* data = calloc(1, sizeof(mapper004_t));
  data->ram_protect.data.enable_prg_ram = 1;
  data->mirroring.data.is_horizontal = !mapper->header->flags6.data.mirroring;
  self->data = data;
}

static void mapper004_deinit(mapper_special* self, mapper* mapper) {
  free(self->data);
  self->data = NULL;
}

static void mapper004_cpu_write(mapper_special* self, mapper* mapper,
                                uint16_t address, uint8_t val) {
  if (address >= 0x8000 && address <= 0x9FFF) {
    // Bank Select, $8000 - $9FFE (even)
    if (address % 2 == 0) {
      ((mapper004_t*)self->data)->bank_select.raw = val;
    } else {
      // Bank Data, $8001-$9FFF (odd)
      ((mapper004_t*)self->data)->bank_data.raw = val;
    }
  }

  if (address >= 0xA000 && address <= 0xBFFF) {
    if (address % 2 == 0) {
      // Mirroring, $A000-$BFFE (even)
      // Unused (we use the iNES/NES2.0 values)
      ((mapper004_t*)self->data)->mirroring.raw = val;
    } else {
      // PRG RAM protect, $A001-BFFF (odd)
      ((mapper004_t*)self->data)->ram_protect.raw = val;
    }
  }

  if (address >= 0xC000 && address <= 0xDFFF) {
    if (address % 2 == 0) {
      // IRQ latch, $C000-$DFFE (even)
      ((mapper004_t*)self->data)->irq_latch = val;
    } else {
      // IRQ reload, $C001-$DFFF (odd)
      ((mapper004_t*)self->data)->irq_reload = true;
    }
  }

  if (address >= 0xE000 && address <= 0xFFFF) {
    // IRQ disable, $E000-$FFFE (even)
    // IRQ enable, $E001-$FFF (odd)
    ((mapper004_t*)self->data)->irq_enable = address % 2 != 0;
  }
}

static uint8_t mapper004_cpu_read(mapper_special* self, mapper* mapper,
                                  uint16_t address) {
  return 0;
}

static void mapper004_ppu_write(mapper_special* self, mapper* mapper,
                                uint16_t address, uint8_t val) {}

static uint8_t mapper004_ppu_read(mapper_special* self, mapper* mapper,
                                  uint16_t address) {
  return 0;
}

#define MAPPER(NUMBER)                                                    \
  {                                                                       \
    .mapper_init = &mapper##NUMBER##_init,                                \
    .mapper_deinit = &mapper##NUMBER##_deinit,                            \
    .cpu_write = &mapper##NUMBER##_cpu_write,                             \
    .cpu_read = &mapper##NUMBER##_cpu_read,                               \
    .ppu_write = &mapper##NUMBER##_ppu_write,                             \
    .ppu_read = &mapper##NUMBER##_ppu_read, .present = true, .data = NULL \
  }
#define NOMAPPER                                                        \
  {                                                                     \
    .mapper_init = NULL, .cpu_write = NULL, .cpu_read = NULL,           \
    .ppu_write = NULL, .ppu_read = NULL, .present = false, .data = NULL \
  }

mapper_special MAPPERS[NUM_MAPPERS] = {MAPPER(000), NOMAPPER, NOMAPPER,
                                       NOMAPPER, MAPPER(004)};
