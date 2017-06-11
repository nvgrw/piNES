#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ppu.h"

/**
 * ppu.c
 */

/**
 * Helper functions
 */

/**
 * Reads the VRAM.
 */
static uint32_t mmap(ppu* ppu, uint32_t address) {
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

/**
 * Sets the PPU address bus to a nametable entry.
 */
static void ppu_cycle_addr_nt(ppu* ppu) {
  ppu->io_addr = 0x2000 + ppu->v.nametable.addr;
}

/**
 * Reads a nametable entry.
 */
static void ppu_cycle_fetch_nt(ppu* ppu, bool garbage) {
  ppu->ren_nt = mmap(ppu, ppu->io_addr);
}

/**
 * Sets the PPU address bus to an attribute table entry.
 */
static void ppu_cycle_addr_at(ppu* ppu) {
  uint16_t v = ppu->v.raw;
  ppu->io_addr = 0x23C0 | (v & 0x0C00) | ((v >> 4) & 0x38) | ((v >> 2) & 0x07);
}

/**
 * Reads an attribute table entry.
 */
static void ppu_cycle_fetch_at(ppu* ppu, bool garbage) {
  uint16_t v = ppu->v.raw;
  uint8_t shift = ((v >> 4) & 4) | (v & 2);
  uint8_t res = ((mmap(ppu, ppu->io_addr) >> shift) & 3) << 2;
  if (!garbage) {
    ppu->ren_at = res;
  }
}

/**
 * Reads the low byte of a BG tile.
 */
static void ppu_cycle_bg_low(ppu* ppu) {
  ppu->io_addr = 0x1000 * (uint16_t)(ppu->ctrl.flags.bg_table) + 
                 16 * (uint16_t)(ppu->ren_nt) +
                 ppu->v.scroll.y_fine;
  ppu->ren_bg_low = mmap(ppu, ppu->io_addr);
}

/**
 * Reads the high byte of a BG tile.
 */
static void ppu_cycle_bg_high(ppu* ppu) {
  ppu->io_addr = 0x1000 * (uint16_t)(ppu->ctrl.flags.bg_table) + 
                 16 * (uint16_t)(ppu->ren_nt) +
                 ppu->v.scroll.y_fine;
  ppu->ren_bg_high = mmap(ppu, ppu->io_addr + 8);
}

/**
 * Combines attributes, low and high bytes of tile.
 */
static void ppu_cycle_tile(ppu* ppu) {
  uint32_t data = 0;
  for (uint8_t i = 0; i < 8; i++) {
    uint8_t p1 = (ppu->ren_bg_low & 0x80) >> 7;
    uint8_t p2 = (ppu->ren_bg_high & 0x80) >> 6;
    ppu->ren_bg_low <<= 1;
    ppu->ren_bg_high <<= 1;
    data <<= 4;
    data |= (uint32_t)(ppu->ren_at | p1 | p2);
  }
  ppu->tile_data |= (uint64_t)data;
}

/**
 * Public funcitons
 * 
 * See ppu.h for descriptions.
 */

void ppu_mem_write(ppu* ppu, uint16_t address, uint8_t value) {
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
    case PPU_ADDR_CTRL: // 0
      ppu->ctrl.raw = value;
      ppu->t.nt_select.nt = ppu->ctrl.flags.nametable;
      ppu->nmi_output = ppu->ctrl.flags.nmi;
      break;
    case PPU_ADDR_MASK: // 1
      ppu->mask.raw = value;
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
      ppu->v.raw += (ppu->ctrl.flags.increment ? 32 : 1);
    } break;
  }
}

uint8_t ppu_mem_read(ppu* ppu, uint16_t address, bool dummy) {
  if (address == PPU_ADDR_OAMDMA) {
    return 0;
  }

  address &= 0x3FFF;
  address -= 0x2000;
  address &= 0x7;

  switch (address) {
    case PPU_ADDR_STATUS: { // 2
      ppu->status.flags.vblank = ppu->nmi_occurred;
      if (!dummy) {
        ppu->w = false;
        ppu->nmi_occurred = false;
      }
      return ppu->status.raw;
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
        ppu->v.raw += (ppu->ctrl.flags.increment ? 32 : 1);
      }
      return ret;
    }
  }
  return 0;
}

void ppu_reset(ppu* ppu) {
  ppu->ctrl.raw = 0;
  ppu->mask.raw = 0;
  ppu->status.raw = (ppu->status.raw & 0x80);
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
  for (uint8_t i = 0; i < 64; i++) {
    ppu->oam.raw32[i] = 0xFFFFFFFF;
  }
  for (uint8_t i = 0; i < 8; i++) {
    ppu->oam_secondary.raw32[i] = 0xFFFFFFFF;
    ppu->oam_tertiary.raw32[i] = 0xFFFFFFFF;
  }
}

