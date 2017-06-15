#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ppu.h"
#include "profiler.h"

/**
 * ppu.c
 */

/**
 * Helper functions
 * 
 * mmap
 *   Reads the VRAM at the given address.
 * 
 * ppu_cycle_addr_nt
 *   Sets the PPU address bus to a nametable entry.
 * 
 * ppu_cycle_fetch_nt
 *   Reads a nametable entry.
 * 
 * ppu_cycle_addr_at
 *   Sets the PPU address bus to an attribute table entry.
 * 
 * ppu_cycle_fetch_at
 *   Reads an attribute table entry.
 * 
 * ppu_cycle_bg_low
 *   Reads the low byte of a BG tile.
 * 
 * ppu_cycle_bg_high
 *   Reads the high byte of a BG tile.
 * 
 * ppu_cycle_tile
 *   Combines attributes, low and high bytes of tile.
 * 
 * ppu_fetch_sprite
 *   Fetches and decodes a sprite from the OAM.
 */
static uint32_t mmap(ppu_t* ppu, uint32_t address) {
  address &= 0x3FFF;
  if (address >= 0x3F00) {
    // Palette access
    if (address % 4 == 0) {
      address &= 0x0F;
    }
    return ppu->palette[address & 0x1F];
  }
  return mmap_ppu_read(ppu->mapper, address);
}

static void ppu_cycle_addr_nt(ppu_t* ppu) {
  ppu->io_addr = 0x2000 + ppu->v.nametable.addr;
}

static void ppu_cycle_fetch_nt(ppu_t* ppu, bool garbage) {
  ppu->ren_nt = mmap(ppu, ppu->io_addr);
}

static void ppu_cycle_addr_at(ppu_t* ppu) {
  uint16_t v = ppu->v.raw;
  ppu->io_addr = 0x23C0 | (v & 0x0C00) | ((v >> 4) & 0x38) | ((v >> 2) & 0x07);
}

static void ppu_cycle_fetch_at(ppu_t* ppu, bool garbage) {
  uint16_t v = ppu->v.raw;
  uint8_t shift = ((v >> 4) & 4) | (v & 2);
  uint8_t res = (mmap(ppu, ppu->io_addr) >> shift) & 3;
  if (!garbage) {
    ppu->ren_at = res;
  }
}

static void ppu_cycle_bg_low(ppu_t* ppu) {
  ppu->io_addr = 0x1000 * (uint16_t)(ppu->ctrl_bg_table) + 
                 16 * (uint16_t)(ppu->ren_nt) +
                 ppu->v.scroll.y_fine;
  ppu->ren_bg_low = mmap(ppu, ppu->io_addr);
}

static void ppu_cycle_bg_high(ppu_t* ppu) {
  ppu->io_addr = 0x1000 * (uint16_t)(ppu->ctrl_bg_table) + 
                 16 * (uint16_t)(ppu->ren_nt) +
                 ppu->v.scroll.y_fine;
  ppu->ren_bg_high = mmap(ppu, ppu->io_addr + 8);
}

static void ppu_cycle_tile(ppu_t* ppu) {
  uint32_t data = 0;
  for (uint8_t i = 0; i < 8; i++) {
    data <<= 4;
    data |= ((ppu->ren_bg_low << i) & 0x80) >> 7;
    data |= ((ppu->ren_bg_high << i) & 0x80) >> 6;
  }
  data |= ppu->ren_at * 0x44444444;
  ppu->tile_data |= (uint64_t)data;
}

