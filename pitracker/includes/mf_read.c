/*
**  (C) 2008 by Remo Dentato (rdentato@users.sourceforge.net)
**  Minor amends by Joe Button 2013
**
** Permission to use, copy, modify and distribute this code and
** its documentation for any purpose is hereby granted without
** fee, provided that the above copyright notice, or equivalent
** attribution acknowledgement, appears in all copies and
** supporting documentation.
**
** Copyright holder makes no representations about the suitability
** of this software for any purpose. It is provided "as is" without
** express or implied warranty.
*/
#include <lv2/lv2plug.in/ns/ext/atom/forge.h>

#define MThd 0x4d546864
#define MTrk 0x4d54726b
#define me_end_of_track            0x2f

static uint8_t * midi_file; // This is now a dummy

/********************************************************************/

/* == Reading Values
**
**  readnum(n)  reads n bytes and assembles them to create an integer
**              if n is 0, reads a variable length representation
*/

static long readvar()
{
  long v=0; int c;

  if ((c = midi_fgetc(midi_file)) == EOF) return -1;

  while (c & 0x80 ) {
    v = (v << 7) | (c & 0x7f);
    if ((c = midi_fgetc(midi_file)) == EOF) return -1;
  }
  v = (v << 7) | c;
  return (v);
}

static long readnum(int k)
{
  long x=0, v = 0;

  if (k == 0) return(readvar());

  while (k-- > 0) {
    if ((x = midi_fgetc(midi_file)) == EOF) return -1;
    v = (v << 8) | x;
  }
  return v;
}

/* === Read messages
**   READ_S(n)  reads n bytes, stores them in a buffer end returns
**              a pointer to the buffer;
*/


//static unsigned char *chrbuf=NULL;
//static long  chrbuf_sz=0;

//static char *chrbuf_set(long sz)
//{
//  if (sz > chrbuf_sz) {
//    free(chrbuf);
//    chrbuf = malloc(sz);
//  }
//  return (char *)chrbuf;
//}

static char *readmsg(long n)
{
  int   c;
//  char *s;

  if (n == 0) return "";

//  chrbuf_set(n);
//  if (chrbuf) {
//    s = chrbuf;
    while (n-- > 0) {   /*** Read the message ***/
      if ((c = midi_fgetc(midi_file)) == EOF) return NULL;
//      *s++ = c;
    }
    return (char*)1; // fudge the return val
//  }
//  return (char *)chrbuf;
}


/* == Finite State Machines
**
**   These macros provide a simple mechanism for defining
** finite state machines (FSM).
**
**   Each state containse a block of instructions:
**
** | STATE(state_name) {
** |   ... C instructions ...
** | }
**
**   To move from a state to another you use the GOTO macro:
**
** | if (c == '*') GOTO(stars);
**
** or, in case of an error) the FAIL(x) macro:
**
** | if (c == '?') FAIL(404);  ... 404 is an error code
**
**   There must be two special states states: ON_FAIL and ON_END
**
*/

#define STATE(x)  x##_: if (0) goto x##_;
#define GOTO(x)   goto x##_
#define FAIL(e)   do {ERROR = e; goto fail_; } while(0)
#define ON_FAIL   fail_:
#define ON_END    FINAL_:
#define FINAL     FINAL_

static long track_time;

/* Get the number of parameters needed by a channel message
** s is the status byte.
*/
static char *nparms = "\2\2\2\2\1\1\2";
#define numparms(s) (nparms[(s & 0x70)>>4])

static int scan_midi()
{
  long tmp;
  long v1, v2;
  int ERROR = 0;
  long tracklen;
  long ntracks;
  long curtrack = 0;
  long status = 0;
  char *msg;
  long chan;


  STATE(mthd) {
    if (readnum(4) != MThd) FAIL(110);
    tmp = readnum(4); /* chunk length */
    if (tmp < 6) FAIL(111);
    v1 = readnum(2);
    ntracks = readnum(2);
    v2 = readnum(2);
    ERROR = h_header(v1, ntracks, v2);
    if (ERROR) FAIL(ERROR);
    if (tmp > 6) readnum(tmp-6);
  }

  STATE(mtrk) {
    if (curtrack++ == ntracks) GOTO(FINAL);
    if (readnum(4) != MTrk) FAIL(120);
    tracklen = readnum(4);
    if (tracklen < 0) FAIL(121);
    track_time = 0;
    status = 0;
    ERROR = h_track(0, curtrack, tracklen);
    if (ERROR) FAIL(ERROR);
  }

  STATE(event) {
    tmp = readnum(0); if (tmp < 0) FAIL(2111);
    track_time += tmp;

    tmp = readnum(1); if (tmp < 0) FAIL(2112);

    if ((tmp & 0x80) == 0) {
      if (status == 0) FAIL(223); /* running status not allowed! */
      GOTO(midi_event);
    }

    status = tmp;
    v1 = -1;
    if (status == 0xFF) GOTO(meta_event);
    if (status == 0xF0) GOTO(sys_event);
    if (status == 0xF7) GOTO(sys_event);
    if (status >  0xF0) FAIL(543);
    tmp = readnum(1);
  }

  STATE(midi_event) {
    chan = 1+(status & 0x0F);
    v1 = tmp;
    v2 = -1;
    if (numparms(status) == 2) {
      v2 = readnum(1);
      if (v2 < 0) FAIL(212);
    }
    ERROR = h_midi_event(track_time, status & 0xF0, chan, v1, v2);
    if (ERROR) FAIL(ERROR);

    GOTO(event);
  }

  STATE(meta_event) {
    v1 = readnum(1);  if (v1 < 0) FAIL(2114);
  }

  STATE(sys_event) {
    v2 = readnum(0);  if (v2 < 0) FAIL(2115);

    msg = readmsg(v2); if (msg == NULL) FAIL(2116);
    if (v1 == me_end_of_track) {
      ERROR = 0;//h_track(1, curtrack, track_time);
      if (ERROR) FAIL(ERROR);
      GOTO(mtrk);
    }
    ERROR = 0;//h_sys_event(track_time, status, v1, v2, msg);
    if (ERROR) FAIL(ERROR);
    status = 0;
    GOTO(event);
  }

  ON_FAIL {
    if (ERROR < 0) ERROR = -ERROR;
    h_error(ERROR, NULL);
  }

  ON_END {
//    free(chrbuf);
//    chrbuf_sz = 0;
//    chrbuf = NULL;
  }

  return ERROR;
}


