#include <stdio.h>
#include <stdlib.h>
#include <nfd.h>
#include <SDL.h>
#include <SDL_image.h>

#include "error.h"
#include "front.h"
#include "front_sdl.h"
#include "region.h"

/**
 * front_sdl.c
 */

char* front_sdl_name = "SDL";

/**
 * Helper functions
 */
void front_sdl_run(front* front) {
  int done = 0;
  SDL_Event event;
  front_sdl_impl* impl = front->impl;

  // Enter render loop, waiting for user to quit
  while (!done) {
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_QUIT:
          done = 1;
          break;
      }
    }
    SDL_RenderClear(impl->renderer);
    SDL_RenderCopy(impl->renderer, impl->ui, NULL, NULL);
    SDL_RenderPresent(impl->renderer);
    SDL_Delay(20);
  }
}

void front_sdl_deinit(front* front) {
  SDL_Quit();
  free(front->impl);
}

/**
 * Public functions
 * 
 * See front_sdl.h for descriptions.
 */
int front_sdl_init(front* front) {
  // Initialise SDL and SDL_image
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    fprintf(stderr, "Could not initialise SDL\n");
    return EC_ERROR;
  }
  if ((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) != IMG_INIT_PNG) {
    fprintf(stderr, "Could not initialise SDL_image\n");
    return EC_ERROR;
  }

  // Load UI image
  SDL_Surface* ui = IMG_Load("assets/pines.png");
  if (!ui) {
    fprintf(stderr, "Could not load pines.png\n");
    return EC_ERROR;
  }

  front_sdl_impl* impl = calloc(1, sizeof(front_sdl_impl));

  // Create window and renderer
  uint32_t width = region_screen_width() * front->scale;
  uint32_t height = region_screen_height() * front->scale;
  impl->window = SDL_CreateWindow("pines", SDL_WINDOWPOS_CENTERED,
                                  SDL_WINDOWPOS_CENTERED, width, height, 0);
  if (!impl->window) {
    fprintf(stderr, "Could not create window\n");
    SDL_FreeSurface(ui);
    free(impl);
    return EC_ERROR;
  }

  impl->renderer = SDL_CreateRenderer(impl->window, -1, 0);
  if (!impl->renderer) {
    fprintf(stderr, "Could not create renderer\n");
    SDL_FreeSurface(ui);
    free(impl);
    return EC_ERROR;
  }

  impl->ui = SDL_CreateTextureFromSurface(impl->renderer, ui);
  SDL_FreeSurface(ui);
  if (!impl->ui) {
    fprintf(stderr, "Could not create texture\n");
    free(impl);
    return EC_ERROR;
  }

  // Fill screen with black
  SDL_SetRenderDrawColor(impl->renderer, 0, 0, 0, 255);
  SDL_RenderClear(impl->renderer);

  front->name = front_sdl_name;
  front->impl = impl;
  front->run = &front_sdl_run;
  front->deinit = &front_sdl_deinit;
  return EC_SUCCESS;
}
