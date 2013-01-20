#include <stdint.h>
#include <stddef.h>
#include <malloc.h>
#include <stdio.h>
#include <unistd.h>
#include <lv2/lv2plug.in/ns/ext/atom/forge.h>
#include "lv2/lv2plug.in/ns/ext/midi/midi.h"
#include <math.h>

#include <pi/audio.h>
#include <pi/reboot.c>
#include <pi/uart.h>

#include "midi.h"
#include "lv2.c"

#define MIDI_IN 0
#define AUDIO_OUT 1

#define PROCESS_CHUNK_SZ 32

event* events;
uint32_t event_index = 0;

extern unsigned int bss_start;
extern unsigned int bss_end;

extern const LV2_Descriptor *lv2_descriptor(uint32_t index);

// Hack around our (hopefully temporary) lack of file IO
extern uint8_t _binary_tune_mid_start;
extern uint8_t _binary_tune_mid_size;

#define EOF -1
uint32_t fp = 0; // midi "file pointer"

int fgetc(uint8_t *dummy) {
    uint8_t x = *(&_binary_tune_mid_start + fp);
    fp++;
    return x;
}

int h_error(unsigned int code, char* message) {
    printf("Error: ");
    dump_int_hex(code);
    printf(message);
    printf("\r\n");
    return 0;
}


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
    for(unsigned int ra=bss_start;ra<bss_end;ra+=4) PUT32(ra,0);

    uart_init();
    setup_heap();
    audio_init();
    lv2_init();

    fp=0;
    uint32_t ra;
    uint32_t counter=0;
    float *process_buf = malloc(sizeof(float) * PROCESS_CHUNK_SZ);
    uint8_t midi_ev[3];

    usleep(1);
    printf("\r\nPiTracker console\r\n");
    LV2_URID midi_midiEvent = lv2_urid_map.map(NULL, LV2_MIDI__MidiEvent);

    const LV2_Descriptor *synthdescriptor = lv2_descriptor(0);
    LV2_Handle synth = synthdescriptor->instantiate(
                                             synthdescriptor,
                                             samplerate,
                                             NULL,
                                             lv2_features);

    event_index = 0;
    scan_midi();
    // reuse event_index for reading the buffer we've just written
    event_index = 0;

    synthdescriptor->connect_port(synth, MIDI_IN, &atom_buffer);
    synthdescriptor->connect_port(synth, AUDIO_OUT, process_buf);

    while (1) {
        if(GET32(AUX_MU_LSR_REG)&0x01) {
            ra = GET32(AUX_MU_IO_REG);
//            hexstring(ra);
            switch(ra) {
                case 0x03:
                    printf("Rebooting\r\n");
                    usleep(2);
                    reboot();
                    break;
                case 0x0d:
                    printf("\r\n");
                    break;
                default:
                    uart_putc(ra);
                    break;
            }
        }
        if (audio_buffer_free_space() >= PROCESS_CHUNK_SZ) {
//            printf("feeding buffer\r\n");
            forge.offset = 0;
            LV2_Atom_Forge_Frame midi_seq_frame;
            uint8_t channel = 0;
            uint8_t frame_time = 1; // TODO

            lv2_atom_forge_sequence_head(&forge, &midi_seq_frame, 0);
            while (events[event_index].time == counter) {
                if (events[event_index].type == noteon) {
                    lv2_atom_forge_frame_time(&forge, frame_time++);
                    midi_ev[0] = NOTE_ON | channel;
                    midi_ev[1] = events[event_index].note_no;
                    midi_ev[2] = 0x50; // velocity
                    lv2_atom_forge_atom(&forge, sizeof(midi_ev), midi_midiEvent);
                    lv2_atom_forge_write(&forge, &midi_ev, 3);
                } else if (events[event_index].type == noteoff) {
                    midi_ev[0] = NOTE_OFF | channel;
                    midi_ev[1] = events[event_index].note_no;
                    midi_ev[2] = 0x1;
                    lv2_atom_forge_frame_time(&forge, frame_time++);
                    lv2_atom_forge_atom(&forge, sizeof(midi_ev), midi_midiEvent);
                    lv2_atom_forge_write(&forge, &midi_ev, 3);
                }
                event_index++;
            }
            lv2_atom_forge_pop(&forge, &midi_seq_frame);
            
            synthdescriptor->run(synth, PROCESS_CHUNK_SZ);
            audio_buffer_write(process_buf, PROCESS_CHUNK_SZ);
            counter++;
        }
    }
}

