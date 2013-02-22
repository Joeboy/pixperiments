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

#ifndef __mdaCombo_H
#define __mdaCombo_H

#include "audioeffectx.h"

class mdaCombo : public AudioEffectX
{
public:
	mdaCombo(audioMasterCallback audioMaster);
	~mdaCombo();

	virtual void process(float **inputs, float **outputs, int32_t sampleFrames);
	virtual void processReplacing(float **inputs, float **outputs, int32_t sampleFrames);
	virtual void setProgramName(char *name);
	virtual void getProgramName(char *name);
	virtual bool getProgramNameIndexed (int32_t category, int32_t index, char* name);
	virtual void setParameter(int32_t index, float value);
	virtual float getParameter(int32_t index);
	virtual void getParameterLabel(int32_t index, char *label);
	virtual void getParameterDisplay(int32_t index, char *text);
	virtual void getParameterName(int32_t index, char *text);
  virtual float filterFreq(float hz);
  virtual void suspend();

	virtual bool getEffectName(char *name);
	virtual bool getVendorString(char *text);
	virtual bool getProductString(char *text);
	virtual int32_t getVendorVersion() { return 1000; }

protected:
	float fParam1;
  float fParam2;
  float fParam3;
  float fParam4;
  float fParam5;
  float fParam6;
  float fParam7;

  float clip, drive, trim, lpf, hpf, mix1, mix2;
  float ff1, ff2, ff3, ff4, ff5, bias;
  float ff6, ff7, ff8, ff9, ff10;
  float hhf, hhq, hh0, hh1; //hpf

  float *buffer, *buffe2;
	int32_t size, bufpos, del1, del2;
  int mode, ster;

	char programName[32];
};

#endif
