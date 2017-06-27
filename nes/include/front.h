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
#include <stdint.h>

#include "sys.h"

/**
 * front.h
 *
 * Defines the struct and basic functions to deal with fronts. A front is an
 * abstraction over specific libraries to display image output and play audio
 * output to the user.
 *
 * See front_*.h for front implementations.
 */

/**
 * Tabs that can be shown by the front.
 */
typedef enum {
  FT_SCREEN,
  FT_CPU,
  FT_PPU,
  FT_APU,
  FT_IO,
  FT_MMC_CPU,
  FT_MMC_PPU
} front_tab_t;

/**
 * The front struct, common to all implementations.
 */
typedef struct {
  // System
  sys_t* sys;

  // UI properties
  front_tab_t tab;
  uint8_t scale;
} front_t;

/**
 * Shows a system-dependent dialog to select a ROM file.
 */
char* front_rom_dialog(void);

/**
 * Allocates the memory for a front.
 */
front_t* front_init(sys_t* sys);

/**
 * Frees dynamic memory for a front.
 */
void front_deinit(front_t* front);
