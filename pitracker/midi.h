#ifndef MIDI_H
#define MIDI_H

#define NOTE_OFF                0x80
#define NOTE_ON                 0x90

enum event_type { noteon, noteoff };

typedef struct {
    uint32_t time;
    uint32_t note_no;
    enum event_type type;
} event;

#endif
