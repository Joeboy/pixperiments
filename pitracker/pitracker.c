#include "pi_hardware.h"
#include "pi_audio.c"
#include "reboot.c"
#include "uart.c"
#include "math.c"
#include "notefreqs.c"
#include "tune.c"


int transpose = 0;

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


void generate(unsigned int *buffer, unsigned int nsamples) {
    static unsigned int t=0;
    static volatile unsigned int note_iter = 0; // Without the volatile the assignment seems to get optimized out. Thanks, gcc.
    float freq;
    unsigned int i;
    unsigned int v;
    float out;
    for (i=0;i<nsamples;i++) {
//        printhex(t);
        if (notes[note_iter].on_time == t) {
            note_on(transpose + notes[note_iter].note_no, notes[note_iter].off_time);
            note_iter++;
            if (note_iter > 41) {
                note_iter = 0;
                t = 0;
            }
        }
        out=0;
        for (v=0;v<NUM_VOICES;v++) {
            if (voices[v].time != -1) {
                float volume = 10 - 3*((float)(voices[v].time & 0xfff) / 0xfff);
                if (voices[v].time < 1000) volume = 1;
                if (volume < 2) volume= 2;
                freq = voices[v].freq;// * (.993 + .014 * (float)(!(t&0x4000)));
                out += 2*volume * sin((256 * freq * 2 * t) / samplerate);
                voices[v].time++;
            }
            if (voices[v].off_time == t) {
                voices[v].time = -1;
            }
        }
        buffer[i] = 256 + (int)(out);
        t++;
    }
}


int notmain ( unsigned int earlypc ) {
    uart_init();
    audio_init();

    unsigned int ra;
    unsigned int i;
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
        if (buffer_hungry()) {
//            uart_print("feeding buffer\r\n");
            generate(buf.buffer + buf.write_p, PROCESS_CHUNK_SZ);
            buf.write_p += PROCESS_CHUNK_SZ;
            if (buf.write_p >= AUDIO_BUFFER_SZ) buf.write_p = 0;
        }// else uart_putc('-');
    }
}

