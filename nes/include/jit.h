#pragma once

typedef struct {
  void (*execute)(cpu_t*);
  uint8_t cycles;
  uint8_t size;
} jit_instruction_t;
