#include "apu_channels.h"
#include "apu.h"

#define DUTY_LENGTH 8
#define DUTY_NUM 4
static const uint8_t DUTY_CYCLE_SEQUENCE[DUTY_NUM][DUTY_LENGTH] = {
    {0, 1, 0, 0, 0, 0, 0, 0},
    {0, 1, 1, 0, 0, 0, 0, 0},
    {0, 1, 1, 1, 1, 0, 0, 0},
    {1, 0, 0, 1, 1, 1, 1, 1}};

// Pulse Sequencer Timer Callback
void apu_channel_pulse_sequencer_clock(apu_timer_context_t* context) {
  apu_channel_pulse_t* channel = (apu_channel_pulse_t*)context;
  channel->current_sequence_position =
      (channel->current_sequence_position + 1) % DUTY_LENGTH;
  channel->duty_cycle_value =
      DUTY_CYCLE_SEQUENCE[channel->c_duty_cycle]
                         [channel->current_sequence_position];
}

#define TRIANGLE_LENGTH 32
static const uint8_t TRIANGLE_CYCLE_SEQUENCE[TRIANGLE_LENGTH] = {
    15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5,  4,  3,  2,  1,  0,
    0,  1,  2,  3,  4,  5,  6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

// Triangle Sequencer Timer Callback
void apu_channel_triangle_sequencer_clock(apu_timer_context_t* context) {
  apu_channel_triangle_t* channel = (apu_channel_triangle_t*)context;

  if (channel->linear_counter > 0 && channel->timer.divider > 0) {
    channel->current_sequence_position =
        (channel->current_sequence_position + 1) % TRIANGLE_LENGTH;
    channel->duty_cycle_value =
        TRIANGLE_CYCLE_SEQUENCE[channel->current_sequence_position];
  }
}

void apu_channel_triangle_linear_counter_clock(
    apu_channel_triangle_t* channel) {
  if (channel->linear_counter_reload_flag) {
    channel->linear_counter = channel->c_linear_counter_reload;
  } else if (channel->linear_counter != 0) {
    channel->linear_counter--;
  }
  if (!channel->control_flag) {
    channel->linear_counter_reload_flag = false;
  }
}

// Noise shift register timer callback
void apu_channel_noise_lfsr_clock(apu_timer_context_t* context) {
  apu_channel_noise_t* channel = (apu_channel_noise_t*)context;

  uint8_t bit0 = channel->shift_register & 0x1;
  uint8_t bit_shift =
      (channel->shift_register >> (channel->mode_flag ? 6 : 1)) & 0x1;
  channel->shift_register >>= 1;
  channel->shift_register |= (bit0 ^ bit_shift) << 14;
}
