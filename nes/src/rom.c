#include "rom.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"
#include "mappers.h"
#include "ppu.h"

#define HEADER_SIZE 16
#define TRAINER_SIZE 512
#define MAGIC_NO 0x1A53454E

/**
 * Mapping Constants
 */
#define MC_WORK_RAM_BASE 0x0
#define MC_WORK_RAM_OCCURRENCES 4
#define MC_WORK_RAM_UPPER \
  (MC_WORK_RAM_BASE + MC_WORK_RAM_OCCURRENCES * WORK_RAM_SIZE)

#define MC_PPU_CTRL_BASE 0x2000
#define MC_PPU_CTRL_SIZE 8
#define MC_PPU_CTRL_OCCURRENCES 1024
#define MC_PPU_CTRL_UPPER \
  (MC_PPU_CTRL_BASE + MC_PPU_CTRL_OCCURRENCES * MC_PPU_CTRL_SIZE)

#define MC_REGISTERS_BASE 0x4000
#define MC_REGISTERS_UPPER (MC_REGISTERS_BASE + REGISTERS_SIZE)

#define MC_CART_EXPANSION_ROM_BASE 0x4020
#define MC_CART_EXPANSION_ROM_UPPER \
  (MC_CART_EXPANSION_ROM_BASE + CART_EXPANSION_ROM_SIZE)

#define MC_SRAM_BASE 0x6000
#define MC_SRAM_UPPER (MC_SRAM_BASE + SRAM_SIZE)

#define MC_PRG_ROM_SIZE 0x4000
#define MC_PRG_ROM1_BASE 0x8000
#define MC_PRG_ROM1_UPPER (MC_PRG_ROM1_BASE + MC_PRG_ROM_SIZE)
#define MC_PRG_ROM2_BASE 0xC000
#define MC_PRG_ROM2_UPPER (MC_PRG_ROM2_BASE + MC_PRG_ROM_SIZE - 1)

#define MC_PATTABLE_SIZE 0x1000
#define MC_PATTABLE0_BASE 0x0
#define MC_PATTABLE0_UPPER (MC_PATTABLE0_BASE + MC_PATTABLE_SIZE)
#define MC_PATTABLE1_BASE 0x1000
#define MC_PATTABLE1_UPPER (MC_PATTABLE1_BASE + MC_PATTABLE_SIZE)
#define MC_NAMETABLE_SIZE 0x400
#define MC_NAMETABLE0_BASE 0x2000
#define MC_NAMETABLE0_UPPER (MC_NAMETABLE0_BASE + MC_NAMETABLE_SIZE)
#define MC_NAMETABLE1_BASE 0x2400
#define MC_NAMETABLE1_UPPER (MC_NAMETABLE1_BASE + MC_NAMETABLE_SIZE)
#define MC_NAMETABLE2_BASE 0x2800
#define MC_NAMETABLE2_UPPER (MC_NAMETABLE2_BASE + MC_NAMETABLE_SIZE)
#define MC_NAMETABLE3_BASE 0x2C00
#define MC_NAMETABLE3_UPPER (MC_NAMETABLE3_BASE + MC_NAMETABLE_SIZE)

int fread_all(FILE* fp, uint8_t* buf, size_t size) {
  uint32_t read_done = 0;
  uint32_t read_pending = size;
  while (!feof(fp) && read_pending > 0) {
    uint32_t read_iter = fread(buf + read_done, 1, read_pending, fp);
    read_done += read_iter;
    read_pending -= read_iter;
  }
  if (read_pending > 0) {
    return 1;
  }
  return 0;
}