void ppu_power(ppu* ppu) {
  // http://wiki.nesdev.com/w/index.php/PPU_power_up_state
  ppu_reset(ppu);
  ppu->status.raw = 0xA0;
  ppu->oam_address = 0;
  // ppu->v.raw = 0;
  ppu->t.raw = 0;
  ppu->x = 0;
}

ppu* ppu_init(void) {
  ppu* ret = malloc(sizeof(ppu));
  ret->driver = PPUD_DIRECT;
  ret->flip = false;
  for (int i = 0; i < PPU_SCREEN_SIZE; i++){
    ret->screen[i] = 0;
  }
  ppu_power(ret);
  return ret;
}

void ppu_cycle(ppu* ppu) {
  bool show_sprites = ppu->mask.flags.show_sprites;
  bool show_bg = ppu->mask.flags.show_bg;
  bool rendering = show_sprites || show_bg;
  bool line_pre = ppu->scanline == PPU_SL_PRERENDER;
  bool line_visible = ppu->scanline < PPU_SL_POSTRENDER;
  bool line_render = line_pre || line_visible;
  bool cycle_pre = ppu->cycle >= 321 && ppu->cycle <= 336;
  bool cycle_visible = ppu->cycle >= 1 && ppu->cycle <= 256;
  bool cycle_fetch = cycle_pre || cycle_visible;
  //uint8_t sprite_size = (ppu->ctrl.flags.sprite_size ? 16 : 8);

  ppu->oam_data_ff = false;

  // Most PPU operations are done only when rendering is enabled
  if (rendering) {
    if (line_visible && cycle_visible) {
      // Render a pixel
      uint8_t pixel = 0;

      bool edge = (ppu->cycle < 8 || (ppu->cycle >= 248 && ppu->cycle < 256));
      bool show_bg_e = show_bg && (!edge || ppu->mask.flags.show_left_bg);
      //bool show_sprites_e = show_sprites && (!edge || ppu->mask.flags.show_left_sprites);

      // Render the background
      uint32_t attr = 0;
      if (show_bg_e) {
        pixel = ((uint32_t)(ppu->tile_data >> 32) >> ((7 - ppu->x) * 4)) & 0x0F;
      } else if ((ppu->v.raw & 0x3F00) == 0x3F00 && !(ppu->mask.flags.show_bg || ppu->mask.flags.show_sprites)) {
        pixel = ppu->v.raw;
      }

      /*
      // TODO: Render the sprites
      if (show_sprites_e) {
        for (int i = 0; i < ppu->spr_ren_count; i++) {
          oam_sprite spr = ppu->oam_tertiary.sprites[i];
          oam_state spr_state = ppu->oam_tertiary_state[i];
          oam_attr spr_attr = {.raw = spr.attr};
          uint32_t xdiff = ppu->cycle + 1 - spr.x;
          if (xdiff >= 8) {
            // Also matches negative values
            continue;
          }
          if (spr_attr.attr.flip_h) {
            xdiff = 7 - xdiff;
          }
          uint8_t spr_pixel = (spr_state.pattern >> (xdiff * 2)) & 3;
          if (!spr_pixel) {
            // Transparent pixel, skip
            continue;
          }
          // Register sprite-0 hit if applicable
          if (ppu->cycle < 256 && pixel && spr_state.oam_index == 0) {
            ppu->status.flags.sprite0_hit = true;
          }
          // Render the pixel unless behind-background placement wanted
          if (spr_attr.attr.priority || !pixel) {
            attr = spr_attr.attr.palette + 4;
            pixel = spr_pixel;
          }
          // Only process the first non-transparent sprite pixel.
          break;
        }
      }
      */

      // Apply the palette
      pixel = ppu->palette[(attr * 4 + pixel) & 0x1F] &
              (ppu->mask.flags.gray ? 0x30 : 0x3F);

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
  }

  /*
  if ((ppu->scanline >= PPU_SL_VISIBLE && ppu->scanline < PPU_SL_POSTRENDER) ||
      ppu->scanline == PPU_SL_PRERENDER) {
    // Background memory management
    render = show_sprites || show_bg;
    
    if (render) {
      if (ppu->cycle == 0) {
        // Idle
      } else if ((/ *ppu->cycle >= 1 && * /ppu->cycle <= 256) ||
          (ppu->cycle >= 321 && ppu->cycle <= 338)) {
        
      } else if (ppu->cycle >= 257 && ppu->cycle <= 320) {
        // Garbage background accesses when evaluating sprites
        switch (ppu->cycle % 8) {
          case 1:
            ppu_cycle_addr_nt(ppu);
            break;
          case 2:
            ppu_cycle_fetch_nt(ppu, true);
            break;
          case 3:
            ppu_cycle_addr_nt(ppu);
            break;
          case 4:
            ppu_cycle_fetch_at(ppu, true);
            oam_sprite spr = ppu->oam_tertiary.sprites[ppu->spr_ren_pos];
            uint16_t y = ppu->scanline - spr.y;
            oam_attr spr_attr = {.raw = spr.attr};
            if (spr_attr.attr.flip_v){
              y = (sprite_size - y - 1);
            }
            if (sprite_size == 16) {
              ppu->pat_addr = 0x1000 * (spr.index & 0x01) +
                              0x10 * (spr.index & 0xFE);
            } else {
              ppu->pat_addr = 0x1000 * (ppu->ctrl.flags.sprite_table) +
                              0x10 * (spr.index & 0xFF);
            }
            ppu->pat_addr += (y & 7) + (y & 8) * 2;
            break;
          case 6:
            ppu_cycle_bg_low(ppu);
            break;
          case 0:
            ppu_cycle_bg_high(ppu);
            if (ppu->spr_ren_pos < ppu->spr_ren_count) {
              ppu->oam_tertiary_state[ppu->spr_ren_pos].pattern = ppu->tile_pat;
              ppu->spr_ren_pos++;
            }
            break;
        }
      } else if (ppu->cycle == 340) {
        ppu_cycle_fetch_nt(ppu, false);
      }
    }

    // Sprite memory management
    if (!ppu->cycle || ppu->scanline == PPU_SL_PRERENDER) {
      // Idle
    } else if (/ *ppu->cycle >= 1 && * /ppu->cycle <= 64) {
      // Clearing secondary OAM
      ppu->oam_data_ff = true;
      ppu->oam_secondary.raw[(ppu->cycle - 1) / 2] = 0xFF;
      ppu->spr_ren_pos = 0;
      ppu->spr_ren_count = ppu->oam_spr_count;
      ppu->oam_spr_n = 0;
      ppu->oam_spr_count = 0;
    } else if (/ *ppu->cycle >= 65 && * /ppu->cycle <= 256) {
      // Sprite evaluation
      if (ppu->cycle % 2 == 1) {
        // Read from primary OAM, ignore for now
      } else if (ppu->oam_spr_count < 9 && ppu->oam_spr_n < 64) {
        // Write to secondary OAM
        uint8_t y = ppu->oam.sprites[ppu->oam_spr_n].y;
        bool range = y >= ppu->scanline &&
                     y < ppu->scanline + sprite_size;
        if (range && ppu->oam_spr_count < 8) {
          // Copy bytes if in Y range
          ppu->oam_secondary.raw32[ppu->oam_spr_count]
            = ppu->oam.raw32[ppu->oam_spr_n];
          ppu->oam_secondary_state[ppu->oam_spr_count].oam_index
            = ppu->oam_spr_n;
          ppu->oam_spr_count++;
        } else if (range) {
          // Overflow
          // TODO: Sprite overflow bug
          ppu->status.flags.overflow = true;
        } else if (ppu->oam_spr_count < 8) {
          // Only copy the Y byte if not in range
          ppu->oam_secondary.sprites[ppu->oam_spr_count].y = y;
        }
        ppu->oam_spr_n++;
      }
    } else if (/ *ppu->cycle >= 257 && * /ppu->cycle <= 320) {
      // Sprite fetches
      memcpy(ppu->oam_tertiary.raw, ppu->oam_secondary.raw, 32);
      memcpy(ppu->oam_tertiary_state, ppu->oam_secondary_state, 8 * sizeof(oam_state));
    } else if (/ *ppu->cycle >= 321 && * /ppu->cycle <= 340) {
      // Background render pipeline initialization
      // Some OAM reads should be done here, but they should have no effect
    }
  }
  */

  // NMI and flag operations
  if (ppu->scanline == 241 && ppu->cycle == 1) {
    // Start VBlank
    ppu->nmi_occurred = true;
    ppu->status.flags.sprite0_hit = false;
  } else if (ppu->scanline == 261 && ppu->cycle == 1) {
    // End VBlank
    ppu->nmi_occurred = false;
    ppu->status.flags.overflow = false;
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
}

void ppu_deinit(ppu* ppu) {
  free(ppu);
}
