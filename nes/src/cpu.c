#include <stdbool.h>
#include <stdio.h>

#include "cpu.h"
#include "rom.h"

#define PAGE_MASK 0xFF00
#define BREAK_MASK 0x10
#define UNUSED_STATUS_MASK 0x20
#define IV_NMI 0xFFFA
#define IV_RESET 0xFFFC
#define IV_IRQ_BRK 0xFFFE

// #define DEBUG 1

cpu* cpu_init() {
  cpu* ret = calloc(1, sizeof(cpu));
  //ret->memory = malloc(sizeof(uint8_t) * MEMORY_SIZE);
  return ret;
}

void cpu_nmi(cpu* cpu, bool nmi) {
  if (nmi && !cpu->nmi_detected) {
    cpu->nmi_detected = true;
    cpu->nmi_pending = true;
  } else if (!nmi) {
    cpu->nmi_detected = false;
  }
}

void cpu_reset(cpu* cpu) {
  cpu->register_status.raw = STATUS_DEFAULT;
  cpu->stack_pointer = STACK_DEFAULT;
  cpu->last_interrupt = INTRT_NONE;
  cpu->status = CS_NONE;
  cpu->busy = 7;
  cpu->nmi_detected = false;
  cpu->nmi_pending = false;

  // Reset on power on (not done for tests)
  cpu_interrupt(cpu, INTRT_RESET);
}

void cpu_deinit(cpu* cpu) {
  //free(cpu->memory);
  free(cpu);
}

void perform_irq(cpu* cpu);
void perform_nmi(cpu* cpu);

bool cpu_cycle(cpu* cpu) {
  cpu->status = CS_NONE;

  // Cycle only if not busy
  if (cpu->busy) {
    cpu->busy--;
    return false;
  }

  if (cpu->nmi_pending) {
    cpu->nmi_pending = false;
    cpu_interrupt(cpu, INTRT_NMI);
  }

  // Handle interrupts
  switch (cpu->last_interrupt) {
    case INTRT_IRQ:
      perform_irq(cpu);
      cpu->last_interrupt = INTRT_NONE;
      cpu->busy = 7 * 3;
      return false;
    case INTRT_NMI:
      perform_nmi(cpu);
      cpu->last_interrupt = INTRT_NONE;
      cpu->busy = 7 * 3;
      return false;
    default:
      break;
  }

  // Fetch & Decode Instruction
  uint8_t opcode = cpu_mem_read8(cpu, cpu->program_counter);
  instruction instr = INSTRUCTION_VECTOR[opcode];
  uint8_t bytes_used = 0;
  uint16_t address = 0;
  bool page_crossed = false;
  // Resolve effective address based on addressing mode
  switch (instr.mode) {
    case AM_ACCUMULATOR:
    case AM_IMPLIED:
      cpu->addressing_special = true;
      bytes_used = 1;
      break;
    case AM_IMMEDIATE:
      address = cpu->program_counter + 1;
      bytes_used = 2;
      break;
    case AM_ABSOLUTE:
      address = cpu_mem_read16(cpu, cpu->program_counter + 1);
      bytes_used = 3;
      break;
    case AM_ZERO_PAGE:
      address = cpu_mem_read8(cpu, cpu->program_counter + 1);
      bytes_used = 2;
      break;
    case AM_RELATIVE: {
      uint8_t offset = cpu_mem_read8(cpu, cpu->program_counter + 1);
      /*
       * The offset is a signed integer, so if geq than 0x80, the value is
       * actually negative.
       * PC incremented by 2 at the end, so the offset is between -126 and 129.
       */
      if (offset < 0x80) {
        address = cpu->program_counter + 2 + offset;
      } else {
        address = cpu->program_counter + 2 + offset - 0x100;
      }
      bytes_used = 2;
    } break;
    case AM_ABSOLUTE_X:
      address = cpu_mem_read16(cpu, cpu->program_counter + 1);
      page_crossed = is_page_crossed(address, address + cpu->register_x);
      address += cpu->register_x;
      bytes_used = 3;
      break;
    case AM_ABSOLUTE_Y:
      address = cpu_mem_read16(cpu, cpu->program_counter + 1);
      page_crossed = is_page_crossed(address, address + cpu->register_y);
      address += cpu->register_y;
      bytes_used = 3;
      break;
    case AM_ZERO_PAGE_X:
      address =
          (cpu_mem_read8(cpu, cpu->program_counter + 1) + cpu->register_x) &
          0xFF;
      // ^ sum wraps around zero page
      bytes_used = 2;
      break;
    case AM_ZERO_PAGE_Y:
      address =
          (cpu_mem_read8(cpu, cpu->program_counter + 1) + cpu->register_y) &
          0xFF;
      // ^ sum wraps around zero page
      bytes_used = 2;
      break;
    case AM_ZERO_PAGE_INDIRECT:
      address = cpu_mem_read16_bug(
          cpu,
          (cpu_mem_read8(cpu, cpu->program_counter + 1) + cpu->register_x) &
              0xFF);
      bytes_used = 2;
      break;
    case AM_ZERO_PAGE_INDIRECT_Y:
      address = cpu_mem_read16_bug(
                    cpu, cpu_mem_read8(cpu, cpu->program_counter + 1));
      page_crossed = is_page_crossed(address, address + cpu->register_y);
      address += cpu->register_y;
      
      bytes_used = 2;
      break;
    case AM_INDIRECT:
      address = cpu_mem_read16_bug(
          cpu, cpu_mem_read16(cpu, cpu->program_counter + 1));
      bytes_used = 3;
      break;
  }

#ifdef DEBUG
  printf(
      "Executing: %s + [%-6s] (0x%02x), pc = 0x%04x, op = 0x%04x, sp = 0x%02x, "
      "st = 0x%02x, a = 0x%02x, x = 0x%02x, y = 0x%02x\n",
      instr.mnemonic, dbg_address_mode_to_string(instr.mode), opcode,
      cpu->program_counter, address, cpu->stack_pointer,
      cpu->register_status.raw, cpu->register_a, cpu->register_x,
      cpu->register_y);
#endif

  // Execute
  uint16_t last_program_counter = cpu->program_counter;
  cpu->program_counter += bytes_used;
  cpu->branch_taken = false;
  if (instr.implementation == NULL) {
    printf("!!! UNSUPPORTED INSTRUCTION !!!\n");
    dbg_print_state(cpu);
    cpu->status = CS_UNSUPPORTED_INSTRUCTION;
    return true;
  }
  instr.implementation(cpu, address);

  cpu->addressing_special = false;

  // Trap detection
  if (cpu->program_counter == last_program_counter) {
    printf("!!! CPU TRAPPED !!!\n");
    dbg_print_state(cpu);
    cpu->status = CS_TRAPPED;
    return true;
  }

  cpu->busy += instr.cycles * 3;
  if (instr.cycle_cross && page_crossed) {
    cpu->busy += 3;
  }
  if (instr.cycle_branch && cpu->branch_taken) {
    cpu->busy += 3;
  }
  return false;
}

