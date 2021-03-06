/**
 * MIT License
 *
 * Copyright (c) 2017
 * Aurel Bily, Alexis I. Marinoiu, Andrei V. Serbanescu, Niklas Vangerow
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define WORK_RAM_SIZE 0x800
#define VIDEO_RAM_SIZE 0x800
#define REGISTERS_SIZE 0x20
#define CART_EXPANSION_ROM_SIZE 0x1FDF
#define SRAM_SIZE 0x2000
#define PPU_TABLES_SIZE 0x3000
#define PPU_PALETTES_SIZE 0x20

typedef enum {
  RE_SUCCESS,
  RE_READ_ERROR,
  RE_INVALID_FILE_FORMAT,
  RE_PRG_READ_ERROR,
  RE_CHR_READ_ERROR,
  RE_UNKNOWN_MAPPER
} rom_error_t;

typedef enum { ROMTYPE_ARCHAIC, ROMTYPE_INES, ROMTYPE_NES2 } rom_type_t;

typedef enum {
  MIRRORTYPE_HORIZONTAL,
  MIRRORTYPE_VERTICAL,
  MIRRORTYPE_4SCREEN
} mirror_type_t;

typedef enum { VMODE_NTSC, VMODE_PAL, VMODE_UNIVERSAL } video_mode_t;

// Referenced from here:
// https://wiki.nesdev.com/w/index.php/INES#iNES_file_format
typedef struct __attribute__((packed)) {
  uint8_t prg_rom;  // Size of PRG ROM in 16 KB units
  uint8_t chr_rom;  // Size of CHR ROM in 8 KB units (0 = CHR RAM)
  union {
    struct {
      uint8_t mirroring : 1;              // 0 = horizontal, 1 = vertical
      uint8_t has_persistent_memory : 1;  // Cart may contain battery-backed PRG
                                          // RAM or other persistent memory.
      uint8_t has_trainer : 1;  // Is trainer present? Trainer emulates mapper,
                                // not commonly used though.
      uint8_t fs_vram : 1;      // Ignore mirror bit, provide 4-screen VRAM.
      uint8_t lower_mapper_num : 4;  // Lower nibble of mapper number
    } data;
    uint8_t raw;
  } flags6;
  union {
    struct {
      uint8_t is_vs_unisystem : 1;  // Is VS Unisystem
      uint8_t is_playchoice : 1;    // Is PlayChoice-10
      uint8_t ines_version : 2;  // NES 1.0 or NES 2.0? Affects flags 8 to 15.
      uint8_t upper_mapper_num : 4;  // Upper nibble of mapper number
    } data;
    uint8_t raw;
  } flags7;
  union {
    struct {
      uint8_t prg_ram_size;  // Specified in 8 KB units, 0 means 8 KB used.
    } nes1;
    struct {
      uint8_t
          extension_mapper_num : 4;  // Additional 8-11 bits of mapper number
      uint8_t submapper_num : 4;     // Submapper number. Not a lot of ROMs make
                                     // use of this.
    } nes2;
    uint8_t raw;
  } flags8;
  union {
    struct {
      uint8_t tv_system : 1;  // TV System (0 = NTSC, 1 = PAL)
      uint8_t : 7;
    } nes1;  // Flags9 is typically ignored
    struct {
      uint8_t prg_rom_additional : 4;  // Additional bits for PRG ROM size
      uint8_t chr_rom_additional : 4;  // Additional bits for CHR ROM size
    } nes2;
    uint8_t raw;
  } flags9;
  union {
    struct {
      uint8_t tv_system : 2;  // TV System (0 = NTSC, 2 = PAL, other = both)
      uint8_t : 2;
      uint8_t no_prg_ram : 1;     // PRG RAM (0 = present, 1 = not present)
      uint8_t bus_conflicts : 1;  // Has bus conflicts?
      uint8_t : 2;
    } nes1;
    struct {
      uint8_t prg_not_battery_backed : 4;  // !BB PRG RAM
      uint8_t prg_battery_backed : 4;      // BB PRG RAM
      // Above sizes are 2^(n + 6), n is value, with exception for n = 0.
    } nes2;
    uint8_t raw;
  } flags10;  // Flags10 is typically ignored
  // Flags below are new in NES 2.0
  union {
    struct {
      uint8_t chr_not_battery_backed : 4;  // !BB CHR RAM
      uint8_t chr_battery_backed : 4;      // BB CHR RAM
      // Above sizes are 2^(n + 6), n is value, with exception for n = 0
    } data;
    uint8_t raw;
  } flags11;
  union {
    struct {
      uint8_t mode : 1;            // 0 = NTSC, 1 = PAL
      uint8_t works_for_both : 1;  // Whether the ROM is mode-agnostic
      uint8_t : 6;
    } data;
    uint8_t raw;
  } flags12;
  // Unimplemented, Vs. System only.
  union {
    uint8_t raw;
  } flags13;
  uint8_t : 8;
  uint8_t : 8;
} rom_header_t;

// https://en.wikibooks.org/wiki/NES_Programming/Memory_Map
typedef struct {
  // CPU
  uint8_t ram[WORK_RAM_SIZE];
  uint8_t registers[REGISTERS_SIZE];
  uint8_t* prg_rom;
  uint8_t* prg_ram;  // NULL if not present

  // PPU
  uint8_t vram[VIDEO_RAM_SIZE];
  uint8_t* chr_rom;  // NULL if not present
  uint8_t* chr_ram;  // NULL if not present
} memory_t;

struct controller;
struct cpu;
struct ppu;
struct apu;

typedef struct {
  rom_header_t* header;
  rom_type_t type;
  // Actual struct storing the data
  memory_t* memory;
  // An indirection struct which the mapper manipulates e.g. for bank switching
  struct {
    uint8_t* ram;
    uint8_t* ppu_ctrl_regs;
    uint8_t* registers;
    uint8_t* cart_expansion_rom;
    uint8_t* sram;
    uint8_t* prg_rom1;
    uint8_t* prg_rom2;

    uint8_t* ppu_pattable0;
    uint8_t* ppu_pattable1;
    uint8_t* ppu_nametable0;
    uint8_t* ppu_nametable1;
    // uint8_t* ppu_palettes;
  } mapped;

  struct controller* controller;
  struct cpu* cpu;
  struct ppu* ppu;
  struct apu* apu;
} mapper_t;

/*
 * Load a rom and return a pointer to its mapper.
 *
 * The pointer is allocated by the method.
 * Please call rom_destroy, passing the mapper pointer, once you are done.
 */
