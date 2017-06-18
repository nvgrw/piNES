#pragma once

#include <SDL.h>
#include <stdbool.h>

#include "front.h"
#include "front_impl.h"
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

  SDL_AudioDeviceID audio_device;

  // Mouse-related
  int32_t mouse_x;
  int32_t mouse_y;
  bool mouse_down;

  // Display message
  char message[512];
  uint32_t message_ticks;

  // Common data
  front_t* front;
} front_sdl_impl_t;

/**
 * Creates an instance of an SDL front.
 */
front_sdl_impl_t* front_sdl_impl_init();

/**
 * Enters the main loop of the SDL front.
 */
void front_sdl_impl_run(front_sdl_impl_t* impl);

/**
 * Frees any memory allocated with the SDL front, and quits SDL.
 */
void front_sdl_impl_deinit(front_sdl_impl_t* impl);