static uint32_t ppu_fetch_sprite(ppu_t* ppu, uint8_t i, uint16_t row) {
  uint16_t bank = ppu->ctrl_sprite_table;
  uint8_t tile = ppu->oam.sprites[i].index;
  oam_attr_t attr = {.raw = ppu->oam.sprites[i].attr};
  uint16_t addr;

  if (attr.attr.flip_v) {
    row = (ppu->sprite_size - 1) - row;
  }
  if (ppu->ctrl_sprite_size) {
    oam_index_t tile_index = {.raw = tile};
    bank = tile_index.index.bank;
    tile = tile_index.index.tile;
    if (row > 7) {
      tile++;
      row -= 8;
    }
  }
  addr = 0x1000 * ((uint16_t)bank) + 0x10 * ((uint16_t)tile) + ((uint16_t)row);
  ppu->ren_bg_low = mmap(ppu, addr);
  ppu->ren_bg_high = mmap(ppu, addr + 8);
  uint32_t data = 0;
  if (attr.attr.flip_h) {
    for (uint8_t i = 0; i < 8; i++) {
      data <<= 4;
      data |= ((ppu->ren_bg_low >> i) & 1);
      data |= ((ppu->ren_bg_high >> i) & 1) << 1;
    }
  } else {
    for (uint8_t i = 0; i < 8; i++) {
      data <<= 4;
      data |= ((ppu->ren_bg_low << i) & 0x80) >> 7;
      data |= ((ppu->ren_bg_high << i) & 0x80) >> 6;
    }
  }
  data |= attr.attr.palette * 0x44444444;
  return data;
}

/**
 * Public funcitons
 * 
 * See ppu.h for descriptions.
 */
void ppu_mem_write(ppu_t* ppu, uint16_t address, uint8_t value) {
  if (address == PPU_ADDR_OAMDMA) {
    uint8_t buf[256];
    mmap_cpu_dma(ppu->mapper, value, buf);
    memcpy(ppu->oam.raw + ppu->oam_address, buf, 256 - ppu->oam_address);
    if (ppu->oam_address) {
      memcpy(ppu->oam.raw, buf + (256 - ppu->oam_address), ppu->oam_address);
    }
    return;
  }

  address &= 0x3FFF;
  address -= 0x2000;
  address &= 0x7;

  switch (address) {
    case PPU_ADDR_PPUCTRL: // 0
      // Destructure register
      ppu->reg_ctrl.raw = value;
      ppu->ctrl_nametable = ppu->reg_ctrl.flags.nametable;
      ppu->ctrl_increment = ppu->reg_ctrl.flags.increment;
      ppu->ctrl_sprite_table = ppu->reg_ctrl.flags.sprite_table;
      ppu->ctrl_bg_table = ppu->reg_ctrl.flags.bg_table;
      ppu->ctrl_sprite_size = ppu->reg_ctrl.flags.sprite_size;
      ppu->ctrl_ppu_master = ppu->reg_ctrl.flags.ppu_master;
      ppu->ctrl_nmi = ppu->reg_ctrl.flags.nmi;
      // Update values
      ppu->sprite_size = (ppu->ctrl_sprite_size ? 16 : 8);
      ppu->increment = (ppu->ctrl_increment ? 32 : 1);
      ppu->t.nt_select.nt = ppu->ctrl_nametable;
      ppu->nmi_output = ppu->ctrl_nmi;
      break;
    case PPU_ADDR_PPUMASK: // 1
      // Destructure register
      ppu->reg_mask.raw = value;
      ppu->mask_gray = ppu->reg_mask.flags.gray;
      ppu->mask_show_left_bg = ppu->reg_mask.flags.show_left_bg;
      ppu->mask_show_left_sprites = ppu->reg_mask.flags.show_left_sprites;
      ppu->mask_show_bg = ppu->reg_mask.flags.show_bg;
      ppu->mask_show_sprites = ppu->reg_mask.flags.show_sprites;
      break;
    case PPU_ADDR_OAMADDR: // 3
      ppu->oam_address = value;
      break;
    case PPU_ADDR_OAMDATA: // 4
      ppu->oam.raw[ppu->oam_address] = value;
      ppu->oam_address++;
      break;
    case PPU_ADDR_PPUSCROLL: // 5
      if (!ppu->w) {
        ppu->t.scroll.x_coarse = value >> 3;
        ppu->x = value & 0x7;
      } else {
        ppu->t.scroll.y_coarse = value >> 3;
        ppu->t.scroll.y_fine = value & 0x7;
      }
      ppu->w = !ppu->w;
      break;
    case PPU_ADDR_PPUADDR: // 6
      if (!ppu->w) {
        ppu->t.raw = (ppu->t.raw & 0xFF) | ((value & 0x3F) << 8);
      } else {
        ppu->t.raw = (ppu->t.raw & 0x7F00) | value;
        ppu->v.raw = ppu->t.raw;
      }
      ppu->w = !ppu->w;
      break;
    case PPU_ADDR_PPUDATA: { // 7
      uint16_t ppu_addr = ppu->v.raw & 0x3FFF;
      if (ppu_addr >= 0x3F00) {
        if (ppu_addr % 4 == 0) {
          ppu_addr &= 0x0F;
        }
        ppu->palette[ppu_addr & 0x1F] = value & 0x3F;
      } else {
        mmap_ppu_write(ppu->mapper, ppu_addr, value);
      }
      ppu->v.raw += ppu->increment;
    } break;
  }
}