/* Utilities */
uint8_t cpu_mem_read8(cpu* cpu, uint16_t address) {
  return mmap_cpu_read(cpu->mapper, address, false);
  //return cpu->memory[address];
}

void cpu_mem_write8(cpu* cpu, uint16_t address, uint8_t value) {
  mmap_cpu_write(cpu->mapper, address, value);
  //cpu->memory[address] = value;
}

uint16_t cpu_mem_read16(cpu* cpu, uint16_t address) {
  return (((uint16_t)cpu_mem_read8(cpu, address + 1)) << 8) |
         (uint16_t)cpu_mem_read8(cpu, address);
}

void cpu_mem_write16(cpu* cpu, uint16_t address, uint16_t value) {
  cpu_mem_write8(cpu, address, value & 0xFF);
  cpu_mem_write8(cpu, address + 1, (value >> 8) & 0xFF);
}

uint16_t cpu_mem_read16_bug(cpu* cpu, uint16_t address) {
  if (is_page_crossed(address, address + 1)) {
    uint16_t wrapped_address = (address & PAGE_MASK) | ((address + 1) & 0xFF);
    return (((uint16_t)cpu_mem_read8(cpu, wrapped_address)) << 8) |
           ((uint16_t)cpu_mem_read8(cpu, address));
  } else {
    // If we don't cross a page, then this behaves like normal
    return cpu_mem_read16(cpu, address);
  }
}

uint8_t pop8(cpu* cpu) {
  cpu->stack_pointer++;
  return cpu_mem_read8(cpu, STACK_PAGE | cpu->stack_pointer);
}

void push8(cpu* cpu, uint8_t value) {
  cpu_mem_write8(cpu, STACK_PAGE | cpu->stack_pointer, value);
  cpu->stack_pointer--;
}

uint16_t pop16(cpu* cpu) {
  uint16_t low = pop8(cpu);
  return (pop8(cpu) << 8) | low;
}

void push16(cpu* cpu, uint16_t value) {
  push8(cpu, (value >> 8) & 0xFF);  // High byte
  push8(cpu, value & 0xFF);         // Low byte
}

bool is_page_crossed(uint16_t address1, uint16_t address2) {
  return (address1 & PAGE_MASK) != ((address1 + 1) & PAGE_MASK);
}

