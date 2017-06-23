#include <SDL.h>
#include <SDL_image.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "apu.h"
#include "controller_sdl.h"
#include "error.h"
#include "front.h"
#include "front_sdl.h"
#include "ppu.h"
#include "profiler.h"
#include "region.h"
#include "sys.h"

/**
 * front_sdl.c
 */

#define BUTTON_LOAD 0
#define BUTTON_START 1
#define BUTTON_STOP 2
#define BUTTON_PAUSE 3
#define BUTTON_CPU 4
#define BUTTON_PPU 5
#define BUTTON_APU 6
#define BUTTON_IO 7
#define BUTTON_MMC_CPU 8
#define BUTTON_MMC_PPU 9
#define BUTTON_TEST 10
#define BUTTON_ZOOM 11
#define BUTTON_FULL 12

#define BUTTON_NUM 13

/**
 * Messages displayed when the user changes the display scaling.
 */
static char* SCALE_FACTORS[] = {
    "Scale set to 1x", "Scale set to 2x", "Scale set to 3x",
};

static char* HEXADECIMAL = "0123456789ABCDEF";

static char* UNSUPPORTED_INSTRUCTION_MESSAGE =
    "0x00: Unsupported instruction encountered!";

#ifdef PROFILER
static uint8_t PROFILER_COLOURS[] = {
    0xFF, 0xFF, 0xFF,  // PROF_START - PROF_EVENTS
    0xFF, 0xFF, 0xFF,  // PROF_EVENTS - PROF_TICKS
    0xFF, 0xFF, 0xFF,  // PROF_TICKS - PROF_SYS_START
    0xFF, 0x11, 0x11,  // PROF_SYS_START - PROF_SYS_CPU
    0x11, 0x11, 0x11,  // PROF_SYS_CPU - PROF_SYS_PPU_BG
    0x11, 0x11, 0xFF,  // PROF_SYS_PPU_BG - PROF_SYS_PPU_SPRITES
    0x11, 0xFF, 0x11,  // PROF_SYS_PPU_SPRITES - PROF_SYS_PPU_LOGIC
    0xFF, 0xFF, 0xFF,  // PROF_SYS_PPU_LOGIC - PROF_SYS_APU
    0xFF, 0xFF, 0xFF,  // PROF_SYS_APU - PROF_SYS_END
    0xFF, 0xFF, 0xFF,  // PROF_SYS_END - PROF_PREFLIP
    0xFF, 0xFF, 0xFF,  // PROF_PREFLIP - PROF_END
};
#endif

/**
 * Helper functions
 *
 * display_number
 *   Displays a number on the UI at the given location.
 *
 * display_text
 *   Displays a string on the UI at the given location.
 *
 * display_message
 *   Set the current display message to the given string, will display for
 *   2 seconds.
 *
 * preflip
 *   Sets up the render target (in case of scaling) and clears the screen.
 *
 * flip
 *   Updates the window with the current screen data, and draws any UI
 *   on top of the screen as necessary.
 */
static void display_number(front_sdl_impl_t* impl, uint32_t num, uint16_t x,
                           uint16_t y) {
  SDL_Rect src = {.x = 96, .y = 0, .w = 5, .h = 8};
  SDL_Rect dest = {.x = x, .y = y, .w = 5, .h = 8};
  if (!num) {
    SDL_RenderCopy(impl->renderer, impl->ui, &src, &dest);
    return;
  }
  while (num) {
    src.x = 96 + (num % 10) * 5;
    SDL_RenderCopy(impl->renderer, impl->ui, &src, &dest);
    dest.x -= 4;
    num /= 10;
  }
}

