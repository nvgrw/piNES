#pragma once

#include "ppu.h"
#include "region.h"

/**
 * sys.h
 * 
 * Struct defining the NES system as a whole.
 */

typedef struct {
  double clock;
  ppu* ppu;
  region region;
} sys;

sys* sys_init(void);

void sys_run(sys* sys, uint32_t ms);

void sys_deinit(sys* sys);
