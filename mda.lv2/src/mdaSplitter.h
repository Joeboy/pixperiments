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

#define NPARAMS  7  ///number of parameters
#define NPROGS   3  ///number of programs

#ifndef __mdaSplitter_H
#define __mdaSplitter_H

#include "audioeffectx.h"

class mdaSplitterProgram
{
public:
  mdaSplitterProgram()
  {
    param[0] = 0.00f;  //mode
    param[1] = 0.50f;  //freq
    param[2] = 0.25f;  //freq mode
    param[3] = 0.50f;  //level (was 2)
    param[4] = 0.50f;  //level mode
    param[5] = 0.50f;  //envelope
    param[6] = 0.50f;  //gain
    strcpy(name, "Frequency/Level Splitter");
  }
private:
  friend class mdaSplitter;
  float param[NPARAMS];
  char name[32];
};


class mdaSplitter : public AudioEffectX
{
public:
  mdaSplitter(audioMasterCallback audioMaster);

  virtual void  process(float **inputs, float **outputs, int32_t sampleFrames);
  virtual void  processReplacing(float **inputs, float **outputs, int32_t sampleFrames);
  virtual void  setProgram(int32_t program);
  virtual void  setProgramName(char *name);
  virtual void  getProgramName(char *name);
	virtual bool getProgramNameIndexed (int32_t category, int32_t which, char* name);
  virtual void  setParameter(int32_t which, float value);
  virtual float getParameter(int32_t which);
  virtual void  getParameterLabel(int32_t which, char *label);
  virtual void  getParameterDisplay(int32_t which, char *text);
  virtual void  getParameterName(int32_t which, char *text);
  virtual void  suspend();
  virtual void  resume();

	virtual bool getEffectName(char *name);
	virtual bool getVendorString(char *text);
	virtual bool getProductString(char *text);
	virtual int32_t getVendorVersion() { return 1000; }

protected:
  mdaSplitterProgram programs[NPROGS];

  ///global internal variables
  float freq, fdisp, buf0, buf1, buf2, buf3;  //filter
  float level, ldisp, env, att, rel;          //level switch
  float ff, ll, pp, i2l, i2r, o2l, o2r;       //routing (freq, level, phase, output)
  int32_t  mode;

};

#endif
