#include <stdio.h>
#include <malloc.h>

#include <config.h>

#ifdef RASPBERRY_PI
#include <pi/hardware.h>
#endif

#ifdef LINUX
#include <linux/hardware.h>
#endif
#include <pi/uart.h>

#include <lv2/lv2plug.in/ns/ext/atom/forge.h>
#include <lv2/lv2plug.in/ns/ext/midi/midi.h>
#include <lv2.h>


#define MThd 0x4d546864
#define MTrk 0x4d54726b
#define me_end_of_track            0x2f

extern unsigned int GET32(uint32_t);

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


enum midi_parse_state { smf_start, midi_file_header, midi_header_length,
                        midi_file_format, midi_num_tracks, midi_get_tpqn,
                        midi_track_header, midi_track_length, midi_get_timedelta,
                        midi_event, midi_size, midi_metacommand_command,
                        midi_command };


typedef struct {
    unsigned int num_tracks;
    unsigned int tpqn; // ticks per quarter note
    unsigned int mpqn; // Microseconds per quarter note
    unsigned int uspt; // Microseconds per tick
    LV2_Atom_Forge *forge;
    unsigned char *buffer;
    LV2_URID midi_midiEvent; // Event type for use by forge
    enum midi_parse_state start_state; // The state our state machine starts in
    enum midi_parse_state return_state; // The state our state machine returns to after it finds an event
    enum midi_parse_state state; // current state
} midi_parser;


midi_parser *new_midi_parser(LV2_Atom_Forge *forge, int smf_wrapper) {
    midi_parser *parser = malloc(sizeof(midi_parser));
    // Set up the state machine so it behaves appropriately, depending on
    // whether we're parsing an smf file or a bare midi stream
    if (smf_wrapper) {
        parser->start_state = parser->state = smf_start;
        parser->return_state = midi_get_timedelta;
    } else {
        parser->start_state = parser->state = parser->return_state = midi_event;
    }
    parser->forge = forge;
    parser->mpqn = 500000;
    parser->buffer = malloc(BUFFER_SZ);
    parser->midi_midiEvent = lv2_urid_map.map(NULL, LV2_MIDI__MidiEvent);
    return parser;
}


