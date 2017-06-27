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

/**
 * profiler.h
 *
 * Functions and macros to allow profiling the emulator in realtime to find
 * bottleneck functions, CPU-intensive code, etc.
 *
 * Enabled with -DPROFILER
 */

/**
 * Points of interest within the program.
 */
typedef enum {
  PROF_START,   // front runloop start
  PROF_EVENTS,  // SDL events polled and done
  PROF_TICKS,   // tick logic done
  PROF_SYS_START,
  PROF_SYS_CPU,
  PROF_SYS_PPU_BG,
  PROF_SYS_PPU_SPRITES,
  PROF_SYS_PPU_LOGIC,
  PROF_SYS_APU,
  PROF_SYS_END,  // sys logic done
  PROF_PREFLIP,  // preflip done
  PROF_END       // flip (UI drawing) done
} profiler_point_t;

#ifdef PROFILER
#define PROFILER_NUM_POINTS PROF_END
#define PROFILER_POINT(P) profiler_set_point(PROF_##P);
void profiler_set_point(profiler_point_t p);
#else
#define PROFILER_NUM_POINTS 0
#define PROFILER_POINT(P)
#endif

float* profiler_get_times();
