#pragma once

#include <stdbool.h>

#include "cpu.h"
#include "ppu.h"
#include "region.h"

/**
 * sys.h
 * 
 * Struct defining the NES system as a whole.
 */

typedef struct {
  double clock;
  cpu* cpu;
  ppu* ppu;
  // apu* apu;
  // controller* ctrl1;
  // controller* ctrl2;
  region region;

  bool running;
} sys;

sys* sys_init(void);

void sys_run(sys* sys, uint32_t ms);

void sys_test(sys* sys);

void sys_deinit(sys* sys);
