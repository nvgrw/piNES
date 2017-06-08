#include <pigpio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "controller.h"

#define LATCH_DURATION 12   // us
#define PULSE_HALF_CYCLE 6  // us
// #define LATCH_WAIT 1667 - LATCH_DURATION - PULSE_HALF_CYCLE * 8  // us

int controller_init(void) {
  if (gpioInitialise() < 0) {
    return EXIT_FAILURE;
  }

  // Pin 17: cntrl_1 data
  gpioSetMode(17, PI_INPUT);
  // Pin 22: cntrl_2 data
  gpioSetMode(22, PI_INPUT);

  // Pin 18: latch
  gpioSetMode(18, PI_OUTPUT);
  // Pin 27: pulse
  gpioSetMode(27, PI_OUTPUT);

  return EXIT_SUCCESS;
}

void deinit_controller(void) { gpioTerminate(); }

void controller_pulse(void) {
  gpioWrite(27, 1);
  gpioDelay(PULSE_HALF_CYCLE);
  gpioWrite(27, 0);
}

void controller_poll(controller_state* cntrl1_state,
                     controller_state* cntrl2_state) {
  // Set latch for 12us
  gpioWrite(18, 1);
  gpioDelay(LATCH_DURATION);
  gpioWrite(18, 0);

  // Poll A
  cntrl_1_state->a = !gpioRead(17);
  cntrl_2_state->a = !gpioRead(22);
  gpioDelay(PULSE_HALF_CYCLE);

  // Poll B
  controller_pulse();
  cntrl_1_state->b = !gpioRead(17);
  cntrl_2_state->b = !gpioRead(22);
  gpioDelay(PULSE_HALF_CYCLE);

  // Poll SELECT
  controller_pulse();
  cntrl_1_state->select = !gpioRead(17);
  cntrl_2_state->select = !gpioRead(22);
  gpioDelay(PULSE_HALF_CYCLE);

  // Poll START
  controller_pulse();
  cntrl_1_state->start = !gpioRead(17);
  cntrl_2_state->start = !gpioRead(22);
  gpioDelay(PULSE_HALF_CYCLE);

  // Poll UP
  controller_pulse();
  cntrl_1_state->up = !gpioRead(17);
  cntrl_2_state->up = !gpioRead(22);
  gpioDelay(PULSE_HALF_CYCLE);

  // Poll DOWN
  controller_pulse();
  cntrl_1_state->down = !gpioRead(17);
  cntrl_2_state->down = !gpioRead(22);
  gpioDelay(PULSE_HALF_CYCLE);

  // Poll LEFT
  controller_pulse();
  cntrl_1_state->left = !gpioRead(17);
  cntrl_2_state->left = !gpioRead(22);
  gpioDelay(PULSE_HALF_CYCLE);

  // Poll RIGHT
  controller_pulse();
  cntrl_1_state->right = !gpioRead(17);
  cntrl_2_state->right = !gpioRead(22);
  gpioDelay(PULSE_HALF_CYCLE);
}
