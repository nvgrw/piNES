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

#include <stdint.h>

#include "rom.h"

#define NUM_MAPPERS 5

typedef struct mapper_special {
  void (*mapper_init)(struct mapper_special* self, mapper_t* mapper);
  void (*mapper_deinit)(struct mapper_special* self, mapper_t* mapper);
  void (*cpu_write)(struct mapper_special* self, mapper_t* mapper,
                    uint16_t address, uint8_t val);
  uint8_t (*cpu_read)(struct mapper_special* self, mapper_t* mapper,
                      uint16_t address);
  void (*ppu_write)(struct mapper_special* self, mapper_t* mapper,
                    uint16_t address, uint8_t val);
  uint8_t (*ppu_read)(struct mapper_special* self, mapper_t* mapper,
                      uint16_t address);
  bool present;
  void* data;
} mapper_special_t;

extern mapper_special_t MAPPERS[NUM_MAPPERS];