rom_error rom_load(mapper** mapper_ptr, const char* path) {
  FILE* fp;
  if (!(fp = fopen(path, "r"))) {
    return RE_READ_ERROR;
  }

  fseek(fp, 0, SEEK_END);
  size_t rom_size = ftell(fp);
  rewind(fp);

  uint8_t header_data[HEADER_SIZE];
  if (fread_all(fp, header_data, HEADER_SIZE)) {
    // No complete header present
    fclose(fp);
    return RE_INVALID_FILE_FORMAT;
  }

  // Make sure we're dealing with archaic NES/iNES/NES2.0
  if (*((uint32_t*)header_data) != MAGIC_NO) {
    fclose(fp);
    return RE_INVALID_FILE_FORMAT;
  }

  // Allocate the mapper & assign it to the double pointer
  mapper* ret = *mapper_ptr = malloc(sizeof(mapper));

  // Allocate the header
  rom_header* header = malloc(sizeof(rom_header));
  // Populate the header
  memcpy(header, header_data + 4, HEADER_SIZE - 4);

  // Determine what type of NES format we're dealing with specifically
  // - Archaic doesn't use bytes 8 - 15
  // - iNES doesn't use bytes 10 - 15
  //   Although flags 10 is an unofficial extension
  rom_type type = ROMTYPE_ARCHAIC;
  size_t prg_rom_size =
      header->prg_rom | header->flags9.nes2.prg_rom_additional << 4;
  if (header->flags7.data.ines_version == 2 &&
      prg_rom_size <= rom_size / 0x4000) {
    type = ROMTYPE_NES2;
  } else if (header->flags7.data.ines_version == 0 &&
             *((uint32_t*)(header_data + HEADER_SIZE - 4)) == 0) {
    type = ROMTYPE_INES;
  }

  ret->header = header;
  ret->type = type;

  // Skip the trainer, if present
  if (header->flags6.data.has_trainer) {
    fseek(fp, TRAINER_SIZE, SEEK_CUR);
  }

  // Populate the memory struct
  memory* mem = calloc(sizeof(memory), 1);
  ret->memory = mem;
  prg_rom_size = rom_get_prg_rom_size(ret);

  mem->prg_rom = malloc(sizeof(uint8_t) * prg_rom_size);
  if (fread_all(fp, mem->prg_rom, prg_rom_size)) {
    fclose(fp);
    rom_destroy(ret);
    return RE_PRG_READ_ERROR;
  }

  size_t chr_rom_size = rom_get_chr_rom_size(ret);
  if (chr_rom_size != 0) {
    mem->chr_rom = malloc(sizeof(uint8_t) * chr_rom_size);
    if (fread_all(fp, mem->chr_rom, chr_rom_size)) {
      fclose(fp);
      rom_destroy(ret);
      return RE_CHR_READ_ERROR;
    }
  }

  fclose(fp);

  // Pre-initialise RAM
  for (uint16_t i = 0; i < 0x800; i++) {
    ret->memory->ram[i] = (i & 4) ? 0xFF : 0x00;
  }

  // Set up the mapped memory with some defaults
  ret->mapped.ram = ret->memory->ram;
  ret->mapped.registers = ret->memory->registers;
  ret->mapped.cart_expansion_rom = NULL;
  ret->mapped.sram = NULL;
  ret->mapped.prg_rom1 = ret->memory->prg_rom;
  ret->mapped.prg_rom2 = ret->memory->prg_rom + MC_PRG_ROM_SIZE;
  ret->mapped.ppu_pattable0 = ret->memory->chr_rom;
  ret->mapped.ppu_pattable1 = ret->mapped.ppu_pattable0 + MC_PATTABLE_SIZE;
  ret->mapped.ppu_nametable0 = ret->memory->vram;
  ret->mapped.ppu_nametable1 = ret->mapped.ppu_nametable0 + MC_NAMETABLE_SIZE;
  //ret->mapped.ppu_palettes = ret->mapped.ppu_nametable1 + MC_NAMETABLE_SIZE;

  if (rom_get_mapper_number(ret) != 0) {
    return RE_UNKNOWN_MAPPER;
  }
  MAPPERS[rom_get_mapper_number(ret)].mapper_init(ret);

  return RE_SUCCESS;
}

void rom_destroy(mapper* mapper) {
  free(mapper->header);
  free(mapper->memory->prg_rom);
  free(mapper->memory->prg_ram);
  free(mapper->memory->chr_rom);
  free(mapper->memory->chr_ram);
  free(mapper->memory);
  free(mapper);
}

// Utilities to query the header
size_t rom_get_prg_rom_size(mapper* mappr) {
  size_t size = mappr->header->prg_rom;
  if (mappr->type == ROMTYPE_NES2) {
    size |= ((size_t)mappr->header->flags9.nes2.prg_rom_additional) << 4;
  }

  return size * 0x4000;  // 16 KB units
}

size_t rom_get_chr_rom_size(mapper* mappr) {
  size_t size = mappr->header->chr_rom;
  if (mappr->type == ROMTYPE_NES2) {
    size |= ((size_t)mappr->header->flags9.nes2.chr_rom_additional) << 4;
  }

  return size * 0x2000;  // 8 KB units
}

mirror_type rom_get_mirror_type(mapper* mapper) {
  if (mapper->header->flags6.data.fs_vram) {
    return MIRRORTYPE_4SCREEN;
  }

  return mapper->header->flags6.data.mirroring ? MIRRORTYPE_VERTICAL
                                               : MIRRORTYPE_HORIZONTAL;
}

