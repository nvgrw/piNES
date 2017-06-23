#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "apu_channels.h"
#include "apu_typedefs.h"
#include "rom.h"

#define APU_SAMPLE_RATE (21470000 / 12.0)
#define APU_ACTUAL_SAMPLE_RATE 44100
#define AUDIO_BUFFER_SIZE 512

#define LU_PULSE_SIZE 31
#define LU_TND_SIZE 203

// Register bitfields
typedef union {
  struct __attribute__((packed)) {
    uint8_t divider_period : 4;
    uint8_t constant_volume_envelope : 1;
    uint8_t length_counter_halt : 1;
    uint8_t : 2;
  } data;  // Common data
  struct __attribute__((packed)) {
    uint8_t divider_period : 4;
    uint8_t constant_volume_envelope : 1;
    uint8_t length_counter_halt : 1;
    uint8_t duty_cycle : 2;
  } data_period;
  struct __attribute__((packed)) {
    uint8_t divider_period : 4;
    uint8_t constant_volume_envelope : 1;
    uint8_t length_counter_halt : 1;
    uint8_t : 2;
  } data_noise;
  uint8_t raw;
} apu_register_4000_4004_t;

typedef union {
  struct __attribute__((packed)) {
    uint8_t shift_count : 3;
    uint8_t negate : 1;
    uint8_t period : 3;
    uint8_t enabled : 1;
  } data;
  uint8_t raw;
} apu_register_4001_4005_t;

typedef union {
  struct __attribute__((packed)) {
    uint8_t timer_low : 8;
  } data;
  uint8_t raw;
} apu_register_4002_4006_t;

typedef union {
  struct __attribute__((packed)) {
    uint8_t : 3;
    uint8_t length_counter_load : 5;
  } data;
  struct __attribute__((packed)) {
    uint8_t timer_high : 3;
    uint8_t length_counter_load : 5;
  } data_period;
  struct __attribute__((packed)) {
    uint8_t : 3;
    uint8_t length_counter_load : 5;
  } data_noise;
  uint8_t raw;
} apu_register_4003_4007_t;

typedef union {
  struct __attribute__((packed)) {
    uint8_t linear_counter_reload : 7;
    uint8_t control : 1;
  } data;
  uint8_t raw;
} apu_register_4008_t;

typedef union {
  struct __attribute__((packed)) {
    uint8_t timer_low : 8;
  } data;
  uint8_t raw;
} apu_register_400a_t;

typedef union {
  struct __attribute__((packed)) {
    uint8_t timer_high : 3;
    uint8_t length_counter_load;
  } data;
  uint8_t raw;
} apu_register_400b_t;

typedef union {
  struct __attribute__((packed)) {
    uint8_t period : 4;
    uint8_t : 3;
    uint8_t mode : 1;
  } data;
  uint8_t raw;
} apu_register_400e_t;

typedef union {
  struct __attribute__((packed)) {
    uint8_t rate_index : 4;
    uint8_t : 2;
    uint8_t loop_flag : 1;
    uint8_t irq_enabled : 1;
  } data;
  uint8_t raw;
} apu_register_4010_t;

typedef union {
  struct __attribute__((packed)) {
    uint8_t direct_load : 7;
    uint8_t : 1;
  } data;
  uint8_t raw;
} apu_register_4011_t;

typedef union {
  struct __attribute__((packed)) {
    uint8_t sample_address : 8;
  } data;
  uint8_t raw;
} apu_register_4012_t;

typedef union {
  struct __attribute__((packed)) {
    uint8_t sample_length : 8;
  } data;
  uint8_t raw;
} apu_register_4013_t;

typedef union {
  struct __attribute__((packed)) {
    uint8_t enable_pulse1 : 1;
    uint8_t enable_pulse2 : 1;
    uint8_t enable_triangle : 1;
    uint8_t enable_noise : 1;
    uint8_t enable_dmc : 1;
    uint8_t : 1;
    uint8_t frame_interrupt : 1;
    uint8_t dmc_interrupt : 1;
  } data;
  uint8_t raw;
} apu_register_4015_status_t;

typedef union {
  struct __attribute__((packed)) {
    uint8_t : 6;
    uint8_t irq_inhibit : 1;
    uint8_t mode : 1;
  } data;
  uint8_t raw;
} apu_register_4017_frame_counter_t;

typedef struct apu {
  mapper_t* mapper;

  // Outputting sound
  double sample_skips;
  apu_buffer_t buffer[AUDIO_BUFFER_SIZE];
  int buffer_cursor;
  bool is_even_cycle;

  // Channels
  apu_channel_pulse_t channel_pulse1;
  apu_channel_pulse_t channel_pulse2;
  apu_channel_triangle_t channel_triangle;
  apu_channel_noise_t channel_noise;
  apu_channel_dmc_t channel_dmc;

  // Utility
  apu_register_4015_status_t previous_status;
  struct {
    bool mode_flag;  // false = 4-step, true = 5-step
    bool irq_inhibit_flag;
    uint16_t cycles;
    uint8_t cycle_index;

    bool reset_queued;
    uint8_t reset_queue_divider;
  } frame_counter;

  double lookup_pulse_table[LU_PULSE_SIZE];
  double lookup_tnd_table[LU_TND_SIZE];
} apu_t;

/**
 * Initialise the APU struct, setting all fields
 * to their default values
 */
apu_t* apu_init(void);

/**
 * Memory acces utility functions
 */
void apu_mem_write(apu_t* apu, uint16_t address, uint8_t val);
uint8_t apu_mem_read(apu_t* apu, uint16_t address);

/**
 * General output methods
 */
void apu_cycle(apu_t* apu, void* context, apu_enqueue_audio_t enqueue_audio,
               apu_get_queue_size_t get_queue_size);
