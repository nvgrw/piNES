#pragma once

/**
 * profiler.h
 *
 * Functions and macros to allow profiling the emulator in realtime to find
 * bottleneck functions, CPU-intensive code, etc.
 */

// #define PROFILER 1

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
#define PROFILER_POINT(P) \
  do {                    \
  } while (0);
#endif

float* profiler_get_times();
