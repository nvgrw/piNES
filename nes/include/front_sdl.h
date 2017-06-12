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
  SDL_Window* window;
  SDL_Renderer* renderer;
  SDL_Texture* ui;
  SDL_Texture* screen_tex;
  SDL_Texture* numbers[10];
  uint32_t palette[64];
  uint32_t screen_pix[PPU_SCREEN_SIZE];

  int32_t mouse_x;
  int32_t mouse_y;
  bool mouse_down;

  // Common data
  front* front;
} front_sdl_impl;

/**
 * Creates an instance of an SDL front.
 */
front_sdl_impl* front_sdl_impl_init();

void front_sdl_impl_run(front_sdl_impl* front);

void front_sdl_impl_flip(front_sdl_impl* front);

void front_sdl_impl_deinit(front_sdl_impl* front);
