#include "pi_hardware.h"
#include "pi_audio.c"
#include "reboot.c"
#include "uart.c"
#include "math.c"
//#include "ringbuffer.c"
#include "notefreqs.c"
#include "tune.c"



#define NUM_VOICES 6

typedef struct {
    float freq;
    int time;
    unsigned int off_time;
} voice;

voice voices[NUM_VOICES];

void note_on(unsigned int note_no, unsigned int off_time) {
    // find a voice that's not on, or otherwise the voice that was
    // started longest ago
    unsigned int i;
    unsigned int best_voice = 0;
    unsigned int best_voice_age = 0;
//    hexstring(n.note_no);
//    hexstring(n.on_time);
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
    voices[best_voice].freq = noteno2freq(note_no);
    voices[best_voice].off_time = off_time;
}

int notmain ( unsigned int earlypc ) {
    uart_init();
    audio_init();

    unsigned int ra;
    unsigned int note_iter = 0, bass_note_iter=0;
    unsigned int t=0;
    unsigned int i;
    int transpose = 0;
    float out;
    float freq, bass_freq=92.5;
    for (i=0;i<4;i++) voices[i].time = -1;
    pause(1);
    uart_print("\r\nPiTracker console\r\n");
    while (1) {
        if(GET32(AUX_MU_LSR_REG)&0x01) {
            ra = GET32(AUX_MU_IO_REG);
//            hexstring(ra);
            switch(ra) {
                case 0x03:
                    uart_print("Rebooting\r\n");
                    pause(2);
                    reboot();
                    break;
                case 0x61:
                    if (transpose < 2) {
                        transpose++;
                        uart_print("transposing up\r\n");
                    }
                    break;
                case 0x7a:
                    if (transpose > -10) {
                        transpose--;
                        uart_print("transposing down\r\n");
                    }
                    break;
                case 0x0d:
                    uart_print("\r\n");
                    break;
                default:
                    uart_putc(ra);
                    break;
            }
        }

        if (!audio_fifo_unavailable()) {
            // calculate and write the next value in our output wave
            if (notes[note_iter].on_time == t) {
                note_on(transpose + notes[note_iter].note_no, notes[note_iter].off_time);
                note_iter++;
                if (note_iter > 41) {
                    note_iter = 0;
                    t = 0;
                    bass_note_iter++;
                    if (bass_note_iter >3) bass_note_iter = 0;
                    bass_freq = noteno2freq(transpose + bass_notes[bass_note_iter]);
                }
            }
            out=0;
            if (!transpose) {
                float wobble = (float)(!(t&0x2000)) - 0.5;
                out +=  15 * sin((256 * bass_freq*2 * t) / samplerate) * (1-wobble*.3);
                out +=  20 * sin((256 * bass_freq * t) / samplerate) * (1-wobble*.3);
                out +=  20 * sin((256 * bass_freq/2 * t) / samplerate) * (1+wobble*.3);
                out +=  20 * sin((256 * bass_freq/4 * t) / samplerate) * (1+wobble*.2);
                out +=  20 * sin((256 * bass_freq/8 * t) / samplerate);
            }
            for (i=0;i<NUM_VOICES;i++) {
                if (voices[i].time != -1) {
                    float volume = 10 - 3*((float)(voices[i].time & 0xfff) / 0xfff);
                    if (voices[i].time < 1000) volume = 1;
                    if (volume < 2) volume= 2;
                    freq = voices[i].freq;// * (.993 + .014 * (float)(!(t&0x4000)));
                    out += 2*volume * sin((256 * freq * 2 * t) / samplerate);
                    voices[i].time++;
                }
                if (voices[i].off_time == t) {
                    voices[i].time = -1;
                }
            }
            *(pwm+BCM2835_PWM_FIFO) = 256 + (int)(out);
            t++;
        }
    }
}