uint8_t ppu_mem_read(ppu_t* ppu, uint16_t address, bool dummy) {
  if (address == PPU_ADDR_OAMDMA) {
    return 0;
  }

  address &= 0x3FFF;
  address -= 0x2000;
  address &= 0x7;

  switch (address) {
    case PPU_ADDR_PPUSTATUS: { // 2
      // Encode register
      ppu->reg_status.flags.last_write = ppu->status_last_write;
      ppu->reg_status.flags.overflow = ppu->status_overflow;
      ppu->reg_status.flags.sprite0_hit = ppu->status_sprite0_hit;
      ppu->reg_status.flags.vblank = ppu->nmi_occurred;
      // Update values
      if (!dummy) {
        ppu->w = false;
        ppu->nmi_occurred = false;
      }
      return ppu->reg_status.raw;
    }
    case PPU_ADDR_OAMDATA: // 4
      if (ppu->oam_data_ff) {
        return 0xFF;
      }
      // TODO: ignore when rendering
      return ppu->oam.raw[ppu->oam_address];
    case PPU_ADDR_PPUDATA: { // 7
      uint16_t ppu_addr = ppu->v.raw & 0x3FFF;
      uint8_t ret = 0;
      if (ppu_addr >= 0x3F00) {
        if (ppu_addr % 4 == 0) {
          ppu_addr &= 0x0F;
        }
        ret = ppu->palette[ppu_addr & 0x1F];
      } else {
        ret = mmap_ppu_read(ppu->mapper, ppu->v.raw);
      }
      if (!dummy) {
        ppu->v.raw += ppu->increment;
      }
      return ret;
    }
  }
  return 0;
}

void ppu_reset(ppu_t* ppu) {
  ppu_mem_write(ppu, PPU_ADDR_PPUCTRL, 0);
  ppu_mem_write(ppu, PPU_ADDR_PPUMASK, 0);
  ppu_mem_write(ppu, PPU_ADDR_PPUSTATUS, ppu->reg_status.raw & 0x80);
  // oam address unchanged
  ppu->w = false;
  ppu->nmi_occurred = false;
  ppu->nmi_output = true;
  ppu->nmi = false;
  ppu_mem_write(ppu, PPU_ADDR_PPUSCROLL, 0);
  ppu_mem_write(ppu, PPU_ADDR_PPUSCROLL, 0);
  ppu->frame_odd = false;
  // TODO: oam set to pattern

  // Top of the picture
  ppu->cycle = 340;
  ppu->scanline = 240;

  ppu->oam_data_ff = false;

  // Reset OAM
  for (uint16_t i = 0; i < 256; i++) {
    ppu->oam.raw[i] = 0xFF;
  }
}

void ppu_power(ppu_t* ppu) {
  // http://wiki.nesdev.com/w/index.php/PPU_power_up_state
  ppu_reset(ppu);
  ppu->reg_status.raw = 0xA0;
  ppu->oam_address = 0;
  // ppu->v.raw = 0;
  ppu->t.raw = 0;
  ppu->x = 0;
}

ppu_t* ppu_init(void) {
  ppu_t* ppu = malloc(sizeof(ppu_t));
  ppu->driver = PPUD_DIRECT;
  ppu->flip = false;
  for (int i = 0; i < PPU_SCREEN_SIZE; i++){
    ppu->screen[i] = 0;
  }
  ppu_power(ppu);
  return ppu;
}

