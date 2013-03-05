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

#include "mdaRoundPan.h"

#include <math.h>
#include <float.h>

AudioEffect *createEffectInstance(audioMasterCallback audioMaster)
{
  return new mdaRoundPan(audioMaster);
}

mdaRoundPan::mdaRoundPan(audioMasterCallback audioMaster)	: AudioEffectX(audioMaster, 1, 2)	// programs, parameters
	, fParam3(0.0f)
	, fParam4(0.0f)
{
  //inits here!
  fParam1 = (float)0.5; //pan
  fParam2 = (float)0.8; //auto

  //size = 1500;
  //bufpos = 0;
	//buffer = new float[size];
  //buffer2 = new float[size];

  setNumInputs(2);
	setNumOutputs(2);
	setUniqueID("mdaRoundPan");    // identify here
	DECLARE_LVZ_DEPRECATED(canMono) ();
	canProcessReplacing();
	strcpy(programName, "Round Panner");
	suspend();		// flush buffer

  //calcs here!
  phi = 0.0;
  dphi = (float)(5.0 / getSampleRate());
}

bool  mdaRoundPan::getProductString(char* text) { strcpy(text, "MDA RoundPan"); return true; }
bool  mdaRoundPan::getVendorString(char* text)  { strcpy(text, "mda"); return true; }
bool  mdaRoundPan::getEffectName(char* name)    { strcpy(name, "RoundPan"); return true; }

void mdaRoundPan::setParameter(int32_t index, float value)
{
	switch(index)
  {
    case 0: fParam1 = value; phi = (float)(6.2831853 * (fParam1 - 0.5)); break;
    case 1: fParam2 = value; break;
  }
  //calcs here
  if (fParam2>0.55)
  {
    dphi = (float)(20.0 * (fParam2 - 0.55) / getSampleRate());
  }
  else
  {
    if (fParam2<0.45)
    {
      dphi = (float)(-20.0 * (0.45 - fParam2) / getSampleRate());
    }
    else
    {
      dphi = 0.0;
    }
  }
}

mdaRoundPan::~mdaRoundPan()
{
	//if(buffer) delete buffer;
  //if(buffer2) delete buffer2;
}

void mdaRoundPan::suspend()
{
	//memset(buffer, 0, size * sizeof(float));
	//memset(buffer2, 0, size * sizeof(float));
}

void mdaRoundPan::setProgramName(char *name)
{
	strcpy(programName, name);
}

void mdaRoundPan::getProgramName(char *name)
{
	strcpy(name, programName);
}

bool mdaRoundPan::getProgramNameIndexed (int32_t category, int32_t index, char* name)
{
	if (index == 0) 
	{
	    strcpy(name, programName);
	    return true;
	}
	return false;
}

float mdaRoundPan::getParameter(int32_t index)
{
	float v=0;

  switch(index)
  {
    case 0: v = fParam1; break;
    case 1: v = fParam2; break;
  }
  return v;
}

void mdaRoundPan::getParameterName(int32_t index, char *label)
{
	switch(index)
  {
    case 0: strcpy(label, "Pan"); break;
    case 1: strcpy(label, "Auto"); break;
  }
}

#include <stdio.h>
static void int2strng(int32_t value, char *string) { sprintf(string, "%d", value); }

void mdaRoundPan::getParameterDisplay(int32_t index, char *text)
{
	switch(index)
  {
    case 0: int2strng((int32_t)(360.0 * (fParam1 - 0.5)), text); break;
    case 1: int2strng((int32_t)(57.296 * dphi * getSampleRate()), text); break;
  }
}

void mdaRoundPan::getParameterLabel(int32_t index, char *label)
{
	switch(index)
  {
    case 0: strcpy(label, "deg"); break;
    case 1: strcpy(label, "deg/sec"); break;
  }
}

//--------------------------------------------------------------------------------
// process

void mdaRoundPan::process(float **inputs, float **outputs, int32_t sampleFrames)
{
	float *in1 = inputs[0];
	float *in2 = inputs[1];
	float *out1 = outputs[0];
	float *out2 = outputs[1];
	float a, c, d, x=0.5, y=(float)0.7854;
  float ph, dph, fourpi=(float)12.566371;

  ph = phi;
  dph = dphi;

  --in1;
	--in2;
	--out1;
	--out2;
	while(--sampleFrames >= 0)
	{
		a = x * (*++in1 + *++in2);

    c = out1[1];
		d = out2[1]; //process from here...

    c += (float)(a * -sin((x * ph) - y)); // output
		d += (float)(a * sin((x * ph) + y));

    ph = ph + dph;

    *++out1 = c;
		*++out2 = d;
	}
  if(ph<0.0) ph = ph + fourpi; else if(ph>fourpi) ph = ph - fourpi;
  phi = ph;
}

void mdaRoundPan::processReplacing(float **inputs, float **outputs, int32_t sampleFrames)
{
	float *in1 = inputs[0];
	float *in2 = inputs[1];
	float *out1 = outputs[0];
	float *out2 = outputs[1];
	float a, c, d, x=0.5, y=(float)0.7854;
  float ph, dph, fourpi=(float)12.566371;

  ph = phi;
  dph = dphi;

	--in1;
	--in2;
	--out1;
	--out2;
	while(--sampleFrames >= 0)
	{
		a = x * (*++in1 + *++in2); //process from here...

		c = (float)(a * -sin((x * ph) - y)); // output
		d = (float)(a * sin((x * ph) + y));

    ph = ph + dph;

    *++out1 = c;
		*++out2 = d;
	}
  if(ph<0.0) ph = ph + fourpi; else if(ph>fourpi) ph = ph - fourpi;
  phi = ph;
}