void cpu_interrupt(cpu* cpu, interrupt_type type) {
  if (type == INTRT_NONE) {
    return;
  }

  if (type == INTRT_RESET) {
    cpu->register_status.flags.i = 1;
    cpu->program_counter = cpu_mem_read16(cpu, IV_RESET);
  }

  if (cpu->register_status.flags.i && type == INTRT_IRQ) {
    return;  // Don't set interrupt if disabled
  }

  if (cpu->last_interrupt == INTRT_NMI) {
    // Don't service interrupts if NMI is triggered
    return;
  }

  // Perform all other interrupts later
  cpu->last_interrupt = type;
}

void perform_irq(cpu* cpu) {
  push16(cpu, cpu->program_counter);
  push8(cpu, cpu->register_status.raw | UNUSED_STATUS_MASK);
  cpu->register_status.flags.i = 1;
  cpu->program_counter = cpu_mem_read16(cpu, IV_IRQ_BRK);
}

void perform_nmi(cpu* cpu) {
  push16(cpu, cpu->program_counter);
  push8(cpu, cpu->register_status.raw | UNUSED_STATUS_MASK);
  cpu->register_status.flags.i = 1;
  cpu->program_counter = cpu_mem_read16(cpu, IV_NMI);
}

// Common to instructions
void cpu_implcommon_set_zs(cpu* cpu, uint8_t value) {
  cpu->register_status.flags.z = value == 0 ? 1 : 0;
  cpu->register_status.flags.s = value >> 7 & 0x1;
}

void cpu_implcommon_adc(cpu* cpu, uint16_t address, bool subtract) {
  uint8_t a = cpu->register_a;
  uint8_t b = cpu_mem_read8(cpu, address);
  if (subtract) {
    b = ~b;
  }

#ifdef DEBUG
  printf("%s 0x%02x +- 0x%02x\n", subtract ? "Sub" : "Add", a, b);
#endif

  uint16_t result = a + b + cpu->register_status.flags.c;
  cpu->register_a = result;

  cpu->register_status.flags.c = result > 0xFF;
  cpu->register_status.flags.v =
      ((a ^ cpu->register_a) & (b ^ cpu->register_a)) >> 7 & 0x1;
  cpu_implcommon_set_zs(cpu, cpu->register_a);
}

void cpu_implcommon_branch(cpu* cpu, uint16_t address, bool taken) {
  if (taken) {
    cpu->program_counter = address;
    cpu->branch_taken = true;
  }
}

/**
 * Implementation of instructions
 */

// Add with carry
void cpu_impl_adc(cpu* cpu, uint16_t address) {
  cpu_implcommon_adc(cpu, address, false);
}

// And with accumulator
void cpu_impl_and(cpu* cpu, uint16_t address) {
  cpu->register_a = cpu->register_a & cpu_mem_read8(cpu, address);
  cpu_implcommon_set_zs(cpu, cpu->register_a);
}

// Arithmetic shift left
void cpu_impl_asl(cpu* cpu, uint16_t address) {
  uint8_t result;
  // Either accumulator OR memory
  if (cpu->addressing_special) {
    // We are dealing with the accumulator

    // Set the carry bit to the old bit 7
    cpu->register_status.flags.c = cpu->register_a >> 7 & 0x1;

    cpu->register_a <<= 1;
    result = cpu->register_a;
  } else {
    uint8_t value = cpu_mem_read8(cpu, address);

    // Set the carry bit to the old bit 7
    cpu->register_status.flags.c = value >> 7 & 0x1;

    result = value << 1;
    cpu_mem_write8(cpu, address, result);
  }

  cpu_implcommon_set_zs(cpu, result);
}

// Branch on carry clear
void cpu_impl_bcc(cpu* cpu, uint16_t address) {
  cpu_implcommon_branch(cpu, address, !cpu->register_status.flags.c);
}

// Branch on carry set
void cpu_impl_bcs(cpu* cpu, uint16_t address) {
  cpu_implcommon_branch(cpu, address, cpu->register_status.flags.c);
}

// Branch on equal
void cpu_impl_beq(cpu* cpu, uint16_t address) {
  cpu_implcommon_branch(cpu, address, cpu->register_status.flags.z);
}

// Bit test
void cpu_impl_bit(cpu* cpu, uint16_t address) {
  uint8_t memval = cpu_mem_read8(cpu, address);
  cpu->register_status.flags.v = (memval >> 6) & 0x1;
  cpu->register_status.flags.s = (memval >> 7) & 0x1;
  cpu->register_status.flags.z = (memval & cpu->register_a) == 0 ? 1 : 0;
}

// Branch on minus
void cpu_impl_bmi(cpu* cpu, uint16_t address) {
  cpu_implcommon_branch(cpu, address, cpu->register_status.flags.s);
}

