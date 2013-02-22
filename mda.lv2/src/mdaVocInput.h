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

#define NPARAMS  5  ///number of parameters
#define NPROGS   1  ///number of programs

#ifndef __mdaVocInput_H
#define __mdaVocInput_H

#include "audioeffectx.h"


class mdaVocInputProgram
{
public:
  mdaVocInputProgram();
private:
  friend class mdaVocInput;
  float param[NPARAMS];
  char name[32];
};


class mdaVocInput : public AudioEffectX
{
public:
  mdaVocInput(audioMasterCallback audioMaster);
  ~mdaVocInput();

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
  virtual void  midi2string(int32_t n, char *text);
  virtual void  suspend();
  virtual void  resume();

	virtual bool getEffectName(char *name);
	virtual bool getVendorString(char *text);
	virtual bool getProductString(char *text);
	virtual int32_t getVendorVersion() { return 1000; }

protected:
  mdaVocInputProgram *programs;

  ///global internal variables
  int32_t  track;        //track input pitch
  float pstep;        //output sawtooth inc per sample
  float pmult;        //tuning multiplier
  float sawbuf;   
  float noise;        //breath noise level
  float lenv, henv;   //LF and overall envelope
  float lbuf0, lbuf1; //LF filter buffers
  float lbuf2;        //previous LF sample
  float lbuf3;        //period measurement
  float lfreq;        //LF filter coeff
  float vuv;          //voiced/unvoiced threshold
  float maxp, minp;   //preferred period range
  double root;        //tuning reference (MIDI note 0 in Hz)
};

#endif
