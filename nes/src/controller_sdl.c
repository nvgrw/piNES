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

#include <SDL.h>

#include "controller.h"
#include "controller_sdl.h"
#include "error.h"

/**
 * controller_sdl.c
 */

/**
 * The internal SDL controller state, updated on SDL events. This gets
 * merged into the system controller on polls.
 */
static controller_pressed_t ctrl1_state;
static controller_pressed_t ctrl2_state;

/**
 * Public functions
 */
void controller_sdl_button(SDL_Event event) {
  uint8_t v = (event.key.state == SDL_PRESSED ? 1 : 0);
  switch (event.key.keysym.sym) {
    case SDLK_k:
      ctrl1_state.a = v;
      break;
    case SDLK_j:
      ctrl1_state.b = v;
      break;
    case SDLK_BACKSPACE:
      ctrl1_state.select = v;
      break;
    case SDLK_RETURN:
      ctrl1_state.start = v;
      break;
    case SDLK_UP:  // pass through
    case SDLK_w:
      ctrl1_state.up = v;
      break;
    case SDLK_DOWN:  // pass through
    case SDLK_s:
      ctrl1_state.down = v;
      break;
    case SDLK_LEFT:  // pass through
    case SDLK_a:
      ctrl1_state.left = v;
      break;
    case SDLK_RIGHT:  // pass through
    case SDLK_d:
      ctrl1_state.right = v;
      break;
  }
}

int controller_sdl_init(void) {
  ctrl1_state.a = 0;
  ctrl1_state.b = 0;
  ctrl1_state.select = 0;
  ctrl1_state.start = 0;
  ctrl1_state.up = 0;
  ctrl1_state.down = 0;
  ctrl1_state.left = 0;
  ctrl1_state.right = 0;
  ctrl2_state.a = 0;
  ctrl2_state.b = 0;
  ctrl2_state.select = 0;
  ctrl2_state.start = 0;
  ctrl2_state.up = 0;
  ctrl2_state.down = 0;
  ctrl2_state.left = 0;
  ctrl2_state.right = 0;
  return EC_SUCCESS;
}

void controller_sdl_poll(controller_t* ctrl) {
  ctrl->pressed1.a |= ctrl1_state.a;
  ctrl->pressed1.b |= ctrl1_state.b;
  ctrl->pressed1.select |= ctrl1_state.select;
  ctrl->pressed1.start |= ctrl1_state.start;
  ctrl->pressed1.up |= ctrl1_state.up;
  ctrl->pressed1.down |= ctrl1_state.down;
  ctrl->pressed1.left |= ctrl1_state.left;
  ctrl->pressed1.right |= ctrl1_state.right;
  ctrl->pressed2.a |= ctrl2_state.a;
  ctrl->pressed2.b |= ctrl2_state.b;
  ctrl->pressed2.select |= ctrl2_state.select;
  ctrl->pressed2.start |= ctrl2_state.start;
  ctrl->pressed2.up |= ctrl2_state.up;
  ctrl->pressed2.down |= ctrl2_state.down;
  ctrl->pressed2.left |= ctrl2_state.left;
  ctrl->pressed2.right |= ctrl2_state.right;
}

void controller_sdl_deinit(void) {
  // Do nothing
}