// Branch on not equal
void cpu_impl_bne(cpu* cpu, uint16_t address) {
  cpu_implcommon_branch(cpu, address, !cpu->register_status.flags.z);
}

// Branch on plus
void cpu_impl_bpl(cpu* cpu, uint16_t address) {
  cpu_implcommon_branch(cpu, address, !cpu->register_status.flags.s);
}

// Software interrupt
void cpu_impl_brk(cpu* cpu, uint16_t address) {
  push16(cpu, cpu->program_counter + 1);
  push8(cpu, cpu->register_status.raw | BREAK_MASK);
  cpu->program_counter = cpu_mem_read16(cpu, IV_IRQ_BRK);

  // Disable interrupts
  cpu->register_status.flags.i = 1;
}

// Branch on overflow clear
void cpu_impl_bvc(cpu* cpu, uint16_t address) {
  cpu_implcommon_branch(cpu, address, !cpu->register_status.flags.v);
}

// Branch on overflow set
void cpu_impl_bvs(cpu* cpu, uint16_t address) {
  cpu_implcommon_branch(cpu, address, cpu->register_status.flags.v);
}

// Clear carry
void cpu_impl_clc(cpu* cpu, uint16_t address) {
  cpu->register_status.flags.c = 0;
}

// Clear decimal
void cpu_impl_cld(cpu* cpu, uint16_t address) {
  cpu->register_status.flags.d = 0;
}

// Clear interrupt disable
void cpu_impl_cli(cpu* cpu, uint16_t address) {
  cpu->register_status.flags.i = 0;
}

// Clear overflow
void cpu_impl_clv(cpu* cpu, uint16_t address) {
  cpu->register_status.flags.v = 0;
}

void cpu_implcommon_cmp(cpu* cpu, uint8_t a, uint8_t b) {
#ifdef DEBUG
  printf("Comparing 0x%02x and 0x%02x\n", a, b);
#endif
  cpu->register_status.flags.c = a >= b ? 1 : 0;
  cpu_implcommon_set_zs(cpu, a - b);
}

// Compare with accumulator
void cpu_impl_cmp(cpu* cpu, uint16_t address) {
  cpu_implcommon_cmp(cpu, cpu->register_a, cpu_mem_read8(cpu, address));
}

// Compare with X
void cpu_impl_cpx(cpu* cpu, uint16_t address) {
  cpu_implcommon_cmp(cpu, cpu->register_x, cpu_mem_read8(cpu, address));
}

// Compare with Y
void cpu_impl_cpy(cpu* cpu, uint16_t address) {
  cpu_implcommon_cmp(cpu, cpu->register_y, cpu_mem_read8(cpu, address));
}

// Decrement
void cpu_impl_dec(cpu* cpu, uint16_t address) {
  uint8_t val = cpu_mem_read8(cpu, address) - 1;
  cpu_mem_write8(cpu, address, val);

  cpu_implcommon_set_zs(cpu, val);
}

// Decrement X
void cpu_impl_dex(cpu* cpu, uint16_t address) {
  cpu->register_x--;
  cpu_implcommon_set_zs(cpu, cpu->register_x);
}

// Decrement Y
void cpu_impl_dey(cpu* cpu, uint16_t address) {
  cpu->register_y--;
  cpu_implcommon_set_zs(cpu, cpu->register_y);
}

// Exclusive or with accumulator
void cpu_impl_eor(cpu* cpu, uint16_t address) {
  cpu->register_a ^= cpu_mem_read8(cpu, address);
  cpu_implcommon_set_zs(cpu, cpu->register_a);
}

// Increment
void cpu_impl_inc(cpu* cpu, uint16_t address) {
  uint8_t val = cpu_mem_read8(cpu, address) + 1;
  cpu_mem_write8(cpu, address, val);

  cpu_implcommon_set_zs(cpu, val);
}

// Increment X
void cpu_impl_inx(cpu* cpu, uint16_t address) {
  cpu->register_x++;

  cpu_implcommon_set_zs(cpu, cpu->register_x);
}

// Increment Y
void cpu_impl_iny(cpu* cpu, uint16_t address) {
  cpu->register_y++;

  cpu_implcommon_set_zs(cpu, cpu->register_y);
}

// Jump
void cpu_impl_jmp(cpu* cpu, uint16_t address) {
  cpu->program_counter = address;
}

// Jump to subroutine
void cpu_impl_jsr(cpu* cpu, uint16_t address) {
  push16(cpu, cpu->program_counter - 1);
  cpu->program_counter = address;
}

