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
 * Returns the screen size appropriate for the set region.
 */
uint32_t region_screen_width(region region);
uint32_t region_screen_height(region region);
