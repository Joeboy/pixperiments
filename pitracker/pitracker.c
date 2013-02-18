#include <stdint.h>
#include <stddef.h>
#include <malloc.h>
#include <stdio.h>
#include <unistd.h>
#include <config.h>

#include <lv2/lv2plug.in/ns/ext/atom/forge.h>

#ifdef RASPBERRY_PI
#include <pi/audio.h>
#include <pi/reboot.h>
#include <pi/uart.h>
#include <pi/kbhit.h>
#include <pi/hardware.h>
#define stdout 0
#endif

#ifdef LINUX
#include <linux/hardware.h>
#include <linux/audio.h>
#include <linux/kbhit.h>
#include <stdlib.h>
#endif

#include <mf_read.h>
#include <lv2.h>


int32_t notmain (uint32_t earlypc) {
    hardware_init();
    int32_t samplerate = audio_init();
    lv2_init(samplerate);
    led_init();
    switch_init();

    printf("\r\nPiTracker console\r\n");
    printf("Samplerate: %d\r\n", samplerate);

    uint32_t inkey;
    unsigned int plugin_id = 0;
    uint32_t counter=0;
    LV2_Atom_Forge_Frame midi_seq_frame;
    int buffer_processed = 0;

    lv2_port *output_left = new_lv2_port(lv2_audio_port, 1);
    lv2_port *output_right = new_lv2_port(lv2_audio_port, 2);
    lv2_port *midi_in = new_lv2_port(lv2_atom_port, 3);

    lv2_descriptors[plugin_id]->connect_port(lv2_handles[plugin_id], midi_in->id, midi_in->buffer);
    lv2_descriptors[plugin_id]->connect_port(lv2_handles[plugin_id], output_left->id, output_left->buffer);
    lv2_descriptors[plugin_id]->connect_port(lv2_handles[plugin_id], output_right->id, output_right->buffer);

    lv2_atom_forge_set_buffer(&forge,
                              midi_in->buffer,
                              LV2_ATOM_BUFFER_SIZE);
    lv2_atom_forge_sequence_head(&forge, &midi_seq_frame, 0);
    
    init_midi_source(&forge);
    
    while (1) {
#ifdef DEBUG
        if(kbhit()) {
            inkey = readch();
            //printf("%x", inkey);
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
                    lv2_descriptors[plugin_id]->connect_port(lv2_handles[plugin_id], midi_in->id, midi_in->buffer);
                    lv2_descriptors[plugin_id]->connect_port(lv2_handles[plugin_id], output_left->id, output_left->buffer);
                    lv2_descriptors[plugin_id]->connect_port(lv2_handles[plugin_id], output_right->id, output_right->buffer);
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
#endif

        if (!buffer_processed) {
            forge_midi_input();

            lv2_atom_forge_pop(&forge, &midi_seq_frame);
            lv2_descriptors[plugin_id]->run(lv2_handles[plugin_id], LV2_AUDIO_BUFFER_SIZE);
            lv2_atom_forge_set_buffer(&forge,
                                      midi_in->buffer,
                                      sizeof(uint8_t) * midi_in->buffer_sz);
            lv2_atom_forge_sequence_head(&forge, &midi_seq_frame, 0);
            buffer_processed = 1;
        }

        if (buffer_processed && audio_buffer_free_space() > LV2_AUDIO_BUFFER_SIZE * 2) {
            audio_buffer_write(output_left->buffer, output_right->buffer, output_left->buffer_sz);
            buffer_processed = 0;
            counter++;
        }
    }
}

#ifdef LINUX
int main() {
    notmain(0);
}
#endif
