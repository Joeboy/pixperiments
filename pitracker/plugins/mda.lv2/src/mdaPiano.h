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

#ifndef __mdaPiano__
#define __mdaPiano__

#include <string.h>

#include "audioeffectx.h"

#define NPARAMS 12       //number of parameters
#define NPROGS   8       //number of programs
#define NOUTS    2       //number of outputs
#define NVOICES 32       //max polyphony
#define SUSTAIN 128
#define SILENCE 0.0001f  //voice choking
#define WAVELEN 586348   //wave data bytes

class mdaPianoProgram
{
  friend class mdaPiano;
public:
	mdaPianoProgram();
	~mdaPianoProgram() {}

private:
  float param[NPARAMS];
  char  name[24];
};


struct VOICE  //voice state
{
  int32_t  delta;  //sample playback
  int32_t  frac;
  int32_t  pos;
  int32_t  end;
  int32_t  loop;

  float env;  //envelope
  float dec;

  float f0;   //first-order LPF
  float f1;
  float ff;

  float outl;
  float outr;
  int32_t  note; //remember what note triggered this
};


struct KGRP  //keygroup
{
  int32_t  root;  //MIDI root note
  int32_t  high;  //highest note
  int32_t  pos;
  int32_t  end;
  int32_t  loop;
};

class mdaPiano : public AudioEffectX
{
public:
	mdaPiano(audioMasterCallback audioMaster);
	~mdaPiano();

	virtual void processReplacing(float **inputs, float **outputs, int32_t sampleframes);

	virtual void setProgram(int32_t program);
	virtual void setProgramName(char *name);
	virtual void getProgramName(char *name);
	virtual void setParameter(int32_t index, float value);
	virtual float getParameter(int32_t index);
	virtual void getParameterLabel(int32_t index, char *label);
	virtual void getParameterDisplay(int32_t index, char *text);
	virtual void getParameterName(int32_t index, char *text);
	virtual void setBlockSize(int32_t blockSize);
  virtual void resume();

	virtual bool getOutputProperties (int32_t index, LvzPinProperties* properties);
	virtual bool getProgramNameIndexed (int32_t category, int32_t index, char* text);
	virtual bool copyProgram (int32_t destination);
	virtual bool getEffectName (char* name);
	virtual bool getVendorString (char* text);
	virtual bool getProductString (char* text);
	virtual int32_t getVendorVersion () {return 1;}
	virtual int32_t canDo (const char* text);

  virtual int32_t getNumMidiInputChannels ()  { return 1; }

  int32_t guiUpdate;
  void guiGetDisplay(int32_t index, char *label);

private:
	int32_t processEvent(const LV2_Atom_Event* ev);
	void update();  //my parameter update
  void noteOn(int32_t note, int32_t velocity);
  void fillpatch(int32_t p, const char *name, float p0, float p1, float p2, float p3, float p4,
                 float p5, float p6, float p7, float p8, float p9, float p10,float p11);

  float param[NPARAMS];
  mdaPianoProgram* programs;
  float Fs, iFs;

  ///global internal variables
  KGRP  kgrp[16];
  VOICE voice[NVOICES];
  int32_t  activevoices, poly, cpos;
  short *waves;
  int32_t  cmax;
  float *comb, cdep, width, trim;
  int32_t  size, sustain;
  float tune, fine, random, stretch;
  float muff, muffvel, sizevel, velsens, volume;
};

#endif
