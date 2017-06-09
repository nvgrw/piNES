#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "rom.h"

#define APU_SAMPLE_RATE 1789800.0
#define APU_ACTUAL_SAMPLE_RATE 44100
#define AUDIO_BUFFER_SIZE 256

/**
 * Lookup tables for the channels
 * TODO: May not actually need to be externed
 */
extern const uint8_t APU_LENGTH_TABLE[];
extern const uint8_t APU_DUTY_TABLE[];
extern const uint16_t APU_NOISE_TABLE[];

// Bitfields
typedef union {
  struct {
    uint8_t divider_period : 4;
    uint8_t constant_volume_envelope : 1;
    uint8_t length_counter_halt : 1;
    uint8_t duty_cycle : 2;
  } data;
  uint8_t raw;
} register_4000_4004;

typedef union {
  struct {
    uint8_t shift_count : 3;
    uint8_t negate : 1;
    uint8_t period : 3;
    uint8_t enabled : 1;
  } data;
  uint8_t raw;
} register_4001_4005;

typedef union {
  struct {
    uint8_t timer_low : 8;
  } data;
  uint8_t raw;
} register_4002_4006;

typedef union {
  struct {
    uint8_t timer_high : 3;
    uint8_t length_counter_load : 5;
  } data;
  uint8_t raw;
} register_4003_4007;

typedef union {
  struct {
    uint8_t linear_counter_reload : 7;
    uint8_t control : 1;
  } data;
  uint8_t raw;
} register_4008;

typedef union {
  struct {
    uint8_t timer_low : 8;
  } data;
  uint8_t raw;
} register_400a;

typedef union {
  struct {
    uint8_t timer_high : 3;
    uint8_t length_counter_load;
  } data;
  uint8_t raw;
} register_400b;

typedef union {
  struct {
    uint8_t divider_period : 4;
    uint8_t constant_volume_envelope : 1;
    uint8_t length_counter_halt : 1;
    uint8_t : 2;
  } data;
  uint8_t raw;
} register_400c;

typedef union {
  struct {
    uint8_t period : 4;
    uint8_t : 3;
    uint8_t mode : 1;
  } data;
  uint8_t raw;
} register_400e;

typedef union {
  struct {
    uint8_t : 3;
    uint8_t length_counter_load : 5;
  } data;
  uint8_t raw;
} register_400f;

typedef union {
  struct {
    uint8_t rate_index : 4;
    uint8_t : 2;
    uint8_t loop_flag : 1;
    uint8_t irq_enabled : 1;
  } data;
  uint8_t raw;
} register_4010;

typedef union {
  struct {
    uint8_t direct_load : 7;
    uint8_t : 1;
  } data;
  uint8_t raw;
} register_4011;

typedef union {
  struct {
    uint8_t sample_address : 8;
  } data;
  uint8_t raw;
} register_4012;

typedef union {
  struct {
    uint8_t sample_length : 8;
  } data;
  uint8_t raw;
} register_4013;

typedef union {
  struct {
    uint8_t enable_pulse0 : 1;
    uint8_t enable_pulse1 : 1;
    uint8_t enable_triangle : 1;
    uint8_t enable_noise : 1;
    uint8_t enable_dmc : 1;
    uint8_t : 1;
    uint8_t frame_interrupt : 1;
    uint8_t dmc_interrupt : 1;
  } data;
  uint8_t raw;
} register_4015_status;

typedef union {
  struct {
    uint8_t : 6;
    uint8_t irq_inhibit : 1;
    uint8_t mode : 1;
  } data;
  uint8_t raw;
} register_4017_frame_counter;

typedef struct {
  mapper* mapper;

  bool linear_counter_reload;
  uint8_t cycle_count;
  double sample_skips;
  uint8_t buffer[AUDIO_BUFFER_SIZE];
  int buffer_cursor;

  double cntr;
} apu;

/**
 * Initialise the APU struct, setting all fields
 * to their default values
 */
apu* apu_init(void);

/**
 * Memory acces utility functions
 */
void apu_mem_write(apu* apu, uint16_t address, uint8_t val);

/**
 * General output methods
 */
void apu_cycle(apu* apu, void* context,
               void (*enqueue_audio)(void* context, uint8_t* buffer, int len));
