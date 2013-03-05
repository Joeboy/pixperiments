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

#ifndef __mdaBeatBox_H
#define __mdaBeatBox_H

#include "audioeffectx.h"

class mdaBeatBox : public AudioEffectX
{
public:
	mdaBeatBox(audioMasterCallback audioMaster);
	~mdaBeatBox();

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
	virtual void suspend();
  virtual void synth();

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
	float fParam8;
  float fParam9;
  float fParam10;
  float fParam11;
  float fParam12;
  float hthr, hfil, sthr, kthr, mix;
  float klev, hlev, slev;
  float  ww,  wwx,  sb1,  sb2,  sf1,  sf2, sf3;
  float kww, kwwx, ksb1, ksb2, ksf1, ksf2;
  float dyne, dyna, dynr, dynm;

  float *hbuf;
  float *kbuf;
  float *sbuf, *sbuf2;
	int32_t hbuflen, hbufpos, hdel;
	int32_t sbuflen, sbufpos, sdel, sfx;
  int32_t kbuflen, kbufpos, kdel, ksfx;
  int32_t rec, recx, recpos;

	char programName[32];
};

#endif
