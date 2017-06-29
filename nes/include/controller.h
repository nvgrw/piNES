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

#include <stdint.h>

/**
 * controller.h
 *
 * Structs and functions for dealing with the standard NES controller devices,
 * and "drivers" to emulate them.
 */

#if defined(IS_PI) && defined(TCP_HOST)
#define NUM_CONTROLLER_DRIVERS 3
#elif defined(IS_PI) || defined(TCP_HOST)
#define NUM_CONTROLLER_DRIVERS 2
#else
#define NUM_CONTROLLER_DRIVERS 1
#endif

/**
 * This struct keeps track of which buttons are currently being pressed on
 * a controller.
 */
typedef struct __attribute__((packed)) {
  uint8_t a : 1;
  uint8_t b : 1;
  uint8_t select : 1;
  uint8_t start : 1;
  uint8_t up : 1;
  uint8_t down : 1;
  uint8_t left : 1;
  uint8_t right : 1;
} controller_pressed_t;

/**
 * The status of a controller register (0x4016 and 0x4017). Games have to
 * write a sequence of 1 (STROBE) followed by 0 (PULL) to 0x4016 in order
 * to start the sequential data polling from a controller.
 */
typedef enum {
  CTRLS_NONE,
  CTRLS_STROBE,
  CTRLS_PULL_A,
  CTRLS_PULL_B,
  CTRLS_PULL_SELECT,
  CTRLS_PULL_START,
  CTRLS_PULL_UP,
  CTRLS_PULL_DOWN,
  CTRLS_PULL_LEFT,
  CTRLS_PULL_RIGHT,
  CTRLS_PULLED
} controller_status_t;

/**
 * This holds the status and pressed buttons data for both of the controllers.
 */
typedef struct controller {
  controller_status_t status1;
  controller_pressed_t pressed1;
  controller_status_t status2;
  controller_pressed_t pressed2;
} controller_t;

/**
 * A controller "driver" is a (platform-)specific implementation which allows
 * the emulator to read controller inputs from a given data source.
 */
typedef struct {
  /**
   * Initialises the controller driver.
   *
   * Returns EC_SUCCESS in the case of success and EC_FAILURE if the driver
   * coundn't be initalised. Common causes of failure are lack of root access
   * (for the GPIO driver).
   */
  int (*init)(void);

  /**
   * Poll the driver and update the data in the struct.
   */
  void (*poll)(controller_t* ctrl);

  /**
   * Deinitialise the controller driver.
   */
  void (*deinit)(void);
} controller_driver_t;

/**
 * Allocate memory for a controller struct.
 */
controller_t* controller_init(void);

/**
 * Zeroes out the pressed buttons of the controllers.
 */
void controller_clear(controller_t* ctrl);

/**
 * Free allocated memory for the controller.
 */
void controller_deinit(controller_t* ctrl);

/**
 * I/O registers, accessible by the CPU.
 */
void controller_mem_write(controller_t* ctrl, uint16_t address, uint8_t value);
uint8_t controller_mem_read(controller_t* ctrl, uint16_t address);

/**
 * Active controller drivers.
 */
extern const controller_driver_t CONTROLLER_DRIVERS[NUM_CONTROLLER_DRIVERS];
