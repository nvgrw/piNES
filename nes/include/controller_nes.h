#pragma once

#include "controller.h"

/**
 * controller_nes.h
 * 
 * The NES controller driver. This interfaces with physical NES controllers
 * via the Raspberry Pi's GPIO pins.
 */

int controller_nes_init(void);
void controller_nes_poll(controller_t* ctrl);
void controller_nes_deinit(void);
