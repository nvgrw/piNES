#include "cpu.h"
#include <assert.h>
#include <stdio.h>

#define PAGE_MASK 0xFF00
#define BREAK_MASK 0x10
#define UNUSED_STATUS_MASK 0x20
#define IV_NMI 0xFFFA
#define IV_RESET 0xFFFC
#define IV_IRQ_BRK 0xFFFE

#define DEBUG 1

cpu* cpu_init() {
  cpu* cpu = calloc(1, sizeof(cpu));
  cpu->memory = malloc(sizeof(uint8_t) * MEMORY_SIZE);
  cpu->register_status.raw = STATUS_DEFAULT;
  cpu->stack_pointer = STACK_DEFAULT;
  cpu->last_interrupt = INTRT_NONE;

  // Reset on power on (not done for tests)
  // cpu_interrupt(cpu, INTRT_RESET);

  return cpu;
}

void cpu_deinit(cpu* cpu) {
  free(cpu->memory);
  free(cpu);
}

void cpu_run(cpu* cpu) {
  while (!cpu->halt) {
    cpu_cycle(cpu);
  }
}

void perform_irq(cpu* cpu);
void perform_nmi(cpu* cpu);

void cpu_cycle(cpu* cpu) {
  // Handle interrupts
  switch (cpu->last_interrupt) {
    case INTRT_IRQ:
      perform_irq(cpu);
      cpu->last_interrupt = INTRT_NONE;
      break;
    case INTRT_NMI:
      perform_nmi(cpu);
      cpu->last_interrupt = INTRT_NONE;
      break;
    default:
      break;
  }

  // Fetch & Decode Instruction
  uint8_t opcode = cpu_mem_read8(cpu, cpu->program_counter);
  instruction instr = INSTRUCTION_VECTOR[opcode];
  uint8_t bytes_used = 0;
  uint16_t address = 0;
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
      address = cpu_mem_read16(cpu, cpu->program_counter + 1) + cpu->register_x;
      bytes_used = 3;
      break;
    case AM_ABSOLUTE_Y:
      address = cpu_mem_read16(cpu, cpu->program_counter + 1) + cpu->register_y;
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
                    cpu, cpu_mem_read8(cpu, cpu->program_counter + 1)) +
                cpu->register_y;
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
  instr.implementation(cpu, address);

  cpu->addressing_special = false;

  // Trap detection
  if (cpu->program_counter == last_program_counter) {
    printf("!!! CPU TRAPPED !!!\n");
    dbg_print_state(cpu);
    exit(EXIT_FAILURE);
  }

// --------- TODO REMOVE: INTERRUPT TESTING ---------
// Interrupt feedback triggering
#define FEEDBACK_PORT 0xbffc
  uint8_t feedback_port = cpu_mem_read8(cpu, FEEDBACK_PORT);
#define IRQ_MASK 0x1
#define NMI_MASK 0x2

  if ((feedback_port & NMI_MASK) == NMI_MASK) {
    feedback_port &= ~NMI_MASK;
    printf("Triggered NMI\n");
    cpu_interrupt(cpu, INTRT_NMI);
  }

  if ((feedback_port & IRQ_MASK) == IRQ_MASK) {
    feedback_port &= ~IRQ_MASK;
    printf("Triggered IRQ\n");
    cpu_interrupt(cpu, INTRT_IRQ);
  }

  cpu_mem_write8(cpu, FEEDBACK_PORT, feedback_port);

#undef IRQ_MASK
#undef NMI_MASK
#undef FEEDBACK_PORT
  // --------- TODO REMOVE: INTERRUPT TESTING ---------
}

/* Utilities */
uint8_t cpu_mem_read8(cpu* cpu, uint16_t address) {
  return cpu->memory[address];
}

void cpu_mem_write8(cpu* cpu, uint16_t address, uint8_t value) {
  cpu->memory[address] = value;
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
  return (address1 & PAGE_MASK) != (address2 & PAGE_MASK);
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

// Implementation of instructions
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
  if (!cpu->register_status.flags.c) {
    cpu->program_counter = address;
  }
}

