#pragma once

#include <stdint.h>

/**
 * region.h
 * 
 * Region-dependent functions.
 */

/**
 * Regions available. Only NTSC and PAL are official.
 */
typedef enum {
  R_NTSC,
  R_PAL,
  R_DENDY,
  R_RGB3,
  R_RGB4,
  R_RGB5
} region;

/**
 * Set / get the current region. Used to make region accessible in any file.
 */
void region_set_current(region region);
region region_get_current(void);

/**
 * Returns the screen size appropriate for the set region.
 */
uint32_t region_screen_width();
uint32_t region_screen_height();
