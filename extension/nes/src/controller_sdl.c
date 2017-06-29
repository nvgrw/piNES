#include <SDL.h>
#include <stdbool.h>

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
static bool should_reset = false;

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
    case SDLK_SPACE:
      if (v) {
        should_reset = true;
      }
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
  if (should_reset) {
    (*((uint8_t*)&ctrl->pressed1)) = 0xFF;
    should_reset = false;
    return;
  }

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
