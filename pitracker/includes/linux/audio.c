#include <malloc.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <alsa/asoundlib.h>

#include <stdlib.h>


snd_pcm_t *playback_handle;

unsigned int audio_buffer_free_space() {
    int avail = snd_pcm_avail_update (playback_handle);
    if (avail == -EPIPE) {
        printf("EPIPE\r\n");
        return 0;
    }
    return 2 * avail;
}

void audio_buffer_write(float *audio_buf_left, float *audio_buf_right, size_t size) {
    int err;
    float ibuf[2 * size];
    for (unsigned int i=0;i<size;i++) {
        ibuf[2*i] = audio_buf_left[i];
        ibuf[2*i + 1] = audio_buf_right[i];
    }
    if ((err = snd_pcm_writei (playback_handle, &ibuf, size*2)) != size*2) {
        fprintf (stderr, "write to audio interface failed (%s)\n",
             snd_strerror (err));
        exit (1);
    }
}


int audio_init () {
    unsigned int requested_rate = 44100;
    unsigned int rate;
    snd_pcm_hw_params_t *hw_params;
    int err;

    if ((err = snd_pcm_open (&playback_handle, "plughw:0", SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        fprintf (stderr, "cannot open audio device (%s)\n", 
             snd_strerror (err));
        exit (1);
    }
       
    if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
        fprintf (stderr, "cannot allocate hardware parameter structure (%s)\n",
             snd_strerror (err));
        exit (1);
    }
             
    if ((err = snd_pcm_hw_params_any (playback_handle, hw_params)) < 0) {
        fprintf (stderr, "cannot initialize hardware parameter structure (%s)\n",
             snd_strerror (err));
        exit (1);
    }

    if ((err = snd_pcm_hw_params_set_access (playback_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
        fprintf (stderr, "cannot set access type (%s)\n",
             snd_strerror (err));
        exit (1);
    }

    if ((err = snd_pcm_hw_params_set_format (playback_handle, hw_params, SND_PCM_FORMAT_FLOAT)) < 0) {
        fprintf (stderr, "cannot set sample format (%s)\n",
             snd_strerror (err));
        exit (1);
    }

    if ((err = snd_pcm_hw_params_set_rate_near (playback_handle, hw_params, &requested_rate, 0)) < 0) {
        fprintf (stderr, "cannot set sample rate (%s)\n",
             snd_strerror (err));
        exit (1);
    }

    snd_pcm_hw_params_get_rate(hw_params, &rate, NULL);

    if ((err = snd_pcm_hw_params_set_channels (playback_handle, hw_params, 2)) < 0) {
        fprintf (stderr, "cannot set channel count (%s)\n",
             snd_strerror (err));
        exit (1);
    }

    if ((err = snd_pcm_hw_params (playback_handle, hw_params)) < 0) {
        fprintf (stderr, "cannot set parameters (%s)\n",
             snd_strerror (err));
        exit (1);
    }

    snd_pcm_hw_params_free (hw_params);
	
    if ((err = snd_pcm_prepare (playback_handle)) < 0) {
        fprintf (stderr, "cannot prepare audio interface for use (%s)\n",
             snd_strerror (err));
        exit (1);
    }
	
    return rate;
}

void audio_cleanup() {
    snd_pcm_close (playback_handle);
}