// Load accumulator
void cpu_impl_lda(cpu* cpu, uint16_t address) {
  cpu->register_a = cpu_mem_read8(cpu, address);
  cpu_implcommon_set_zs(cpu, cpu->register_a);
}

// Load x
void cpu_impl_ldx(cpu* cpu, uint16_t address) {
  cpu->register_x = cpu_mem_read8(cpu, address);
  cpu_implcommon_set_zs(cpu, cpu->register_x);
}

// Load y
void cpu_impl_ldy(cpu* cpu, uint16_t address) {
  cpu->register_y = cpu_mem_read8(cpu, address);
  cpu_implcommon_set_zs(cpu, cpu->register_y);
}

// Logical shift right
void cpu_impl_lsr(cpu* cpu, uint16_t address) {
  uint8_t result;
  // Either accumulator OR memory
  if (cpu->addressing_special) {
    // We are dealing with the accumulator

    // Set the carry bit to the old bit 0
    cpu->register_status.flags.c = cpu->register_a & 0x1;

    cpu->register_a >>= 1;
    result = cpu->register_a;
  } else {
    uint8_t value = cpu_mem_read8(cpu, address);

    // Set the carry bit to the old bit 0
    cpu->register_status.flags.c = value & 0x1;

    result = value >> 1;
    cpu_mem_write8(cpu, address, result);
  }

  cpu_implcommon_set_zs(cpu, result);
}

// No operation
void cpu_impl_nop(cpu* cpu, uint16_t address) {}

// Or with accumulator
void cpu_impl_ora(cpu* cpu, uint16_t address) {
  cpu->register_a |= cpu_mem_read8(cpu, address);
  cpu_implcommon_set_zs(cpu, cpu->register_a);
}

// Push accumulator
void cpu_impl_pha(cpu* cpu, uint16_t address) { push8(cpu, cpu->register_a); }

// Push processor status
void cpu_impl_php(cpu* cpu, uint16_t address) {
  push8(cpu, cpu->register_status.raw | BREAK_MASK);
}

// Pop accumulator
void cpu_impl_pla(cpu* cpu, uint16_t address) {
  cpu->register_a = pop8(cpu);
  cpu_implcommon_set_zs(cpu, cpu->register_a);
}

// Pop processor status
void cpu_impl_plp(cpu* cpu, uint16_t address) {
  cpu->register_status.raw = pop8(cpu);
  // Set the unused bit & unset bcd mode
  cpu->register_status.raw |= STATUS_DEFAULT;
}

// Rotate left
void cpu_impl_rol(cpu* cpu, uint16_t address) {
  uint8_t value;
  if (cpu->addressing_special) {
    // We're dealing with the accumulator
    value = cpu->register_a;
  } else {
    value = cpu_mem_read8(cpu, address);
  }

  uint8_t old_bit_7 = value >> 7 & 0x1;
  value <<= 1;
  // Bit 0 is filled with the current value of the carry flag
  value |= cpu->register_status.flags.c;
  cpu->register_status.flags.c = old_bit_7;

  if (cpu->addressing_special) {
    // We're dealing with the accumulator
    cpu->register_a = value;
  } else {
    cpu_mem_write8(cpu, address, value);
  }

  cpu_implcommon_set_zs(cpu, value);
}

// Rotate right
void cpu_impl_ror(cpu* cpu, uint16_t address) {
  uint8_t value;
  // Either accumulator OR memory
  if (cpu->addressing_special) {
    // We are dealing with the accumulator
    value = cpu->register_a;
  } else {
    value = cpu_mem_read8(cpu, address);
  }

  uint8_t old_bit_0 = value & 0x1;
  value >>= 1;
  // Bit 7 is filled with the current value of the carry flag
  value |= cpu->register_status.flags.c << 7;
  cpu->register_status.flags.c = old_bit_0;

  if (cpu->addressing_special) {
    // We're dealing with the accumulator
    cpu->register_a = value;
  } else {
    cpu_mem_write8(cpu, address, value);
  }

  cpu_implcommon_set_zs(cpu, value);
}

// Return from interrupt
void cpu_impl_rti(cpu* cpu, uint16_t address) {
  cpu_impl_plp(cpu, address);
  cpu->program_counter = pop16(cpu);
}

// Return from subroutine
void cpu_impl_rts(cpu* cpu, uint16_t address) {
  cpu->program_counter = pop16(cpu) + 1;
}

// Subtract with carry
void cpu_impl_sbc(cpu* cpu, uint16_t address) {
  cpu_implcommon_adc(cpu, address, true);
}

// Set carry
void cpu_impl_sec(cpu* cpu, uint16_t address) {
  cpu->register_status.flags.c = 1;
}

// Set decimal
void cpu_impl_sed(cpu* cpu, uint16_t address) {
  cpu->register_status.flags.d = 1;
}

