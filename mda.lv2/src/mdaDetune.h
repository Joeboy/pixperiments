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

#define NPARAMS  4  ///number of parameters
#define NPROGS   3  ///number of programs
#define BUFMAX   4096

#ifndef __mdaDetune_H
#define __mdaDetune_H

#include "audioeffectx.h"

struct mdaDetuneProgram
{
  friend class mdaDetune;
  float param[NPARAMS];
  char name[32];
};


class mdaDetune : public AudioEffectX
{
public:
  mdaDetune(audioMasterCallback audioMaster);

  virtual void  process(float **inputs, float **outputs, int32_t sampleFrames);
  virtual void  processReplacing(float **inputs, float **outputs, int32_t sampleFrames);
  virtual void  setProgram(int32_t program);
  virtual void  setProgramName(char *name);
  virtual void  getProgramName(char *name);
  virtual bool getProgramNameIndexed (int32_t category, int32_t index, char* name);
  virtual void  setParameter(int32_t index, float value);
  virtual float getParameter(int32_t index);
  virtual void  getParameterLabel(int32_t index, char *label);
  virtual void  getParameterDisplay(int32_t index, char *text);
  virtual void  getParameterName(int32_t index, char *text);
  virtual void  suspend();

	virtual bool getEffectName(char *name);
	virtual bool getVendorString(char *text);
	virtual bool getProductString(char *text);
	virtual int32_t getVendorVersion() { return 1000; }

protected:
	mdaDetuneProgram programs[NPROGS];
	float buf[BUFMAX];
	float win[BUFMAX];

  ///global internal variables
  int32_t  buflen;           //buffer length
  float bufres;           //buffer resolution display
  float semi;             //detune display
  int32_t  pos0;             //buffer input
  float pos1, dpos1;      //buffer output, rate
  float pos2, dpos2;      //downwards shift
  float wet, dry;         //ouput levels
};

#endif