// Branch on carry set
void cpu_impl_bcs(cpu* cpu, uint16_t address) {
  if (cpu->register_status.flags.c) {
    cpu->program_counter = address;
  }
}

// Branch on equal
void cpu_impl_beq(cpu* cpu, uint16_t address) {
  if (cpu->register_status.flags.z) {
    cpu->program_counter = address;
  }
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
  if (cpu->register_status.flags.s) {
    cpu->program_counter = address;
  }
}

// Branch on not equal
void cpu_impl_bne(cpu* cpu, uint16_t address) {
  if (!cpu->register_status.flags.z) {
    cpu->program_counter = address;
  }
}

// Branch on plus
void cpu_impl_bpl(cpu* cpu, uint16_t address) {
  if (!cpu->register_status.flags.s) {
    cpu->program_counter = address;
  }
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
  if (!cpu->register_status.flags.v) {
    cpu->program_counter = address;
  }
}

// Branch on overflow set
void cpu_impl_bvs(cpu* cpu, uint16_t address) {
  if (cpu->register_status.flags.v) {
    cpu->program_counter = address;
  }
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

// Constants
#define NULL_INSTRUCTION \
  { .mode = AM_ACCUMULATOR, .mnemonic = "###", .implementation = NULL }
const instruction INSTRUCTION_VECTOR[NUM_INSTRUCTIONS] = {
    {.mode = AM_IMPLIED, .mnemonic = "BRK", .implementation = &cpu_impl_brk},
    {.mode = AM_ZERO_PAGE_INDIRECT,
     .mnemonic = "ORA",
     .implementation = &cpu_impl_ora},
    NULL_INSTRUCTION,
    NULL_INSTRUCTION,
    NULL_INSTRUCTION,
    {.mode = AM_ZERO_PAGE, .mnemonic = "ORA", .implementation = &cpu_impl_ora},
    {.mode = AM_ZERO_PAGE, .mnemonic = "ASL", .implementation = &cpu_impl_asl},
    NULL_INSTRUCTION,
    {.mode = AM_IMPLIED, .mnemonic = "PHP", .implementation = &cpu_impl_php},
    {.mode = AM_IMMEDIATE, .mnemonic = "ORA", .implementation = &cpu_impl_ora},
    {.mode = AM_ACCUMULATOR,
     .mnemonic = "ASL",
     .implementation = &cpu_impl_asl},
    NULL_INSTRUCTION,
    NULL_INSTRUCTION,
    {.mode = AM_ABSOLUTE, .mnemonic = "ORA", .implementation = &cpu_impl_ora},
    {.mode = AM_ABSOLUTE, .mnemonic = "ASL", .implementation = &cpu_impl_asl},
    NULL_INSTRUCTION,
    {.mode = AM_RELATIVE, .mnemonic = "BPL", .implementation = &cpu_impl_bpl},
    {.mode = AM_ZERO_PAGE_INDIRECT_Y,
     .mnemonic = "ORA",
     .implementation = &cpu_impl_ora},
    NULL_INSTRUCTION,
    NULL_INSTRUCTION,
    NULL_INSTRUCTION,
    {.mode = AM_ZERO_PAGE_X,
     .mnemonic = "ORA",
     .implementation = &cpu_impl_ora},
    {.mode = AM_ZERO_PAGE_X,
     .mnemonic = "ASL",
     .implementation = &cpu_impl_asl},
    NULL_INSTRUCTION,
    {.mode = AM_IMPLIED, .mnemonic = "CLC", .implementation = &cpu_impl_clc},
    {.mode = AM_ABSOLUTE_Y, .mnemonic = "ORA", .implementation = &cpu_impl_ora},
    NULL_INSTRUCTION,
    NULL_INSTRUCTION,
    NULL_INSTRUCTION,
    {.mode = AM_ABSOLUTE_X, .mnemonic = "ORA", .implementation = &cpu_impl_ora},
    {.mode = AM_ABSOLUTE_X, .mnemonic = "ASL", .implementation = &cpu_impl_asl},
    NULL_INSTRUCTION,
    {.mode = AM_ABSOLUTE, .mnemonic = "JSR", .implementation = &cpu_impl_jsr},
    {.mode = AM_ZERO_PAGE_INDIRECT,
     .mnemonic = "AND",
     .implementation = &cpu_impl_and},
    NULL_INSTRUCTION,
    NULL_INSTRUCTION,
    {.mode = AM_ZERO_PAGE, .mnemonic = "BIT", .implementation = &cpu_impl_bit},
    {.mode = AM_ZERO_PAGE, .mnemonic = "AND", .implementation = &cpu_impl_and},
    {.mode = AM_ZERO_PAGE, .mnemonic = "ROL", .implementation = &cpu_impl_rol},
    NULL_INSTRUCTION,
    {.mode = AM_IMPLIED, .mnemonic = "PLP", .implementation = &cpu_impl_plp},
    {.mode = AM_IMMEDIATE, .mnemonic = "AND", .implementation = &cpu_impl_and},
    {.mode = AM_ACCUMULATOR,
     .mnemonic = "ROL",
     .implementation = &cpu_impl_rol},
    NULL_INSTRUCTION,
    {.mode = AM_ABSOLUTE, .mnemonic = "BIT", .implementation = &cpu_impl_bit},
    {.mode = AM_ABSOLUTE, .mnemonic = "AND", .implementation = &cpu_impl_and},
    {.mode = AM_ABSOLUTE, .mnemonic = "ROL", .implementation = &cpu_impl_rol},
    NULL_INSTRUCTION,
    {.mode = AM_RELATIVE, .mnemonic = "BMI", .implementation = &cpu_impl_bmi},
    {.mode = AM_ZERO_PAGE_INDIRECT_Y,
     .mnemonic = "AND",
     .implementation = &cpu_impl_and},
    NULL_INSTRUCTION,
    NULL_INSTRUCTION,
    NULL_INSTRUCTION,
    {.mode = AM_ZERO_PAGE_X,
     .mnemonic = "AND",
     .implementation = &cpu_impl_and},
    {.mode = AM_ZERO_PAGE_X,
     .mnemonic = "ROL",
     .implementation = &cpu_impl_rol},
    NULL_INSTRUCTION,
    {.mode = AM_IMPLIED, .mnemonic = "SEC", .implementation = &cpu_impl_sec},
    {.mode = AM_ABSOLUTE_Y, .mnemonic = "AND", .implementation = &cpu_impl_and},
    NULL_INSTRUCTION,
    NULL_INSTRUCTION,
    NULL_INSTRUCTION,
    {.mode = AM_ABSOLUTE_X, .mnemonic = "AND", .implementation = &cpu_impl_and},
    {.mode = AM_ABSOLUTE_X, .mnemonic = "ROL", .implementation = &cpu_impl_rol},
    NULL_INSTRUCTION,
    {.mode = AM_IMPLIED, .mnemonic = "RTI", .implementation = &cpu_impl_rti},
    {.mode = AM_ZERO_PAGE_INDIRECT,
     .mnemonic = "EOR",
     .implementation = &cpu_impl_eor},
    NULL_INSTRUCTION,
    NULL_INSTRUCTION,
    NULL_INSTRUCTION,
    {.mode = AM_ZERO_PAGE, .mnemonic = "EOR", .implementation = &cpu_impl_eor},
    {.mode = AM_ZERO_PAGE, .mnemonic = "LSR", .implementation = &cpu_impl_lsr},
    NULL_INSTRUCTION,
    {.mode = AM_IMPLIED, .mnemonic = "PHA", .implementation = &cpu_impl_pha},
    {.mode = AM_IMMEDIATE, .mnemonic = "EOR", .implementation = &cpu_impl_eor},
    {.mode = AM_ACCUMULATOR,
     .mnemonic = "LSR",
     .implementation = &cpu_impl_lsr},
    NULL_INSTRUCTION,
    {.mode = AM_ABSOLUTE, .mnemonic = "JMP", .implementation = &cpu_impl_jmp},
    {.mode = AM_ABSOLUTE, .mnemonic = "EOR", .implementation = &cpu_impl_eor},
    {.mode = AM_ABSOLUTE, .mnemonic = "LSR", .implementation = &cpu_impl_lsr},
    NULL_INSTRUCTION,
    {.mode = AM_RELATIVE, .mnemonic = "BVC", .implementation = &cpu_impl_bvc},
    {.mode = AM_ZERO_PAGE_INDIRECT_Y,
     .mnemonic = "EOR",
     .implementation = &cpu_impl_eor},
    NULL_INSTRUCTION,
    NULL_INSTRUCTION,
    NULL_INSTRUCTION,
    {.mode = AM_ZERO_PAGE_X,
     .mnemonic = "EOR",
     .implementation = &cpu_impl_eor},
    {.mode = AM_ZERO_PAGE_X,
     .mnemonic = "LSR",
     .implementation = &cpu_impl_lsr},
    NULL_INSTRUCTION,
    {.mode = AM_IMPLIED, .mnemonic = "CLI", .implementation = &cpu_impl_cli},
    {.mode = AM_ABSOLUTE_Y, .mnemonic = "EOR", .implementation = &cpu_impl_eor},
    NULL_INSTRUCTION,
    NULL_INSTRUCTION,
    NULL_INSTRUCTION,
    {.mode = AM_ABSOLUTE_X, .mnemonic = "EOR", .implementation = &cpu_impl_eor},
    {.mode = AM_ABSOLUTE_X, .mnemonic = "LSR", .implementation = &cpu_impl_lsr},
    NULL_INSTRUCTION,
    {.mode = AM_IMPLIED, .mnemonic = "RTS", .implementation = &cpu_impl_rts},
    {.mode = AM_ZERO_PAGE_INDIRECT,
     .mnemonic = "ADC",
     .implementation = &cpu_impl_adc},
    NULL_INSTRUCTION,
    NULL_INSTRUCTION,
    NULL_INSTRUCTION,
    {.mode = AM_ZERO_PAGE, .mnemonic = "ADC", .implementation = &cpu_impl_adc},
    {.mode = AM_ZERO_PAGE, .mnemonic = "ROR", .implementation = &cpu_impl_ror},
    NULL_INSTRUCTION,
    {.mode = AM_IMPLIED, .mnemonic = "PLA", .implementation = &cpu_impl_pla},
    {.mode = AM_IMMEDIATE, .mnemonic = "ADC", .implementation = &cpu_impl_adc},
    {.mode = AM_ACCUMULATOR,
     .mnemonic = "ROR",
     .implementation = &cpu_impl_ror},
    NULL_INSTRUCTION,
    {.mode = AM_INDIRECT, .mnemonic = "JMP", .implementation = &cpu_impl_jmp},
    {.mode = AM_ABSOLUTE, .mnemonic = "ADC", .implementation = &cpu_impl_adc},
    {.mode = AM_ABSOLUTE, .mnemonic = "ROR", .implementation = &cpu_impl_ror},
    NULL_INSTRUCTION,
    {.mode = AM_RELATIVE, .mnemonic = "BVS", .implementation = &cpu_impl_bvs},
    {.mode = AM_ZERO_PAGE_INDIRECT_Y,
     .mnemonic = "ADC",
     .implementation = &cpu_impl_adc},
    NULL_INSTRUCTION,
    NULL_INSTRUCTION,
    NULL_INSTRUCTION,
    {.mode = AM_ZERO_PAGE_X,
     .mnemonic = "ADC",
     .implementation = &cpu_impl_adc},
    {.mode = AM_ZERO_PAGE_X,
     .mnemonic = "ROR",
     .implementation = &cpu_impl_ror},
    NULL_INSTRUCTION,
    {.mode = AM_IMPLIED, .mnemonic = "SEI", .implementation = &cpu_impl_sei},
    {.mode = AM_ABSOLUTE_Y, .mnemonic = "ADC", .implementation = &cpu_impl_adc},
    NULL_INSTRUCTION,
    NULL_INSTRUCTION,
    NULL_INSTRUCTION,
    {.mode = AM_ABSOLUTE_X, .mnemonic = "ADC", .implementation = &cpu_impl_adc},
    {.mode = AM_ABSOLUTE_X, .mnemonic = "ROR", .implementation = &cpu_impl_ror},
    NULL_INSTRUCTION,
    NULL_INSTRUCTION,
    {.mode = AM_ZERO_PAGE_INDIRECT,
     .mnemonic = "STA",
     .implementation = &cpu_impl_sta},
    NULL_INSTRUCTION,
    NULL_INSTRUCTION,
    {.mode = AM_ZERO_PAGE, .mnemonic = "STY", .implementation = &cpu_impl_sty},
    {.mode = AM_ZERO_PAGE, .mnemonic = "STA", .implementation = &cpu_impl_sta},
    {.mode = AM_ZERO_PAGE, .mnemonic = "STX", .implementation = &cpu_impl_stx},
    NULL_INSTRUCTION,
    {.mode = AM_IMPLIED, .mnemonic = "DEY", .implementation = &cpu_impl_dey},
    NULL_INSTRUCTION,
    {.mode = AM_IMPLIED, .mnemonic = "TXA", .implementation = &cpu_impl_txa},
    NULL_INSTRUCTION,
    {.mode = AM_ABSOLUTE, .mnemonic = "STY", .implementation = &cpu_impl_sty},
    {.mode = AM_ABSOLUTE, .mnemonic = "STA", .implementation = &cpu_impl_sta},
    {.mode = AM_ABSOLUTE, .mnemonic = "STX", .implementation = &cpu_impl_stx},
    NULL_INSTRUCTION,
    {.mode = AM_RELATIVE, .mnemonic = "BCC", .implementation = &cpu_impl_bcc},
    {.mode = AM_ZERO_PAGE_INDIRECT_Y,
     .mnemonic = "STA",
     .implementation = &cpu_impl_sta},
    NULL_INSTRUCTION,
    NULL_INSTRUCTION,
    {.mode = AM_ZERO_PAGE_X,
     .mnemonic = "STY",
     .implementation = &cpu_impl_sty},
    {.mode = AM_ZERO_PAGE_X,
     .mnemonic = "STA",
     .implementation = &cpu_impl_sta},
    {.mode = AM_ZERO_PAGE_Y,
     .mnemonic = "STX",
     .implementation = &cpu_impl_stx},
    NULL_INSTRUCTION,
    {.mode = AM_IMPLIED, .mnemonic = "TYA", .implementation = &cpu_impl_tya},
    {.mode = AM_ABSOLUTE_Y, .mnemonic = "STA", .implementation = &cpu_impl_sta},
    {.mode = AM_IMPLIED, .mnemonic = "TXS", .implementation = &cpu_impl_txs},
    NULL_INSTRUCTION,
    NULL_INSTRUCTION,
    {.mode = AM_ABSOLUTE_X, .mnemonic = "STA", .implementation = &cpu_impl_sta},
    NULL_INSTRUCTION,
    NULL_INSTRUCTION,
    {.mode = AM_IMMEDIATE, .mnemonic = "LDY", .implementation = &cpu_impl_ldy},
    {.mode = AM_ZERO_PAGE_INDIRECT,
     .mnemonic = "LDA",
     .implementation = &cpu_impl_lda},
    {.mode = AM_IMMEDIATE, .mnemonic = "LDX", .implementation = &cpu_impl_ldx},
    NULL_INSTRUCTION,
    {.mode = AM_ZERO_PAGE, .mnemonic = "LDY", .implementation = &cpu_impl_ldy},
    {.mode = AM_ZERO_PAGE, .mnemonic = "LDA", .implementation = &cpu_impl_lda},
    {.mode = AM_ZERO_PAGE, .mnemonic = "LDX", .implementation = &cpu_impl_ldx},
    NULL_INSTRUCTION,
    {.mode = AM_IMPLIED, .mnemonic = "TAY", .implementation = &cpu_impl_tay},
    {.mode = AM_IMMEDIATE, .mnemonic = "LDA", .implementation = &cpu_impl_lda},
    {.mode = AM_IMPLIED, .mnemonic = "TAX", .implementation = &cpu_impl_tax},
    NULL_INSTRUCTION,
    {.mode = AM_ABSOLUTE, .mnemonic = "LDY", .implementation = &cpu_impl_ldy},
    {.mode = AM_ABSOLUTE, .mnemonic = "LDA", .implementation = &cpu_impl_lda},
    {.mode = AM_ABSOLUTE, .mnemonic = "LDX", .implementation = &cpu_impl_ldx},
    NULL_INSTRUCTION,
    {.mode = AM_RELATIVE, .mnemonic = "BCS", .implementation = &cpu_impl_bcs},
    {.mode = AM_ZERO_PAGE_INDIRECT_Y,
     .mnemonic = "LDA",
     .implementation = &cpu_impl_lda},
    NULL_INSTRUCTION,
    NULL_INSTRUCTION,
    {.mode = AM_ZERO_PAGE_X,
     .mnemonic = "LDY",
     .implementation = &cpu_impl_ldy},
    {.mode = AM_ZERO_PAGE_X,
     .mnemonic = "LDA",
     .implementation = &cpu_impl_lda},
    {.mode = AM_ZERO_PAGE_Y,
     .mnemonic = "LDX",
     .implementation = &cpu_impl_ldx},
    NULL_INSTRUCTION,
    {.mode = AM_IMPLIED, .mnemonic = "CLV", .implementation = &cpu_impl_clv},
    {.mode = AM_ABSOLUTE_Y, .mnemonic = "LDA", .implementation = &cpu_impl_lda},
    {.mode = AM_IMPLIED, .mnemonic = "TSX", .implementation = &cpu_impl_tsx},
    NULL_INSTRUCTION,
    {.mode = AM_ABSOLUTE_X, .mnemonic = "LDY", .implementation = &cpu_impl_ldy},
    {.mode = AM_ABSOLUTE_X, .mnemonic = "LDA", .implementation = &cpu_impl_lda},
    {.mode = AM_ABSOLUTE_Y, .mnemonic = "LDX", .implementation = &cpu_impl_ldx},
    NULL_INSTRUCTION,
    {.mode = AM_IMMEDIATE, .mnemonic = "CPY", .implementation = &cpu_impl_cpy},
    {.mode = AM_ZERO_PAGE_INDIRECT,
     .mnemonic = "CMP",
     .implementation = &cpu_impl_cmp},
    NULL_INSTRUCTION,
    NULL_INSTRUCTION,
    {.mode = AM_ZERO_PAGE, .mnemonic = "CPY", .implementation = &cpu_impl_cpy},
    {.mode = AM_ZERO_PAGE, .mnemonic = "CMP", .implementation = &cpu_impl_cmp},
    {.mode = AM_ZERO_PAGE, .mnemonic = "DEC", .implementation = &cpu_impl_dec},
    NULL_INSTRUCTION,
    {.mode = AM_IMPLIED, .mnemonic = "INY", .implementation = &cpu_impl_iny},
    {.mode = AM_IMMEDIATE, .mnemonic = "CMP", .implementation = &cpu_impl_cmp},
    {.mode = AM_IMPLIED, .mnemonic = "DEX", .implementation = &cpu_impl_dex},
    NULL_INSTRUCTION,
    {.mode = AM_ABSOLUTE, .mnemonic = "CPY", .implementation = &cpu_impl_cpy},
    {.mode = AM_ABSOLUTE, .mnemonic = "CMP", .implementation = &cpu_impl_cmp},
    {.mode = AM_ABSOLUTE, .mnemonic = "DEC", .implementation = &cpu_impl_dec},
    NULL_INSTRUCTION,
    {.mode = AM_RELATIVE, .mnemonic = "BNE", .implementation = &cpu_impl_bne},
    {.mode = AM_ZERO_PAGE_INDIRECT_Y,
     .mnemonic = "CMP",
     .implementation = &cpu_impl_cmp},
    NULL_INSTRUCTION,
    NULL_INSTRUCTION,
    NULL_INSTRUCTION,
    {.mode = AM_ZERO_PAGE_X,
     .mnemonic = "CMP",
     .implementation = &cpu_impl_cmp},
    {.mode = AM_ZERO_PAGE_X,
     .mnemonic = "DEC",
     .implementation = &cpu_impl_dec},
    NULL_INSTRUCTION,
    {.mode = AM_IMPLIED, .mnemonic = "CLD", .implementation = &cpu_impl_cld},
    {.mode = AM_ABSOLUTE_Y, .mnemonic = "CMP", .implementation = &cpu_impl_cmp},
    NULL_INSTRUCTION,
    NULL_INSTRUCTION,
    NULL_INSTRUCTION,
    {.mode = AM_ABSOLUTE_X, .mnemonic = "CMP", .implementation = &cpu_impl_cmp},
    {.mode = AM_ABSOLUTE_X, .mnemonic = "DEC", .implementation = &cpu_impl_dec},
    NULL_INSTRUCTION,
    {.mode = AM_IMMEDIATE, .mnemonic = "CPX", .implementation = &cpu_impl_cpx},
    {.mode = AM_ZERO_PAGE_INDIRECT,
     .mnemonic = "SBC",
     .implementation = &cpu_impl_sbc},
    NULL_INSTRUCTION,
    NULL_INSTRUCTION,
    {.mode = AM_ZERO_PAGE, .mnemonic = "CPX", .implementation = &cpu_impl_cpx},
    {.mode = AM_ZERO_PAGE, .mnemonic = "SBC", .implementation = &cpu_impl_sbc},
    {.mode = AM_ZERO_PAGE, .mnemonic = "INC", .implementation = &cpu_impl_inc},
    NULL_INSTRUCTION,
    {.mode = AM_IMPLIED, .mnemonic = "INX", .implementation = &cpu_impl_inx},
    {.mode = AM_IMMEDIATE, .mnemonic = "SBC", .implementation = &cpu_impl_sbc},
    {.mode = AM_IMPLIED, .mnemonic = "NOP", .implementation = &cpu_impl_nop},
    NULL_INSTRUCTION,
    {.mode = AM_ABSOLUTE, .mnemonic = "CPX", .implementation = &cpu_impl_cpx},
    {.mode = AM_ABSOLUTE, .mnemonic = "SBC", .implementation = &cpu_impl_sbc},
    {.mode = AM_ABSOLUTE, .mnemonic = "INC", .implementation = &cpu_impl_inc},
    NULL_INSTRUCTION,
    {.mode = AM_RELATIVE, .mnemonic = "BEQ", .implementation = &cpu_impl_beq},
    {.mode = AM_ZERO_PAGE_INDIRECT_Y,
     .mnemonic = "SBC",
     .implementation = &cpu_impl_sbc},
    NULL_INSTRUCTION,
    NULL_INSTRUCTION,
    NULL_INSTRUCTION,
    {.mode = AM_ZERO_PAGE_X,
     .mnemonic = "SBC",
     .implementation = &cpu_impl_sbc},
    {.mode = AM_ZERO_PAGE_X,
     .mnemonic = "INC",
     .implementation = &cpu_impl_inc},
    NULL_INSTRUCTION,
    {.mode = AM_IMPLIED, .mnemonic = "SED", .implementation = &cpu_impl_sed},
    {.mode = AM_ABSOLUTE_Y, .mnemonic = "SBC", .implementation = &cpu_impl_sbc},
    NULL_INSTRUCTION,
    NULL_INSTRUCTION,
    NULL_INSTRUCTION,
    {.mode = AM_ABSOLUTE_X, .mnemonic = "SBC", .implementation = &cpu_impl_sbc},
    {.mode = AM_ABSOLUTE_X, .mnemonic = "INC", .implementation = &cpu_impl_inc},
    NULL_INSTRUCTION,
};

#undef NULL_INSTRUCTION

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
