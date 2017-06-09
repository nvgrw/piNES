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

// Bitfield structs
typedef struct {
  union {
    struct {
      uint8_t envelope_period_or_volume : 4;
      uint8_t constant_volume : 1;
      uint8_t loop_or_disable_length_counter : 1;
      uint8_t duty_cycle : 2;
    } fields;
    uint8_t raw;
  } reg1;

  union {
    struct {
      uint8_t shift_count : 3;
      uint8_t negative : 1;
      uint8_t period : 3;
      uint8_t enabled : 1;
    } fields;
    uint8_t raw;
  } reg2;

  union {
    struct {
      uint8_t timer_low;
    } fields;
    uint8_t raw;
  } reg3;

  union {
    struct {
      uint8_t high : 3;
      uint8_t length_counter_load : 5;
    } fields;
    uint8_t raw;
  } reg4;
} apu_pulse_channel;

typedef struct {
  union {
    struct {
      uint8_t lin_count_reload_val : 7;
      uint8_t len_count_disable : 1;
    } fields;
    uint8_t raw;
  } reg1;

  union {
    struct {
      uint8_t timer_low;
    } fields;
    uint8_t raw;
  } reg2;

  union {
    struct {
      uint8_t timer_high : 3;
      uint8_t len_count_load : 5;
    } fields;
    uint8_t raw;
  } reg3;

} apu_triangle_channel;

typedef struct {
  union {
    struct {
      uint8_t env_per_or_volume : 4;
      uint8_t const_volume : 1;
      uint8_t loop_env_or_dis_len : 1;
      uint8_t : 2;
    } fields;
    uint8_t raw;
  } reg1;

  union {
    struct {
      uint8_t noise_period : 4;
      uint8_t : 3;
      uint8_t loop_noise : 1;
    } fields;
    uint8_t raw;
  } reg2;

  union {
    struct {
      uint8_t : 3;
      uint8_t lenn_count_load : 5;
    } fields;
    uint8_t raw;
  } reg3;
} apu_noise_channel;

typedef struct {
  union {
    struct {
      uint8_t freq_index : 4;
      uint8_t : 2;
      uint8_t loop_sample : 1;
      uint8_t irq_enable : 1;
    } fields;
    uint8_t raw;
  } reg1;

  union {
    struct {
      uint8_t direct_load : 7;
      uint8_t : 1;
    } fields;
    uint8_t raw;
  } reg2;

  union {
    struct {
      uint8_t sample_address;
    } fields;
    uint8_t raw;
  } reg3;

  union {
    struct {
      uint8_t sample_length;
    } fields;
    uint8_t raw;
  } reg4;
} apu_dmc_channel;

typedef struct {
  uint8_t pulse1 : 1;
  uint8_t pulse2 : 1;
  uint8_t triangle : 1;
  uint8_t noise : 1;
} apu_len_count_enable;

typedef union {
  union {
    apu_len_count_enable len_count_enable;
    uint8_t dmc_enable : 1;
  } fields;
  uint8_t raw;
} apu_control;

typedef union {
  union {
    apu_len_count_enable len_count_enable;
    uint8_t dmc_interrupt : 1;
  } fields;
  uint8_t raw;
} apu_status;

typedef union {
  struct {
    uint8_t : 6;
    uint8_t disable_frame : 1;
    uint8_t frame5_sequence : 1;
  } fields;

  uint8_t raw;
} frame_counter;

typedef struct {
  // pulse_channel pulse[2];
  // triangle_channel triangle;
  // noise_channel noise;
  // dmc_channel dmc;

  // control control;
  // status status;

  // frame_counter frame_counter;

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
