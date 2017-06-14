#pragma once

#include <SDL.h>

#include "controller.h"

/**
 * controller_sdl.h
 * 
 * The SDL controller driver. The SDL front forwards key down and key up
 * events via controller_sdl_button.
 */

void controller_sdl_button(SDL_Event event);
int controller_sdl_init(void);
void controller_sdl_poll(controller_t* ctrl);
void controller_sdl_deinit(void);