static void display_text(front_sdl_impl_t* impl, char* str, uint16_t x,
                         uint16_t y) {
  SDL_Rect src = {.x = 0, .y = 0, .w = 5, .h = 8};
  SDL_Rect dest = {.x = x, .y = y, .w = 5, .h = 8};
  while (*str != 0) {
    uint8_t ch = *str;
    if (ch >= 97 && ch <= 122) {
      // Convert to uppercase
      ch -= 32;
    }
    if (ch >= 32 && ch <= 96) {
      ch -= 32;
      src.x = 16 + (ch % 38) * 5;
      src.y = (ch / 38) * 8;
      SDL_RenderCopy(impl->renderer, impl->ui, &src, &dest);
    }
    dest.x += 4;
    str++;
  }
}

static void display_message(front_sdl_impl_t* impl, char* str) {
  size_t len = strlen(str);
  if (len > 511) {
    len = 511;
  }
  strncpy(impl->message, str, len);
  impl->message[len] = 0;
  impl->message_ticks = 2000;
}

static void preflip(front_sdl_impl_t* impl) {
  if (impl->front->scale != 1) {
    // If there is a scaling factor, we render everything to a texture first
    SDL_SetRenderTarget(impl->renderer, impl->prescaled_tex);
  } else {
    SDL_SetRenderTarget(impl->renderer, NULL);
  }

  // Fill screen with black
  SDL_SetRenderDrawColor(impl->renderer, 0, 0, 0, 255);
  SDL_RenderClear(impl->renderer);

  PROFILER_POINT(PREFLIP)
}

