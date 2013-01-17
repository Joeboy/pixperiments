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

//#include "mf_priv.h"

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

  if ((c = fgetc(midi_file)) == EOF) return -1;

  while (c & 0x80 ) {
    v = (v << 7) | (c & 0x7f);
    if ((c = fgetc(midi_file)) == EOF) return -1;
  }
  v = (v << 7) | c;
  return (v);
}

static long readnum(int k)
{
  long x=0, v = 0;

  if (k == 0) return(readvar());

  while (k-- > 0) {
    if ((x = fgetc(midi_file)) == EOF) return -1;
    v = (v << 8) | x;
  }
  return v;
}

/* === Read messages
**   READ_S(n)  reads n bytes, stores them in a buffer end returns
**              a pointer to the buffer;
*/


static unsigned char *chrbuf=NULL;
static long  chrbuf_sz=0;

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
  char *s;

  if (n == 0) return "";

//  chrbuf_set(n);
//  if (chrbuf) {
//    s = chrbuf;
    while (n-- > 0) {   /*** Read the message ***/
      if ((c = fgetc(midi_file)) == EOF) return NULL;
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
  unsigned char *msg;
  long chan;


  STATE(mthd) {
    if (readnum(4) != MThd) FAIL(110);
    tmp = readnum(4); /* chunk length */
    if (tmp < 6) FAIL(111);
    v1 = readnum(2);
    ntracks = readnum(2);
    v2 = readnum(2);
    ERROR = 0;//h_header(v1, ntracks, v2);
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
    ERROR = 0;//h_track(0, curtrack, tracklen);
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


/* == Read Midi files
*/

/*
****f*  readmidifile/mf_read
* NAME
*  mf_read
*
* SYNOPSIS
*   error = mf_read(fname)
*
* FUNCTION
*   Reads a midifile and calls the appropriate handlers for
*   each event encountered.
*
* INPUTS
*   fname - the midifile file name
*
* RESULTS
*   error - 0 if everything went fine or an error code.
*
********
*/

//int mf_read(char *fname)
//{
//  int ret;
//  if ((midi_file = fopen(fname, "rb")) == NULL) {
//    h_error(79,"File not found");
//    return -1;
//  }
//  ret = scan_midi();
//  fclose(midi_file); midi_file = NULL;
//
//  free(chrbuf); chrbuf=NULL;   chrbuf_sz=0;
//
//  return ret;
//}

