/*
  Copyright 2008-2011 David Robillard <http://drobilla.net>
  Copyright 1999-2000 Paul Kellett (Maxim Digital Audio)

  This is free software: you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License,
  or (at your option) any later version.

  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this software. If not, see <http://www.gnu.org/licenses/>.
*/

#include "mdaLooplex.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#if __linux__
#include <pthread.h>
#include <unistd.h>
#endif


AudioEffect *createEffectInstance(audioMasterCallback audioMaster)
{
  return new mdaLooplex(audioMaster);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//replacement for fxIdle that Steinberg stupidly removed from VST2.4 forcing loads of plug-ins to set up their own timers...

#define IDLE_MSEC  40   //25 Hz

class IdleList  //wzDSP objects to receive idle timer (same timer is used for all instances)
{
public:
  IdleList(mdaLooplex *effect, IdleList *next);

  mdaLooplex    *effect;
  IdleList *next;
  bool      remove;

} static idleList(0, 0);  //idleList->next points to start of idle list


#if _WIN32
  #include <windows.h>
  #pragma comment(lib, "user32.lib")
  static UINT timer = 0;
  VOID CALLBACK TimerCallback(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
#elif __linux__
  #define IDLE_MICS (IDLE_MSEC * 1000)
  static pthread_t thread = 0;
  static unsigned int timer = 0;
  static void TimerCallback();
  static void * ThreadCallback(void * args)
  {
      while (timer)
      {
          TimerCallback();
          usleep(IDLE_MICS);
      }
      thread = 0;
      pthread_exit(0);
      return 0;
  }
  void TimerCallback()
#else //OSX
  #include <Carbon/Carbon.h>
  EventLoopTimerRef timer = 0;
  pascal void TimerCallback(EventLoopTimerRef timerRef, void *userData)
#endif
{
  IdleList *prev = &idleList;
  IdleList *item = prev->next;
  while(item)
  {
    if(item->remove)
    {
      prev->next = item->next;  //remove item from list and close gap
      delete item;
      item = prev->next;

      if(prev == &idleList && prev->next == 0) //stop timer after last item removed
      {
        #if _WIN32
          KillTimer(NULL, timer);
        #elif defined(__linux__)
          timer = 0;
          while (thread) usleep(1000);
        #else //OSX
          RemoveEventLoopTimer(timer);
        #endif
          timer = 0;
      }
    }
    else
    {
      item->effect->idle();  //call idle() for each item in list
      prev = item;
      item = item->next;
    }
  }
}


IdleList::IdleList(mdaLooplex *effect, IdleList *next) : effect(effect), next(next), remove(false)
{
  if(effect && !timer) //start timer
  {
    #if WIN32
      timer = SetTimer(NULL, 0, IDLE_MSEC, TimerCallback);
    #elif __linux__
      timer = 1;
      pthread_attr_t attr;
      pthread_attr_init(&attr);
      pthread_attr_setstacksize(&attr, 16 * 1024);
      int policy;
      
      if (pthread_attr_getschedpolicy(&attr, &policy) == 0)
      {
          struct sched_param param;
          param.sched_priority = sched_get_priority_min(policy);
          pthread_attr_setschedpolicy(&attr, SCHED_OTHER);
          pthread_attr_setschedparam(&attr, &param);
      }
      pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
      pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, 0);

      if (pthread_create(&thread, &attr, &ThreadCallback, 0) != 0)
      {
          thread = 0;
          timer = 0;
          fprintf(stderr, "Error: mdaLooplex.cpp (line %d)\n", __LINE__);
      }
      pthread_attr_destroy(&attr);
    #else //OSX
      double ms = kEventDurationMillisecond * (double)IDLE_MSEC;
      InstallEventLoopTimer(GetCurrentEventLoop(), ms, ms, NewEventLoopTimerUPP(TimerCallback), 0, &timer);
    #endif
  }
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


mdaLooplexProgram::mdaLooplexProgram()
{
	param[0]  = 0.00f; //max delay
	param[1]  = 0.00f; //reset (malloc)
	param[2]  = 0.00f; //record (sus ped)
	param[3]  = 1.00f; //input mix
	param[4]  = 0.50f; //input pan
	param[5]  = 1.00f; //feedback (mod whl)
	param[6]  = 1.00f; //out mix
	//param[7]  = 0.00f;

	strcpy (name, "MDA Looplex");
}


mdaLooplex::mdaLooplex(audioMasterCallback audioMaster) : AudioEffectX(audioMaster, NPROGS, NPARAMS)
{

  Fs = 44100.0f; //won't we know the sample rate by the time we need it?

  //initialise...
  bypass = bypassed = busy = status = 0; //0=not recorded 1=first pass 2=looping
  bufmax = 882000;
  buffer = new short[bufmax + 10];
  memset(buffer, 0, (bufmax + 10) * sizeof(short));
  bufpos = 0;
  buflen = 0;
  recreq = 0;
  oldParam0 = oldParam1 = oldParam2 = 0.0f;
  modwhl = 1.0f;

  programs = new mdaLooplexProgram[NPROGS];
	if(programs) setProgram(0);

  setUniqueID("mdaLoopLex");

  if(audioMaster)
	{
		setNumInputs(NOUTS);
		setNumOutputs(NOUTS);
		canProcessReplacing();
    //needIdle(); idle is broken in VST2.4
	}

  update();
	suspend();

  idleList.next = new IdleList(this, idleList.next); //add to idle list, start timer if not running...
}


void mdaLooplex::update()  //parameter change
{
  float * param = programs[curProgram].param;

  if(fabs(param[1] - oldParam1) > 0.1f)
  {
    oldParam1 = param[1];
    if(fabs(oldParam0 - param[0]) > 0.01f)
    {
      oldParam0 = param[0];
      bypass = 1;  //re-assign memory
    }
  }

  if(param[2] > 0.5f && oldParam2 < 0.5f)
  {
    if(recreq == 0) recreq = 1;
    oldParam2 = param[2];
  }
  if(param[2] < 0.5f && oldParam2 > 0.5f)
  {
    if(recreq == 1) recreq = 0;
    oldParam2 = param[2];
  }

  in_mix = 2.0f * param[3] * param[3];

  in_pan = param[4];

  feedback = param[5];

  out_mix = 0.000030517578f * param[6] * param[6];
}


void mdaLooplex::setSampleRate(float sampleRate)
{
	AudioEffectX::setSampleRate(sampleRate);
  Fs = sampleRate;
}


void mdaLooplex::resume()
{
  //should reset position here...
  bufpos = 0;

  //needIdle();  //idle broken in VST2.4
  DECLARE_LVZ_DEPRECATED (wantEvents) ();
}


void mdaLooplex::idle()
{
  if(bypassed)
  {
    if(busy) return; //only do once per bypass
    busy = 1;
    float * param = programs[curProgram].param;
    bufmax = 2 * (int32_t)Fs * (int32_t)(10.5f + 190.0f * param[0]);
    if(buffer) delete [] buffer;
    buffer = new short[bufmax + 10];
    if(buffer) memset(buffer, 0, (bufmax + 10) * sizeof(short)); else bufmax = 0;

    bypass = busy = status = bufpos = buflen = 0;
  }
}


mdaLooplex::~mdaLooplex ()  //destroy any buffers...
{
  for(IdleList *item=idleList.next; item; item=item->next)  //remove from idle list, stop timer if last item
  {
    if(item->effect == this)
    {
      item->remove = true;
      #if _WIN32 //and stop timer in case our last instance is about to unload
        TimerCallback(0, 0, 0, 0);
      #elif __linux__
        TimerCallback();
      #else
        TimerCallback(0, 0);
      #endif
      break;
    }
  }

  if(programs) delete [] programs;

  if(buffer)
  {
    FILE *fp; //dump loop to file

    if(buflen && (fp = fopen("looplex.wav", "wb")))
    {
      char wh[44];
      memcpy(wh, "RIFF____WAVEfmt \20\0\0\0\1\0\2\0________\4\0\20\0data____", 44);

      int32_t l = 36 + buflen * 2;
      wh[4] = (char)(l & 0xFF);  l >>= 8;
      wh[5] = (char)(l & 0xFF);  l >>= 8;
      wh[6] = (char)(l & 0xFF);  l >>= 8;
      wh[7] = (char)(l & 0xFF);

      l = (int32_t)(Fs + 0.5f);
      wh[24] = (char)(l & 0xFF);  l >>= 8;
      wh[25] = (char)(l & 0xFF);  l >>= 8;
      wh[26] = (char)(l & 0xFF);  l >>= 8;
      wh[27] = (char)(l & 0xFF);

      l = 4 * (int32_t)(Fs + 0.5f);
      wh[28] = (char)(l & 0xFF);  l >>= 8;
      wh[29] = (char)(l & 0xFF);  l >>= 8;
      wh[30] = (char)(l & 0xFF);  l >>= 8;
      wh[31] = (char)(l & 0xFF);

      l = buflen * 2;
      wh[40] = (char)(l & 0xFF);  l >>= 8;
      wh[41] = (char)(l & 0xFF);  l >>= 8;
      wh[42] = (char)(l & 0xFF);  l >>= 8;
      wh[43] = (char)(l & 0xFF);

      fwrite(wh, 1, 44, fp);

      #if __BIG_ENDIAN__
        char *c = (char *)buffer;
        char t;

        for(l=0; l<buflen; l++) //swap endian-ness
        {
          t = *c;
          *c = *(c + 1);
          c++;
          *c = t;
          c++;
        }
      #endif

      fwrite(buffer, sizeof(short), buflen, fp);
      fclose(fp);
    }

    delete [] buffer;
  }
}


void mdaLooplex::setProgram(int32_t program)
{
	curProgram = program;
    update();
}


void mdaLooplex::setParameter(int32_t index, float value)
{
  programs[curProgram].param[index] = value;
  update();
}


float mdaLooplex::getParameter(int32_t index)     { return programs[curProgram].param[index]; }
void  mdaLooplex::setProgramName(char *name)   { strcpy(programs[curProgram].name, name); }
void  mdaLooplex::getProgramName(char *name)   { strcpy(name, programs[curProgram].name); }
void  mdaLooplex::setBlockSize(int32_t blockSize) {	AudioEffectX::setBlockSize(blockSize); }
bool  mdaLooplex::getEffectName(char* name)    { strcpy(name, "Looplex"); return true; }
bool  mdaLooplex::getVendorString(char* text)  {	strcpy(text, "mda"); return true; }
bool  mdaLooplex::getProductString(char* text) { strcpy(text, "MDA Looplex"); return true; }


bool mdaLooplex::getProgramNameIndexed(int32_t category, int32_t index, char* text)
{
	if ((unsigned int)index < NPROGS)
	{
		strcpy(text, programs[index].name);
		return true;
	}
	return false;
}


bool mdaLooplex::copyProgram(int32_t destination)
{
  if(destination<NPROGS)
  {
    programs[destination] = programs[curProgram];
    return true;
  }
	return false;
}


int32_t mdaLooplex::canDo(char* text)
{
  if(strcmp(text, "receiveLvzEvents") == 0) return 1;
  if(strcmp(text, "receiveLvzMidiEvent") == 0) return 1;
  return -1;
}


void mdaLooplex::getParameterName(int32_t index, char *label)
{
	switch (index)
	{
		case  0: strcpy(label, "Max Del"); break;
		case  1: strcpy(label, "Reset"); break;
		case  2: strcpy(label, "Record"); break;
		case  3: strcpy(label, "In Mix"); break;
		case  4: strcpy(label, "In Pan"); break;
		case  5: strcpy(label, "Feedback"); break;
		case  6: strcpy(label, "Out Mix"); break;

    default: strcpy(label, "        ");
	}
}


void mdaLooplex::getParameterDisplay(int32_t index, char *text)
{
	char string[16];
	float * param = programs[curProgram].param;

  switch(index)
  {
    case  0: sprintf(string, "%4d s", (int)(10.5f + 190.0f * param[index])); break; //10 to 200 sec

    case  1: sprintf(string, "%5.1f MB", (float)bufmax / 524288.0f); break;

    case  2: if(recreq) strcpy(string, "RECORD"); else strcpy(string, "MONITOR"); break;

    case  3:
    case  6: if(param[index] < 0.01f) strcpy(string, "OFF");
             else sprintf(string, "%.1f dB", 20.0f * log10(param[index] * param[index])); break;

    case  5: if(param[index] < 0.01f) strcpy(string, "OFF");
             else sprintf(string, "%.1f dB", 20.0f * log10(param[index])); break;

    case  4: if(param[index] < 0.505f)
             {
               if(param[index] > 0.495f) strcpy(string, "C"); else
                 sprintf(string, "L%.0f", 100.0f - 200.0f * param[index]);
             }
             else sprintf(string, "R%.0f", 200.0f * param[index] - 100.0f); break;

    default: strcpy(string, " ");
  }
	string[8] = 0;
	strcpy(text, (char *)string);
}


void mdaLooplex::getParameterLabel(int32_t index, char *label)
{
  switch(index)
  {
    case  2: strcpy(label, "(susped)"); break;
    case  5: strcpy(label, "(modwhl)"); break;

    default: strcpy(label, "        ");
  }
}


void mdaLooplex::process(float **inputs, float **outputs, int32_t sampleFrames)
{
  notes[0] = EVENTS_DONE;  //mark events buffer as done
}


void mdaLooplex::processReplacing(float **inputs, float **outputs, int32_t sampleFrames)
{
  float* in1 = inputs[0];
  float* in2 = inputs[1];
  float* out1 = outputs[0];
	float* out2 = outputs[1];
	int32_t event=0, frame=0, frames;
  float l, r, dl, dr, d0 = 0.0f, d1;
  float imix = in_mix, ipan = in_pan, omix = out_mix, fb = feedback * modwhl;
  int32_t x;

  if((bypassed = bypass)) return;

  while(frame<sampleFrames)
  {
    frames = notes[event++];
    if(frames>sampleFrames) frames = sampleFrames;
    frames -= frame;
    frame += frames;

    while(--frames>=0)
    {
      l = *in1++;
      r = *in2++;

      //input mix
      r *= imix * ipan;
      l *= imix - ipan * imix;

      if(recreq == 1 && status == 0) status = 1; //first pass
      if(recreq == 0 && status == 1) status = 2; //later pass

      if(status)
      {
        //dither
        d1 = d0;
		#if WIN32
          d0 = 0.000061f * (float)(rand() - 16384);
        #else
          d0 = 0.000061f * (float)((rand() & 32767) - 16384);
		#endif

        //left delay
        dl = fb * (float)buffer[bufpos];
        if(recreq)
        {
          x = (int32_t)(32768.0f * l + dl + d0 - d1 + 100000.5f) - 100000;
          if(x > 32767) x = 32767; else if(x < -32768) x = -32768;
          buffer[bufpos] = (short)x;
        }
        bufpos++;

        //right delay
        dr = fb * (float)buffer[bufpos];
        if(recreq)
        {
          x = (int32_t)(32768.0f * r + dr - d0 + d1 + 100000.5f) - 100000;
          if(x > 32767) x = 32767; else if(x < -32768) x = -32768;
          buffer[bufpos] = (short)x;
        }
        bufpos++;

        //looping
        if(bufpos >= bufmax)
        {
          buflen = bufmax;
          bufpos -= buflen;
          status = 2;
        }
        else
        {
          if(status == 1) buflen = bufpos; else if(bufpos >= buflen) bufpos -= buflen;
        }

        //output
        l += omix * dl;
        r += omix * dr;
      }

      *out1++ = l;
      *out2++ = r;
    }

    if(frame<sampleFrames)
    {
      int32_t note = notes[event++];
      //int32_t vel  = notes[event++];

      if(note == 2)
      {
        bufpos = 0; //resync
      }
      else if(note == 1)
      {
        if(recreq) recreq = 0; else recreq = 1; //toggle recording
      }
    }
  }
  notes[0] = EVENTS_DONE;  //mark events buffer as done
}


int32_t mdaLooplex::processEvents(LvzEvents* ev)
{
  int32_t npos=0;

  for (int32_t i=0; i<ev->numEvents; i++)
	{
		if((ev->events[i])->type != kLvzMidiType) continue;
		LvzMidiEvent* event = (LvzMidiEvent*)ev->events[i];
		char* midiData = event->midiData;

    switch(midiData[0] & 0xf0) //status byte (all channels)
    {
      case 0x80: //note off
        //notes[npos++] = event->deltaFrames; //delta
        //notes[npos++] = midiData[1] & 0x7F; //note
        //notes[npos++] = 0;                  //vel
        break;

      case 0x90: //note on
        //notes[npos++] = event->deltaFrames; //delta
        //notes[npos++] = midiData[1] & 0x7F; //note
        //notes[npos++] = midiData[2] & 0x7F; //vel
        break;

      case 0xB0: //controller
        switch(midiData[1])
        {
          case 0x01:  //mod wheel
            modwhl = 1.0f - 0.007874f * (float)midiData[2];
            break;

          case 0x40:  //sustain
            if(midiData[2] > 63)
            {
              notes[npos++] = event->deltaFrames; //delta
              notes[npos++] = 1; //note
              notes[npos++] = 127; //vel
            }
            break;

          case 0x42:  //soft/sost
          case 0x43:
            if(midiData[2] > 63)
            {
              notes[npos++] = event->deltaFrames; //delta
              notes[npos++] = 2; //note
              notes[npos++] = 127; //vel
            }
            break;

          default:  //all notes off
            if(midiData[1]>0x7A)
            {

            }
            break;
        }
        break;

      default: break;
    }

    if(npos>EVENTBUFFER) npos -= 3; //discard events if buffer full!!
    event++;
	}
  notes[npos] = EVENTS_DONE;
	return 1;
}

