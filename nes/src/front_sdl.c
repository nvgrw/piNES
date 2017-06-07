#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL.h>
#include <SDL_image.h>

#include "error.h"
#include "front.h"
#include "front_sdl.h"
#include "region.h"
#include "ppu.h"
#include "sys.h"

#define dbg(X) printf(X "\n");

#define BUTTON_LOAD 0
#define BUTTON_RUN 1
#define BUTTON_STOP 2
#define BUTTON_PAUSE 3
#define BUTTON_STEP 4
#define BUTTON_CPU 5
#define BUTTON_PPU 6
#define BUTTON_APU 7
#define BUTTON_IO 8
#define BUTTON_TEST 9

#define BUTTON_NUM 10

/**
 * front_sdl.c
 */

/**
 * Helper functions
 */
void front_sdl_impl_deinit(front_sdl_impl* impl) {
  SDL_Quit();
  free(impl);
}

void front_sdl_impl_run(front_sdl_impl* impl) {
  bool running = true;
  uint32_t last_tick = SDL_GetTicks();
  sys* sys = impl->front->sys;

  // Enter render loop, waiting for user to quit
  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_QUIT:
          running = false;
          break;
        case SDL_MOUSEMOTION:
          impl->mouse_x = event.motion.x;
          impl->mouse_y = event.motion.y;
          break;
        case SDL_MOUSEBUTTONDOWN:
          impl->mouse_down = true;
          break;
        case SDL_MOUSEBUTTONUP:
          impl->mouse_down = false;
          if (impl->mouse_x >= 0 && impl->mouse_x < 10 * 16 + 16 &&
              impl->mouse_y >= 0 && impl->mouse_y < 16) {
            switch (impl->mouse_x / 16) {
              case BUTTON_LOAD: {
                char* path = front_rom_dialog();
                if (path != NULL) {
                  printf("path: %s\n", path);
                  free(path);
                }
              } break;
              case BUTTON_RUN:
                sys->running = true;
                break;
              case BUTTON_STOP:
                sys->running = false;
                //sys_reset(sys);
                break;
              case BUTTON_PAUSE:
                sys->running = false;
                break;
              case BUTTON_STEP:
                break;
              case BUTTON_TEST:
                sys_test(sys);
                break;
            }
          }
          break;
      }
    }
    uint32_t this_tick = SDL_GetTicks();
    sys_run(sys, this_tick - last_tick);
    if (sys->ppu->flip) {
      void* pixels;
      uint32_t pitch;
      if (SDL_LockTexture(impl->screen_tex, NULL, &pixels, (void*)&pitch)) {
        //printf("err: %s\n", SDL_GetError());
      } else {
        for (int i = 0; i < PPU_SCREEN_SIZE; i++) {
          impl->screen_pix[i] = impl->palette[sys->ppu->screen[i]];
        }
        memcpy(pixels, impl->screen_pix, PPU_SCREEN_SIZE_BYTES);
        SDL_UnlockTexture(impl->screen_tex);
        SDL_RenderCopy(impl->renderer, impl->screen_tex, NULL, NULL);
        front_sdl_impl_flip(impl);
      }
      sys->ppu->flip = false;
    } else if (!sys->running) {
      front_sdl_impl_flip(impl);
    }
    last_tick = this_tick;
    SDL_Delay(10);
  }
}

void front_sdl_impl_flip(front_sdl_impl* impl) {
  // Render the UI
  SDL_Rect src;
  src.x = 0;
  src.y = 32;
  src.w = 16;
  src.h = 16;
  SDL_Rect dest;
  dest.x = 0;
  dest.y = 0;
  dest.w = 16;
  dest.h = 16;
  if (impl->mouse_y >= 0 && impl->mouse_y < 16) {
    // Button backgrounds
    bool sel = false;
    for (int i = 0; i < 10; i++) {
      sel = impl->mouse_x >= i * 16 && impl->mouse_x < i * 16 + 16;
      if (sel && impl->mouse_down) {
        src.x = 16;
      }
      SDL_RenderCopy(impl->renderer, impl->ui, &src, &dest);
      if (sel) {
        src.x = 32;
        SDL_RenderCopy(impl->renderer, impl->ui, &src, &dest);
      }
      src.x = 0;
      dest.x += 16;
    }

    // Button icons
    src.x = 0;
    src.y = 48;
    src.w = 10 * 16;
    dest.x = 0;
    dest.w = 10 * 16;
    SDL_RenderCopy(impl->renderer, impl->ui, &src, &dest);
  }
  SDL_RenderPresent(impl->renderer);
  SDL_RenderClear(impl->renderer);
}

/**
 * Public functions
 * 
 * See front_sdl.h for descriptions.
 */
front_sdl_impl* front_sdl_impl_init(front* front) {
  // Initialise SDL and SDL_image
  if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
    fprintf(stderr, "Could not initialise SDL\n");
    return NULL;
  }
  if ((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) != IMG_INIT_PNG) {
    fprintf(stderr, "Could not initialise SDL_image\n");
    return NULL;
  }

  // Load UI image
  SDL_Surface* ui = IMG_Load("assets/pines.png");
  if (!ui) {
    fprintf(stderr, "Could not load pines.png\n");
    return NULL;
  }

  // Initialise front implementation struct
  front_sdl_impl* impl = malloc(sizeof(front_sdl_impl));
  impl->mouse_x = -1;
  impl->mouse_y = -1;

  // Initialise palette
  for (int i = 0; i < 32; i++) {
    impl->palette[i] = 0xFF000000 | ((uint32_t*)ui->pixels)[(i % 16) * 8 + (i >> 8) * ui->w];
  }

  // Create window
  uint32_t width = region_screen_width(front->sys->region) * front->scale;
  uint32_t height = region_screen_height(front->sys->region) * front->scale;
  impl->window = SDL_CreateWindow("pines", SDL_WINDOWPOS_CENTERED,
                                  SDL_WINDOWPOS_CENTERED, width, height,
                                  SDL_WINDOW_OPENGL);
  if (!impl->window) {
    fprintf(stderr, "Could not create window\n");
    SDL_FreeSurface(ui);
    free(impl);
    return NULL;
  }

  // Create renderer
  impl->renderer = SDL_CreateRenderer(impl->window, -1,
                                      SDL_RENDERER_ACCELERATED |
                                      SDL_RENDERER_TARGETTEXTURE);
  if (!impl->renderer) {
    fprintf(stderr, "Could not create renderer\n");
    SDL_FreeSurface(ui);
    free(impl);
    return NULL;
  }

  // Create UI texture
  impl->ui = SDL_CreateTextureFromSurface(impl->renderer, ui);
  SDL_FreeSurface(ui);
  if (!impl->ui) {
    fprintf(stderr, "Could not create UI texture\n");
    free(impl);
    return NULL;
  }

  // Create surface for main screen
  impl->screen_tex = SDL_CreateTexture(impl->renderer, SDL_PIXELFORMAT_ARGB8888,
                                       SDL_TEXTUREACCESS_STREAMING, 256, 240);
  if (!impl->screen_tex) {
    fprintf(stderr, "Could not create screen texture\n");
    SDL_DestroyTexture(impl->ui);
    free(impl);
    return NULL;
  }

  // Fill screen with black
  SDL_SetRenderDrawColor(impl->renderer, 0, 0, 0, 255);
  SDL_RenderClear(impl->renderer);

  impl->front = front;
  return impl;
}
