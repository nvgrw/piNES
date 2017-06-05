#pragma once

#include "SDL.h"

#include "front.h"

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
} front_sdl_impl;

/**
 * Creates an instance of an SDL front.
 */
int front_sdl_init(front* front);
