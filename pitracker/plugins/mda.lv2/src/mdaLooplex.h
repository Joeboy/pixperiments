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

#ifndef __mdaLooplex__
#define __mdaLooplex__

#include <string.h>

#include "audioeffectx.h"

#define NPARAMS  7       //number of parameters
#define NPROGS   1       //number of programs
#define NOUTS    2       //number of outputs
#define SILENCE  0.00001f


class mdaLooplexProgram
{
  friend class mdaLooplex;
public:
	mdaLooplexProgram();
private:
  float param[NPARAMS];
  char  name[24];
};


class mdaLooplex : public AudioEffectX
{
public:
	mdaLooplex(audioMasterCallback audioMaster);
	~mdaLooplex();

	virtual void process(float **inputs, float **outputs, int32_t sampleframes);
	virtual void processReplacing(float **inputs, float **outputs, int32_t sampleframes);
	virtual int32_t processEvents(LvzEvents* events);

	virtual void setProgram(int32_t program);
	virtual void setProgramName(char *name);
	virtual void getProgramName(char *name);
	virtual void setParameter(int32_t index, float value);
	virtual float getParameter(int32_t index);
	virtual void getParameterLabel(int32_t index, char *label);
	virtual void getParameterDisplay(int32_t index, char *text);
	virtual void getParameterName(int32_t index, char *text);
	virtual void setSampleRate(float sampleRate);
	virtual void setBlockSize(int32_t blockSize);
    virtual void resume();

	virtual bool getProgramNameIndexed (int32_t category, int32_t index, char* text);
	virtual bool copyProgram (int32_t destination);
	virtual bool getEffectName (char* name);
	virtual bool getVendorString (char* text);
	virtual bool getProductString (char* text);
	virtual int32_t getVendorVersion () {return 1;}
	virtual int32_t canDo (char* text);

	virtual int32_t getNumMidiInputChannels ()  { return 1; }

  void idle();

private:
	void update();  //my parameter update

  mdaLooplexProgram* programs;
  float Fs;

  #define EVENTBUFFER 120
  #define EVENTS_DONE 99999999
  int32_t notes[EVENTBUFFER + 8];  //list of delta|note|velocity for current block

  ///global internal variables
  float in_mix, in_pan, out_mix, feedback, modwhl;

  short *buffer;
  int32_t bufpos, buflen, bufmax, mode;

  int32_t bypass, bypassed, busy, status, recreq;
  float oldParam0, oldParam1, oldParam2;

};

#endif