static void flip(front_sdl_impl_t* impl) {
  int x_edge = impl->screen_rect->w + impl->screen_rect->x * 2;
  int y_edge = impl->screen_rect->h + impl->screen_rect->y * 2;
  int x_mid = x_edge / 2;
  int y_mid = y_edge / 2;

  SDL_Rect src;
  SDL_Rect dest;

  sys_t* sys = impl->front->sys;
  mapper_t* mapper = sys->mapper;
  uint16_t pc = sys->cpu->program_counter;
  switch (impl->front->tab) {
    case FT_PPU: {
      // PPU OAM data
      ppu_t* ppu = sys->ppu;
      display_number(impl, ppu->spr_count_max, x_edge - 16, 8);
      uint16_t spr_y = 16;
      for (int i = 0; i < 64 && spr_y < y_edge; i++) {
        if (ppu->oam.sprites[i].y < 0xEF) {
          display_number(impl, ppu->oam.sprites[i].y, x_edge - 106, spr_y);
          display_number(impl, ppu->oam.sprites[i].index, x_edge - 76, spr_y);
          display_number(impl, ppu->oam.sprites[i].attr, x_edge - 46, spr_y);
          display_number(impl, ppu->oam.sprites[i].x, x_edge - 16, spr_y);
          spr_y += 8;
        }
      }

      // Display the PPU palette
      SDL_Rect rect;
      rect.w = 16;
      rect.h = 8;
      if (impl->mouse_y >= y_edge - 16) {
        // Display the NES palette instead
        for (int i = 0; i < 64; i++) {
          rect.h = 4;
          uint32_t colour = sys->ppu->nes_palette_direct[i];
          SDL_SetRenderDrawColor(impl->renderer, colour >> 16, colour >> 8,
                                 colour, 0xFF);
          rect.x = (i % 16) * 16;
          rect.y = (i / 16) * 4 + (impl->screen_rect->h - 16);
          SDL_RenderFillRect(impl->renderer, &rect);
        }
      } else {
        for (int i = 0; i < 32; i++) {
          uint32_t colour = sys->ppu->palette_cache[i];
          SDL_SetRenderDrawColor(impl->renderer, colour >> 16, colour >> 8,
                                 colour, 0xFF);
          rect.x = (i % 16) * 16;
          rect.y = (i / 16) * 8 + (impl->screen_rect->h - 16);
          SDL_RenderFillRect(impl->renderer, &rect);
          display_number(impl, sys->ppu->palette[i] & 0x3F, rect.x + 6,
                         rect.y - 3);
        }
      }

      // Show colour at current mouse position
      display_number(impl, impl->mouse_x, x_edge - 106, y_edge - 36);
      display_number(impl, impl->mouse_y, x_edge - 76, y_edge - 36);
      if (impl->mouse_x < 256 && impl->mouse_y < 240) {
        display_number(impl, sys->ppu->screen_dbg[impl->mouse_x + impl->mouse_y * 256],
                       x_edge - 46, y_edge - 36);
      }
    } break;
    case FT_APU: {
      // Display the audio buffer
      dest.x = 0;
      dest.y = 0;
      dest.w = 1;
      dest.h = 1;
      SDL_SetRenderDrawColor(impl->renderer, 0x11, 0xFF, 0x11, 0xFF);
      //int step = AUDIO_BUFFER_SIZE / 256;
      int j = sys->apu->buffer_cursor;
      for (int i = 0; i < 256; i++) {
        dest.y = y_edge - 1 - sys->apu->buffer[j % AUDIO_BUFFER_SIZE] * 32.0;
        SDL_RenderFillRect(impl->renderer, &dest);
        j++;
        dest.x++;
      }
    } break;
    case FT_IO: {
      // Display controller sprites with active buttons
      for (int i = 0; i < 2; i++) {
        src.x = 48;
        src.y = 32;
        src.w = dest.w = 24;
        src.h = dest.h = 16;
        dest.y = y_edge - 16;
        dest.x = i * 24;
        SDL_RenderCopy(impl->renderer, impl->ui, &src, &dest);
        controller_pressed_t pressed = sys->controller->pressed1;
        if (i == 1) {
          pressed = sys->controller->pressed2;
        }
#define BUTTON(SX, SY, SW, SH, BUT)                        \
  if (BUT) {                                               \
    src.x = 72 + SX;                                       \
    src.y = 32 + SY;                                       \
    src.w = dest.w = SW;                                   \
    src.h = dest.h = SH;                                   \
    dest.x = i * 24 + SX;                                  \
    dest.y = (y_edge - 16) + SY;                           \
    SDL_RenderCopy(impl->renderer, impl->ui, &src, &dest); \
  }
        BUTTON(17, 6, 4, 4, pressed.a)
        BUTTON(12, 6, 4, 4, pressed.b)
        BUTTON(12, 2, 4, 3, pressed.select)
        BUTTON(17, 2, 4, 3, pressed.start)
        BUTTON(5, 3, 3, 2, pressed.up)
        BUTTON(5, 8, 3, 2, pressed.down)
        BUTTON(3, 5, 2, 3, pressed.left)
        BUTTON(8, 5, 2, 3, pressed.right)
#undef BUTTON
      }
    } break;
    case FT_MMC_CPU:  // pass through
    case FT_MMC_PPU: {
      // Display the memory map
      if (mapper != NULL) {
        uint32_t* pixels;
        uint32_t pitch;
        if (SDL_LockTexture(impl->screen_tex, NULL, (void**)&pixels,
                            (void*)&pitch)) {
          // printf("err: %s\n", SDL_GetError());
        } else {
          if (impl->front->tab == FT_MMC_CPU) {
            for (uint32_t i = 0; i < 0x10000; i++) {
              pixels[i] =
                  0xFF000000 | (mmap_cpu_read(mapper, i, true) * 0x10101);
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
          SDL_RenderCopy(impl->renderer, impl->screen_tex, NULL, NULL);
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
    src.y = 16;
    src.w = dest.w = BUTTON_NUM * 16;
    SDL_RenderCopy(impl->renderer, impl->ui, &src, &dest);
  }

  if (sys->status != SS_NONE) {
    // Render system status
    src.x = (sys->status - 1) * 24;
    src.y = 48;
    src.w = dest.w = 24;
    src.h = dest.h = 24;
    dest.x = x_mid - 12;
    dest.y = y_mid - 12;
    SDL_RenderCopy(impl->renderer, impl->ui, &src, &dest);
  } else if (!sys->running) {
    // Display pines logo
    src.x = 128;
    src.y = 32;
    src.w = dest.w = 32;
    src.h = dest.h = 40;
    dest.x = x_mid - 16;
    dest.y = y_mid - 20;
    SDL_RenderCopy(impl->renderer, impl->ui, &src, &dest);
  }

  // Render display message, if any
  if (impl->message_ticks) {
    display_text(impl, impl->message, (impl->screen_rect->w - 6) - strlen(impl->message) * 4,
                 impl->screen_rect->h - 12);
  }

  PROFILER_POINT(END)

#ifdef PROFILER
  // Display profiler data
  float* times = profiler_get_times();
  dest.x = 0;
  dest.y = impl->screen_rect->h - 6;
  dest.h = 6;
  for (int i = 0; i < PROFILER_NUM_POINTS; i++) {
    dest.w = times[i] * 256;
    SDL_SetRenderDrawColor(impl->renderer, PROFILER_COLOURS[i * 3],
                           PROFILER_COLOURS[i * 3 + 1],
                           PROFILER_COLOURS[i * 3 + 2], 0xFF);
    SDL_RenderFillRect(impl->renderer, &dest);
    dest.x += dest.w;
  }
#endif

  if (impl->front->scale != 1) {
    SDL_SetRenderTarget(impl->renderer, NULL);
    SDL_RenderCopy(impl->renderer, impl->prescaled_tex, NULL, NULL);
  }

  SDL_RenderPresent(impl->renderer);
  SDL_RenderClear(impl->renderer);
}

/**
 * Public functions
 *
 * See front_sdl.h for descriptions.
 */
front_sdl_impl_t* front_sdl_impl_init(front_t* front) {
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
  front_sdl_impl_t* impl = calloc(1, sizeof(front_sdl_impl_t));
  impl->screen_rect = malloc(sizeof(SDL_Rect));
  impl->screen_rect->x = 0;
  impl->screen_rect->y = 0;
  impl->screen_rect->w = 256;
  impl->screen_rect->h = 256;
  impl->full = false;
  impl->mouse_x = 0;
  impl->mouse_y = 0;
  impl->mouse_down = false;

  // Initialise palette
  for (int i = 0; i < 16 * 4; i++) {
    uint32_t colour = ((uint32_t*)ui->pixels)[(i % 16) + (i / 16) * ui->w * 4];
    uint8_t r;
    uint8_t g;
    uint8_t b;
    SDL_GetRGB(colour, ui->format, &r, &g, &b);
    front->sys->ppu->nes_palette_direct[i] = 0xFF000000 | (r << 16) | (g << 8) | b;
  }

  // Create window
  impl->window =
      SDL_CreateWindow("pines", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                       256, 256, SDL_WINDOW_OPENGL);
  if (!impl->window) {
    fprintf(stderr, "Could not create window\n");
    SDL_FreeSurface(ui);
    free(impl);
    return NULL;
  }

  // Create renderer
  impl->renderer = SDL_CreateRenderer(
      impl->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
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

  // Create surface for main screen
  impl->prescaled_tex =
      SDL_CreateTexture(impl->renderer, SDL_PIXELFORMAT_ARGB8888,
                        SDL_TEXTUREACCESS_TARGET, 256, 256);
  if (!impl->prescaled_tex) {
    fprintf(stderr, "Could not create scaling texture\n");
    SDL_DestroyTexture(impl->screen_tex);
    SDL_DestroyTexture(impl->ui);
    SDL_DestroyRenderer(impl->renderer);
    SDL_DestroyWindow(impl->window);
    free(impl);
    return NULL;
  }

  // Initialise audio
  SDL_AudioSpec audio_want, audio_have;
  audio_want.freq = APU_ACTUAL_SAMPLE_RATE;
  audio_want.format = AUDIO_F32;
  audio_want.samples = AUDIO_BUFFER_SIZE;
  audio_want.callback = NULL;
  audio_want.channels = 1;
  audio_want.userdata = impl;
  impl->audio_device = 
      SDL_OpenAudioDevice(NULL, 0, &audio_want, &audio_have, 0);
  if (!impl->audio_device) {
    fprintf(stderr, "Could not create audio device, %s\n", SDL_GetError());
    SDL_DestroyTexture(impl->screen_tex);
    SDL_DestroyTexture(impl->ui);
    SDL_DestroyRenderer(impl->renderer);
    SDL_DestroyWindow(impl->window);
    free(impl);
    return NULL;
  }
  // Enables queuing
  SDL_PauseAudioDevice(impl->audio_device, false);

  impl->front = front;
  preflip(impl);

  display_message(impl, "pines emulator initialised");

  return impl;
}

static void front_sdl_impl_audio_enqueue(void* context, apu_buffer_t* buffer,
                                         int len);
static apu_queued_size_t front_sdl_impl_audio_get_queue_size(void* context);

void front_sdl_impl_run(front_sdl_impl_t* impl) {
  bool running = true;
  bool force_flip = false;
  uint32_t last_tick = SDL_GetTicks();
  sys_t* sys = impl->front->sys;

  // Enter render loop, waiting for user to quit
  while (running) {
    PROFILER_POINT(START)

    // Process SDL events
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_QUIT:
          running = false;
          break;
        case SDL_KEYDOWN:  // pass through
        case SDL_KEYUP:
          controller_sdl_button(event);
          break;
        case SDL_MOUSEMOTION:
          impl->mouse_x = event.motion.x / impl->front->scale;
          impl->mouse_y = event.motion.y / impl->front->scale;
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
                // The dialog stalls SDL, don't count ticks
                last_tick = SDL_GetTicks();
                if (path != NULL) {
                  sys_rom(sys, path);
                  free(path);
                  switch (sys->status) {
                    case SS_ROM_DAMAGED:
                      display_message(impl, "Invalid ROM file!");
                      break;
                    case SS_ROM_MAPPER:
                      display_message(impl, "Unsupported ROM mapper!");
                      break;
                    default:
                      break;
                  }
                }
              } break;
              case BUTTON_START:
                SDL_PauseAudioDevice(impl->audio_device, false);
                sys_start(sys);
                switch (sys->status) {
                  case SS_ROM_MISSING:
                    display_message(impl, "No ROM loaded!");
                    break;
                  default:
                    break;
                }
                break;
              case BUTTON_STOP:
                SDL_PauseAudioDevice(impl->audio_device, true);
                sys_stop(sys);
                break;
              case BUTTON_PAUSE:
                sys_pause(sys);
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
              case BUTTON_ZOOM: {
                if (impl->full) {
                  break;
                }
                uint8_t scale = impl->front->scale;
                switch (scale) {
                  case 1:
                    scale = 2;
                    break;
                  case 2:
                    scale = 3;
                    break;
                  default:
                    scale = 1;
                    break;
                }
                impl->front->scale = scale;
                display_message(impl, SCALE_FACTORS[scale - 1]);
                if (!impl->full) {
                  SDL_SetWindowSize(impl->window, 256 * scale, 256 * scale);
                  force_flip = true;
                }
              } break;
              case BUTTON_FULL:
                impl->full = !impl->full;
                if (impl->full) {
#ifdef ALT_FULLSCREEN
                  SDL_SetWindowSize(impl->window, 256, 256);
                  SDL_SetWindowFullscreen(impl->window, SDL_WINDOW_FULLSCREEN_DESKTOP);
                  int w;
                  int h;
                  SDL_GetWindowSize(impl->window, &w, &h);
                  impl->screen_rect->x = 0;
                  impl->screen_rect->y = 0;
                  impl->screen_rect->w = w;
                  impl->screen_rect->h = h;
                  if (w < h) {
                    impl->screen_rect->y = (h - w) / 2;
                    impl->screen_rect->w = impl->screen_rect->h = w;
                  } else {
                    impl->screen_rect->x = (w - h) / 2;
                    impl->screen_rect->w = impl->screen_rect->h = h;
                  }
                  impl->front->scale = 1;
#else
                  SDL_SetWindowSize(impl->window, 800, 600);
                  SDL_SetWindowFullscreen(impl->window, SDL_WINDOW_FULLSCREEN);
                  int w;
                  int h;
                  SDL_GetWindowSize(impl->window, &w, &h);
                  impl->screen_rect->x = 100;
                  impl->screen_rect->y = 0;
                  impl->screen_rect->w = 600;
                  impl->screen_rect->h = 600;
                  impl->front->scale = 1;
#endif
                } else {
                  impl->screen_rect->x = 0;
                  impl->screen_rect->y = 0;
                  impl->screen_rect->w = 256;
                  impl->screen_rect->h = 256;
                  SDL_SetWindowFullscreen(impl->window, 0);
                  SDL_SetWindowSize(impl->window, 256 * impl->front->scale, 256 * impl->front->scale);
                  force_flip = true;
                }
                break;
            }
          }
          break;
      }
    }

    PROFILER_POINT(EVENTS)

    // Calculate time passed
    uint32_t this_tick = SDL_GetTicks();
    uint32_t ticks_passed = this_tick - last_tick;
    last_tick = this_tick;

    // Take away display message time, if any
    if (impl->message_ticks) {
      if (ticks_passed >= impl->message_ticks) {
        impl->message_ticks = 0;
      } else {
        impl->message_ticks -= ticks_passed;
      }
    }

    PROFILER_POINT(TICKS)

    // Run the system for the time passed and display graphics
    if (sys_run(sys, ticks_passed, impl, front_sdl_impl_audio_enqueue,
                front_sdl_impl_audio_get_queue_size)) {
      // The system crashed
      switch (sys->status) {
        case SS_CPU_UNSUPPORTED_INSTRUCTION:
          display_message(impl, UNSUPPORTED_INSTRUCTION_MESSAGE);
          impl->message[2] = HEXADECIMAL[sys->cpu->last_opcode >> 4];
          impl->message[3] = HEXADECIMAL[sys->cpu->last_opcode & 0xF];
          break;
        default:
          break;
      }
    }

    if (sys->running &&
        (impl->front->tab == FT_SCREEN || impl->front->tab == FT_PPU ||
         impl->front->tab == FT_APU || impl->front->tab == FT_IO)) {
      // If the system is running, flip on demand
      if (sys->ppu->flip || force_flip) {
        uint32_t* pixels;
        uint32_t pitch;
        preflip(impl);
        if (SDL_LockTexture(impl->screen_tex, NULL, (void**)&pixels,
                            (void*)&pitch)) {
          // printf("err: %s\n", SDL_GetError());
        } else {
          memcpy(pixels, sys->ppu->screen, PPU_SCREEN_SIZE_BYTES);
          memset((uint8_t*)pixels + 240 * pitch, 0, 16 * pitch);
          SDL_UnlockTexture(impl->screen_tex);
          SDL_RenderCopy(impl->renderer, impl->screen_tex, NULL, impl->screen_rect);
          flip(impl);
        }
        sys->ppu->flip = false;
      }
      SDL_Delay(2);
    } else {
      // Otherwise flip on every 100ms
      preflip(impl);
      flip(impl);
      SDL_Delay(100);
    }
  }
}

static void front_sdl_impl_audio_enqueue(void* context, apu_buffer_t* buffer,
                                         int len) {
  front_sdl_impl_t* impl = (front_sdl_impl_t*)context;
  SDL_ClearQueuedAudio(impl->audio_device);
  SDL_QueueAudio(impl->audio_device, buffer, len * sizeof(apu_buffer_t));
}

static apu_queued_size_t front_sdl_impl_audio_get_queue_size(void* context) {
  front_sdl_impl_t* impl = (front_sdl_impl_t*)context;
  return SDL_GetQueuedAudioSize(impl->audio_device);
}

void front_sdl_impl_deinit(front_sdl_impl_t* impl) {
  SDL_DestroyTexture(impl->screen_tex);
  SDL_DestroyTexture(impl->ui);
  SDL_DestroyRenderer(impl->renderer);
  SDL_DestroyWindow(impl->window);
  SDL_Quit();
  free(impl);
}
