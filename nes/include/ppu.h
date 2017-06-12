#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "rom.h"

/**
 * ppu.h
 * 
 * Functions for the PPU (Picture Processing Unit).
 */

// Internal VRAM (video ram) of the PPU
#define PPU_MEMORY 16384

// Dimensions of the PPU picture (internal)
#define PPU_SCANLINES 262
#define PPU_CYCLES 341
#define PPU_SCREEN_SIZE 61440
#define PPU_SCREEN_SIZE_BYTES (61440 << 2)

// Important scanlines
#define PPU_SL_VISIBLE 0
#define PPU_SL_POSTRENDER 240
#define PPU_SL_PRERENDER 261

// CPU mapped addresses
#define PPU_ADDR_CTRL 0x0
#define PPU_ADDR_MASK 0x1
#define PPU_ADDR_STATUS 0x2
#define PPU_ADDR_OAMADDR 0x3
#define PPU_ADDR_OAMDATA 0x4
#define PPU_ADDR_PPUSCROLL 0x5
#define PPU_ADDR_PPUADDR 0x6
#define PPU_ADDR_PPUDATA 0x7
#define PPU_ADDR_OAMDMA 0x4014

/**
 * Scroll register struct used for the internal v and t registers. These are
 * actually accessed via the CPU mapped addresses 0x2005 and 0x2006.
 */
typedef union {
  struct __attribute__((packed)) {
    uint8_t x_coarse : 5;
    uint8_t y_coarse : 5;
    uint8_t nx : 1;
    uint8_t ny : 1;
    uint8_t y_fine : 3;
    uint8_t : 1;
  } scroll;
  struct __attribute__((packed)) {
    uint16_t : 10;
    uint16_t nt : 2;
    uint16_t : 4;
  } nt_select;
  struct __attribute__((packed)) {
    uint16_t addr : 12;
    uint16_t : 4;
  } nametable;
  uint16_t raw;
} scroll_reg;

/**
 * Methods to render the signal coming from the PPU.
 */
typedef enum {
  PPUD_DIRECT,
  PPUD_SIGNAL
} ppu_driver;

/**
 * Structs for the OAM.
 */
typedef union {
  struct __attribute__((packed)) {
    uint8_t bank : 1;
    uint8_t tile : 7;
  } index;
  uint8_t raw;
} oam_index;

typedef union {
  struct __attribute__((packed)) {
    uint8_t palette : 2;
    uint8_t : 3;
    uint8_t priority : 1;
    uint8_t flip_h : 1;
    uint8_t flip_v : 1;
  } attr;
  uint8_t raw;
} oam_attr;

typedef struct __attribute__((packed)) {
  uint8_t y : 8;
  uint8_t index : 8;
  uint8_t attr : 8;
  uint8_t x : 8;
} oam_sprite;

typedef struct __attribute__((packed)) {
  uint8_t oam_index;
  uint16_t pattern;
} oam_state;

/**
 * The main PPU struct. Holds internal state, memory, and registers.
 */
typedef struct {
  union {
    struct __attribute__((packed)) {
      uint8_t nametable : 2;
      uint8_t increment : 1;
      uint8_t sprite_table : 1;
      uint8_t bg_table : 1;
      uint8_t sprite_size : 1;
      uint8_t ppu_master : 1;
      uint8_t nmi : 1;
    } flags;
    uint8_t raw;
  } ctrl;
  union {
    struct __attribute__((packed)) {
      uint8_t gray : 1;
      uint8_t show_left_bg : 1;
      uint8_t show_left_sprites : 1;
      uint8_t show_bg : 1;
      uint8_t show_sprites : 1;
      uint8_t : 3;
    } flags;
    struct __attribute__((packed)) {
      uint8_t : 5;
      uint8_t emph_r : 1;
      uint8_t emph_g : 1;
      uint8_t emph_b : 1;
    } emph_ntsc;
    struct __attribute__((packed)) {
      uint8_t : 5;
      uint8_t emph_g : 1;
      uint8_t emph_r : 1;
      uint8_t emph_b : 1;
    } emph_pal;
    uint8_t raw;
  } mask;
  union {
    struct __attribute__((packed)) {
      uint8_t last_write : 5;
      uint8_t overflow : 1;
      uint8_t sprite0_hit : 1;
      uint8_t vblank : 1;
    } flags;
    uint8_t raw;
  } status;

  // Memory
  uint8_t memory[PPU_MEMORY];
  mapper* mapper;

  // Status
  uint16_t cycle;
  uint16_t scanline;
  bool frame_odd;

  // Special R/W conditions
  bool oam_data_ff;

  // Internal registers
  scroll_reg v; // Current VRAM address
  scroll_reg t; // Temporary VRAM address
  uint8_t x;
  bool w;
  bool nmi_occurred;
  bool nmi_output;
  bool nmi;

  // Palette
  uint8_t palette[32];

  // Rendering
  uint16_t io_addr;

  // Background
  uint8_t ren_nt;
  uint8_t ren_at;
  uint8_t ren_bg_low;
  uint8_t ren_bg_high;
  uint64_t tile_data;

  // Sprite rendering
  union {
    oam_sprite sprites[64];
    uint32_t raw32[64];
    uint8_t raw[256];
  } oam;
  uint8_t oam_address;

  uint16_t spr_count;
  uint32_t spr_pat[8];
  uint8_t spr_pos[8];
  uint8_t spr_priority[8];
  uint8_t spr_index[8];

  uint16_t spr_count_max;
  /*
  union {
    oam_sprite sprites[8];
    uint32_t raw32[8];
    uint8_t raw[32];
  } oam_secondary, oam_tertiary;
  oam_state oam_secondary_state[8];
  oam_state oam_tertiary_state[8];
  uint8_t oam_spr_n;
  uint8_t oam_spr_count;
  uint8_t spr_ren_pos;
  uint8_t spr_ren_count;
  */

  // Visual output
  ppu_driver driver;
  bool flip;
  uint8_t screen[PPU_SCREEN_SIZE];
} ppu;

/**
 * Memory access from the mapper to the PPU.
 */
void ppu_mem_write(ppu* ppu, uint16_t address, uint8_t value);
uint8_t ppu_mem_read(ppu* ppu, uint16_t address, bool dummy);

/**
 * Initialises and returns a PPU. This implicitly "powers on" the PPU.
 */
ppu* ppu_init(void);

/**
 * Resets the PPU, equivalent to pressing the reset button on a NES.
 */
void ppu_reset(ppu* ppu);

/**
 * Powers on the PPU, equivalent to powering on the NES.
 */
void ppu_power(ppu* ppu);

/**
 * Executes a single PPU cycle.
 */
void ppu_cycle(ppu* ppu);

/**
 * Frees any dynamic memory allocated for the PPU.
 */
void ppu_deinit(ppu* ppu);
