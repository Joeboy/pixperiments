#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>

#define AUDIO_BUFFER_SZ 128
uint32_t samplerate;

#define SET_GPIO_ALT(g,a) *(gpio+(((g)/10))) |= (((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3))

void ringbuffer_init();

int32_t audio_buffer_free_space();

void audio_buffer_write(float*chunk, uint32_t chunk_sz);

void audio_init();

#endif