// Set interrupt disable
void cpu_impl_sei(cpu* cpu, uint16_t address) {
  cpu->register_status.flags.i = 1;
}

// Store accumulator
void cpu_impl_sta(cpu* cpu, uint16_t address) {
  cpu_mem_write8(cpu, address, cpu->register_a);
}

// Store x
void cpu_impl_stx(cpu* cpu, uint16_t address) {
  cpu_mem_write8(cpu, address, cpu->register_x);
}

// Store y
void cpu_impl_sty(cpu* cpu, uint16_t address) {
  cpu_mem_write8(cpu, address, cpu->register_y);
}

// Transfer accumulator to x
void cpu_impl_tax(cpu* cpu, uint16_t address) {
  cpu->register_x = cpu->register_a;
  cpu_implcommon_set_zs(cpu, cpu->register_x);
}

// Transfer accumulator to y
void cpu_impl_tay(cpu* cpu, uint16_t address) {
  cpu->register_y = cpu->register_a;
  cpu_implcommon_set_zs(cpu, cpu->register_y);
}

// Transfer stack pointer to x
void cpu_impl_tsx(cpu* cpu, uint16_t address) {
  cpu->register_x = cpu->stack_pointer;
  cpu_implcommon_set_zs(cpu, cpu->register_x);
}

// Transfer x to accumulator
void cpu_impl_txa(cpu* cpu, uint16_t address) {
  cpu->register_a = cpu->register_x;
  cpu_implcommon_set_zs(cpu, cpu->register_a);
}

// Transfer x to stack pointer
void cpu_impl_txs(cpu* cpu, uint16_t address) {
  cpu->stack_pointer = cpu->register_x;
}

// Transfer y to accumulator
void cpu_impl_tya(cpu* cpu, uint16_t address) {
  cpu->register_a = cpu->register_y;
  cpu_implcommon_set_zs(cpu, cpu->register_a);
}

// Macros to define instructions
#define I(A, B, C) \
  { .mode = AM_ ## A, .mnemonic = #B, .implementation = &cpu_impl_ ## B, \
    .cycles = C, .cycle_cross = false, .cycle_branch = false },
#define IB(A, B, C) \
  { .mode = AM_ ## A, .mnemonic = #B, .implementation = &cpu_impl_ ## B, \
    .cycles = C, .cycle_cross = true, .cycle_branch = true },
#define IC(A, B, C) \
  { .mode = AM_ ## A, .mnemonic = #B, .implementation = &cpu_impl_ ## B, \
    .cycles = C, .cycle_cross = true, .cycle_branch = false },
#define NI \
  { .mode = AM_ACCUMULATOR, .mnemonic = "###", .implementation = NULL },

