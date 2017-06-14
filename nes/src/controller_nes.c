#include <stdbool.h>
#include <stdlib.h>

#ifdef IS_PI
#include <pigpio.h>
#endif

#include "controller.h"
#include "controller_nes.h"
#include "error.h"

/**
 * controller_nes.c
 */

/**
 * GPIO pin numbers connected to the NES controllers.
 */
#define PIN_CTRL1_DATA 17
#define PIN_CTRL2_DATA 22
#define PIN_LATCH 18
#define PIN_PULSE 27

#define LATCH_DURATION 12   // us (microseconds)
#define PULSE_HALF_CYCLE 6  // us
// #define LATCH_WAIT 1667 - LATCH_DURATION - PULSE_HALF_CYCLE * 8  // us

#ifdef IS_PI
static void controller_pulse(void) {
  gpioWrite(PIN_PULSE, 1);
  gpioDelay(PULSE_HALF_CYCLE);
  gpioWrite(PIN_PULSE, 0);
}
#endif

/**
 * Public functions
 */
int controller_nes_init(void) {
 // TODO: This may need further modification to trigger on sigint.
#ifdef IS_PI
  if (gpioInitialise() < 0) {
    return EXIT_FAILURE;
  }

  gpioSetMode(PIN_CTRL1_DATA, PI_INPUT);
  gpioSetMode(PIN_CTRL2_DATA, PI_INPUT);

  gpioSetMode(PIN_LATCH, PI_OUTPUT);
  gpioSetMode(PIN_PULSE, PI_OUTPUT);
#endif
  return EXIT_SUCCESS;
}

void controller_nes_poll(controller_t* ctrl) {
#ifdef IS_PI
  // Set latch for 12us
  gpioWrite(PIN_LATCH, 1);
  gpioDelay(LATCH_DURATION);
  gpioWrite(PIN_LATCH, 0);

  // Poll A
  ctrl->pressed1.a |= !gpioRead(PIN_CTRL1_DATA);
  ctrl->pressed2.a |= !gpioRead(22);
  gpioDelay(PULSE_HALF_CYCLE);

  // Poll B
  controller_pulse();
  ctrl->pressed1.b |= !gpioRead(PIN_CTRL1_DATA);
  ctrl->pressed2.b |= !gpioRead(PIN_CTRL2_DATA);
  gpioDelay(PULSE_HALF_CYCLE);

  // Poll SELECT
  controller_pulse();
  ctrl->pressed1.select |= !gpioRead(PIN_CTRL1_DATA);
  ctrl->pressed2.select |= !gpioRead(PIN_CTRL2_DATA);
  gpioDelay(PULSE_HALF_CYCLE);

  // Poll START
  controller_pulse();
  ctrl->pressed1.start |= !gpioRead(PIN_CTRL1_DATA);
  ctrl->pressed2.start |= !gpioRead(PIN_CTRL2_DATA);
  gpioDelay(PULSE_HALF_CYCLE);

  // Poll UP
  controller_pulse();
  ctrl->pressed1.up |= !gpioRead(PIN_CTRL1_DATA);
  ctrl->pressed2.up |= !gpioRead(PIN_CTRL2_DATA);
  gpioDelay(PULSE_HALF_CYCLE);

  // Poll DOWN
  controller_pulse();
  ctrl->pressed1.down |= !gpioRead(PIN_CTRL1_DATA);
  ctrl->pressed2.down |= !gpioRead(PIN_CTRL2_DATA);
  gpioDelay(PULSE_HALF_CYCLE);

  // Poll LEFT
  controller_pulse();
  ctrl->pressed1.left |= !gpioRead(PIN_CTRL1_DATA);
  ctrl->pressed2.left |= !gpioRead(PIN_CTRL2_DATA);
  gpioDelay(PULSE_HALF_CYCLE);

  // Poll RIGHT
  controller_pulse();
  ctrl->pressed1.right |= !gpioRead(PIN_CTRL1_DATA);
  ctrl->pressed2.right |= !gpioRead(PIN_CTRL2_DATA);
  gpioDelay(PULSE_HALF_CYCLE);
#endif
}

void controller_nes_deinit(void) {
#ifdef IS_PI
  gpioTerminate();
#endif
}