uint32_t rom_get_mapper_number(mapper* mapper) {
  // We don't currently support NES2.0 submappers

  uint32_t mapper_num = mapper->header->flags6.data.lower_mapper_num;
  mapper_num |= ((uint32_t)mapper->header->flags7.data.upper_mapper_num) << 4;

  if (mapper->type == ROMTYPE_NES2) {
    mapper_num |= ((uint32_t)mapper->header->flags8.nes2.extension_mapper_num)
                  << 8;
  }

  return mapper_num;
}

video_mode rom_get_video_mode(mapper* mapper) {
  if (mapper->type == ROMTYPE_NES2) {
    if (mapper->header->flags12.data.works_for_both) {
      return VMODE_UNIVERSAL;
    }

    if (mapper->header->flags12.data.mode == 1) {
      return VMODE_PAL;
    }

    // Else: Default (NTSC)
  }

  if (mapper->type == ROMTYPE_INES) {
    uint8_t tv_system = mapper->header->flags10.nes1.tv_system;
    if (tv_system == 2) {
      return VMODE_PAL;
    }

    if (tv_system != 0) {
      return VMODE_UNIVERSAL;
    }

    // Else: Default (NTSC)
  }

  return VMODE_NTSC;
}

bool rom_has_persistent_memory(mapper* mapper) {
  return mapper->header->flags6.data.has_persistent_memory;
}

bool rom_has_bus_conflicts(mapper* mapper) {
  if (mapper->type == ROMTYPE_INES) {
    return mapper->header->flags10.nes1.bus_conflicts;
  }

  // Probably no bus conflicts...
  // Or we determine this explicitly using the mapper number.
  return false;
}

// Memory access functions
void mmap_cpu_write(mapper* mapper, uint16_t address, uint8_t val) {
  if (address >= MC_WORK_RAM_BASE && address < MC_WORK_RAM_UPPER) {
    mapper->mapped.ram[(address - MC_WORK_RAM_BASE) % WORK_RAM_SIZE] = val;
    return;
  }if ((address >= MC_PPU_CTRL_BASE && address < MC_PPU_CTRL_UPPER) || address == 0x4014) {
    ppu_mem_write((ppu*)mapper->ppu, ((address - MC_PPU_CTRL_BASE) % MC_PPU_CTRL_SIZE) + MC_PPU_CTRL_BASE, val);
    return;
  }
  if (address >= MC_REGISTERS_BASE && address < MC_REGISTERS_UPPER) {
    // TODO: Call aurel's function to deal with register writes
    // mapped.registers[address - MC_REGISTERS_BASE] = val;
    return;
  }

  mapper->mapped.prg_rom1[address % 0x4000] = val;
  return;

  if (address >= MC_SRAM_BASE && address < MC_SRAM_UPPER) {
    mapper->mapped.sram[address - MC_SRAM_BASE] = val;
    return;
  }

  MAPPERS[rom_get_mapper_number(mapper)].cpu_write(mapper, address, val);
}

uint8_t mmap_cpu_read(mapper* mapper, uint16_t address, bool dummy) {
  if (address >= MC_WORK_RAM_BASE && address < MC_WORK_RAM_UPPER) {
    return mapper->mapped.ram[(address - MC_WORK_RAM_BASE) % WORK_RAM_SIZE];
  }
  if ((address >= MC_PPU_CTRL_BASE && address < MC_PPU_CTRL_UPPER) ||
      address == 0x4014) {
    return ppu_mem_read((ppu*)mapper->ppu, address, dummy);
  }
  if (address >= MC_REGISTERS_BASE && address < MC_REGISTERS_UPPER) {
    return 0; //mapper->mapped.registers[address - MC_REGISTERS_BASE];
  }
  return mapper->mapped.prg_rom1[address % 0x4000];

  if (address >= MC_CART_EXPANSION_ROM_BASE &&
      address < MC_CART_EXPANSION_ROM_UPPER) {
    return 0;
    return mapper->mapped
        .cart_expansion_rom[address - MC_CART_EXPANSION_ROM_BASE];
  }
  if (address >= MC_SRAM_BASE && address < MC_SRAM_UPPER) {
    return 0;
    return mapper->mapped.sram[address - MC_SRAM_BASE];
  }
  if (address >= MC_PRG_ROM1_BASE && address < MC_PRG_ROM1_UPPER) {
    return mapper->mapped.prg_rom1[address - MC_PRG_ROM1_BASE];
  }
  if (address >= MC_PRG_ROM2_BASE && address <= MC_PRG_ROM2_UPPER) {
    return 0;
    return mapper->mapped.prg_rom2[address - MC_PRG_ROM2_BASE];
  }

  return MAPPERS[rom_get_mapper_number(mapper)].cpu_read(mapper, address);
}

