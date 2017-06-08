#pragma once

#include <stdbool.h>

#include "cpu.h"
#include "ppu.h"
#include "region.h"
#include "rom.h"

/**
 * sys.h
 * 
 * Struct defining the NES system as a whole.
 */

typedef enum {
  SS_NONE,
  SS_ROM_MISSING = 1,
  SS_ROM_DAMAGED,
  SS_ROM_MAPPER
} sys_status;

typedef struct {
  double clock;
  cpu* cpu;
  ppu* ppu;
  mapper* mapper;
  // apu* apu;
  // controller* ctrl1;
  // controller* ctrl2;
  region region;

  sys_status status;
  bool running;
} sys;

sys* sys_init(void);

void sys_run(sys* sys, uint32_t ms);

void sys_rom(sys* sys, char* path);
void sys_start(sys* sys);
void sys_stop(sys* sys);
void sys_pause(sys* sys);
void sys_step(sys* sys);
void sys_test(sys* sys);

void sys_deinit(sys* sys);
