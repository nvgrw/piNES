#include "region.h"

/**
 * region.c
 */

uint32_t region_screen_width(region region) {
  return 256;
}

uint32_t region_screen_height(region region) {
  switch (region) {
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
