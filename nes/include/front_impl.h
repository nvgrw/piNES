#pragma once

// TODO: conditionally select the appropriate front
// #ifdef FRONT_SDL
#include "front_sdl.h"
#define front_impl front_sdl_impl
#define front_impl_init front_sdl_impl_init
#define front_impl_run front_sdl_impl_run
#define front_impl_deinit front_sdl_impl_deinit
// #endif
