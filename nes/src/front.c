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

#include <nfd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "front.h"
#include "sys.h"

/**
 * front.c
 */

char* front_rom_dialog(void) {
  nfdchar_t* nfd_path = NULL;
  nfdresult_t result = NFD_OpenDialog(NULL, NULL, &nfd_path);

  if (result == NFD_OKAY) {
    return (char*)nfd_path;
  }

  if (result != NFD_CANCEL) {
    fprintf(stderr, "Error: %s\n", NFD_GetError());
  }
  return NULL;
}

front_t* front_init(sys_t* sys) {
  front_t* front = malloc(sizeof(front_t));
  front->sys = sys;
  front->tab = FT_SCREEN;
  front->scale = 1;
  return front;
}

void front_deinit(front_t* front) { free(front); }