uint32_t make_number(uint8_t* buf, unsigned int num_bytes) {
    // Assemble correctly ordered 32 bit number from byte buffer
    uint32_t number = 0;
    for (unsigned int i=0;i<num_bytes;i++) number = number << 8 | buf[i];
    return number;
}

enum midi_parse_state { midi_start, midi_file_header, midi_header_length,
                        midi_file_format, midi_num_tracks, midi_get_tpqn,
                        midi_track_header, midi_track_length, midi_get_timedelta,
                        midi_event, midi_size, midi_metacommand_command,
                        midi_command };

uint8_t midi_byte_buffer[8];

typedef struct {
    unsigned int num_tracks;
    unsigned int tpqn;
    unsigned int mpqn;
    LV2_Atom_Forge *forge;
    unsigned int uspt; // Microseconds per tick
} midi_parser;


void refresh_parser_timing(midi_parser *parser) {
    parser->uspt =  (float)parser->mpqn / (float)parser->tpqn;
    printf("us per tick: %x\r\n", parser->uspt);
}

midi_parser *new_midi_parser(LV2_Atom_Forge *forge) {
    midi_parser *parser = malloc(sizeof(midi_parser));
    parser->forge = forge;
    parser->mpqn = 500000;
    return parser;
}

unsigned int process_midi_byte(midi_parser *parser, uint8_t byte) {
    // Fill an atom buffer (TODO)
    // Return the number of microseconds elapsed since track start
    static enum midi_parse_state state = midi_start;
    static unsigned int bytes_needed = 0;
    static unsigned int byte_buffer_index = 0;
    static unsigned int track_length;
    static unsigned int timedelta;
    static uint8_t command;
    static unsigned int event_size;
    static unsigned int chan;
    static unsigned int total_msecs = 0;

//    printf("%x\r\n", byte);
    if (bytes_needed >0 ) {
        midi_byte_buffer[byte_buffer_index] = byte;
        bytes_needed--;
        byte_buffer_index++;
        if (bytes_needed > 0) return total_msecs;
    }
//    printf("state=%x\r\n", state);
    switch(state) {
        case midi_start:
            state = midi_file_header;
            midi_byte_buffer[byte_buffer_index++] = byte;
            bytes_needed = 3;
            break;
        case midi_file_header:
            if (make_number(midi_byte_buffer, 4) != MThd) {
                printf("Bad file header!\r\n");
            }
            state = midi_header_length;
            bytes_needed = 4;
            byte_buffer_index = 0;
            break;
        case midi_header_length:
            if (make_number(midi_byte_buffer, 4) != 6) {
                printf("Bad track header length!\r\n");
            }
            state = midi_file_format;
            bytes_needed = 2;
            byte_buffer_index = 0;
            break;
        case midi_file_format:
            if (make_number(midi_byte_buffer, 2) != 0) {
                printf("This code is written for single track midi files, so yours might not work!\r\n");
            }
            state = midi_num_tracks;
            bytes_needed = 2;
            byte_buffer_index = 0;
            break;
        case midi_num_tracks:
            parser->num_tracks = make_number(midi_byte_buffer, 2);
            state = midi_get_tpqn;
            bytes_needed = 2;
            byte_buffer_index = 0;
            break;
        case midi_get_tpqn: // Ticks per quarter note
            parser->tpqn = make_number(midi_byte_buffer, 2);
            printf("tpqn: %x\r\n", parser->tpqn);
            refresh_parser_timing(parser);
            state = midi_track_header;
            bytes_needed = 4;
            byte_buffer_index = 0;
            break;
        case midi_track_header:
            if (make_number(midi_byte_buffer, 4) != MTrk) {
                printf("Bad track header!\r\n");
            }
            state = midi_track_length;
            bytes_needed = 4;
            byte_buffer_index = 0;
            break;
        case midi_track_length:
            track_length = make_number(midi_byte_buffer, 4);
            state = midi_get_timedelta;
            timedelta = 0;
            byte_buffer_index = 0;
            break;
        case midi_get_timedelta:
            timedelta = (timedelta << 7) | (byte & 0x7f);
//            printf("td byte: %x\r\n", byte);
            if (!(byte & 0x80)) {
                total_msecs += parser->uspt * timedelta / 1000;
                state = midi_event;
            }
            break;
        case midi_event:
//            printf("td = %x\r\n", timedelta);
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
            } else {
//                printf("Got bare midi cmd\r\n");
                command = byte & 0xf0;
                chan = 1+(byte & 0xf);
                bytes_needed = event_size = numparms(byte);
                byte_buffer_index = 0;
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
            printf("cmd=%x, size=%d\r\n", command, event_size);
            timedelta = 0;
            state = midi_get_timedelta;
            byte_buffer_index = 0;
            break;
        default:
            printf("This shouldn't happen\r\n");
            break;
    }
    return total_msecs;
}

