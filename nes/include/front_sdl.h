/**
 * MIT License
 *
 * Copyright (c) 2017
 * Aurel Bily, Alexis I. Marinoiu, Andrei V. Serbanescu, Niklas Vangerow
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

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
  // SDL and video
  SDL_Window* window;
  SDL_Renderer* renderer;
  SDL_Texture* ui;
  SDL_Texture* screen_tex;
  SDL_Texture* prescaled_tex;

  // Fullscreen
  SDL_Rect* screen_rect;
  bool full;

  // Audio
  SDL_AudioDeviceID audio_device;

  // Mouse
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
