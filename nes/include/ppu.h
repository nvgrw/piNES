#pragma once

#include <stdbool.h>
#include <stdint.h>

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
#define PPU_ADDR_CTRL 0x2000
#define PPU_ADDR_MASK 0x2001
#define PPU_ADDR_STATUS 0x2002
#define PPU_ADDR_OAMADDR 0x2003
#define PPU_ADDR_OAMDATA 0x2004
#define PPU_ADDR_PPUSCROLL 0x2005
#define PPU_ADDR_PPUADDR 0x2006
#define PPU_ADDR_PPUDATA 0x2007
#define PPU_ADDR_OAMDMA 0x4014

/**
 * Scroll register struct used for the internal v and t registers. These are
 * actually accessed via the CPU mapped addresses 0x2005 and 0x2006.
 */
typedef union {
  struct {
    uint8_t x_coarse : 5;
    uint8_t y_coarse : 5;
    uint8_t nx : 1;
    uint8_t ny : 1;
    uint8_t y_fine : 3;
  } scroll;
  struct {
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
  struct {
    uint8_t bank : 1;
    uint8_t tile : 7;
  } index;
  uint8_t raw;
} oam_index;

typedef union {
  struct {
    uint8_t palette : 2;
    uint8_t : 3;
    uint8_t priority : 1;
    uint8_t flip_h : 1;
    uint8_t flip_v : 1;
  } attr;
  uint8_t raw;
} oam_attr;

/**
 * The main PPU struct. Holds internal state, memory, and registers.
 */
typedef struct {
  union {
    struct {
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
    struct {
      uint8_t gray : 1;
      uint8_t show_left_bg : 1;
      uint8_t show_left_sprites : 1;
      uint8_t show_bg : 1;
      uint8_t show_sprites : 1;
      uint8_t : 3;
    } flags;
    struct {
      uint8_t : 5;
      uint8_t emph_r : 1;
      uint8_t emph_g : 1;
      uint8_t emph_b : 1;
    } emph_ntsc;
    struct {
      uint8_t : 5;
      uint8_t emph_g : 1;
      uint8_t emph_r : 1;
      uint8_t emph_b : 1;
    } emph_pal;
    uint8_t raw;
  } mask;
  union {
    struct {
      uint8_t last_write : 5;
      uint8_t overflow : 1;
      uint8_t sprite0_hit : 1;
      uint8_t vblank : 1;
    } flags;
    uint8_t raw;
  } status;
  uint8_t oam_address;

  // TODO: this is virtual
  uint8_t oam_data;

  uint8_t memory[PPU_MEMORY];

  // Status
  uint16_t cycle;
  uint16_t scanline;
  bool frame_odd;

  uint32_t ignore_writes;

  // Internal registers
  scroll_reg v;
  scroll_reg t;
  uint8_t x;
  bool w;

  // Palette
  uint32_t palette[32];

  // Background rendering
  uint32_t pat_addr;
  uint16_t io_addr;
  uint16_t tile_pat;
  uint16_t tile_attr;
  uint32_t bg_pat;
  uint32_t bg_attr;

  // Sprite rendering
  uint8_t oam[256];
  union {
    struct {
      uint8_t y : 8;
      uint8_t index : 8;
      uint8_t attr : 8;
      uint8_t x : 8;
    } sprites[8];
    uint8_t raw[32];
  } oam_secondary;

  // Visual output
  ppu_driver driver;
  bool flip;
  uint8_t screen[PPU_SCREEN_SIZE];
} ppu;

void ppu_mem_write(ppu* ppu, uint16_t address, uint8_t value);
uint8_t ppu_mem_read(ppu* ppu, uint16_t address);

void ppu_reset(ppu* ppu);
void ppu_power(ppu* ppu);

ppu* ppu_init(void);

void ppu_cycle(ppu* ppu);

void ppu_deinit(ppu* ppu);
