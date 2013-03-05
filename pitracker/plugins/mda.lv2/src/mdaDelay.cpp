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

#include "mdaDelay.h"

#include <math.h>
#include <float.h>

AudioEffect *createEffectInstance(audioMasterCallback audioMaster)
{
  return new mdaDelay(audioMaster);
}

mdaDelay::mdaDelay(audioMasterCallback audioMaster)	: AudioEffectX(audioMaster, 1, 6)	// programs, parameters
{
  //inits here!
  fParam0 = 0.50f; //left delay
  fParam1 = 0.27f; //right ratio
  fParam2 = 0.70f; //feedback
  fParam3 = 0.50f; //tone
  fParam4 = 0.33f; //wet mix
  fParam5 = 0.50f; //output

  size = 32766;  //set max delay time at max sample rate
	buffer = new float[size + 2]; //spare just in case!
  ipos = 0;
  fil0 = 0.0f;

  setNumInputs(2);
	setNumOutputs(2);
	setUniqueID("mdaDelay");  //identify here
	DECLARE_LVZ_DEPRECATED(canMono) ();
	canProcessReplacing();
	strcpy(programName, "Delay");

  suspend();		//flush buffer
  setParameter(0, 0.5);
}

bool  mdaDelay::getProductString(char* text) { strcpy(text, "MDA Delay"); return true; }
bool  mdaDelay::getVendorString(char* text)  { strcpy(text, "mda"); return true; }
bool  mdaDelay::getEffectName(char* name)    { strcpy(name, "Delay"); return true; }

void mdaDelay::setParameter(int32_t index, float value)
{
	float tmp;

  switch(index)
  {
    case 0: fParam0 = value; break;
    case 1: fParam1 = value; break;
    case 2: fParam2 = value; break;
    case 3: fParam3 = value; break;
    case 4: fParam4 = value; break;
    case 5: fParam5 = value; break;
 }
  //calcs here
  ldel = (int32_t)(size * fParam0 * fParam0);
  if(ldel<4) ldel=4;

  switch(int(fParam1 * 17.9f)) //fixed left/right ratios
  {
    case  17: tmp = 0.5000f; break;
    case  16: tmp = 0.6667f; break;
    case  15: tmp = 0.7500f; break;
    case  14: tmp = 0.8333f; break;
    case  13: tmp = 1.0000f; break;
    case  12: tmp = 1.2000f; break;
    case  11: tmp = 1.3333f; break;
    case  10: tmp = 1.5000f; break;
    case   9: tmp = 2.0000f; break;
    default: tmp = 4.0f * fParam1; break; //variable ratio
  }
  rdel = (int32_t)(size * fParam0 * fParam0 * tmp);
  if(rdel>size) rdel=size;
  if(rdel<4) rdel=4;

  fil = fParam3;

  if(fParam3>0.5f)  //simultaneously change crossover frequency & high/low mix
  {
    fil = 0.5f * fil - 0.25f;
    lmix = -2.0f * fil;
    hmix = 1.0f;
  }
  else
  {
    hmix = 2.0f * fil;
    lmix = 1.0f - hmix;
  }
  fil = (float)exp(-6.2831853f * pow(10.0f, 2.2f + 4.5f * fil) / getSampleRate());

  fbk = 0.495f * fParam2;
  wet = 1.0f - fParam4;
  wet = fParam5 * (1.0f - wet * wet); //-3dB at 50% mix
  dry = fParam5 * 2.0f * (1.0f - fParam4 * fParam4);

  //if(fParam2>0.99) { fbk=0.5f; wet=0.0f; } //freeze
}

mdaDelay::~mdaDelay()
{
	if(buffer) delete [] buffer;
}

void mdaDelay::suspend()
{
	memset(buffer, 0, size * sizeof(float));
}

void mdaDelay::setProgramName(char *name)
{
	strcpy(programName, name);
}

void mdaDelay::getProgramName(char *name)
{
	strcpy(name, programName);
}

bool mdaDelay::getProgramNameIndexed (int32_t category, int32_t index, char* name)
{
	if (index == 0) 
	{
	    strcpy(name, programName);
	    return true;
	}
	return false;
}

float mdaDelay::getParameter(int32_t index)
{
	float v=0;

  switch(index)
  {
    case 0: v = fParam0; break;
    case 1: v = fParam1; break;
    case 2: v = fParam2; break;
    case 3: v = fParam3; break;
    case 4: v = fParam4; break;
    case 5: v = fParam5; break;
  }
  return v;
}

