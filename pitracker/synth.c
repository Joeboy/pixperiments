#include "midi.h"

#define NUM_VOICES 6

typedef struct {
    uint32_t note_no;
    float freq;
    int32_t time;
} voice;


voice voices[NUM_VOICES];

void note_on(uint32_t note_no) {
    // find a voice that's not on, or otherwise the voice that was
    // started longest ago
    uint32_t i;
    uint32_t best_voice = 0;
    uint32_t best_voice_age = 0;
    for (i=1;i<NUM_VOICES;i++) {
        if (voices[i].time == -1) {
            best_voice = i;
            break;
        }
        if (voices[i].time > best_voice_age) {
            best_voice_age = voices[i].time;
            best_voice = i;
        }
    }
    voices[best_voice].time = 0;
    voices[best_voice].note_no = note_no;
    voices[best_voice].freq = noteno2freq(note_no);
}

void note_off(uint32_t note_no) {
    uint32_t i;
    for (i=0;i<NUM_VOICES;i++) {
        if (voices[i].note_no == note_no) {
            voices[i].time = -1;
        }
    }
}


void generate(uint8_t *midi_buf, uint32_t *buffer, uint32_t nsamples) {
    static uint32_t t=0;
    float freq;
    uint32_t i;
    uint32_t v;
    float out;

    uint32_t midi_buf_index;
    for (midi_buf_index=0;midi_buf[midi_buf_index] != 0; midi_buf_index += 3) {
        switch (midi_buf[midi_buf_index] & 0xf0) {
            case NOTE_ON:
                note_on(midi_buf[midi_buf_index+1]);
                break;
            case NOTE_OFF:
                note_off(midi_buf[midi_buf_index+1]);
                break;
            default:
                break;
        }
    }

    for (i=0;i<nsamples;i++) {
        out=0;
        for (v=0;v<NUM_VOICES;v++) {
            if (voices[v].time != -1) {
                float volume = 10 - 2*((float)(voices[v].time & 0xfff) / 0xfff);
                if (voices[v].time < 1000) volume = 1;
                if (volume < 2) volume= 2;
                freq = voices[v].freq;
                out += 2*volume * sin((256 * (uint32_t)freq * 2 * t) / (float)samplerate);
                voices[v].time++;
            }
        }
        buffer[i] = 256 + (int32_t)(2*out);
        t++;
    }
}
