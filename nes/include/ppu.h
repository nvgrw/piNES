#pragma once

#include <stdbool.h>
#include <stdint.h>

/**
 * ppu.h
 * 
 * Functions for the PPU (Picture Processing Unit).
 */

#define PPU_MEMORY 16384

#define PPU_SCANLINES 262
#define PPU_CYCLES 341

#define PPU_SL_VISIBLE 0
#define PPU_SL_POSTRENDER 240
#define PPU_SL_PRERENDER 261

typedef union {
  struct {
    uint8_t x_coarse : 5;
    uint8_t y_coarse : 5;
    uint8_t nx : 1;
    uint8_t ny : 1;
    uint8_t y_fine : 3;
  } scroll;
  uint16_t raw;
} scroll_reg;

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
      uint8_t emph_r : 1;
      uint8_t emph_g : 1;
      uint8_t emph_b : 1;
    } flags_ntsc;
    struct {
      uint8_t gray : 1;
      uint8_t show_left_bg : 1;
      uint8_t show_left_sprites : 1;
      uint8_t show_bg : 1;
      uint8_t show_sprites : 1;
      uint8_t emph_g : 1;
      uint8_t emph_r : 1;
      uint8_t emph_b : 1;
    } flags_pal;
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
  uint8_t oam_data;
  
  uint8_t address;
  uint8_t data;

  uint8_t memory[PPU_MEMORY];

  // Status
  uint16_t cycle;
  uint16_t scanline;
  bool frame_odd;

  // Internal registers
  scroll_reg v;
  scroll_reg t;
  uint8_t x;
  bool w;
} ppu;

ppu* ppu_init(void);

void ppu_cycle(ppu* ppu);

void ppu_mem_write(ppu* ppu, uint16_t address, uint8_t value);
uint8_t ppu_mem_read(ppu* ppu, uint16_t address);

void ppu_deinit(ppu* ppu);
