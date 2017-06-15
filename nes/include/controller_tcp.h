#pragma once

#include "controller.h"

/**
 * controller_tcp.h
 *
 * Driver for interfacing with physical NES controllers via
 * a TCP connection with the Raspberry Pi and then via the Pi's GPIO
 * pins.
 */

int controller_tcp_init(void);
void controller_tcp_poll(controller_t* ctrl);
void controller_nes_deinit(void);