void mmap_cpu_dma(mapper* mapper, uint8_t address, uint8_t* buf) {
  uint16_t cur = address * 0x100;
  ((cpu*)mapper->cpu)->busy += 513 * 3;
  for (uint16_t i = 0; i < 256; i++) {
    buf[i] = mmap_cpu_read(mapper, cur, false);
    cur++;
  }
}

void mmap_ppu_write(mapper* mapper, uint16_t address, uint8_t val) {
  if (address >= 0x3000 && address < 0x3000 + 0xF00) {
    address -= 0x1000;
  }

  if (address >= MC_PATTABLE0_BASE && address <= MC_PATTABLE0_UPPER) {
    mapper->mapped.ppu_pattable0[address - MC_PATTABLE0_BASE] = val;
    return;
  }
  if (address >= MC_PATTABLE1_BASE && address < MC_PATTABLE1_UPPER) {
    mapper->mapped.ppu_pattable1[address - MC_PATTABLE1_BASE] = val;
    return;
  }
  if (address >= MC_NAMETABLE0_BASE && address < MC_NAMETABLE0_UPPER) {
    mapper->mapped.ppu_nametable0[address - MC_NAMETABLE0_BASE] = val;
    return;
  }

  // Nametable
  if (address >= MC_NAMETABLE0_BASE && address < MC_NAMETABLE3_UPPER) {
    mirror_type mt = rom_get_mirror_type(mapper);
    uint8_t* nametable = mapper->mapped.ppu_nametable0;
    if (address < MC_NAMETABLE1_UPPER && mt == MIRRORTYPE_VERTICAL) {
      nametable = mapper->mapped.ppu_nametable1;
    } else if (address >= MC_NAMETABLE2_BASE && address < MC_NAMETABLE2_UPPER &&
               mt == MIRRORTYPE_HORIZONTAL) {
      nametable = mapper->mapped.ppu_nametable1;
    } else if (address >= MC_NAMETABLE3_BASE && mt == MIRRORTYPE_VERTICAL) {
      nametable = mapper->mapped.ppu_nametable1;
    }
    nametable[address & 0x3FF] = val;
    return;
  }

  MAPPERS[rom_get_mapper_number(mapper)].ppu_write(mapper, address, val);
}

uint8_t mmap_ppu_read(mapper* mapper, uint16_t address) {
  if (address >= 0x3000 && address < 0x3000 + 0xF00) {
    address -= 0x1000;
  }

  if (address >= MC_PATTABLE0_BASE && address <= MC_PATTABLE0_UPPER) {
    return mapper->mapped.ppu_pattable0[address - MC_PATTABLE0_BASE];
  }
  if (address >= MC_PATTABLE1_BASE && address < MC_PATTABLE1_UPPER) {
    return mapper->mapped.ppu_pattable1[address - MC_PATTABLE1_BASE];
  }
  if (address >= MC_NAMETABLE0_BASE && address < MC_NAMETABLE0_UPPER) {
    return mapper->mapped.ppu_nametable0[address - MC_NAMETABLE0_BASE];
  }

  // Nametable
  if (address >= MC_NAMETABLE0_BASE && address < MC_NAMETABLE3_UPPER) {
    mirror_type mt = rom_get_mirror_type(mapper);
    uint8_t* nametable = mapper->mapped.ppu_nametable0;
    if (address < MC_NAMETABLE1_UPPER && mt == MIRRORTYPE_VERTICAL) {
      nametable = mapper->mapped.ppu_nametable1;
    } else if (address >= MC_NAMETABLE2_BASE && address < MC_NAMETABLE2_UPPER &&
               mt == MIRRORTYPE_HORIZONTAL) {
      nametable = mapper->mapped.ppu_nametable1;
    } else if (address >= MC_NAMETABLE3_BASE && mt == MIRRORTYPE_VERTICAL) {
      nametable = mapper->mapped.ppu_nametable1;
    }
    return nametable[address & 0x3FF];
  }

  return MAPPERS[rom_get_mapper_number(mapper)].ppu_read(mapper, address);
}
