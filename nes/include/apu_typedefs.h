#pragma once

typedef int8_t apu_buffer_t;
typedef int32_t apu_queued_size_t;
typedef void (*apu_enqueue_audio_t)(void* context, apu_buffer_t* buffer,
                                    int len);
typedef apu_queued_size_t (*apu_get_queue_size_t)(void* context);
