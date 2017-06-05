#pragma once

#include <stdbool.h>
#include <stdint.h>

/**
 * front.h
 * 
 * Defines the struct and basic functions to deal with fronts. A front is an
 * abstraction over specific libraries to display image output and play audio
 * output to the user.
 * 
 * See front_*.h for front implementations.
 */

/**
 * Tabs that can be shown by the front.
 */
typedef enum {
  FT_SCREEN,
  FT_CPU
} front_tab;

/**
 * The front struct, common to all implementations.
 */
typedef struct front {
  // UI properties
  front_tab tab;
  uint8_t scale;

  // Implementation metadata
  char* name;

  // Implementation-specific
  void (*run)(void*);
  void (*deinit)(void*);
  void* impl;
} front;

/**
 * Shows a system-dependent dialog to select a ROM file.
 */
int front_rom_dialog(char* path);

front* front_init(void);

/**
 * Helper functions to call implementation-specific functions.
 */
void front_run(front* front);
void front_deinit(front* front);
