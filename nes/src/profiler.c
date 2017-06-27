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
#include <stdint.h>

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

float* profiler_get_times() { return times; }
#endif
