#ifndef AUDIO_H
#define AUDIO_H

unsigned int audio_buffer_free_space();
void audio_buffer_write(float *audio_buf_left, float *audio_buf_right, size_t size);
int audio_init();

#endif