void ppu_cycle(ppu_t* ppu) {
  bool show_sprites = ppu->mask_show_sprites;
  bool show_bg = ppu->mask_show_bg;
  bool rendering = show_sprites || show_bg;
  bool line_pre = ppu->scanline == PPU_SL_PRERENDER;
  bool line_visible = ppu->scanline < PPU_SL_POSTRENDER;
  bool line_render = line_pre || line_visible;
  bool cycle_pre = ppu->cycle >= 321 && ppu->cycle <= 336;
  bool cycle_visible = ppu->cycle >= 1 && ppu->cycle <= 256;
  bool cycle_fetch = cycle_pre || cycle_visible;

  ppu->oam_data_ff = false;

  // Most PPU operations are done only when rendering is enabled
  if (rendering) {
    if (line_visible && cycle_visible) {
      // Render a pixel
      uint8_t pixel = 0;

      bool edge = (ppu->cycle < 8 || (ppu->cycle >= 248 && ppu->cycle < 256));
      bool show_bg_e = show_bg && (!edge || ppu->mask_show_left_bg);
      bool show_sprites_e = show_sprites && (!edge || ppu->mask_show_left_sprites);

      // Render background
      if (show_bg_e) {
        pixel = ((uint32_t)(ppu->tile_data >> 32) >> ((7 - ppu->x) * 4)) & 0x0F;
      } else if ((ppu->v.raw & 0x3F00) == 0x3F00 && !(ppu->mask_show_bg || ppu->mask_show_sprites)) {
        pixel = ppu->v.raw;
      }

      // PROFILER_POINT(SYS_PPU_BG)

      // Render sprites
      if (show_sprites_e) {
        uint8_t spr_found = 0xFF;
        uint8_t spr_pixel = 0;
        for (uint8_t i = 0; i < ppu->spr_count; i++) {
          int16_t offset = (ppu->cycle - 1) - ppu->spr_pos[i];
          if (offset < 0 || offset > 7) {
            continue;
          }
          offset = 7 - offset;
          spr_pixel = (ppu->spr_pat[i] >> (offset * 4)) & 0x0F;
          if (spr_pixel % 4 != 0) {
            // Non-transparent pixel, stop processing sprites
            spr_found = i;
            break;
          }
        }
        if (spr_found != 0xFF) {
          // Register sprite-0 hit if applicable
          if (cycle_visible && pixel && ppu->spr_index[spr_found] == 0) {
            ppu->status_sprite0_hit = true;
          }
          // Render the pixel unless behind-background placement wanted
          if (!ppu->spr_priority[spr_found] || !pixel) {
            pixel = spr_pixel + 0x10;
          }
        }
      }

      // PROFILER_POINT(SYS_PPU_SPRITES)

      // Apply the palette
      pixel = ppu->palette[pixel & 0x1F] &
              (ppu->mask_gray ? 0x30 : 0x3F);

      // Use the current driver to render the pixel
      switch (ppu->driver) {
        case PPUD_DIRECT:
          ppu->screen[ppu->cycle + ppu->scanline * 256] = pixel & 0x1F;
          // Emphasis | (reg.EmpRGB << 6);
          break;
        case PPUD_SIGNAL:
          // TODO: Emulate NTSC signal generation + noise + decoding
          break;
      }
    }

    if (line_render && cycle_fetch) {
      // Background evaluation for visible pixels and next scanline
      ppu->tile_data <<= 4;
      switch (ppu->cycle % 8) {
        case 1:
          ppu_cycle_addr_nt(ppu);
          break;
        case 2:
          ppu_cycle_fetch_nt(ppu, false);
          break;
        case 3:
          ppu_cycle_addr_at(ppu);
          break;
        case 4:
          ppu_cycle_fetch_at(ppu, false);
          break;
        case 6:
          ppu_cycle_bg_low(ppu);
          break;
        case 0:
          ppu_cycle_bg_high(ppu);
          ppu_cycle_tile(ppu);
          break;
      }
    }

    if (line_pre && ppu->cycle >= 280 && ppu->cycle <= 304 && show_bg) {
      // vert(v) = vert(t)
      ppu->v.scroll.y_coarse = ppu->t.scroll.y_coarse;
      ppu->v.scroll.ny = ppu->t.scroll.ny;
      ppu->v.scroll.y_fine = ppu->t.scroll.y_fine;
    }

    if (line_render) {
      if (cycle_fetch && ppu->cycle % 8 == 0) {
        // Increment hori(v)
        if (ppu->v.scroll.x_coarse < 31) {
          ppu->v.scroll.x_coarse++;
        } else {
          ppu->v.scroll.x_coarse = 0;
          ppu->v.scroll.nx = 1 - ppu->v.scroll.nx;
        }
      }

      if (ppu->cycle == 256) {
        // Increment vert(v)
        if (ppu->v.scroll.y_fine < 7) {
          ppu->v.scroll.y_fine++;
        } else {
          ppu->v.scroll.y_fine = 0;
          uint8_t y = ppu->v.scroll.y_coarse;
          if (y == 29) {
            y = 0;
            ppu->v.scroll.ny = 1 - ppu->v.scroll.ny;
          } else if (y == 31) {
            y = 0;
          } else {
            y++;
          }
          ppu->v.scroll.y_coarse = y;
        }
      }

      if (ppu->cycle == 257) {
        // hori(v) = hori(t)
        ppu->v.scroll.x_coarse = ppu->t.scroll.x_coarse;
        ppu->v.scroll.nx = ppu->t.scroll.nx;
      }
    }

    if (ppu->cycle == 257) {
      // Evaluate sprites (not HW-accurate for now)
      ppu->spr_count_max += ppu->spr_count;
      ppu->spr_count = 0;
      if (line_visible) {
        for (uint8_t i = 0; i < 64 && ppu->spr_count < 9; i++) {
          uint8_t x = ppu->oam.sprites[i].x;
          uint8_t y = ppu->oam.sprites[i].y;
          oam_attr_t attr = {.raw = ppu->oam.sprites[i].attr};
          if (ppu->scanline < y) {
            continue;
          }
          uint16_t row = ppu->scanline - y;
          if (row >= ppu->sprite_size) {
            continue;
          }
          if (ppu->spr_count < 8) {
            ppu->spr_pat[ppu->spr_count] = ppu_fetch_sprite(ppu, i, row);
            ppu->spr_pos[ppu->spr_count] = x;
            ppu->spr_priority[ppu->spr_count] = attr.attr.priority;
            ppu->spr_index[ppu->spr_count] = i;
            ppu->spr_count++;
          } else {
            ppu->status_overflow = true;
          }
        }
      }
    }
  }

  if (ppu->scanline == 0) {
    ppu->spr_count_max = 0;
  }

  // NMI and flag operations
  if (ppu->scanline == 241 && ppu->cycle == 1) {
    // Start VBlank
    ppu->nmi_occurred = true;
    ppu->status_sprite0_hit = false;
  } else if (ppu->scanline == 261 && ppu->cycle == 1) {
    // End VBlank
    ppu->nmi_occurred = false;
    ppu->status_overflow = false;
  }
  ppu->nmi = ppu->nmi_occurred && ppu->nmi_output;

  // Increment cycle and scanline counters
  ppu->cycle++;
  uint16_t sl_cycles = PPU_CYCLES;
  if (ppu->scanline == PPU_SL_PRERENDER && ppu->frame_odd && show_bg) {
    // Skip last cycle of prerender on odd frames if BG is rendered
    sl_cycles--;
  }
  if (ppu->cycle >= sl_cycles) {
    ppu->cycle = 0;
    ppu->scanline++;
    if (ppu->scanline >= PPU_SCANLINES) {
      ppu->flip = true;
      ppu->scanline = PPU_SL_VISIBLE;
      ppu->frame_odd = !ppu->frame_odd;
    }
  }

  // PROFILER_POINT(SYS_PPU_LOGIC)
}

void ppu_deinit(ppu_t* ppu) {
  free(ppu);
}