rom_error_t rom_load(mapper_t** mapper, const char* path);

void rom_destroy(mapper_t* mapper);

// Utilities to query the header
/**
 * Get the PRG ROM size in bytes
 */
size_t rom_get_prg_rom_size(mapper_t* mapper);

/**
 * Get the CHR ROM size in bytes
 */
size_t rom_get_chr_rom_size(mapper_t* mapper);

/**
 * Get ROM mirroring type
 */
mirror_type_t rom_get_mirror_type(mapper_t* mapper);

/**
 * Get ROM mapper number
 */
uint32_t rom_get_mapper_number(mapper_t* mapper);

/**
 * Get video mode
 * This was only added reliably to the NES2.0 standard, so if a video mode is
 * not determinable then we default to NTSC.
 */
video_mode_t rom_get_video_mode(mapper_t* mapper);

bool rom_has_persistent_memory(mapper_t* mapper);

bool rom_has_bus_conflicts(mapper_t* mapper);

/**
 * Read / write from within the CPU
 */
void mmap_cpu_write(mapper_t* mapper, uint16_t address, uint8_t val);
uint8_t mmap_cpu_read(mapper_t* mapper, uint16_t address, bool dummy);
void mmap_cpu_dma(mapper_t* mapper, uint8_t address, uint8_t* buf);

/**
 * Read / write from within the PPU
 */
void mmap_ppu_write(mapper_t* mapper, uint16_t address, uint8_t val);
uint8_t mmap_ppu_read(mapper_t* mapper, uint16_t address);