unsigned int process_midi_byte(midi_parser *parser, uint8_t byte) {
    // Accept a byte at a time from a midi stream, writing midi events
    // to an atom buffer as they are detected.
    // parser->return state should be midi_get_timedelta if the stream is
    // an smf_file, and midi_event if it's a bare midi stream.
    //
    // Return the number of microseconds elapsed since track start
    // Not going to promise this state machine is correct. WFM so far.
    static unsigned int bytes_needed = 0;
    static unsigned int buffer_index = 0;
//    static unsigned int track_length;
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

    switch(parser->state) {
        case smf_start:
            parser->state = midi_file_header;
            parser->buffer[buffer_index++] = byte;
            bytes_needed = 3;
            break;
        case midi_file_header:
            if (make_number(parser->buffer, 4) != MThd) {
                printf("Bad file header!\r\n");
            }
            parser->state = midi_header_length;
            bytes_needed = 4;
            break;
        case midi_header_length:
            if (make_number(parser->buffer, 4) != 6) {
                printf("Bad track header length!\r\n");
            }
            parser->state = midi_file_format;
            bytes_needed = 2;
            break;
        case midi_file_format:
            if (make_number(parser->buffer, 2) != 0) {
                printf("This code is written for single track midi files, so yours might not work!\r\n");
            }
            parser->state = midi_num_tracks;
            bytes_needed = 2;
            break;
        case midi_num_tracks:
            parser->num_tracks = make_number(parser->buffer, 2);
            parser->state = midi_get_tpqn;
            bytes_needed = 2;
            break;
        case midi_get_tpqn: // Ticks per quarter note
            parser->tpqn = make_number(parser->buffer, 2);
//            printf("tpqn: %x\r\n", parser->tpqn);
            parser->uspt =  (float)parser->mpqn / (float)parser->tpqn;
//            printf("us per tick: %x\r\n", parser->uspt);

            parser->state = midi_track_header;
            bytes_needed = 4;
            break;
        case midi_track_header:
            if (make_number(parser->buffer, 4) != MTrk) {
                printf("Bad track header!\r\n");
            }
            parser->state = midi_track_length;
            bytes_needed = 4;
            break;
        case midi_track_length:
//            track_length = make_number(parser->buffer, 4);
            parser->state = midi_get_timedelta;
            timedelta = 0;
            break;
        case midi_get_timedelta:
            timedelta = (timedelta << 7) | (byte & 0x7f);
            if (!(byte & 0x80)) {
                total_msecs += parser->uspt * timedelta / 1000;
                parser->state = midi_event;
            }
            break;
        case midi_event:
            if (byte == 0xfe) { // active sense
                break;
            } else if (byte == 0xff) {
                // metacommand
                parser->state = midi_metacommand_command;
            } else if (byte == 0xf0) {
                // sysex
                command = 0xf0;
                event_size = 0;
                parser->state = midi_size;
            } else if (byte == 0xf7) {
                // continued sysex
                command = 0xf7;
                event_size = 0;
                parser->state = midi_size;
            } else if (byte > 0xf0) {
                printf("Unexpected byte in midi stream: %x\r\n", byte);
            } else if (byte & 0x80) {
                // Bare midi cmd
                command = byte;
                parser->buffer[0] = byte;
                buffer_index = 1;
                bytes_needed = numparms(byte);
                event_size = 1 + bytes_needed;
                parser->state = midi_command;
            } else {
                // Running status
                parser->buffer[0] = command;
                parser->buffer[1] = byte;
                buffer_index = 2;
                bytes_needed = numparms(command) - 1;
                event_size = 2 + bytes_needed;
                parser->state = midi_command;
            }
            break;
        case midi_metacommand_command:
            command = byte;
            event_size = 0;
            parser->state = midi_size;
            break;
        case midi_size:
            event_size = (event_size << 7) | (byte & 0x7f);
            if (!(byte & 0x80)) {
                bytes_needed = event_size;
                parser->state = midi_command;  
            }
            break;
        case midi_command:
//            printf("Command detected!\r\n");
//            for (int i=0;i<event_size;i++) {
//                printf("%x ", parser->buffer[i]);
//            }
//            printf("\r\n");
            if ((parser->buffer[0] & 0xf0) == 0x90 && parser->buffer[2] == 0) {
                // Note on + velocity 0 - translate to Note off
                parser->buffer[0] = 0x80 | (parser->buffer[0] & 0x0f);
            }
            lv2_atom_forge_frame_time(parser->forge, 1/*frame_time++TODO*/);
            lv2_atom_forge_atom(parser->forge, event_size, parser->midi_midiEvent);
            lv2_atom_forge_write(parser->forge, parser->buffer, event_size);
            timedelta = 0;
            parser->state = parser->return_state;
            break;
        default:
            printf("This shouldn't happen\r\n");
            break;
    }
    return total_msecs;
}

// Hack around our (hopefully temporary) lack of file IO
extern uint8_t _binary_tune_mid_start;

static midi_parser *smf_parser;
static midi_parser *realtime_parser;


void init_midi_source(LV2_Atom_Forge *forge) {
    smf_parser = new_midi_parser(forge, 1);
    realtime_parser = new_midi_parser(forge, 0);
    reset_timer_ms();
}

void forge_midi_input() {
#ifdef DEBUG
    // play data from the midi file
    static unsigned int midi_stream_index = 0;
    static unsigned int stream_msecs = 0;
    while (get_timer_ms() > stream_msecs) {
        stream_msecs = process_midi_byte(smf_parser, (uint8_t)*(&_binary_tune_mid_start + midi_stream_index++));
    }
#else
    while (uart_input_ready()) {
        process_midi_byte(realtime_parser, uart_read());
    }
#endif
}
