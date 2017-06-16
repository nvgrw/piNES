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
#define PPU_ADDR_PPUCTRL 0x0
#define PPU_ADDR_PPUMASK 0x1
#define PPU_ADDR_PPUSTATUS 0x2
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
} scroll_reg_t;

/**
 * Methods to render the signal coming from the PPU.
 */
typedef enum {
  PPUD_DIRECT,
  PPUD_SIGNAL
} ppu_driver_t;

/**
 * Structs for the OAM.
 */
typedef union {
  struct __attribute__((packed)) {
    uint8_t bank : 1;
    uint8_t tile : 7;
  } index;
  uint8_t raw;
} oam_index_t;

typedef union {
  struct __attribute__((packed)) {
    uint8_t palette : 2;
    uint8_t : 3;
    uint8_t priority : 1;
    uint8_t flip_h : 1;
    uint8_t flip_v : 1;
  } attr;
  uint8_t raw;
} oam_attr_t;

typedef struct __attribute__((packed)) {
  uint8_t y : 8;
  uint8_t index : 8;
  uint8_t attr : 8;
  uint8_t x : 8;
} oam_sprite_t;

typedef struct __attribute__((packed)) {
  uint8_t oam_index;
  uint16_t pattern;
} oam_state_t;

/**
 * The main PPU struct. Holds internal state, memory, and registers.
 */
typedef struct {
  // Register PPUCTRL
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
  } reg_ctrl;
  uint8_t ctrl_nametable;
  uint8_t ctrl_increment;
  uint8_t ctrl_sprite_table;
  uint8_t ctrl_bg_table;
  uint8_t ctrl_sprite_size;
  uint8_t ctrl_ppu_master;
  uint8_t ctrl_nmi;

  uint8_t sprite_size;
  uint8_t increment;

  // Register PPUMASK
  union {
    struct __attribute__((packed)) {
      uint8_t gray : 1;
      uint8_t show_left_bg : 1;
      uint8_t show_left_sprites : 1;
      uint8_t show_bg : 1;
      uint8_t show_sprites : 1;
      uint8_t : 3;
    } flags;
    /*struct __attribute__((packed)) {
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
    } emph_pal;*/
    uint8_t raw;
  } reg_mask;
  uint8_t mask_gray;
  uint8_t mask_show_left_bg;
  uint8_t mask_show_left_sprites;
  uint8_t mask_show_bg;
  uint8_t mask_show_sprites;

  // Register PPUSTATUS
  union {
    struct __attribute__((packed)) {
      uint8_t last_write : 5;
      uint8_t overflow : 1;
      uint8_t sprite0_hit : 1;
      uint8_t vblank : 1;
    } flags;
    uint8_t raw;
  } reg_status;
  uint8_t status_last_write;
  uint8_t status_overflow;
  uint8_t status_sprite0_hit;

  // Memory
  mapper_t* mapper;

  // Status
  uint16_t cycle;
  uint16_t scanline;
  bool frame_odd;

  // Special R/W conditions
  uint8_t data_buf;
  uint8_t last_reg_write;
  bool oam_data_ff;

  // Internal registers
  scroll_reg_t v; // Current VRAM address
  scroll_reg_t t; // Temporary VRAM address
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
    oam_sprite_t sprites[64];
    uint8_t raw[256];
  } oam;
  uint8_t oam_address;
  uint16_t spr_count;
  uint32_t spr_pat[8];
  uint8_t spr_pos[8];
  uint8_t spr_priority[8];
  uint8_t spr_index[8];

  // Sprite debugging
  uint16_t spr_count_max;

  // Visual output
  ppu_driver_t driver;
  bool flip;
  uint8_t screen[PPU_SCREEN_SIZE];
} ppu_t;

/**
 * Memory access from the mapper to the PPU.
 */
void ppu_mem_write(ppu_t* ppu, uint16_t address, uint8_t value);
uint8_t ppu_mem_read(ppu_t* ppu, uint16_t address, bool dummy);

/**
 * Initialises and returns a PPU. This implicitly "powers on" the PPU.
 */
ppu_t* ppu_init(void);

/**
 * Resets the PPU, equivalent to pressing the reset button on a NES.
 */
void ppu_reset(ppu_t* ppu);

/**
 * Powers on the PPU, equivalent to powering on the NES.
 */
void ppu_power(ppu_t* ppu);

/**
 * Executes a single PPU cycle.
 */
void ppu_cycle(ppu_t* ppu);

/**
 * Frees any dynamic memory allocated for the PPU.
 */
void ppu_deinit(ppu_t* ppu);
