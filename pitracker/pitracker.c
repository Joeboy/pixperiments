#include <stdint.h>
#include <stddef.h>
#include <malloc.h>
#include <pi/hardware.h>
#include <pi/audio.c>
#include <pi/reboot.c>
#include <pi/uart.c>
#include <math.c>
#include "notefreqs.c"
#include "synth.c"
#include "midi.h"


#define PROCESS_CHUNK_SZ 32

event* events;


// Hack around our (hopefully temporary) lack of file IO
extern uint8_t _binary_tune_mid_start;
extern uint8_t _binary_tune_mid_size;

#define EOF -1
uint32_t fp = 0;

int fgetc(uint8_t *dummy) {
    uint8_t x = *(&_binary_tune_mid_start + fp);
    fp++;
    return x;
}

int h_error(unsigned int code, char* message) {
    uart_print("Error: ");
    printhex(code);
    uart_print(message);
    uart_print("\r\n");
    return 0;
}

uint32_t event_index = 0;

int h_track(int dummy, uint32_t trackno, uint32_t length) {
    events = malloc(sizeof(event) * length);
    return 0;
}

int h_midi_event(long track_time, unsigned int command, unsigned int chan, unsigned int v1, unsigned int v2) {
//    printf("status_lsb: %x\n", command);
    event e;
    switch(command) {
        case 0x90:
            e.time = (uint32_t)track_time;
            e.note_no = v1-40;
            e.type = noteon;
            events[event_index] = e;
            event_index++;
            break;
        case 0x80:
            e.time = (uint32_t)track_time;
            e.note_no = v1-40;
            e.type = noteoff;
            events[event_index] = e;
            event_index++;
            break;
        default:
            break;
    }
    return 0;
}
#include "mf_read.c"



int32_t notmain (uint32_t earlypc) {
    uart_init();
    audio_init();

    fp=0;
    uint32_t ra;
    uint32_t counter=0;
    uint32_t midi_buf_index;
    uint8_t *midi_buf = malloc(sizeof(uint8_t) * 64);
    float *process_buf = malloc(sizeof(float) * PROCESS_CHUNK_SZ);

    pause(1);
    uart_print("\r\nPiTracker console\r\n");
    synthDescriptor = NULL; // annoying workaround for gcc optimization

    const LV2_Descriptor *synthdescriptor = lv2_descriptor(0);
    LV2_Handle synth = synthdescriptor->instantiate(
                                             synthdescriptor,
                                             samplerate,
                                             NULL,
                                             NULL);

    event_index = 0;
    scan_midi();
    // reuse event_index for reading the buffer we've just written
    event_index = 0;

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
                case 0x0d:
                    uart_print("\r\n");
                    break;
                default:
                    uart_putc(ra);
                    break;
            }
        }
        synthdescriptor->connect_port(synth, MIDI_IN, midi_buf);
        synthdescriptor->connect_port(synth, AUDIO_OUT, process_buf);
        if (audio_buffer_free_space() >= PROCESS_CHUNK_SZ) {
//            uart_print("feeding buffer\r\n");
            midi_buf_index = 0;
            while (events[event_index].time == counter) {
                if (events[event_index].type == noteon) {
                    midi_buf[midi_buf_index] = NOTE_ON | 0; // channel
                } else if (events[event_index].type == noteoff) {
                    midi_buf[midi_buf_index] = NOTE_OFF | 0; // channel
                }
                midi_buf_index++;
                midi_buf[midi_buf_index] = events[event_index].note_no;
                midi_buf_index++;
                midi_buf[midi_buf_index] = 64; // velocity
                midi_buf_index++;
                event_index++;
            }

            midi_buf[midi_buf_index] = 0;
            synthdescriptor->run(synth, PROCESS_CHUNK_SZ);
            audio_buffer_write(process_buf, PROCESS_CHUNK_SZ);
            counter++;
        }
    }
}

