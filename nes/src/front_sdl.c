#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL.h>
#include <SDL_image.h>

#include "controller_sdl.h"
#include "error.h"
#include "front.h"
#include "front_sdl.h"
#include "region.h"
#include "ppu.h"
#include "sys.h"

/**
 * front_sdl.c
 */

#define BUTTON_LOAD 0
#define BUTTON_START 1
#define BUTTON_STOP 2
#define BUTTON_PAUSE 3
#define BUTTON_STEP 4
#define BUTTON_CPU 5
#define BUTTON_PPU 6
#define BUTTON_APU 7
#define BUTTON_IO 8
#define BUTTON_MMC_CPU 9
#define BUTTON_MMC_PPU 10
#define BUTTON_TEST 11

#define BUTTON_NUM 12

/**
 * Helper functions
 * 
 * display_number
 *   Displays a number on the UI at the given location.
 * 
 * flip
 *   Updates the window with the current screen data, and draws any UI
 *   on top of the screen as necessary.
 */
static void display_number(front_sdl_impl* impl, uint32_t num, uint16_t x, uint16_t y) {
  SDL_Rect src = {.x = 48, .y = 32, .w = 8, .h = 8};
  SDL_Rect dest = {.x = x, .y = y, .w = 8, .h = 8};
  if (!num) {
    SDL_RenderCopy(impl->renderer, impl->ui, &src, &dest);
    return;
  }
  while (num) {
    src.x = 48 + (num % 10) * 8;
    SDL_RenderCopy(impl->renderer, impl->ui, &src, &dest);
    dest.x -= 4;
    num /= 10;
  }
}

