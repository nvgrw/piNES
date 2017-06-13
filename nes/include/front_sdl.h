#pragma once

#include <stdbool.h>
#include <SDL.h>

#include "front_impl.h"
#include "front.h"
#include "ppu.h"

/**
 * front_sdl.h
 * 
 * SDL front.
 */

/**
 * SDL-specific front data.
 */
typedef struct {
  // SDL and video-related
  SDL_Window* window;
  SDL_Renderer* renderer;
  SDL_Texture* ui;
  SDL_Texture* screen_tex;
  SDL_Texture* prescaled_tex;

  // The palette, stored in ARGB8888 format
  uint32_t palette[64];

  // Mouse-related
  int32_t mouse_x;
  int32_t mouse_y;
  bool mouse_down;

  // Display message
  char message[512];
  uint32_t message_ticks;

  // Common data
  front* front;
} front_sdl_impl;

/**
 * Creates an instance of an SDL front.
 */
front_sdl_impl* front_sdl_impl_init();

/**
 * Enters the main loop of the SDL front.
 */
void front_sdl_impl_run(front_sdl_impl* front);

/**
 * Frees any memory allocated with the SDL front, and quits SDL.
 */
void front_sdl_impl_deinit(front_sdl_impl* front);
