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
