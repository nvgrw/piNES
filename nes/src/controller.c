#include <stdio.h>
#include <stdint.h>
#include <pigpio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <signal.h>
#define LATCH_DURATION 12 // us
#define PULSE_HALF_CYCLE 6 // us
#define LATCH_WAIT 1667 - LATCH_DURATION - PULSE_HALF_CYCLE * 8 // us
static volatile bool running = true;

void sigIntHandler(int a) {
  running = false;
}

void pulse() {
  gpioWrite(27, 1);
  gpioDelay(PULSE_HALF_CYCLE);
  gpioWrite(27, 0);
}

typedef struct {
  uint8_t a : 1;
  uint8_t b : 1;
  uint8_t select : 1;
  uint8_t start : 1;
  uint8_t up : 1;
  uint8_t down : 1;
  uint8_t left : 1;
  uint8_t right : 1;
} controller_state;

void poll_controllers(controller_state* cntrl_1_state, controller_state* cntrl_2_state) {
  
  // Set latch for 12us
  gpioWrite(18, 1);
  gpioDelay(LATCH_DURATION);
  gpioWrite(18, 0);
  
  // Poll A
  cntrl_1_state->a = !gpioRead(17); 
  cntrl_2_state->a = !gpioRead(22); 
  gpioDelay(PULSE_HALF_CYCLE);

  // Poll B
  pulse();
  cntrl_1_state->b = !gpioRead(17);
  cntrl_2_state->b = !gpioRead(22); 
  gpioDelay(PULSE_HALF_CYCLE);

  // Poll SELECT
  pulse();
  cntrl_1_state->select = !gpioRead(17);
  cntrl_2_state->select = !gpioRead(22); 
  gpioDelay(PULSE_HALF_CYCLE);

  // Poll START
  pulse();
  cntrl_1_state->start = !gpioRead(17);
  cntrl_2_state->start = !gpioRead(22); 
  gpioDelay(PULSE_HALF_CYCLE);

  // Poll UP
  pulse();
  cntrl_1_state->up = !gpioRead(17);
  cntrl_2_state->up = !gpioRead(22); 
  gpioDelay(PULSE_HALF_CYCLE);

  // Poll DOWN
  pulse();
  cntrl_1_state->down = !gpioRead(17);
  cntrl_2_state->down = !gpioRead(22); 
  gpioDelay(PULSE_HALF_CYCLE);

  // Poll LEFT
  pulse();
  cntrl_1_state->left = !gpioRead(17);
  cntrl_2_state->left = !gpioRead(22); 
  gpioDelay(PULSE_HALF_CYCLE);

  // Poll RIGHT
  pulse();
  cntrl_1_state->right = !gpioRead(17);
  cntrl_2_state->right = !gpioRead(22); 
  gpioDelay(PULSE_HALF_CYCLE);
}

void init_controller(void) {
  if (gpioInitialise() < 0) exit(1); 
 
  // Pin 17: cntrl_1 data
  gpioSetMode(17, PI_INPUT);
  // Pin 22: cntrl_2 data
  gpioSetMode(22, PI_INPUT);

  // Pin 18: latch
  gpioSetMode(18, PI_OUTPUT);
  // Pin 27: pulse
  gpioSetMode(27, PI_OUTPUT);
}

void deinit_controller(void) {
  gpioTerminate();
}

int main(int argc, char** argv) {
  init_controller();
  gpioSetSignalFunc(SIGINT, sigIntHandler);

  while (running) {
    controller_state cntrl_1_state;
    controller_state cntrl_2_state;
    poll_controllers(&cntrl_1_state, &cntrl_2_state);

    printf("CONTROLLER 1: A %d, B %d, SELECT %d, START %d, UP %d, DOWN %d, LEFT %d,\
 RIGHT %d  ", cntrl_1_state.a, cntrl_1_state.b, cntrl_1_state.select, cntrl_1_state.start, cntrl_1_state.up,
    cntrl_1_state.down, cntrl_1_state.left, cntrl_1_state.right);

    printf("CONTROLLER 2: A %d, B %d, SELECT %d, START %d, UP %d, DOWN %d, LEFT %d,\
 RIGHT %d \n", cntrl_2_state.a, cntrl_2_state.b, cntrl_2_state.select, cntrl_2_state.start, cntrl_2_state.up,
    cntrl_2_state.down, cntrl_2_state.left, cntrl_2_state.right);
 
    gpioDelay(LATCH_WAIT);

    // system("clear");
  }
   deinit_controller();
  return EXIT_SUCCESS;
}


