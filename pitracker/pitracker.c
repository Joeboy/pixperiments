#include "stdint.h"
#include "pi_hardware.h"
#include "pi_audio.c"
#include "reboot.c"
#include "uart.c"
#include "math.c"
#include "notefreqs.c"
#include "tune.c"


int32_t transpose = 0;

#define NUM_VOICES 4

typedef struct {
    float freq;
    int32_t time;
} voice;

#define NOTE_OFF                0x80
#define NOTE_ON                 0x90

uint8_t midi_buf[64];

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
    voices[best_voice].freq = noteno2freq(note_no);
}


void generate(uint32_t *buffer, uint32_t nsamples) {
    static uint32_t t=0;
    float freq;
    uint32_t i;
    uint32_t v;
    float out;

    uint32_t midi_buf_index;
    for (midi_buf_index=0;midi_buf[midi_buf_index] != 0; midi_buf_index += 3) {
        if ((midi_buf[midi_buf_index] & 0xf0) == NOTE_ON) {
            note_on(midi_buf[midi_buf_index+1]);
        }
    }

    for (i=0;i<nsamples;i++) {
        out=0;
        for (v=0;v<NUM_VOICES;v++) {
            if (voices[v].time != -1) {
                float volume = 10 - 3*((float)(voices[v].time & 0xfff) / 0xfff);
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


int32_t notmain (uint32_t earlypc) {
    uart_init();
    audio_init();

    uint32_t ra;
    uint32_t i;
    uint32_t note_index = 0;
    uint32_t counter=0;
    uint32_t midi_buf_index;
    for (i=0;i<NUM_VOICES;i++) voices[i].time = -1;
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
            midi_buf_index = 0;
            while (notes[note_index].on_time == counter) {
                midi_buf[midi_buf_index] = NOTE_ON | 0; // channel
                midi_buf_index++;
                midi_buf[midi_buf_index] = notes[note_index].note_no;
                midi_buf_index++;
                midi_buf[midi_buf_index] = 64; // velocity
                midi_buf_index++;
                note_index++;
                if (note_index > 41) note_index=counter=0;
            }

            midi_buf[midi_buf_index] = 0;
            generate(buf.buffer + buf.write_p, PROCESS_CHUNK_SZ);
            buf.write_p += PROCESS_CHUNK_SZ;
            if (buf.write_p >= AUDIO_BUFFER_SZ) buf.write_p = 0;
            counter++;
        }
    }
}

