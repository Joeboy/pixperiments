#include <stdint.h>
#include <stddef.h>
#include <malloc.h>
#include <stdio.h>
#include <unistd.h>

#include "mf_read.h"

#include <lv2/lv2plug.in/ns/ext/atom/forge.h>

#ifdef RASPBERRY_PI
#include <pi/audio.h>
#include <pi/reboot.h>
#include <pi/uart.h>
#include <pi/kbhit.h>
#include <pi/hardware.h>
#define EOF -1
#define stdout 0
#endif

#ifdef LINUX
#include <linux/hardware.h>
#include <linux/audio.h>
#include <linux/kbhit.h>
#include <stdlib.h>
#endif

#include <lv2.h>

#define LV2_AUDIO_BUFFER_SIZE 0x40
#define LV2_MIDI_BUFFER_SIZE 256


enum lv2_port_type { audio, atom };

typedef struct {
    enum lv2_port_type type;
    uint32_t id;
    void *buffer;
    size_t buffer_sz;
} lv2_port;


// Hack around our (hopefully temporary) lack of file IO
extern uint8_t _binary_tune_mid_start;


int32_t notmain (uint32_t earlypc) {
    hardware_init();
    int32_t samplerate = audio_init();
    lv2_init(samplerate);
    led_init();
    switch_init();

    printf("\r\nPiTracker console\r\n");
    printf("Samplerate: %d\r\n", samplerate);

    uint32_t inkey;
    unsigned int switch_countdown = 0; // switch debouncing timer
    unsigned int plugin_id = 0;
    uint32_t counter=0;
    unsigned int midi_stream_index = 0;
    unsigned int stream_msecs = 0;
    unsigned int msecs = 0;
    LV2_Atom_Forge_Frame midi_seq_frame;
    int buffer_processed = 0;

    lv2_port output_left, output_right, midi_in;
    output_left.id = 1;
    output_left.type = audio;
    output_left.buffer = malloc(sizeof(float) * LV2_AUDIO_BUFFER_SIZE);
    output_left.buffer_sz = LV2_AUDIO_BUFFER_SIZE;
    output_right.id = 2;
    output_right.type = audio;
    output_right.buffer = malloc(sizeof(float) * LV2_AUDIO_BUFFER_SIZE);
    output_right.buffer_sz = LV2_AUDIO_BUFFER_SIZE;
    midi_in.id = 3;
    midi_in.type = atom;
    midi_in.buffer = malloc(sizeof(uint8_t) * LV2_MIDI_BUFFER_SIZE);
    midi_in.buffer_sz = LV2_MIDI_BUFFER_SIZE;

    lv2_descriptors[plugin_id]->connect_port(lv2_handles[plugin_id], midi_in.id, midi_in.buffer);
    lv2_descriptors[plugin_id]->connect_port(lv2_handles[plugin_id], output_left.id, output_left.buffer);
    lv2_descriptors[plugin_id]->connect_port(lv2_handles[plugin_id], output_right.id, output_right.buffer);

    lv2_atom_forge_set_buffer(&forge,
                              midi_in.buffer,
                              LV2_MIDI_BUFFER_SIZE);
    lv2_atom_forge_sequence_head(&forge, &midi_seq_frame, 0);
    
    midi_parser *parser = new_midi_parser(&forge);

    while (1) {
        if (switch_countdown) switch_countdown--;
        else {
            if (get_switch_state()) {
                switch_countdown = 500000;
                plugin_id++;
                if (plugin_id > 2) plugin_id = 0;
                    lv2_descriptors[plugin_id]->connect_port(lv2_handles[plugin_id], output_left.id, output_left.buffer);
                    lv2_descriptors[plugin_id]->connect_port(lv2_handles[plugin_id], output_right.id, output_right.buffer);
            }
        }

        if(kbhit()) {
            inkey = readch();
//            printf("%x", inkey);
            switch(inkey) {
                case 0x03:
#ifdef RASPBERRY_PI
                    printf("Rebooting\r\n");
                    usleep(2);
                    reboot();
#else
                    printf("Exiting\r\n");
                    close_keyboard();
                    exit(0);
#endif
                    break;
                case 0x31:
                    plugin_id++;
                    if (plugin_id > 2) plugin_id = 0;
                    lv2_descriptors[plugin_id]->connect_port(lv2_handles[plugin_id], midi_in.id, midi_in.buffer);
                    lv2_descriptors[plugin_id]->connect_port(lv2_handles[plugin_id], output_left.id, output_left.buffer);
                    lv2_descriptors[plugin_id]->connect_port(lv2_handles[plugin_id], output_right.id, output_right.buffer);
                    break;
                case 0x0d:
                    printf("\r\n");
                    break;
                default:
                    putc(inkey, stdout);
#ifdef LINUX
                    fflush(stdout);
#endif
                    break;
            }
        }

        if (!buffer_processed) {
            while (stream_msecs <= msecs) {
                stream_msecs = process_midi_byte(parser, (uint8_t)*(&_binary_tune_mid_start + midi_stream_index++));
            }

            lv2_atom_forge_pop(&forge, &midi_seq_frame);
            lv2_descriptors[plugin_id]->run(lv2_handles[plugin_id], LV2_AUDIO_BUFFER_SIZE);
            lv2_atom_forge_set_buffer(&forge,
                                      midi_in.buffer,
                                      LV2_MIDI_BUFFER_SIZE);
            lv2_atom_forge_sequence_head(&forge, &midi_seq_frame, 0);
            buffer_processed = 1;
        }

        if (buffer_processed && audio_buffer_free_space() > LV2_AUDIO_BUFFER_SIZE * 2) {
            audio_buffer_write(output_left.buffer, output_right.buffer, output_left.buffer_sz);
            buffer_processed = 0;
            counter++;
            // Not sure why 500 rather than 1000 here. Something is screwey:
            msecs = (float)500 * (float)counter * (float)LV2_AUDIO_BUFFER_SIZE / (float)samplerate;
        }
    }
}

#ifdef LINUX
int main() {
    notmain(0);
}
#endif
