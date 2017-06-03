#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define MEMORY_SIZE 0x10000
#define STACK_PAGE 0x100
#define STACK_DEFAULT 0xFF
#define STATUS_DEFAULT 0x20
#define NUM_INSTRUCTIONS 0x100

typedef enum { INTRT_NONE, INTRT_IRQ, INTRT_NMI, INTRT_RESET } interrupt_type;

/*
 * Based on specification(s) from:
 *
 * http://nesdev.com/6502.txt
 */
typedef struct {
  /* Registers */
  uint8_t register_a; /* Accumulation Register */
  uint8_t register_x; /* Register X */
  uint8_t register_y; /* Register Y */

  /*
   * Status Register:
   *  Bit No.       7   6   5   4   3   2   1   0
   *                S   V       B   D   I   Z   C
   */
  union {
    struct {
      uint8_t c : 1; /* Carry Bit */
      uint8_t z : 1; /* Zero Bit */
      uint8_t i : 1; /* Interrupt Disable */
      uint8_t d : 1; /* BCD Mode (not implementd by NES 6502) */
      uint8_t b : 1; /* Software interrupt (BRK) */
      uint8_t : 1;   /* Unused. Should be 1 at all times */
      uint8_t v : 1; /* Overflow */
      uint8_t s : 1; /* Sign flag */
    } flags;
    uint8_t raw;
  } register_status;

  uint16_t program_counter;
  uint8_t stack_pointer;

  /* Memory */
  /*
   * TODO: NES Memory maps a lot of addresses, so once the CPU functional tests
   * pass, this needs to be revised
   */
  uint8_t* memory;

  /* Misc */
  bool halt;
  bool addressing_special;
  interrupt_type last_interrupt;
} cpu;

/* Opcode Vector */
/* Opcode Handler Vector */
/* Address Mode Vector */

/* Functions */
cpu* cpu_init();
void cpu_deinit(cpu* cpu);
void cpu_run(cpu* cpu);
void cpu_cycle(cpu* cpu);

/* Utilities */
uint8_t cpu_mem_read8(cpu* cpu, uint16_t address);
void cpu_mem_write8(cpu* cpu, uint16_t address, uint8_t value);
uint16_t cpu_mem_read16(cpu* cpu, uint16_t address);
void cpu_mem_write16(cpu* cpu, uint16_t address, uint16_t value);

/**
 * The 6502 has a bug where reading a 16 byte value that crosses a page will
 * wrap around the page instead
 */
uint16_t cpu_mem_read16_bug(cpu* cpu, uint16_t address);

uint8_t pop8(cpu* cpu);
void push8(cpu* cpu, uint8_t value);
uint16_t pop16(cpu* cpu);
void push16(cpu* cpu, uint16_t value);

bool is_page_crossed(uint16_t address1, uint16_t address2);

void cpu_interrupt(cpu* cpu, interrupt_type type);

typedef enum {
  /* A: Accumulator implied as operand */
  AM_ACCUMULATOR,
  /* i: Implied by the instruction */
  AM_IMPLIED,
  /* #: Operand used directly */
  AM_IMMEDIATE,
  /* a: Absolute, full 16-bit address specified */
  AM_ABSOLUTE,
  /* zp: Zero page, address in first memory page */
  AM_ZERO_PAGE,
  /* r: Relative, signed offset added to PC */
  AM_RELATIVE,
  /* a,x: Absolute indexed with X */
  AM_ABSOLUTE_X,
  /* a,y: Absolute indexed with Y */
  AM_ABSOLUTE_Y,
  /* zp,x: Zero page indexed with X */
  AM_ZERO_PAGE_X,
  /* zp,y: Zero page indexed with Y */
  AM_ZERO_PAGE_Y,
  /* (zp,x): Zero page indexed indirect */
  AM_ZERO_PAGE_INDIRECT,
  /* (zp),y: Zero page indexed indirect with Y */
  AM_ZERO_PAGE_INDIRECT_Y,
  /* (a): Indirect */
  AM_INDIRECT,
} address_mode;

typedef struct {
  address_mode mode;
  char* mnemonic;
  void (*implementation)(cpu* cpu, uint16_t address);
} instruction;

extern const instruction INSTRUCTION_VECTOR[NUM_INSTRUCTIONS];
