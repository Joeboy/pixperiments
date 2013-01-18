#include "midi.h"

#define NUM_VOICES 6
#define VOICE_CLAMPER  (float)1/NUM_VOICES

enum voice_state {on, released, off};

typedef struct {
    enum voice_state state;
    uint32_t note_no;
    float freq;
    float env;
    uint32_t time;
    uint32_t released_time;
} voice;


voice voices[NUM_VOICES];

void note_on(uint32_t note_no) {
    // find a voice that's not on, or otherwise the voice that was
    // started longest ago
    uint32_t i;
    uint32_t best_voice = 0;
    uint32_t best_voice_age = 0;
    for (i=1;i<NUM_VOICES;i++) {
        if (voices[i].state == off) {
            best_voice = i;
            break;
        }
        if (voices[i].time > best_voice_age) {
            best_voice_age = voices[i].time;
            best_voice = i;
        }
    }
    voices[best_voice].state = on;
    voices[best_voice].time = 0;
    voices[best_voice].note_no = note_no;
    voices[best_voice].freq = noteno2freq(note_no);
}

void note_off(uint32_t note_no) {
    uint32_t i;
    for (i=0;i<NUM_VOICES;i++) {
        if (voices[i].note_no == note_no) {
            voices[i].state = released;
            voices[i].released_time = 0;
        }
    }
}

float envelope(voice *vp) {
    float env;
    uint32_t attack_time = 250;
    float attack = 0.9;
    uint32_t decay_time = 5000;
    float sustain = 0.2;
    uint32_t release_time = 10000;
    voice v = *vp;

    if (v.state == on) {
        if (v.time < attack_time) env = attack*((float)(v.time) / attack_time);
        else if (v.time < (attack_time + decay_time)) env = (float)attack - (float)(attack - sustain) * (float)(((float)v.time - attack_time) / (float)decay_time);
        else env = sustain;
        (*vp).env = env;
    } else if (v.state == released) {
        if (v.released_time > release_time) {
            v.state = off;
            env = 0;
        } else {
            // Ramp down from whatever the last envelope value was (not necessarily the sustain value)
            env = (float)v.env - (float)v.env*(float)((float)v.released_time / (float)release_time);
        }
    } else env = 0; // should never happen
    return env;
}

float waveform(voice v) {
    uint32_t tremolo_period = 6000;
    static uint8_t direction=1;
    static uint32_t x=1;
    float s1 = sin((256 * (uint32_t)v.freq * 2 * v.time) / (float)samplerate);
    s1 *= (float)1-0.95*(float)x/tremolo_period;
    x += (direction ? 1 : -1);
    if (x == 0 || x > tremolo_period) direction = !direction;
    float s2 = square((256 * (uint32_t)v.freq * 2 * v.time) / (float)samplerate);
    uint32_t p1 = 10000;
    if (v.time < p1) {
        return s1 + 0.07 * s2 * (1 - ((float)v.time / p1));
    }
    return s1;
}

void synth_run(uint8_t *midi_buf, float *buffer, uint32_t nsamples) {
    static uint32_t t=1; // 0 gets optimized out
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
            if (voices[v].state == off) continue;
            out += VOICE_CLAMPER * envelope(&(voices[v])) * waveform(voices[v]);

            voices[v].time++;
            if (voices[v].state == released) voices[v].released_time++;
        }
        buffer[i] = out;
        t++;
    }
}


void synth_init() {
    uint32_t i;
    for (i=0;i<NUM_VOICES;i++) voices[i].state = off;
}
