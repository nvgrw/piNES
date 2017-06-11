#pragma once

#include <stdint.h>

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

/**
 * Initialise the controller driver.
 * Don't forget to call controller_deinit when done to clear out the GPIO pins.
 *
 * TODO: This may need further modification to trigger on sigint.
 *
 * Returns EXIT_SUCCESS in the case of success and EXIT_FAILURE if the GPIO
 * coundn't be initalised. Common causes of failure are lack of root access.
 * Root access is required to write to the memory locations for GPIO.
 */
int controller_init(void);

/**
 * Deinitialise the controller driver.
 */
void controller_deinit(void);

void controller_poll(controller_state* cntrl1_state,
                     controller_state* cntrl2_state);

inline bool has_controller_available(void);