const instruction INSTRUCTION_VECTOR[NUM_INSTRUCTIONS] = {
    I(IMPLIED, brk, 7)
    I(ZERO_PAGE_INDIRECT, ora, 6)
    NI
    NI
    NI
    I(ZERO_PAGE, ora, 3)
    I(ZERO_PAGE, asl, 5)
    NI
    I(IMPLIED, php, 3)
    I(IMMEDIATE, ora, 2)
    I(ACCUMULATOR, asl, 2)
    NI
    NI
    I(ABSOLUTE, ora, 4)
    I(ABSOLUTE, asl, 6)
    NI
    IB(RELATIVE, bpl, 2)
    IC(ZERO_PAGE_INDIRECT_Y, ora, 5)
    NI
    NI
    NI
    I(ZERO_PAGE_X, ora, 4)
    I(ZERO_PAGE_X, asl, 6)
    NI
    I(IMPLIED, clc, 2)
    IC(ABSOLUTE_Y, ora, 4)
    NI
    NI
    NI
    IC(ABSOLUTE_X, ora, 4)
    I(ABSOLUTE_X, asl, 7)
    NI
    I(ABSOLUTE, jsr, 6)
    I(ZERO_PAGE_INDIRECT, and, 6)
    NI
    NI
    I(ZERO_PAGE, bit, 3)
    I(ZERO_PAGE, and, 3)
    I(ZERO_PAGE, rol, 5)
    NI
    I(IMPLIED, plp, 4)
    I(IMMEDIATE, and, 2)
    I(ACCUMULATOR, rol, 2)
    NI
    I(ABSOLUTE, bit, 4)
    I(ABSOLUTE, and, 4)
    I(ABSOLUTE, rol, 6)
    NI
    IB(RELATIVE, bmi, 2)
    IC(ZERO_PAGE_INDIRECT_Y, and, 5)
    NI
    NI
    NI
    I(ZERO_PAGE_X, and, 4)
    I(ZERO_PAGE_X, rol, 6)
    NI
    I(IMPLIED, sec, 2)
    IC(ABSOLUTE_Y, and, 4)
    NI
    NI
    NI
    IC(ABSOLUTE_X, and, 4)
    I(ABSOLUTE_X, rol, 7)
    NI
    I(IMPLIED, rti, 6)
    I(ZERO_PAGE_INDIRECT, eor, 6)
    NI
    NI
    NI
    I(ZERO_PAGE, eor, 3)
    I(ZERO_PAGE, lsr, 5)
    NI
    I(IMPLIED, pha, 3)
    I(IMMEDIATE, eor, 2)
    I(ACCUMULATOR, lsr, 2)
    NI
    I(ABSOLUTE, jmp, 3)
    I(ABSOLUTE, eor, 4)
    I(ABSOLUTE, lsr, 6)
    NI
    IB(RELATIVE, bvc, 2)
    IC(ZERO_PAGE_INDIRECT_Y, eor, 5)
    NI
    NI
    NI
    I(ZERO_PAGE_X, eor, 4)
    I(ZERO_PAGE_X, lsr, 6)
    NI
    I(IMPLIED, cli, 2)
    IC(ABSOLUTE_Y, eor, 4)
    NI
    NI
    NI
    IC(ABSOLUTE_X, eor, 4)
    I(ABSOLUTE_X, lsr, 7)
    NI
    I(IMPLIED, rts, 6)
    I(ZERO_PAGE_INDIRECT, adc, 6)
    NI
    NI
    NI
    I(ZERO_PAGE, adc, 3)
    I(ZERO_PAGE, ror, 5)
    NI
    I(IMPLIED, pla, 4)
    I(IMMEDIATE, adc, 2)
    I(ACCUMULATOR, ror, 2)
    NI
    I(INDIRECT, jmp, 5)
    I(ABSOLUTE, adc, 4)
    I(ABSOLUTE, ror, 6)
    NI
    IB(RELATIVE, bvs, 2)
    IC(ZERO_PAGE_INDIRECT_Y, adc, 5)
    NI
    NI
    NI
    I(ZERO_PAGE_X, adc, 4)
    I(ZERO_PAGE_X, ror, 6)
    NI
    I(IMPLIED, sei, 2)
    IC(ABSOLUTE_Y, adc, 4)
    NI
    NI
    NI
    IC(ABSOLUTE_X, adc, 4)
    I(ABSOLUTE_X, ror, 7)
    NI
    NI
    I(ZERO_PAGE_INDIRECT, sta, 6)
    NI
    NI
    I(ZERO_PAGE, sty, 3)
    I(ZERO_PAGE, sta, 3)
    I(ZERO_PAGE, stx, 3)
    NI
    I(IMPLIED, dey, 2)
    NI
    I(IMPLIED, txa, 2)
    NI
    I(ABSOLUTE, sty, 4)
    I(ABSOLUTE, sta, 4)
    I(ABSOLUTE, stx, 4)
    NI
    IB(RELATIVE, bcc, 2)
    I(ZERO_PAGE_INDIRECT_Y, sta, 6)
    NI
    NI
    I(ZERO_PAGE_X, sty, 4)
    I(ZERO_PAGE_X, sta, 4)
    I(ZERO_PAGE_Y, stx, 4)
    NI
    I(IMPLIED, tya, 2)
    I(ABSOLUTE_Y, sta, 5)
    I(IMPLIED, txs, 2)
    NI
    NI
    I(ABSOLUTE_X, sta, 5)
    NI
    NI
    I(IMMEDIATE, ldy, 2)
    I(ZERO_PAGE_INDIRECT, lda, 6)
    I(IMMEDIATE, ldx, 2)
    NI
    I(ZERO_PAGE, ldy, 3)
    I(ZERO_PAGE, lda, 3)
    I(ZERO_PAGE, ldx, 3)
    NI
    I(IMPLIED, tay, 2)
    I(IMMEDIATE, lda, 2)
    I(IMPLIED, tax, 2)
    NI
    I(ABSOLUTE, ldy, 4)
    I(ABSOLUTE, lda, 4)
    I(ABSOLUTE, ldx, 4)
    NI
    IB(RELATIVE, bcs, 2)
    IC(ZERO_PAGE_INDIRECT_Y, lda, 5)
    NI
    NI
    I(ZERO_PAGE_X, ldy, 4)
    I(ZERO_PAGE_X, lda, 4)
    I(ZERO_PAGE_Y, ldx, 4)
    NI
    I(IMPLIED, clv, 2)
    IC(ABSOLUTE_Y, lda, 4)
    I(IMPLIED, tsx, 2)
    NI
    IC(ABSOLUTE_X, ldy, 4)
    IC(ABSOLUTE_X, lda, 4)
    IC(ABSOLUTE_Y, ldx, 4)
    NI
    I(IMMEDIATE, cpy, 2)
    I(ZERO_PAGE_INDIRECT, cmp, 6)
    NI
    NI
    I(ZERO_PAGE, cpy, 3)
    I(ZERO_PAGE, cmp, 3)
    I(ZERO_PAGE, dec, 5)
    NI
    I(IMPLIED, iny, 2)
    I(IMMEDIATE, cmp, 2)
    I(IMPLIED, dex, 2)
    NI
    I(ABSOLUTE, cpy, 4)
    I(ABSOLUTE, cmp, 4)
    I(ABSOLUTE, dec, 6)
    NI
    IB(RELATIVE, bne, 2)
    IC(ZERO_PAGE_INDIRECT_Y, cmp, 5)
    NI
    NI
    NI
    I(ZERO_PAGE_X, cmp, 4)
    I(ZERO_PAGE_X, dec, 6)
    NI
    I(IMPLIED, cld, 2)
    IC(ABSOLUTE_Y, cmp, 4)
    NI
    NI
    NI
    IC(ABSOLUTE_X, cmp, 4)
    I(ABSOLUTE_X, dec, 7)
    NI
    I(IMMEDIATE, cpx, 2)
    I(ZERO_PAGE_INDIRECT, sbc, 6)
    NI
    NI
    I(ZERO_PAGE, cpx, 3)
    I(ZERO_PAGE, sbc, 3)
    I(ZERO_PAGE, inc, 5)
    NI
    I(IMPLIED, inx, 2)
    I(IMMEDIATE, sbc, 2)
    I(IMPLIED, nop, 2)
    NI
    I(ABSOLUTE, cpx, 4)
    I(ABSOLUTE, sbc, 4)
    I(ABSOLUTE, inc, 6)
    NI
    IB(RELATIVE, beq, 2)
    IC(ZERO_PAGE_INDIRECT_Y, sbc, 5)
    NI
    NI
    NI
    I(ZERO_PAGE_X, sbc, 4)
    I(ZERO_PAGE_X, inc, 6)
    NI
    I(IMPLIED, sed, 2)
    IC(ABSOLUTE_Y, sbc, 4)
    NI
    NI
    NI
    IC(ABSOLUTE_X, sbc, 4)
    I(ABSOLUTE_X, inc, 7)
    NI
};

