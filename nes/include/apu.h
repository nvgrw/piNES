#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

/* using dummy memory for now
until the cpu is done */

/**
 * Lookup tables for the channels
 */
extern const uint8_t APU_LENGTH_TABLE[];
extern const uint8_t APU_DUTY_TABLE[];
extern const uint16_t APU_NOISE_TABLE[];

/* NES is little endian */
typedef union {
  struct {
    struct {
      uint8_t envelope_period_or_volume : 4;
      uint8_t constant_volume : 1;
      uint8_t loop_or_disable_length_counter : 1;
      uint8_t duty_cycle : 2;
    } reg1;

    /* Sweep unit */
    struct {
      uint8_t shift_count : 3;
      uint8_t negative : 1;
      uint8_t period : 3;
      uint8_t enabled : 1;
    } reg2;

    struct {
      uint8_t timer_low;
    } reg3;

    struct {
      uint8_t high : 3;
      uint8_t length_counter_load : 5;
      /* also resets duty and starts envelope_period_or_volume..
          apparently */
    } reg4;
  } regs;

  uint8_t regs_raw;
} pulse_channel;

/*
typedef union {
  struct {
    struct {
      uint8_t lin_count_reload_val : 7;
      uint8_t len_count_disable : 1;
    } reg1;

    struct {
      uint8_t timer_low;
    } reg2;

    struct {
      uint8_t timer_high : 3;
      uint8_t len_count_load : 5;
    } reg3;

  } regs;

  uint8_t regs_raw;
} triangle_channel;
*/

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

} triangle_channel;

typedef union {
  struct {
    struct {
      uint8_t env_per_or_volume : 4;
      uint8_t const_volume : 1;
      uint8_t loop_env_or_dis_len : 1;
      uint8_t : 2;
    } reg1;

    struct {
      uint8_t noise_period : 2;
      uint8_t : 3;
      uint8_t loop_noise : 1;
    } reg2;

    struct {
      uint8_t : 3;
      uint8_t lenn_count_load : 5;
    } reg3;
  } regs;

  uint8_t regs_raw;
} noise_channel;

typedef union {
  struct {
    struct {
      uint8_t freq_index : 4;
      uint8_t : 2;
      uint8_t loop_sample : 1;
      uint8_t irq_enable : 1;
    } reg1;

    struct {
      uint8_t direct_load : 7;
      uint8_t : 1;
    } reg2;

    struct {
      uint8_t sample_address;
    } reg3;

  } regs;
  uint8_t regs_raw;
} dmc_channel;

typedef struct {
  uint8_t pulse1 : 1;
  uint8_t pulse2 : 1;
  uint8_t triangle : 1;
  uint8_t noise : 1;
} len_count_enable;

typedef union {
  union {
    len_count_enable len_count_enable;
    uint8_t dmc_enable : 1;
  } fields;
  uint8_t raw;
} control;

typedef union {
  union {
    len_count_enable len_count_enable;
    uint8_t dmc_interrupt : 1;
  } fields;
  uint8_t raw;
} status;

typedef union {
  struct {
    uint8_t : 6;
    uint8_t disable_frame : 1;
    uint8_t frame5_sequence : 1;
  } fields;

  uint8_t raw;
} frame_counter;

typedef struct {
  pulse_channel pulse[2];
  triangle_channel triangle;
  noise_channel noise;
  dmc_channel dmc;

  control control;
  status status;

  frame_counter frame_counter;

  uint8_t linear_counter_reload;
  uint8_t last_buff_val;
} apu;

/**
 * Initialise the APU struct, setting all fields
 * to their default values
 */
apu* apu_init(void);

/**
 * Memory acces utility functions
 */

bool apu_mem_is_valid(uint16_t address);
uint16_t apu_mem_translate(uint16_t address);
void apu_mem_write(apu* apu, uint16_t address, uint8_t val);

/**
 *  DMC methods
 */

uint8_t apu_dmc_output(apu* apuc);

/**
 * Pulse methods
 */

uint8_t apu_pulse0_output(apu* apu);
uint8_t apu_pulse1_output(apu* apu);
uint8_t apu_pulses_output(apu* apu);

/**
 * Triangle methods
 */
uint8_t apu_triangle_output(apu* apu);
/**
 * Noise methods
 */

uint8_t apu_noise_output(apu* apu);
/**
 * APU mixer methods
 */

/**
 * General output methods
 */
void apu_cycle(apu* apu);
