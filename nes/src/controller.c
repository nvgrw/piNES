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

controller_state poll_controller(void) {
  controller_state state;
  
  // Set latch for 12us
  gpioWrite(18, 1);
  gpioDelay(LATCH_DURATION);
  gpioWrite(18, 0);
  
  // Poll A
  state.a = !gpioRead(17);
  gpioDelay(PULSE_HALF_CYCLE);

  // Poll B
  pulse();
  state.b = !gpioRead(17);
  gpioDelay(PULSE_HALF_CYCLE);

  // Poll SELECT
  pulse();
  state.select = !gpioRead(17);
  gpioDelay(PULSE_HALF_CYCLE);

  // Poll START
  pulse();
  state.start = !gpioRead(17);
  gpioDelay(PULSE_HALF_CYCLE);

  // Poll UP
  pulse();
  state.up = !gpioRead(17);
  gpioDelay(PULSE_HALF_CYCLE);

  // Poll DOWN
  pulse();
  state.down = !gpioRead(17);
  gpioDelay(PULSE_HALF_CYCLE);

  // Poll LEFT
  pulse();
  state.left = !gpioRead(17);
  gpioDelay(PULSE_HALF_CYCLE);

  // Poll RIGHT
  pulse();
  state.right = !gpioRead(17);
  gpioDelay(PULSE_HALF_CYCLE);
  
  return state;
}

void init_controller(void) {
  if (gpioInitialise() < 0) exit(1); 
 
  // Pin 17: data
  gpioSetMode(17, PI_INPUT);
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
    controller_state state = poll_controller();
    printf("A: %d, B: %d, SELECT: %d, START: %d, UP: %d, DOWN: %d, LEFT: %d,\
 RIGHT: %d\n", state.a, state.b, state.select, state.start, state.up,
    state.down, state.left, state.right);
 
    gpioDelay(LATCH_WAIT);

    // system("clear");
  }
  deinit_controller();
  return EXIT_SUCCESS;
}


