#ifdef __cplusplus
extern "C" {
#endif

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
#endif

#ifdef LINUX
#include <linux/hardware.h>
#include <linux/audio.h>
#include <linux/kbhit.h>
#include <stdlib.h>
#endif

#include <mf_read.h>
#include <lv2.h>


#include <pi/debug.h>

int32_t notmain (uint32_t earlypc) {
    hardware_init();
    debug("\r\nPiTracker console\r\n");

    int32_t samplerate = audio_init();
    debug("Samplerate: %d\r\n", samplerate);
    led_init();
    switch_init();

    Lv2World *lv2_world = lv2_init(samplerate);
    debug("Lv2World created\r\n");

    uint32_t inkey;
    uint32_t counter=0;
    LV2_Atom_Forge_Frame midi_seq_frame;
    int buffer_processed = 0;
    const Lv2Plugin *plugin;
    plugin = lv2_world->plugin_list + 1;

    lv2_port *midi_in, *output_left, *output_right;
    if (plugin->midi_input_port_index != -1) {
        midi_in = new_lv2_port(lv2_atom_port, plugin->midi_input_port_index);
        plugin->descriptor->connect_port(plugin->handle, midi_in->index, midi_in->buffer);
    }
    output_left = new_lv2_port(lv2_audio_port, plugin->audio_output_left_port_index);
    plugin->descriptor->connect_port(plugin->handle, output_left->index, output_left->buffer);
    output_right = new_lv2_port(lv2_audio_port, plugin->audio_output_right_port_index);
    plugin->descriptor->connect_port(plugin->handle, output_right->index, output_right->buffer);

    init_midi_source(lv2_world);
    
    while (1) {
#ifdef DEBUG
        if(kbhit()) {
            inkey = readch();
            //debug("%x", inkey);
            switch(inkey) {
                case 0x03:
#ifdef RASPBERRY_PI
                    debug("Rebooting\r\n");
                    usleep(2);
                    reboot();
#else
                    debug("Exiting\r\n");
                    close_keyboard();
                    exit(0);
#endif
                    break;
                case 0x31:
//                    if (plugin->next) plugin = plugin->next;
//                    else plugin = lv2_world->plugin_list;
//                    plugin->descriptor->connect_port(plugin->handle, midi_in->id, midi_in->buffer);
//                    plugin->descriptor->connect_port(plugin->handle, output_left->id, output_left->buffer);
//                    plugin->descriptor->connect_port(plugin->handle, output_right->id, output_right->buffer);
                    break;
                case 0x0d:
                    debug("\r\n");
                    break;
                default:
                    uart_putc(inkey);
#ifdef LINUX
                    fflush(stdout);
#endif
                    break;
            }
        }
#endif

        if (!buffer_processed) {
            if (plugin->midi_input_port_index != -1) {
                lv2_atom_forge_set_buffer(&lv2_world->forge,
                                          (uint8_t*)midi_in->buffer,
                                          LV2_ATOM_BUFFER_SIZE);
                lv2_atom_forge_sequence_head(&lv2_world->forge, &midi_seq_frame, 0);
                forge_midi_input();
                lv2_atom_forge_pop(&lv2_world->forge, &midi_seq_frame);
            }
            plugin->descriptor->run(plugin->handle, LV2_AUDIO_BUFFER_SIZE);

            buffer_processed = 1;
        }

        if (buffer_processed && audio_buffer_free_space() > LV2_AUDIO_BUFFER_SIZE * 2) {
            audio_buffer_write((float*)output_left->buffer, (float*)output_right->buffer, output_left->buffer_sz);
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
#ifdef __cplusplus
}
#endif
