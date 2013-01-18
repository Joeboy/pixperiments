#include "stdint.h"
#include "pi_hardware.h"
#include "pi_audio.c"
#include "reboot.c"
#include "uart.c"
#include "math.c"
#include "notefreqs.c"
#include "synth.c"
#include "midi.h"


#define EVENTLIST_SIZE 1024

event events[EVENTLIST_SIZE];


// Hack around our (hopefully temporary) lack of file IO
extern uint8_t _binary_tune_mid_start;
extern uint8_t _binary_tune_mid_size;

#define NULL 0
#define EOF 999
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


int h_midi_event(long track_time, unsigned int command, unsigned int chan, unsigned int v1, unsigned int v2) {
//    printf("status_lsb: %x\n", command);
    event e;
    if (event_index >= EVENTLIST_SIZE) return 0; // ignore events that don't fit in the buffer
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
    synth_init();

    fp=0;
    uint32_t ra;
    uint32_t counter=0;
    uint32_t midi_buf_index;
    uint8_t midi_buf[64];
    float process_buf[PROCESS_CHUNK_SZ];

    pause(1);
    uart_print("\r\nPiTracker console\r\n");

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
        if (audio_buffer_hungry()) {
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
                if (event_index >= EVENTLIST_SIZE) event_index=counter=0;
            }

            midi_buf[midi_buf_index] = 0;
            synth_run(midi_buf, process_buf, PROCESS_CHUNK_SZ);
            feed_audio_buffer(process_buf, PROCESS_CHUNK_SZ);
            counter++;
        }
    }
}

