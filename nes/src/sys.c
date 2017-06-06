#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "ppu.h"
#include "sys.h"

/**
 * sys.c
 */

sys* sys_init(void) {
  sys* ret = malloc(sizeof(sys));
  ret->clock = 0.0;
  ret->ppu = ppu_init();
  ret->region = R_NTSC;
  return ret;
}

// TODO: move this to region
#define CLOCKS_PER_SECOND 21477272.0
#define CPU_CYCLES_PER_SECOND (CLOCKS_PER_SECOND / 12.0)
#define PPU_CYCLES_PER_SECOND (CLOCKS_PER_SECOND / 4.0)

#define CLOCK_MS (12.0 / 21477.272)

void sys_run(sys* sys, uint32_t ms) {
  sys->clock += ms;
  while (sys->clock >= CLOCK_MS) {
    // cpu cycle

    ppu_cycle(sys->ppu);
    ppu_cycle(sys->ppu);
    ppu_cycle(sys->ppu);

    // apu cycle

    sys->clock -= CLOCK_MS;
  }
}

void sys_deinit(sys* sys) {
  ppu_deinit(sys->ppu);
  free(sys);
}
