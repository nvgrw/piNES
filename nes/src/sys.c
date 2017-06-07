#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "cpu.h"
#include "ppu.h"
#include "sys.h"

/**
 * sys.c
 */

sys* sys_init(void) {
  sys* ret = malloc(sizeof(sys));
  ret->clock = 0.0;
  ret->cpu = cpu_init();
  ret->ppu = ppu_init();
  //ret->apu = apu_init();
  ret->region = R_NTSC;
  ret->running = false;
  return ret;
}

// TODO: move this to region
#define CLOCKS_PER_MILLISECOND 21477.272
#define CPU_CYCLES_PER_SECOND (CLOCKS_PER_SECOND / 12.0)
#define PPU_CYCLES_PER_SECOND (CLOCKS_PER_SECOND / 4.0)
#define CLOCK_PERIOD (4.0 / CLOCKS_PER_MILLISECOND)

void sys_run(sys* sys, uint32_t ms) {
  if (sys->running) {
    sys->clock += ms;
    uint8_t cpu_busy = 0;
    while (sys->clock >= CLOCK_PERIOD && sys->running) {
      // cpu cycle
      if (!cpu_busy) {
        cpu_busy += cpu_cycle(sys->cpu) * 3;
        if (!cpu_busy) {
          // Trapped, stop execution
          sys->running = false;
        }
      }
      cpu_busy--;

      ppu_cycle(sys->ppu);

      // apu cycle

      sys->clock -= CLOCK_PERIOD;
    }
  }
}

void sys_test(sys* sys) {
  FILE* fp = fopen("tests/6502_functional_test.bin", "r");
  fseek(fp, 0, SEEK_END);
  size_t size = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  fread(sys->cpu->memory + 0x400, 1, size, fp);
  fclose(fp);
  sys->cpu->program_counter = 0x400;
  sys->running = true;
}

void sys_deinit(sys* sys) {
  ppu_deinit(sys->ppu);
  cpu_deinit(sys->cpu);
  free(sys);
}
