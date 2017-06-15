#pragma once

#include <stdbool.h>

#include "apu.h"
#include "apu_typedefs.h"
#include "controller.h"
#include "cpu.h"
#include "ppu.h"
#include "region.h"
#include "rom.h"

/**
 * sys.h
 *
 * Structs defining the NES system as a whole.
 */

/**
 * The system status code, used to indicate errors.
 */
typedef enum {
  // No error
  SS_NONE,

  // ROM loader errors
  SS_ROM_MISSING,
  SS_ROM_DAMAGED,
  SS_ROM_MAPPER,

  // CPU errors
  SS_CPU_TRAPPED,
  SS_CPU_UNSUPPORTED_INSTRUCTION
} sys_status;

/**
 * The main system struct, which holds all of the components.
 */
typedef struct {
  double clock;
  controller_t* controller;
  cpu_t* cpu;
  ppu_t* ppu;
  mapper* mapper;
  apu_t* apu;
  region region;

  sys_status status;
  bool running;
} sys;

/**
 * Allocates memory for a system and initialises all of its components.
 */
sys* sys_init(void);

/**
 * Advances the clock of the system by the given number of milliseconds.
 * Returns true if the system stopped for any reason.
 */
bool sys_run(sys* sys, uint32_t ms, void* context,
             apu_enqueue_audio_t enqueue_audio,
             apu_get_queue_size_t get_queue_size);

/**
 * Loads a ROM with the given path.
 */
void sys_rom(sys* sys, char* path);

/**
 * Starts or resumes the system.
 */
void sys_start(sys* sys);

/**
 * Stops (and resets) the system.
 */
void sys_stop(sys* sys);

/**
 * Pauses the system.
 */
void sys_pause(sys* sys);

/**
 * Advances the system by one PPU cycle.
 */
void sys_step(sys* sys);

/**
 * Runs the tests binary on the system.
 */
void sys_test(sys* sys);

/**
 * Frees any dynamic memory allocated for the system.
 */
void sys_deinit(sys* sys);
