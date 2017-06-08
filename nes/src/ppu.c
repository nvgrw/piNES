#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ppu.h"

/**
 * ppu.c
 */

void ppu_mem_write(ppu* ppu, uint16_t address, uint8_t value) {
  if (ppu->ignore_writes) {
    // Ignore writes to these registers for some time after reset
    switch (address) {
      case PPU_ADDR_CTRL:
      case PPU_ADDR_MASK:
      case PPU_ADDR_PPUSCROLL:
      case PPU_ADDR_PPUADDR:
        return;
    }
  }

  switch (address) {
    case PPU_ADDR_CTRL:
      ppu->ctrl.raw = value;
      ppu->nmi_output = ppu->ctrl.flags.nmi;
      break;
    case PPU_ADDR_MASK:
      ppu->mask.raw = value;
      break;
    case PPU_ADDR_OAMADDR:
      ppu->oam_address = value;
      break;
    case PPU_ADDR_OAMDATA:
      ppu->oam[ppu->oam_address] = value;
      ppu->oam_address++;
      break;
    case PPU_ADDR_PPUSCROLL:
      if (!ppu->w) {
        ppu->t.scroll.x_coarse = value >> 3;
        ppu->x = value & 0x7;
      } else {
        ppu->t.scroll.y_coarse = value >> 3;
        ppu->t.scroll.y_fine = value;
      }
      ppu->w = !ppu->w;
      break;
    case PPU_ADDR_PPUADDR:
      if (!ppu->w) {
        ppu->t.raw = (ppu->t.raw & 0xFF) | ((value & 0x3F) << 8);
      } else {
        ppu->t.raw = (ppu->t.raw & 0x7FFF) | value;
      }
      ppu->w = !ppu->w;
      break;
    case PPU_ADDR_PPUDATA:
      mmap_ppu_write(ppu->mapper, ppu->t.raw, value);
      ppu->t.raw += (ppu->ctrl.flags.increment ? 1 : 32);
      break;
    case PPU_ADDR_OAMDMA: {
      uint8_t buf[256];
      mmap_cpu_dma(ppu->mapper, value, buf);
      memcpy(ppu->oam + ppu->oam_address, buf, 256 - ppu->oam_address);
      if (ppu->oam_address) {
        memcpy(ppu->oam, buf + (256 - ppu->oam_address), ppu->oam_address);
      }
    } break;
  }
}

uint8_t ppu_mem_read(ppu* ppu, uint16_t address) {
  switch (address) {
    case PPU_ADDR_STATUS: {
      ppu->w = false;
      ppu->status.flags.vblank = ppu->nmi_occurred;
      ppu->nmi_occurred = false;
      return ppu->status.raw;
    }
    case PPU_ADDR_OAMDATA:
      // TODO: ignore when rendering
      return ppu->oam[ppu->oam_address];
    case PPU_ADDR_PPUDATA: {
      uint8_t ret = mmap_ppu_read(ppu->mapper, ppu->t.raw);
      ppu->t.raw += (ppu->ctrl.flags.increment ? 1 : 32);
      return ret;
    }
  }
  return 0;
}