#undef I
#undef IB
#undef IC
#undef NI

// Debugging utils
const char* dbg_address_mode_to_string(address_mode mode) {
  switch (mode) {
    case AM_ACCUMULATOR:
      return "A";
    case AM_IMPLIED:
      return "i";
    case AM_IMMEDIATE:
      return "#";
    case AM_ABSOLUTE:
      return "a";
    case AM_ZERO_PAGE:
      return "zp";
    case AM_RELATIVE:
      return "r";
    case AM_ABSOLUTE_X:
      return "a,x";
    case AM_ABSOLUTE_Y:
      return "a,y";
    case AM_ZERO_PAGE_X:
      return "zp,x";
    case AM_ZERO_PAGE_Y:
      return "zp,y";
    case AM_ZERO_PAGE_INDIRECT:
      return "(zp,x)";
    case AM_ZERO_PAGE_INDIRECT_Y:
      return "(zp),y";
    case AM_INDIRECT:
      return "(a)";
  }

  return "no string representation";
}

void dbg_print_state(cpu* cpu) {
  printf("--- Processor State ---\n");
  printf("pc : 0x%04x\n", cpu->program_counter);
  printf("sp : 0x%02x\n", cpu->stack_pointer);
  printf("a  : 0x%02x\n", cpu->register_a);
  printf("x  : 0x%02x\n", cpu->register_x);
  printf("y  : 0x%02x\n", cpu->register_y);
  printf("st : 0x%02x\n", cpu->register_status.raw);
  printf(" --> Status\n");
  printf(" 7 6 5 4 3 2 1 0\n S V   B D I Z C\n %d %d %d %d %d %d %d %d\n",
         cpu->register_status.flags.s, cpu->register_status.flags.v,
         cpu->register_status.raw >> 5 & 0x1, cpu->register_status.flags.b,
         cpu->register_status.flags.d, cpu->register_status.flags.i,
         cpu->register_status.flags.z, cpu->register_status.flags.c);
  printf(" --> Stack\n");
  for (uint16_t i = cpu->stack_pointer + 1; i <= 0xFF; i++) {
    printf(" 0x%02x: 0x%02x\n", i, cpu_mem_read8(cpu, STACK_PAGE | i));
  }
  printf("-----------------------\n");
}