void mdaDelay::getParameterName(int32_t index, char *label)
{
	switch(index)
  {
    case 0: strcpy(label, "L Delay"); break;
    case 1: strcpy(label, "R Delay"); break;
    case 2: strcpy(label, "Feedback"); break;
    case 3: strcpy(label, "Fb Tone"); break;
    case 4: strcpy(label, "FX Mix"); break;
    case 5: strcpy(label, "Output"); break;
  }
}

#include <stdio.h>
static void int2strng(int32_t value, char *string) { sprintf(string, "%d", value); }

void mdaDelay::getParameterDisplay(int32_t index, char *text)
{
	switch(index)
  {
    case 0: int2strng((int32_t)(ldel * 1000.0f / getSampleRate()), text); break;
    case 1: int2strng((int32_t)(100 * rdel / ldel), text); break;
    case 2: int2strng((int32_t)(99 * fParam2), text); break;
    case 3: int2strng((int32_t)(200 * fParam3 - 100), text); break;
    case 4: int2strng((int32_t)(100 * fParam4), text); break;
    case 5: int2strng((int32_t)(20 * log10(2.0 * fParam5)), text); break;
  }
}

void mdaDelay::getParameterLabel(int32_t index, char *label)
{
	switch(index)
  {
    case 0:  strcpy(label, "ms"); break;
    case 3:  strcpy(label, "Lo <> Hi"); break;
    case 5:  strcpy(label, "dB"); break;
    default: strcpy(label, "%"); break;
  }
}

//--------------------------------------------------------------------------------
// process

void mdaDelay::process(float **inputs, float **outputs, int32_t sampleFrames)
{
	float *in1 = inputs[0];
	float *in2 = inputs[1];
	float *out1 = outputs[0];
	float *out2 = outputs[1];
	float a, b, c, d, ol, or_, w=wet, y=dry, fb=fbk;
  float lx=lmix, hx=hmix, f=fil, f0=fil0, tmp;
  int32_t i=ipos, l, r, s=size;

  l = (i + ldel) % (s + 1);
  r = (i + rdel) % (s + 1);

  --in1;
	--in2;
	--out1;
	--out2;
  while(--sampleFrames >= 0)
	{
		a = *++in1;
    b = *++in2;
		c = out1[1];
		d = out2[1];

    ol = *(buffer + l);
    or_ = *(buffer + r);

    tmp = w * (a + b) + fb * (ol + or_);
    f0 = f * (f0 - tmp) + tmp;
    *(buffer + i) = lx * f0 + hx * tmp;

    i--; if(i<0) i=s;
    l--; if(l<0) l=s;
    r--; if(r<0) r=s;

    *++out1 = c + y * a + ol;
		*++out2 = d + y * b + or_;
	}
  ipos = i;
  if(fabs(f0)<1.0e-10) fil0=0.0f; else fil0=f0;
}

void mdaDelay::processReplacing(float **inputs, float **outputs, int32_t sampleFrames)
{
	float *in1 = inputs[0];
	float *in2 = inputs[1];
	float *out1 = outputs[0];
	float *out2 = outputs[1];
	float a, b, ol, or_, w=wet, y=dry, fb=fbk;
  float lx=lmix, hx=hmix, f=fil, f0=fil0, tmp;
  int32_t i=ipos, l, r, s=size;

  l = (i + ldel) % (s + 1);
  r = (i + rdel) % (s + 1);

	--in1;
	--in2;
	--out1;
	--out2;
  while(--sampleFrames >= 0)
	{
		a = *++in1;
    b = *++in2;

    ol = *(buffer + l); //delay outputs
    or_ = *(buffer + r);

    tmp = w * (a + b) + fb * (ol + or_); //mix input & feedback
    f0 = f * (f0 - tmp) + tmp;    //low-pass filter
    *(buffer + i) = lx * f0 + hx * tmp; //delay input

    i--; if(i<0) i=s;
    l--; if(l<0) l=s;
    r--; if(r<0) r=s;

    *++out1 = y * a + ol; //mix wet & dry
		*++out2 = y * b + or_;
	}
  ipos = i;
  if(fabs(f0)<1.0e-10) fil0=0.0f; else fil0=f0; //trap denormals
}
