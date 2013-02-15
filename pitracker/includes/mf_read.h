#ifndef MF_READ_H
#define MF_READ_H
#include <lv2/lv2plug.in/ns/ext/atom/forge.h>

typedef struct {
    unsigned int num_tracks;
    unsigned int tpqn; // ticks per quarter note
    unsigned int mpqn; // Microseconds per quarter note
    unsigned int uspt; // Microseconds per tick
    LV2_Atom_Forge *forge;
    unsigned char *buffer;
    LV2_URID midi_midiEvent; // Event type for use by forge
} midi_parser;

midi_parser *new_midi_parser(LV2_Atom_Forge *forge);

unsigned int process_midi_byte(midi_parser *parser, uint8_t byte);

#endif
