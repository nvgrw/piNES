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
  ret->mapper = NULL;
  ret->region = R_NTSC;
  ret->status = SS_NONE;
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
    while (sys->clock >= CLOCK_PERIOD && sys->running) {
      cpu_nmi(sys->cpu, sys->ppu->nmi);
      if (cpu_cycle(sys->cpu)) {
        // Trapped, stop execution
        sys->running = false;
      }

      ppu_cycle(sys->ppu);

      // apu cycle

      sys->clock -= CLOCK_PERIOD;
    }
  }
}

void sys_rom(sys* sys, char* path) {
  rom_error error = rom_load(&sys->mapper, path);
  switch (error) {
    case RE_SUCCESS:
      sys->status = SS_NONE;
      break;
    case RE_READ_ERROR:
    case RE_INVALID_FILE_FORMAT:
    case RE_PRG_READ_ERROR:
    case RE_CHR_READ_ERROR:
      sys->status = SS_ROM_DAMAGED;
      sys->mapper = NULL;
      break;
    case RE_UNKNOWN_MAPPER:
      sys->status = SS_ROM_MAPPER;
      sys->mapper = NULL;
      break;
  }
  if (sys->mapper != NULL) {
    sys->cpu->mapper = sys->mapper;
    sys->ppu->mapper = sys->mapper;
    sys->mapper->cpu = sys->cpu;
    sys->mapper->ppu = sys->ppu;
  }
}

void sys_start(sys* sys) {
  if (sys->mapper == NULL) {
    sys->status = SS_ROM_MISSING;
    return;
  }
  cpu_reset(sys->cpu);
  sys->running = true;
}

void sys_stop(sys* sys) {
  sys->running = false;
  // also reset
}

void sys_pause(sys* sys) {
  sys->running = false;
}

void sys_step(sys* sys) {
  if (sys->mapper == NULL) {
    sys->status = SS_ROM_MISSING;
    return;
  }
  // stub
}

void sys_test(sys* sys) {
  // TODO: make this work again
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