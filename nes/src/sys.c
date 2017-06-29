/**
 * MIT License
 *
 * Copyright (c) 2017
 * Aurel Bily, Alexis I. Marinoiu, Andrei V. Serbanescu, Niklas Vangerow
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "apu.h"
#include "controller.h"
#include "cpu.h"
#include "ppu.h"
#include "profiler.h"
#include "sys.h"

/**
 * sys.c
 */

sys_t* sys_init(void) {
  sys_t* sys = malloc(sizeof(sys_t));
  sys->clock = 0.0;
  sys->cpu = cpu_init();
  sys->ppu = ppu_init();
  sys->apu = apu_init();
  sys->controller = controller_init();
  sys->mapper = NULL;

  sys->region = R_NTSC;
  sys->status = SS_NONE;
  sys->running = false;
  for (int i = 0; i < NUM_CONTROLLER_DRIVERS; i++) {
    (*CONTROLLER_DRIVERS[i].init)();
  }
  return sys;
}

// TODO: move this to region
#define CLOCKS_PER_MILLISECOND 21477.272
#define CLOCK_PERIOD (12.0 / CLOCKS_PER_MILLISECOND)

static void sys_reset(sys_t* sys);

bool sys_run(sys_t* sys, uint32_t ms, void* context,
             apu_enqueue_audio_t enqueue_audio,
             apu_get_queue_size_t get_queue_size) {
  if (sys->running) {
    PROFILER_POINT(SYS_START)

    sys->clock += ms;
    while (sys->clock >= CLOCK_PERIOD) {
      cpu_nmi(sys->cpu, sys->ppu->nmi);
      if (cpu_cycle(sys->cpu)) {
        // Trapped, stop execution
        switch (sys->cpu->status) {
          case CS_UNSUPPORTED_INSTRUCTION:
            sys->status = SS_CPU_UNSUPPORTED_INSTRUCTION;
            break;
          default:
            break;
        }
        sys->running = false;
        return true;
      }

      PROFILER_POINT(SYS_CPU)

      ppu_cycle(sys->ppu);
      ppu_cycle(sys->ppu);
      ppu_cycle(sys->ppu);

      PROFILER_POINT(SYS_PPU_LOGIC)

      apu_cycle(sys->apu, context, enqueue_audio, get_queue_size);

      // PROFILER_POINT(SYS_APU)

      sys->clock -= CLOCK_PERIOD;
    }

    PROFILER_POINT(SYS_END)

    if (sys->ppu->flip) {
      controller_clear(sys->controller);
      for (int i = 0; i < NUM_CONTROLLER_DRIVERS; i++) {
        (*CONTROLLER_DRIVERS[i].poll)(sys->controller);
      }

      if (*((uint8_t*)&sys->controller->pressed1) == 0xFF) {
        sys_stop(sys);
        sys_start(sys);
      }
    }
  }
  return false;
}

static void sys_reset(sys_t* sys) { cpu_reset(sys->cpu); }

sys_status_t sys_rom(sys_t* sys, char* path) {
  if (sys->mapper != NULL) {
    rom_destroy(sys->mapper);
    sys->mapper = NULL;
  }

  rom_error_t error = rom_load(&sys->mapper, path);
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
    sys->apu->mapper = sys->mapper;
    sys->mapper->cpu = sys->cpu;
    sys->mapper->ppu = sys->ppu;
    sys->mapper->apu = sys->apu;
    sys->mapper->controller = sys->controller;
    sys_reset(sys);
  }
  return sys->status;
}

void sys_start(sys_t* sys) {
  if (sys->mapper == NULL) {
    sys->status = SS_ROM_MISSING;
    return;
  }
  sys->running = true;
}

void sys_stop(sys_t* sys) {
  sys->running = false;
  sys_reset(sys);
}

void sys_pause(sys_t* sys) { sys->running = false; }

void sys_step(sys_t* sys) {
  if (sys->mapper == NULL) {
    sys->status = SS_ROM_MISSING;
    return;
  }
  // stub
}

void sys_test(sys_t* sys) {
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

void sys_deinit(sys_t* sys) {
  for (int i = 0; i < NUM_CONTROLLER_DRIVERS; i++) {
    (*CONTROLLER_DRIVERS[i].deinit)();
  }
  controller_deinit(sys->controller);
  ppu_deinit(sys->ppu);
  cpu_deinit(sys->cpu);
  free(sys);
}
