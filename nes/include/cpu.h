#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "rom.h"

/**
 * cpu.h
 * 
 * CPU struct and functions.
 */

#define MEMORY_SIZE 0x10000
#define STACK_PAGE 0x100
#define STACK_DEFAULT 0xFF
#define STATUS_DEFAULT 0x20
#define NUM_INSTRUCTIONS 0x100

/**
 * The CPU status code, used to indicate errors.
 */
typedef enum {
  CS_NONE,
  CS_TRAPPED,
  CS_UNSUPPORTED_INSTRUCTION
} cpu_status_t;

typedef enum { INTRT_NONE, INTRT_IRQ, INTRT_NMI, INTRT_RESET } interrupt_type_t;

struct jit_instruction;

/*
 * Based on specification(s) from:
 *
 * http://nesdev.com/6502.txt
 */
typedef struct {
  // Registers
  uint8_t register_a;  // Accumulation Register
  uint8_t register_x;  // Register X
  uint8_t register_y;  // Register Y

  /*
   * Status Register:
   *  Bit No.       7   6   5   4   3   2   1   0
   *                S   V       B   D   I   Z   C
   */
  union {
    struct {
      uint8_t c : 1;  // Carry Bit
      uint8_t z : 1;  // Zero Bit
      uint8_t i : 1;  // Interrupt Disable
      uint8_t d : 1;  // BCD Mode (not implementd by NES 6502)
      uint8_t b : 1;  // Software interrupt (BRK)
      uint8_t : 1;    // Unused. Should be 1 at all times
      uint8_t v : 1;  // Overflow
      uint8_t s : 1;  // Sign flag
    } flags;
    uint8_t raw;
  } register_status;

  uint16_t program_counter;
  uint8_t stack_pointer;

  // Memory
  /*
   * TODO: NES Memory maps a lot of addresses, so once the CPU functional tests
   * pass, this needs to be revised
   */
  mapper* mapper;
  uint8_t* memory;

  struct jit_instruction* compiled;

  // Misc
  bool branch_taken;
  bool nmi_detected;
  bool nmi_pending;
  cpu_status_t status;
  uint8_t last_opcode;
  uint32_t busy;
  interrupt_type_t last_interrupt;
} cpu_t;

/**
 * JIT (Just-In-Time) compilation structs
 */
typedef struct jit_instruction {
  bool compiled;
  void (*execute)(cpu_t*, uint16_t);
  uint16_t address;
  uint8_t cycles;
  uint8_t size;
} jit_instruction_t;

// Functions
cpu_t* cpu_init();
void cpu_nmi(cpu_t* cpu, bool nmi);
void cpu_reset(cpu_t* cpu);
void cpu_deinit(cpu_t* cpu);
bool cpu_cycle(cpu_t* cpu);

void cpu_interrupt(cpu_t* cpu, interrupt_type_t type);

typedef enum {
  // A: Accumulator implied as operand
  AM_ACCUMULATOR,
  // i: Implied by the instruction
  AM_IMPLIED,
  // #: Operand used directly
  AM_IMMEDIATE,
  // a: Absolute, full 16-bit address specified
  AM_ABSOLUTE,
  // zp: Zero page, address in first memory page
  AM_ZERO_PAGE,
  // r: Relative, signed offset added to PC
  AM_RELATIVE,
  // a,x: Absolute indexed with X
  AM_ABSOLUTE_X,
  // a,y: Absolute indexed with Y
  AM_ABSOLUTE_Y,
  // zp,x: Zero page indexed with X
  AM_ZERO_PAGE_X,
  // zp,y: Zero page indexed with Y
  AM_ZERO_PAGE_Y,
  // (zp,x): Zero page indexed indirect
  AM_ZERO_PAGE_INDIRECT,
  // (zp),y: Zero page indexed indirect with Y
  AM_ZERO_PAGE_INDIRECT_Y,
  // (a): Indirect
  AM_INDIRECT,
} address_mode_t;

typedef struct {
  address_mode_t mode;
  char* mnemonic;
  void (*implementation)(cpu_t* cpu, uint16_t address);
  uint8_t cycles;
  bool cycle_cross;
} instruction_t;

extern const instruction_t INSTRUCTION_VECTOR[NUM_INSTRUCTIONS];

// Debugging utils
const char* dbg_address_mode_to_string(address_mode_t mode);

void dbg_print_state(cpu_t* cpu);
