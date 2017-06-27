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
  SS_CPU_UNSUPPORTED_INSTRUCTION
} sys_status_t;

/**
 * The main system struct, which holds all of the components.
 */
typedef struct {
  double clock;
  controller_t* controller;
  cpu_t* cpu;
  ppu_t* ppu;
  mapper_t* mapper;
  apu_t* apu;
  region region;

  sys_status_t status;
  bool running;
} sys_t;

/**
 * Allocates memory for a system and initialises all of its components.
 */
sys_t* sys_init(void);

/**
 * Advances the clock of the system by the given number of milliseconds.
 * Returns true if the system stopped for any reason.
 */
bool sys_run(sys_t* sys, uint32_t ms, void* context,
             apu_enqueue_audio_t enqueue_audio,
             apu_get_queue_size_t get_queue_size);

/**
 * Loads a ROM with the given path.
 */
sys_status_t sys_rom(sys_t* sys, char* path);

/**
 * Starts or resumes the system.
 */
void sys_start(sys_t* sys);

/**
 * Stops (and resets) the system.
 */
void sys_stop(sys_t* sys);

/**
 * Pauses the system.
 */
void sys_pause(sys_t* sys);

/**
 * Advances the system by one PPU cycle.
 */
void sys_step(sys_t* sys);

/**
 * Runs the tests binary on the system.
 */
void sys_test(sys_t* sys);

/**
 * Frees any dynamic memory allocated for the system.
 */
void sys_deinit(sys_t* sys);