static void flip(front_sdl_impl* impl) {
  SDL_Rect src;
  SDL_Rect dest;

  sys* sys = impl->front->sys;
  mapper* mapper = sys->mapper;
  uint16_t pc = sys->cpu->program_counter;
  switch (impl->front->tab) {
    case FT_PPU: {
      // PPU OAM data
      ppu* ppu = sys->ppu;
      display_number(impl, ppu->spr_count_max, 240, 8);
      uint16_t spr_y = 16;
      for (int i = 0; i < 64 && spr_y < 256; i++) {
        if (ppu->oam.sprites[i].y < 0xEF) {
          display_number(impl, ppu->oam.sprites[i].y, 150, spr_y);
          display_number(impl, ppu->oam.sprites[i].index, 180, spr_y);
          display_number(impl, ppu->oam.sprites[i].attr, 210, spr_y);
          display_number(impl, ppu->oam.sprites[i].x, 240, spr_y);
          spr_y += 8;
        }
      }

      // Display the PPU palette
      SDL_Rect rect;
      rect.w = 16;
      rect.h = 8;
      if (impl->mouse_y >= 240) {
        // Display the NES palette instead
        for (int i = 0; i < 64; i++) {
          rect.h = 4;
          uint32_t colour = impl->palette[i];
          SDL_SetRenderDrawColor(impl->renderer, colour >> 16,
                                 colour >> 8, colour, 0xFF);
          rect.x = (i % 16) * 16;
          rect.y = (i / 16) * 4 + 240;
          SDL_RenderFillRect(impl->renderer, &rect);
        }
      } else {
        for (int i = 0; i < 32; i++) {
          uint32_t colour = impl->palette[sys->ppu->palette[i] & 0x3F];
          SDL_SetRenderDrawColor(impl->renderer, colour >> 16,
                                 colour >> 8, colour, 0xFF);
          rect.x = (i % 16) * 16;
          rect.y = (i / 16) * 8 + 240;
          SDL_RenderFillRect(impl->renderer, &rect);
        }
      }
    } break;
    case FT_IO: {
      // Display controller sprites with active buttons
      for (int i = 0; i < 2; i++) {
        src.x = 80;
        src.y = 88;
        src.w = dest.w = 24;
        src.h = dest.h = 16;
        dest.y = 240;
        dest.x = i * 24;
        SDL_RenderCopy(impl->renderer, impl->ui, &src, &dest);
        controller_pressed_t pressed = sys->controller->pressed1;
        if (i == 1) {
          pressed = sys->controller->pressed2;
        }
#define BUTTON(SX, SY, SW, SH, BUT) \
  if (BUT) { src.x = 104 + SX; src.y = 88 + SY; src.w = dest.w = SW; \
    src.h = dest.h = SH; dest.x = i * 24 + SX; dest.y = 240 + SY; \
    SDL_RenderCopy(impl->renderer, impl->ui, &src, &dest); }
        BUTTON(17, 6, 4, 4, pressed.a)
        BUTTON(12, 6, 4, 4, pressed.b)
        BUTTON(12, 2, 4, 3, pressed.select)
        BUTTON(17, 2, 4, 3, pressed.start)
        BUTTON(5,  3, 3, 2, pressed.up)
        BUTTON(5,  8, 3, 2, pressed.down)
        BUTTON(3,  5, 2, 3, pressed.left)
        BUTTON(8,  5, 2, 3, pressed.right)
#undef BUTTON
      }
    } break;
    case FT_MMC_CPU: // pass through
    case FT_MMC_PPU: {
      // Display the memory map
      if (mapper != NULL) {
        uint32_t* pixels;
        uint32_t pitch;
        if (SDL_LockTexture(impl->screen_tex, NULL, (void**)&pixels,
                            (void*)&pitch)) {
          //printf("err: %s\n", SDL_GetError());
        } else {
          if (impl->front->tab == FT_MMC_CPU) {
            for (uint32_t i = 0; i < 0x10000; i++) {
              pixels[i] = 0xFF000000 | (mmap_cpu_read(mapper, i, true) * 0x10101);
              if (i == pc) {
                pixels[i] = 0xFFFF0000;
              }
            }
          } else {
            for (uint32_t i = 0; i < 0x4000; i++) {
              uint8_t val = mmap_ppu_read(mapper, i);
              pixels[i * 4] = 0xFF000000 | ((val >> 6) * 0x404040);
              pixels[i * 4 + 1] = 0xFF000000 | (((val >> 4) & 0x3) * 0x404040);
              pixels[i * 4 + 2] = 0xFF000000 | (((val >> 2) & 0x3) * 0x404040);
              pixels[i * 4 + 3] = 0xFF000000 | ((val & 0x3) * 0x404040);
            }
          }
          SDL_UnlockTexture(impl->screen_tex);
          src.x = dest.x = 0;
          src.y = dest.y = 0;
          src.w = dest.w = 256;
          src.h = dest.h = 256;
          SDL_RenderCopy(impl->renderer, impl->screen_tex, &src, &dest);
        }
      }
    } break;
    default:
      break;
  }

  // Render the UI
  src.x = 0;
  src.y = 32;
  src.w = 16;
  src.h = 16;
  dest.x = 0;
  dest.y = 0;
  dest.w = 16;
  dest.h = 16;
  if (impl->mouse_y >= 0 && impl->mouse_y < 16) {
    // Button backgrounds
    bool sel = false;
    for (int i = 0; i < BUTTON_NUM; i++) {
      sel = impl->mouse_x >= i * 16 && impl->mouse_x < i * 16 + 16;
      if (i >= BUTTON_CPU && i <= BUTTON_MMC_PPU &&
          impl->front->tab == i - BUTTON_CPU + FT_CPU) {
        src.x = 16;
      }
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
    src.x = dest.x = 0;
    src.y = 48;
    src.w = dest.w = BUTTON_NUM * 16;
    SDL_RenderCopy(impl->renderer, impl->ui, &src, &dest);
  }
  if (impl->front->sys->status != SS_NONE) {
    src.x = 80 + (impl->front->sys->status - 1) * 24;
    src.y = 64;
    src.w = dest.w = 24;
    src.h = dest.h = 24;
    dest.x = 116;
    dest.y = 116;
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
  for (int i = 0; i < 16 * 4; i++) {
    impl->palette[i] = 0xFF000000 | ((uint32_t*)ui->pixels)[(i % 16) * 8 + (i / 16) * ui->w * 8];
  }

  // Create window
  uint32_t width = 256;
  //region_screen_width(front->sys->region) * front->scale;
  uint32_t height = 256;
  //region_screen_height(front->sys->region) * front->scale;
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
    SDL_DestroyWindow(impl->window);
    free(impl);
    return NULL;
  }

  // Create UI texture
  impl->ui = SDL_CreateTextureFromSurface(impl->renderer, ui);
  SDL_FreeSurface(ui);
  if (!impl->ui) {
    fprintf(stderr, "Could not create UI texture\n");
    SDL_DestroyRenderer(impl->renderer);
    SDL_DestroyWindow(impl->window);
    free(impl);
    return NULL;
  }

  // Create surface for main screen
  impl->screen_tex = SDL_CreateTexture(impl->renderer, SDL_PIXELFORMAT_ARGB8888,
                                       SDL_TEXTUREACCESS_STREAMING, 256, 256);
  if (!impl->screen_tex) {
    fprintf(stderr, "Could not create screen texture\n");
    SDL_DestroyTexture(impl->ui);
    SDL_DestroyRenderer(impl->renderer);
    SDL_DestroyWindow(impl->window);
    free(impl);
    return NULL;
  }

  // Fill screen with black
  SDL_SetRenderDrawColor(impl->renderer, 0, 0, 0, 255);
  SDL_RenderClear(impl->renderer);

  impl->front = front;
  return impl;
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
        case SDL_KEYDOWN: // pass through
        case SDL_KEYUP:
          controller_sdl_button(event);
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
          if (impl->mouse_x >= 0 && impl->mouse_x < BUTTON_NUM * 16 + 16 &&
              impl->mouse_y >= 0 && impl->mouse_y < 16) {
            uint8_t but_sel = impl->mouse_x / 16;
            switch (but_sel) {
              case BUTTON_LOAD: {
                char* path = front_rom_dialog();
                if (path != NULL) {
                  sys_rom(sys, path);
                  free(path);
                }
              } break;
              case BUTTON_START:
                sys_start(sys);
                break;
              case BUTTON_STOP:
                sys_stop(sys);
                break;
              case BUTTON_PAUSE:
                sys_pause(sys);
                break;
              case BUTTON_STEP:
                sys_step(sys);
                break;
              case BUTTON_CPU:
              case BUTTON_PPU:
              case BUTTON_APU:
              case BUTTON_IO:
              case BUTTON_MMC_CPU:
              case BUTTON_MMC_PPU:
                if (impl->front->tab == but_sel - BUTTON_CPU + FT_CPU) {
                  impl->front->tab = FT_SCREEN;
                } else {
                  impl->front->tab = but_sel - BUTTON_CPU + FT_CPU;
                }
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
    last_tick = this_tick;
    if (sys->running && (impl->front->tab == FT_SCREEN ||
                         impl->front->tab == FT_PPU ||
                         impl->front->tab == FT_IO)) {
      // If the system is running, flip on demand
      if (sys->ppu->flip) {
        uint32_t* pixels;
        uint32_t pitch;
        if (SDL_LockTexture(impl->screen_tex, NULL, (void**)&pixels, (void*)&pitch)) {
          //printf("err: %s\n", SDL_GetError());
        } else {
          for (int i = 0; i < PPU_SCREEN_SIZE; i++) {
            pixels[i] = impl->palette[sys->ppu->screen[i]];
          }
          memset((uint8_t*)pixels + 240 * pitch, 0, 16 * pitch);
          SDL_UnlockTexture(impl->screen_tex);
          SDL_RenderCopy(impl->renderer, impl->screen_tex, NULL, NULL);
          flip(impl);
        }
        sys->ppu->flip = false;
      }
      SDL_Delay(2);
    } else {
      // Otherwise flip on every 100ms
      flip(impl);
      SDL_Delay(100);
    }
  }
}

void front_sdl_impl_deinit(front_sdl_impl* impl) {
  SDL_DestroyTexture(impl->screen_tex);
  SDL_DestroyTexture(impl->ui);
  SDL_DestroyRenderer(impl->renderer);
  SDL_DestroyWindow(impl->window);
  SDL_Quit();
  free(impl);
}
