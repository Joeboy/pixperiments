#include <stdint.h>
#include <stddef.h>
#include <malloc.h>
#include <stdio.h>
#include <unistd.h>

#include <lv2/lv2plug.in/ns/lv2core/lv2.h>
#include <lv2/lv2plug.in/ns/ext/atom/forge.h>
#include "lv2/lv2plug.in/ns/ext/midi/midi.h"

#ifdef RASPBERRY_PI
#include <pi/audio.h>
#include <pi/reboot.c>
#include <pi/uart.h>
#include <pi/hardware.h>
#endif

#include <lv2.h>

#define PROCESS_CHUNK_SZ 32

#define MIDI_IN 0
#define AUDIO_OUT 1

static LV2_URID midi_midiEvent;

enum event_type { noteon, noteoff };

typedef struct {
    uint32_t time;
    uint32_t note_no;
    enum event_type type;
} event;

static event* events;

static uint32_t event_index = 0;

static float *process_buf;


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


static void connect_ports(unsigned int plugin_id) {
    lv2_descriptors[plugin_id]->connect_port(lv2_handles[plugin_id], MIDI_IN, &atom_buffer);
    lv2_descriptors[plugin_id]->connect_port(lv2_handles[plugin_id], AUDIO_OUT, process_buf);
    printf(lv2_descriptors[plugin_id]->URI);
    printf("\r\n");
}

static void note_on_or_off(unsigned int command, uint8_t channel,
                           uint32_t frame_time, uint8_t note_no,
                           uint8_t velocity) {
    uint8_t midi_ev[3];
    midi_ev[0] = command | channel;
    midi_ev[1] = note_no;
    midi_ev[2] = velocity;
    lv2_atom_forge_frame_time(&forge, frame_time++);
    lv2_atom_forge_atom(&forge, sizeof(midi_ev), midi_midiEvent);
    lv2_atom_forge_write(&forge, &midi_ev, 3);
}


int32_t notmain (uint32_t earlypc) {
    hardware_init();
    uart_init();
    audio_init();
    lv2_init();
    led_init();
    switch_init();

    uint32_t inkey;
    unsigned int switch_countdown = 0; // switch debouncing timer
    uint32_t counter=0;
    unsigned int tune_playing = 1;
    unsigned int all_notes_off_i = 0;
    unsigned int led_timer = 0;
    unsigned int plugin_id = 0;

    printf("\r\nPiTracker console\r\n");
    printf("Use keys 1-3 to switch between plugins\r\n");
    printf("Q turns tune off\r\n");

    process_buf = malloc(sizeof(float) * PROCESS_CHUNK_SZ);

    midi_midiEvent = lv2_urid_map.map(NULL, LV2_MIDI__MidiEvent);
    LV2_Atom_Forge_Frame midi_seq_frame;

    event_index = 0;
    scan_midi();
    // reuse event_index for reading the buffer we've just written
    event_index = 0;

    connect_ports(plugin_id);

    while (1) {
        if (switch_countdown) switch_countdown--;
        else {
            if (get_switch_state()) {
                switch_countdown = 500000;
                plugin_id++;
                if (plugin_id > 2) plugin_id = 0;
                connect_ports(plugin_id);
            }
        }

        if(uart_input_ready()) {
            inkey = uart_read();
//            dump_int_hex(inkey);
            switch(inkey) {
                case 0x03:
                    printf("Rebooting\r\n");
                    usleep(2);
                    reboot();
                    break;
                case 0x31:
                    plugin_id = 0;
                    connect_ports(plugin_id);
                    break;
                case 0x32:
                    plugin_id = 1;
                    connect_ports(plugin_id);
                    break;
                case 0x33:
                    plugin_id = 2;
                    connect_ports(plugin_id);
                    break;
                case 0x71:
                    tune_playing = !tune_playing;
                    if (tune_playing) {
                        printf("Tune on\r\n");
                        // TODO
                    } else {
                        printf("Tune off\r\n");
                        all_notes_off_i = 1;
                    }
                    break;
                case 0x0d:
                    printf("\r\n");
                    break;
                default:
                    uart_putc(inkey);
                    break;
            }
        }
        if (audio_buffer_free_space() >= PROCESS_CHUNK_SZ) {
//            printf("feeding buffer\r\n");
            forge.offset = 0;
            uint8_t channel = 0;
            uint32_t frame_time = 1; // TODO

            lv2_atom_forge_sequence_head(&forge, &midi_seq_frame, 0);
            while (tune_playing && events[event_index].time == counter) {
                if (events[event_index].type == noteon) {
                    led_on();
                    led_timer = 1;
                    note_on_or_off(LV2_MIDI_MSG_NOTE_ON, channel, frame_time, events[event_index].note_no, 0x50);
                } else if (events[event_index].type == noteoff) {
                    PUT32(GPCLR0,1<<16);
                    led_timer = 1;
                    note_on_or_off(LV2_MIDI_MSG_NOTE_OFF, channel, frame_time, events[event_index].note_no, 0x1);
                }
                event_index++;
            }
            if (all_notes_off_i) {
                note_on_or_off(LV2_MIDI_MSG_NOTE_OFF, 0, 1, all_notes_off_i++, 0x1);
                if (all_notes_off_i == 127) all_notes_off_i = 0;
            }
            if (led_timer) {
                if (led_timer++ > 10) {
                    led_timer = 0;
                    led_off();
                }
            }
            lv2_atom_forge_pop(&forge, &midi_seq_frame);
            
            lv2_descriptors[plugin_id]->run(lv2_handles[plugin_id], PROCESS_CHUNK_SZ);
            audio_buffer_write(process_buf, PROCESS_CHUNK_SZ);
            counter++;
        }
    }
}

