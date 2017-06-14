#include <stdint.h>
#include <SDL.h>

#include "profiler.h"

/**
 * profiler.c
 */

#ifdef PROFILER
static uint8_t samples = 0;
static uint32_t ticks[PROFILER_NUM_POINTS] = {0};
static float times[PROFILER_NUM_POINTS] = {0.0};
static int last_tick = 0;

void profiler_set_point(profiler_point_t p) {
  int next_tick = SDL_GetTicks();
  if (p == PROF_START) {
    last_tick = next_tick;
    return;
  }
  ticks[p - 1] += next_tick - last_tick;
  last_tick = next_tick;
}

float* profiler_get_times() {
  samples++;
  if (samples >= 40) {
    uint32_t total_ticks = 0;
    for (int i = 0; i < PROFILER_NUM_POINTS; i++) {
      total_ticks += ticks[i];
    }
    for (int i = 0; i < PROFILER_NUM_POINTS; i++) {
      times[i] = ((float)ticks[i] / (float)total_ticks);
      ticks[i] = 0;
    }
    samples = 0;
  }
  return times;
}
#else
static float times[1] = {0};

float* profiler_get_times() {
  return times;
}
#endif