uint8_t ppu_mem_read_dummy(ppu* ppu, uint16_t address) {
  switch (address) {
    case PPU_ADDR_STATUS:
      return ppu->status.raw;
    case PPU_ADDR_OAMADDR:
      return ppu->oam_address;
    case PPU_ADDR_OAMDATA:
      return mmap_ppu_read(ppu->mapper, ppu->t.raw);
    case PPU_ADDR_PPUDATA:
      return 0;
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
  ppu->nmi_output = false;
  ppu->nmi = false;
  ppu_mem_write(ppu, PPU_ADDR_PPUSCROLL, 0);
  ppu_mem_write(ppu, PPU_ADDR_PPUSCROLL, 0);
  // ppu->data = 0; // TODO: read buffer set to 0
  ppu->frame_odd = false;
  // TODO: oam set to pattern

  // Top of the picture
  ppu->cycle = 0;
  ppu->scanline = PPU_SL_PRERENDER;

  ppu->ignore_writes = 29658 * 3;
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

uint32_t mmap(ppu* ppu, uint32_t addr) {
  addr &= 0x3FFF;
  if(addr >= 0x3F00) {
    // Palette access
    if(addr % 4 == 0) {
      addr &= 0x0F;
    }
    return ppu->palette[addr & 0x1F];
  }
  return mmap_ppu_read(ppu->mapper, addr);
}

void ppu_cycle_addr_nt(ppu* ppu) {
  ppu->io_addr = 0x2000 + ppu->v.nametable.addr;
}

void ppu_cycle_fetch_nt(ppu* ppu) {
  ppu->pat_addr = 0x1000 * ppu->ctrl.flags.bg_table
                  + 16 * mmap(ppu, ppu->io_addr) + ppu->v.scroll.y_fine;
  ppu->bg_pat  = (ppu->bg_pat  >> 16) + 0x00010000 * ppu->tile_pat;
  ppu->bg_attr = (ppu->bg_attr >> 16) + 0x55550000 * ppu->tile_attr;
}

void ppu_cycle_addr_at(ppu* ppu) {
  ppu->io_addr = 0x23C0 + 0x800 * ppu->v.scroll.ny + 0x400 * ppu->v.scroll.nx +
                 8 * (ppu->v.scroll.y_coarse / 4)
                 + (ppu->v.scroll.x_coarse / 4);
}

void ppu_cycle_fetch_at(ppu* ppu) {
  ppu->tile_attr = (mmap(ppu, ppu->io_addr) >>
                    ((ppu->v.scroll.x_coarse & 2) +
                     2 * (ppu->v.scroll.y_coarse & 2))) & 3;
}

void ppu_cycle_bg_low(ppu* ppu) {
  ppu->tile_pat = mmap(ppu, ppu->pat_addr);
}

void ppu_cycle_bg_high(ppu* ppu) {
  uint32_t p = ppu->tile_pat | (mmap(ppu, ppu->pat_addr | 8) << 8);
  p = (p & 0xF00F) | ((p & 0x0F00) >> 4) | ((p & 0x00F0) << 4);
  p = (p & 0xC3C3) | ((p & 0x3030) >> 2) | ((p & 0x0C0C) << 2);
  p = (p & 0x9999) | ((p & 0x4444) >> 1) | ((p & 0x2222) << 1);
  ppu->tile_pat = p;
}

void ppu_cycle(ppu* ppu) {
  if (ppu->ignore_writes > 0) {
    ppu->ignore_writes--;
  }

  // Special operations
  switch (ppu->scanline) {
    case 0:
      if (ppu->cycle == 0 && ppu->frame_odd) {
        // Skip on odd cycles, unless ... TODO
        ppu->cycle++;
      }
      break;
    case 241:
      if (ppu->cycle == 1) {
        ppu->nmi_occurred = true;
      }
      break;
    case 261:
      if (ppu->cycle == 1) {
        ppu->nmi_occurred = false;
      }
      break;
  }

  bool render = false;

  // Memory look-up
  if ((ppu->scanline >= PPU_SL_VISIBLE && ppu->scanline < PPU_SL_POSTRENDER) ||
      ppu->scanline == PPU_SL_PRERENDER) {
    render = ppu->mask.flags.show_sprites || ppu->mask.flags.show_bg;
    if ((ppu->cycle >= 1 && ppu->cycle <= 256) ||
        (ppu->cycle >= 321 && ppu->cycle <= 338)) {
      switch (ppu->cycle % 8) {
        case 1:
          ppu_cycle_addr_nt(ppu);
          break;
        case 2:
          ppu_cycle_fetch_nt(ppu);
          break;
        case 3:
          ppu_cycle_addr_at(ppu);
          break;
        case 4:
          ppu_cycle_fetch_at(ppu);
          break;
        case 6:
          ppu_cycle_bg_low(ppu);
          break;
        case 0:
          ppu_cycle_bg_high(ppu);
          break;
      }
    } else if (ppu->cycle == 340) {
      ppu_cycle_fetch_nt(ppu);
    }
  }

  uint32_t pixel = 0;
  if (render) {
    // Render pixel
    bool edge = (ppu->cycle < 8 || (ppu->cycle >= 248 && ppu->cycle < 256));
    bool show_bg = ppu->mask.flags.show_bg &&
                   (!edge || ppu->mask.flags.show_left_bg);
    //bool show_sprites = ppu->mask.flags.show_sprites &&
    //                    (!edge || ppu->mask.flags.show_left_sprites);

    // Render the background
    uint32_t xpos = 15 - (((ppu->cycle & 7) + ppu->x + 8 * !!(ppu->cycle & 7)) & 15);
    uint32_t attr = 0;
    if(show_bg) {
      pixel = (ppu->bg_pat >> (xpos * 2)) & 3;
      attr = (ppu->bg_attr >> (xpos * 2)) & (pixel ? 3 : 0);
    } else if ((ppu->v.raw & 0x3F00) == 0x3F00 && !(ppu->mask.flags.show_bg || ppu->mask.flags.show_sprites)) {
      pixel = ppu->v.raw;
    }
    // TODO: sprites
    pixel = ppu->palette[(attr * 4 + pixel) & 0x1F] &
            (ppu->mask.flags.gray ? 0x30 : 0x3F);
  }

  if (ppu->scanline >= PPU_SL_VISIBLE && ppu->scanline < PPU_SL_POSTRENDER &&
      ppu->cycle >= 0 && ppu->cycle <= 256) {
    // Use the current driver to render the pixel
    switch (ppu->driver) {
      case PPUD_DIRECT:
        ppu->screen[ppu->cycle + ppu->scanline * 256] = pixel;
        // Emphasis | (reg.EmpRGB << 6);
        break;
      case PPUD_SIGNAL:
        // TODO: Emulate NTSC signal generation + noise + decoding
        break;
    }
  }

  if (render) {
    // Increment positions
    switch (ppu->cycle) {
      case 256:
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
        break;
      case 257:
        // hori(v) = hori(t)
        ppu->v.scroll.x_coarse = ppu->t.scroll.x_coarse;
        ppu->v.scroll.nx = ppu->t.scroll.nx;
        break;
    }
    if (ppu->scanline == PPU_SL_PRERENDER &&
        ppu->cycle >= 280 && ppu->cycle <= 340 && render) {
      // vert(v) = vert(t)
      ppu->v.scroll.y_coarse = ppu->t.scroll.y_coarse;
      ppu->v.scroll.ny = ppu->t.scroll.ny;
      ppu->v.scroll.y_fine = ppu->t.scroll.y_fine;
    }
    if ((ppu->cycle >= 328 || ppu->cycle <= 256) &&
        ppu->cycle % 8 == 0 && ppu->cycle != 0) {
      // Increment hori(v)
      if (ppu->v.scroll.x_coarse < 31) {
        ppu->v.scroll.x_coarse++;
      } else {
        ppu->v.scroll.x_coarse = 0;
        ppu->v.scroll.nx = 1 - ppu->v.scroll.nx;
      }
    }
  }

  // Increment cycle and scanline counters
  ppu->cycle++;
  if (ppu->cycle >= PPU_CYCLES) {
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
