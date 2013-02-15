#include <mf_read.h>

#include <stdio.h>
#include <malloc.h>

#include <lv2/lv2plug.in/ns/ext/atom/forge.h>
#include <lv2/lv2plug.in/ns/ext/midi/midi.h>
#include <lv2.h>


#define MThd 0x4d546864
#define MTrk 0x4d54726b
#define me_end_of_track            0x2f

static char *nparms = "\2\2\2\2\1\1\2";
#define numparms(s) (nparms[(s & 0x70)>>4])

// Not sure what the "right" size for this would be. 8 seems harmless for my purposes.
#define BUFFER_SZ 8


static uint32_t make_number(uint8_t* buf, unsigned int num_bytes) {
    // Assemble correctly ordered number from byte buffer
    unsigned int number = 0;
    for (unsigned int i=0;i<num_bytes;i++) number = number << 8 | buf[i];
    return number;
}


enum midi_parse_state { midi_start, midi_file_header, midi_header_length,
                        midi_file_format, midi_num_tracks, midi_get_tpqn,
                        midi_track_header, midi_track_length, midi_get_timedelta,
                        midi_event, midi_size, midi_metacommand_command,
                        midi_command };



midi_parser *new_midi_parser(LV2_Atom_Forge *forge) {
    midi_parser *parser = malloc(sizeof(midi_parser));
    parser->forge = forge;
    parser->mpqn = 500000;
    parser->buffer = malloc(BUFFER_SZ);
    parser->midi_midiEvent = lv2_urid_map.map(NULL, LV2_MIDI__MidiEvent);
    return parser;
}

unsigned int process_midi_byte(midi_parser *parser, uint8_t byte) {
    // Accept a byte at a time from a midi file, writing midi events
    // to an atom buffer as they are detected.
    // Return the number of microseconds elapsed since track start
    static enum midi_parse_state state = midi_start;
    static unsigned int bytes_needed = 0;
    static unsigned int buffer_index = 0;
    static unsigned int track_length;
    static unsigned int timedelta;
    static uint8_t command;
    static unsigned int event_size;
    static unsigned int total_msecs = 0;

    if (bytes_needed > 0) {
        // Collect bytes in the buffer
        if (buffer_index >= BUFFER_SZ) {
            printf("Warning: midi buffer overflow\r\n");
        } else {
            parser->buffer[buffer_index++] = byte;
        }
        bytes_needed--;
        if (bytes_needed > 0) return total_msecs;
        else buffer_index = 0;
    }

    switch(state) {
        case midi_start:
            state = midi_file_header;
            parser->buffer[buffer_index++] = byte;
            bytes_needed = 3;
            break;
        case midi_file_header:
            if (make_number(parser->buffer, 4) != MThd) {
                printf("Bad file header!\r\n");
            }
            state = midi_header_length;
            bytes_needed = 4;
            break;
        case midi_header_length:
            if (make_number(parser->buffer, 4) != 6) {
                printf("Bad track header length!\r\n");
            }
            state = midi_file_format;
            bytes_needed = 2;
            break;
        case midi_file_format:
            if (make_number(parser->buffer, 2) != 0) {
                printf("This code is written for single track midi files, so yours might not work!\r\n");
            }
            state = midi_num_tracks;
            bytes_needed = 2;
            break;
        case midi_num_tracks:
            parser->num_tracks = make_number(parser->buffer, 2);
            state = midi_get_tpqn;
            bytes_needed = 2;
            break;
        case midi_get_tpqn: // Ticks per quarter note
            parser->tpqn = make_number(parser->buffer, 2);
//            printf("tpqn: %x\r\n", parser->tpqn);
            parser->uspt =  (float)parser->mpqn / (float)parser->tpqn;
//            printf("us per tick: %x\r\n", parser->uspt);

            state = midi_track_header;
            bytes_needed = 4;
            break;
        case midi_track_header:
            if (make_number(parser->buffer, 4) != MTrk) {
                printf("Bad track header!\r\n");
            }
            state = midi_track_length;
            bytes_needed = 4;
            break;
        case midi_track_length:
            track_length = make_number(parser->buffer, 4);
            state = midi_get_timedelta;
            timedelta = 0;
            break;
        case midi_get_timedelta:
            timedelta = (timedelta << 7) | (byte & 0x7f);
            if (!(byte & 0x80)) {
                total_msecs += parser->uspt * timedelta / 1000;
                state = midi_event;
            }
            break;
        case midi_event:
            if (byte == 0xff) {
                // metacommand
                state = midi_metacommand_command;
            } else if (byte == 0xf0) {
                // sysex
                command = 0xf0;
                event_size = 0;
                state = midi_size;
            } else if (byte == 0xf7) {
                // continued sysex
                command = 0xf7;
                event_size = 0;
                state = midi_size;
            } else if (byte > 0xf0) {
                printf("Unexpected byte in midi stream: %x\r\n", byte);
            } else if (byte & 0x80) {
                // Bare midi cmd
                command = byte;
                parser->buffer[0] = byte;
                buffer_index = 1;
                bytes_needed = numparms(byte);
                event_size = 1 + bytes_needed;
                state = midi_command;
            } else {
                // Running status
                parser->buffer[0] = command;
                parser->buffer[1] = byte;
                buffer_index = 2;
                bytes_needed = numparms(command) - 1;
                event_size = 2 + bytes_needed;
                state = midi_command;
            }
            break;
        case midi_metacommand_command:
            command = byte;
            event_size = 0;
            state = midi_size;
            break;
        case midi_size:
            event_size = (event_size << 7) | (byte & 0x7f);
            if (!(byte & 0x80)) {
                bytes_needed = event_size;
                state = midi_command;  
            }
            break;
        case midi_command:
            lv2_atom_forge_frame_time(parser->forge, 1/*frame_time++TODO*/);
            lv2_atom_forge_atom(parser->forge, event_size, parser->midi_midiEvent);
            lv2_atom_forge_write(parser->forge, parser->buffer, event_size);
            timedelta = 0;
            state = midi_get_timedelta;
            break;
        default:
            printf("This shouldn't happen\r\n");
            break;
    }
    return total_msecs;
}

