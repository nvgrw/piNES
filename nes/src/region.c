#include "region.h"

/**
 * region.c
 */

static region region_current = R_NTSC;

void region_set_current(region region) {
  region_current = region;
}

region region_get_current(void) {
  return region_current;
}

uint32_t region_screen_width(void) {
  return 256;
}

uint32_t region_screen_height(void) {
  switch (region_current) {
    case R_NTSC:
    case R_RGB3:
    case R_RGB4:
    case R_RGB5:
      return 240;
    case R_PAL:
    case R_DENDY:
      return 239;
  }
}
