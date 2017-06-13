#include <math.h>
#include <stdio.h>

#include "apu.h"
#include "cpu.h"

apu_t* apu_init(void) { return calloc(1, sizeof(apu_t)); }

void apu_mem_write(apu_t* apu, uint16_t address, uint8_t val) {}

static void apu_write_to_buffer(apu_t* apu, uint8_t value) {
  apu->buffer[apu->buffer_cursor] = value;
  apu->buffer_cursor++;
  apu->buffer_cursor = apu->buffer_cursor % AUDIO_BUFFER_SIZE;
}

void apu_cycle(apu_t* apu, void* context,
               void (*enqueue_audio)(void* context, uint8_t* buffer, int len)) {
  // This function is called at the rate of the PPU. Only do something useful
  // every 3 cycles.

  if (apu->cycle_count != 0) {
    apu->cycle_count--;
    return;
  }

  apu->cycle_count = 2;

  // Frame counter
  if (apu->is_even_cycle) {
  }
  apu->is_even_cycle = !apu->is_even_cycle;

  uint8_t val = 0;

  // Create output
  // uint8_t val = apu_pulses_output(apu) + apu_tnd_output(apu);

  // Skip samples/ downsample
  if (apu->sample_skips <= 1.0) {
    apu_write_to_buffer(apu, val);
    if (apu->buffer_cursor == 0) {
      enqueue_audio(context, apu->buffer, AUDIO_BUFFER_SIZE);
    }
    apu->sample_skips += APU_SAMPLE_RATE / APU_ACTUAL_SAMPLE_RATE;
  } else {
    apu->sample_skips -= 1.0;
  }
}
