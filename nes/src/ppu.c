#include <stdbool.h>
#include <stdlib.h>

#include "ppu.h"

/**
 * ppu.c
 */

ppu* ppu_init(void) {
  ppu* ppu = malloc(sizeof(ppu));
  ppu->cycle = 0;
  ppu->scanline = PPU_SL_PRERENDER;
  ppu->frame_odd = false;
  return ppu;
}

void ppu_cycle_fetch_nt(ppu* ppu) {
  
}

void ppu_cycle_fetch_at(ppu* ppu) {
  
}

void ppu_cycle_bg_low(ppu* ppu) {
  
}

void ppu_cycle_bg_high(ppu* ppu) {
  
}

void ppu_cycle(ppu* ppu) {
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
        ppu->status.flags.vblank = 1;
      }
      break;
    case 261:
      if (ppu->cycle == 1) {
        ppu->status.flags.vblank = 0;
      }
      break;
  }

  bool render = false;

  // Memory look-up
  if ((ppu->scanline >= PPU_SL_VISIBLE && ppu->scanline < PPU_SL_POSTRENDER) ||
      ppu->scanline == PPU_SL_PRERENDER) {
    render = ppu->mask.flags_ntsc.show_sprites || ppu->mask.flags_ntsc.show_bg;
    if ((ppu->cycle >= 1 && ppu->cycle <= 256) ||
        (ppu->cycle >= 321 && ppu->cycle <= 338)) {
      switch (ppu->cycle % 8) {
        case 2:
          ppu_cycle_fetch_nt(ppu);
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
        ppu->cycle >= 280 && ppu->cycle <= 340) {
      // vert(v) = vert(t)
      ppu->v.scroll.y_coarse = ppu->t.scroll.y_coarse;
      ppu->v.scroll.ny = ppu->t.scroll.ny;
      ppu->v.scroll.y_fine = ppu->t.scroll.y_fine;
    }
    if (ppu->cycle >= 328 || ppu->cycle <= 256) {
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
      ppu->scanline = PPU_SL_VISIBLE;
      ppu->frame_odd = !ppu->frame_odd;
    }
  }
}

void ppu_mem_write(ppu* ppu, uint16_t address, uint8_t value) {
  switch (address) {
    case 0x2000:
      ppu->ctrl.raw = value;
      break;
    case 0x2001:
      ppu->mask.raw = value;
      break;
    case 0x2003:
      ppu->oam_address = value;
      break;
    case 0x2004:
      ppu->oam_data = value;
      break;
    case 0x2005:
      if (!ppu->w) {
        ppu->t.scroll.x_coarse = value >> 3;
        ppu->x = value & 0x7;
      } else {
        ppu->t.scroll.y_coarse = value >> 3;
        ppu->t.scroll.y_fine = value;
      }
      ppu->w = !ppu->w;
      break;
    case 0x2006:
      if (!ppu->w) {
        ppu->t.raw = (ppu->t.raw & 0xFF) | ((value & 0x3F) << 8);
      } else {
        ppu->t.raw = (ppu->t.raw & 0x7FFF) | value;
      }
      ppu->w = !ppu->w;
      break;
    case 0x2007:
      // TODO: increment according to 0x2000 : 2
      break;
    case 0x4014:
      // TODO: OAM write from CPU to PPU
      break;
  }
}

uint8_t ppu_mem_read(ppu* ppu, uint16_t address) {
  switch (address) {
    case 0x2002:
      ppu->w = false;
      return ppu->status.raw;
    case 0x2003:
      return ppu->oam_address;
    case 0x2004:
      return ppu->oam_data;
    case 0x2007:
      // TODO: maybe increment?
      return ppu->data;
  }
  return 0;
}

void ppu_deinit(ppu* ppu) {
  free(ppu);
}
