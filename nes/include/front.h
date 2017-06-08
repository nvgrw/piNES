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
  FT_MMC
} front_tab;

/**
 * The front struct, common to all implementations.
 */
typedef struct {
  // System
  sys* sys;

  // UI properties
  front_tab tab;
  uint8_t scale;
} front;

/**
 * Shows a system-dependent dialog to select a ROM file.
 */
char* front_rom_dialog(void);

front* front_init(sys* sys);

void front_deinit(front* front);
