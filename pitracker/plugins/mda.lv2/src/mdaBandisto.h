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

#ifndef __mdaBandisto_H
#define __mdaBandisto_H

#include "audioeffectx.h"

class mdaBandisto : public AudioEffectX
{
public:
	mdaBandisto(audioMasterCallback audioMaster);
	~mdaBandisto();

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

	virtual bool getEffectName(char *name);
	virtual bool getVendorString(char *text);
	virtual bool getProductString(char *text);
	virtual int32_t getVendorVersion() { return 1000; }

protected:
	float fParam1, fParam2, fParam3, fParam4;
	float fParam5, fParam6, fParam7, fParam8;
  float fParam9, fParam10;
  float driv1, trim1;
  float driv2, trim2;
  float driv3, trim3;
  float fi1, fb1, fo1, fi2, fb2, fo2, fb3, slev;
  int valve;
	char programName[32];
};

#endif
