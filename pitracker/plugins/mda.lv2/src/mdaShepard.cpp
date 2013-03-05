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

#include "mdaShepard.h"

#include <math.h>
#include <float.h>
#include <stdlib.h>

AudioEffect *createEffectInstance(audioMasterCallback audioMaster)
{
  return new mdaShepard(audioMaster);
}

mdaShepard::mdaShepard(audioMasterCallback audioMaster)	: AudioEffectX(audioMaster, 1, 3)	// programs, parameters
{
  double x, a, twopi=6.2831853;
  int i;

  //inits here!
  fParam0 = 0.20f; //mode
  fParam1 = 0.70f; //rate
  fParam2 = 0.50f; //level

  max = 512;
	buf1 = new float[max];
	buf2 = new float[max];

  setNumInputs(2);
	setNumOutputs(2);
	setUniqueID("mdaShepard");    //
	DECLARE_LVZ_DEPRECATED(canMono) ();
	canProcessReplacing();
	strcpy(programName, "Shepard Tone Generator");

  for(max=0; max<511; max++)
  {
    pos = (float)(twopi * max / 511.f); //generate wavetables
    x=0.0;
    a=1.0;
    *(buf2 + max) = (float)sin(pos);
    for(i=0; i<8; i++)
    {
      x+= a * sin(fmod((double)pos,twopi));
      a *= 0.5;
      pos *= 2.0;
    }
    *(buf1 + max) = (float)x;
  }
  *(buf1 + 511) = 0.f;
  *(buf2 + 511) = 0.f; //wrap end for interpolation
  pos=0.f;
  rate=1.f;

  setParameter(0, 0.2f); //go and set initial values!
}

bool  mdaShepard::getProductString(char* text) { strcpy(text, "MDA Shepard"); return true; }
bool  mdaShepard::getVendorString(char* text)  { strcpy(text, "mda"); return true; }
bool  mdaShepard::getEffectName(char* name)    { strcpy(name, "Shepard"); return true; }

void mdaShepard::setParameter(int32_t index, float value)
{
	switch(index)
  {
    case 0: fParam0 = value; break;
    case 1: fParam1 = value; break;
    case 2: fParam2 = value; break;
  }
  //calcs here
  mode = int(2.95f * fParam0);
  drate = 1.f + 10.f * (float)pow(fParam1-0.5,3.0) / getSampleRate();
  out = 0.4842f * (float)pow(10.0f, 2.f * fParam2 - 1.f);
}

mdaShepard::~mdaShepard()
{
	if(buf1) delete [] buf1;
	if(buf2) delete [] buf2;
}

void mdaShepard::suspend()
{

}

void mdaShepard::setProgramName(char *name)
{
	strcpy(programName, name);
}

void mdaShepard::getProgramName(char *name)
{
	strcpy(name, programName);
}

bool mdaShepard::getProgramNameIndexed (int32_t category, int32_t index, char* name)
{
	if (index == 0) 
	{
	    strcpy(name, programName);
	    return true;
	}
	return false;
}

float mdaShepard::getParameter(int32_t index)
{
	float v=0;

  switch(index)
  {
    case 0: v = fParam0; break;
    case 1: v = fParam1; break;
    case 2: v = fParam2; break;
  }
  return v;
}

void mdaShepard::getParameterName(int32_t index, char *label)
{
	switch(index)
  {
    case 0: strcpy(label, "Mode"); break;
    case 1: strcpy(label, "Rate"); break;
    case 2: strcpy(label, "Output"); break;
  }
}

#include <stdio.h>
static void int2strng(int32_t value, char *string) { sprintf(string, "%d", value); }

void mdaShepard::getParameterDisplay(int32_t index, char *text)
{
	switch(index)
  {
    case 0: switch(mode)
            {
              case 0: strcpy(text, "TONES"); break;
              case 1: strcpy(text, "RING MOD"); break;
              case 2: strcpy(text, "TONES+IN"); break;
            } break;
    case 1: int2strng((int32_t)(200 * fParam1 - 100), text); break;
    case 2: int2strng((int32_t)(40 * fParam2 - 20), text); break;
  }
}

void mdaShepard::getParameterLabel(int32_t index, char *label)
{
	switch(index)
  {
    case 0: strcpy(label, ""); break;
    case 1: strcpy(label, "%"); break;
    case 2: strcpy(label, "dB"); break;
  }
}

//--------------------------------------------------------------------------------
// process

void mdaShepard::process(float **inputs, float **outputs, int32_t sampleFrames)
{
	float *in1 = inputs[0];
	float *in2 = inputs[1];
	float *out1 = outputs[0];
	float *out2 = outputs[1];
	float a, b, c;//, d;
  float r=rate, dr=drate, o=out, p=pos, di;
  int32_t x=max, m=mode, i1, i2;

	--in1;
	--in2;
	--out1;
	--out2;

	while(--sampleFrames >= 0)
	{
		a = *++in1 + *++in2;
		c = out1[1];
		//d = out2[1];

    r *= dr;
    if(r>2.f)
    {
      r *= 0.5f;
      p *= 0.5f;
    }
    else if(r<1.f)
    {
      r *= 2.f;
      p *= 2.f; if(p>x)p-=x;
    }

    p += r;
    if(p>x) p -= x;

    i1 = int(p); //interpolate position
    i2 = i1 + 1;
    di = (float)i2 - p;

    b =         di  * ( *(buf1 + i1) + (r - 2.f) * *(buf2 + i1) );
    b += (1.f - di) * ( *(buf1 + i2) + (r - 2.f) * *(buf2 + i2) );
    b *= o / r;

    if(m>0) { if(m==2) b += 0.5f*a; else b *= a; } //ring mod or add

    *++out1 = c + b;
		*++out2 = c + b;
	}
  pos=p; rate=r;
}

void mdaShepard::processReplacing(float **inputs, float **outputs, int32_t sampleFrames)
{
	float *in1 = inputs[0];
	float *in2 = inputs[1];
	float *out1 = outputs[0];
	float *out2 = outputs[1];
	float a, b;
  float r=rate, dr=drate, o=out, p=pos, di;
  int32_t x=max, m=mode, i1, i2;

	--in1;
	--in2;
	--out1;
	--out2;

	while(--sampleFrames >= 0)
	{
		a = *++in1 + *++in2;

    r *= dr;
    if(r>2.f)
    {
      r *= 0.5f;
      p *= 0.5f;
    }
    else if(r<1.f)
    {
      r *= 2.f;
      p *= 2.f; if(p>x)p-=x;
    }

    p += r;
    if(p>x) p -= x;

    i1 = int(p); //interpolate position
    i2 = i1 + 1;
    di = (float)i2 - p;

    b =         di  * ( *(buf1 + i1) + (r - 2.f) * *(buf2 + i1) );
    b += (1.f - di) * ( *(buf1 + i2) + (r - 2.f) * *(buf2 + i2) );
    b *= o / r;

    if(m>0) { if(m==2) b += 0.5f*a; else b *= a; } //ring mod or add

    *++out1 = b;
		*++out2 = b;
	}
  pos=p; rate=r;
}
