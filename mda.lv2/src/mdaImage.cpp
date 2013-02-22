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

#include "mdaImage.h"

#include <math.h>
#include <float.h>

AudioEffect *createEffectInstance(audioMasterCallback audioMaster)
{
  return new mdaImage(audioMaster);
}

mdaImage::mdaImage(audioMasterCallback audioMaster)	: AudioEffectX(audioMaster, 1, 6)	// programs, parameters
{
  fParam1 = 0.0f; //mode
  fParam2 = 0.75f; //width
  fParam3 = 0.5f; //skew
  fParam4 = 0.75f; //centre
  fParam5 = 0.5f; //balance
  fParam6 = 0.5f; //output

  setNumInputs(2);
	setNumOutputs(2);
	setUniqueID("mdaImage");    // identify here
	DECLARE_LVZ_DEPRECATED(canMono) ();
	canProcessReplacing();
	strcpy(programName, "Stereo Image / MS Matrix");

  setParameter(0, 0.6f); //go and set initial values!
}

bool  mdaImage::getProductString(char* text) { strcpy(text, "MDA Image"); return true; }
bool  mdaImage::getVendorString(char* text)  { strcpy(text, "mda"); return true; }
bool  mdaImage::getEffectName(char* name)    { strcpy(name, "Image"); return true; }

void mdaImage::setParameter(int32_t index, float value)
{
	float w, k, c, b, g;

//add channel invert presets: SM->LR -100, -100, +100, +100


  switch(index)
  {
    case 0: fParam1 = value; break;
    case 1: fParam2 = value; break;
    case 2: fParam3 = value; break;
    case 3: fParam4 = value; break;
    case 4: fParam5 = value; break;
    case 5: fParam6 = value; break;
  }
  //calcs here
  w = 4.f * fParam2 - 2.f; //width
  k = 2.f * fParam3;       //balance
  c = 4.f * fParam4 - 2.f; //depth
  b = 2.f * fParam5;       //pan
  g = (float)pow(10.0, 2.0 * fParam6 - 1.0);

  switch(int(fParam1*3.9))
  {
    case 0: //SM->LR
      r2l =  g * c * (2.f - b);
      l2l =  g * w * (2.f - k);
      r2r =  g * c * b;
      l2r = -g * w * k;
      break;

    case 1: //MS->LR
      l2l =  g * c * (2.f - b);
      r2l =  g * w * (2.f - k);
      l2r =  g * c * b;
      r2r = -g * w * k;
      break;

    case 2: //LR->LR
      g *= 0.5f;
      l2l = g * (c * (2.f - b) + w * (2.f - k));
      r2l = g * (c * (2.f - b) - w * (2.f - k));
      l2r = g * (c * b - w * k);
      r2r = g * (c * b + w * k);
      break;

    case 3: //LR->MS
      g *= 0.5f;
      l2l =  g * (2.f - b) * (2.f - k);
      r2l =  g * (2.f - b) * k;
      l2r = -g * b * (2.f - k);
      r2r =  g * b * k;
      break;
  }
}

mdaImage::~mdaImage()
{
}

void mdaImage::suspend()
{
}

void mdaImage::setProgramName(char *name)
{
	strcpy(programName, name);
}

void mdaImage::getProgramName(char *name)
{
	strcpy(name, programName);
}

bool mdaImage::getProgramNameIndexed (int32_t category, int32_t index, char* name)
{
	if (index == 0) 
	{
	    strcpy(name, programName);
	    return true;
	}
	return false;
}

float mdaImage::getParameter(int32_t index)
{
	float v=0;

  switch(index)
  {
    case 0: v = fParam1; break;
    case 1: v = fParam2; break;
    case 2: v = fParam3; break;
    case 3: v = fParam4; break;
    case 4: v = fParam5; break;
    case 5: v = fParam6; break;
  }
  return v;
}

void mdaImage::getParameterName(int32_t index, char *label)
{
	switch(index)
  {
    case 0: strcpy(label, "Mode"); break;
    case 1: strcpy(label, "S Width"); break;
    case 2: strcpy(label, "S Pan"); break;
    case 3: strcpy(label, "M Level"); break;
    case 4: strcpy(label, "M Pan"); break;
    case 5: strcpy(label, "Output"); break;
  }
}

#include <stdio.h>
static void int2strng(int32_t value, char *string) { sprintf(string, "%d", value); }

void mdaImage::getParameterDisplay(int32_t index, char *text)
{
	switch(index)
  {
    case 0: switch(int(fParam1*3.9))
            {
              case 0: strcpy(text, "SM->LR"); break;
              case 1: strcpy(text, "MS->LR"); break;
              case 2: strcpy(text, "LR->LR"); break;
              case 3: strcpy(text, "LR->MS"); break;

           } break;
    case 1: int2strng((int32_t)(400 * fParam2 - 200), text); break;
    case 2: int2strng((int32_t)(200 * fParam3 - 100), text); break;
    case 3: int2strng((int32_t)(400 * fParam4 - 200), text); break;
    case 4: int2strng((int32_t)(200 * fParam5 - 100), text); break;
    case 5: int2strng((int32_t)(40 * fParam6 - 20), text); break;
  }
}

void mdaImage::getParameterLabel(int32_t index, char *label)
{
	switch(index)
  {
    case 0: strcpy(label, ""); break;
    case 1: strcpy(label, "%"); break;
    case 2: strcpy(label, "L<->R"); break;
    case 3: strcpy(label, "%"); break;
    case 4: strcpy(label, "L<->R"); break;
    case 5: strcpy(label, "dB"); break;
  }
}

//--------------------------------------------------------------------------------
// process

void mdaImage::process(float **inputs, float **outputs, int32_t sampleFrames)
{
	float *in1 = inputs[0];
	float *in2 = inputs[1];
	float *out1 = outputs[0];
	float *out2 = outputs[1];
	float a, b, c, d, ll=l2l, lr=l2r, rl=r2l, rr=r2r;

  --in1;
	--in2;
	--out1;
	--out2;

  while(--sampleFrames >= 0)
	{
		a = *++in1;
    b = *++in2;
		c = out1[1];
		d = out2[1]; //process from here...

    d += rr*b + lr*a;
    c += ll*a + rl*b;

    *++out1 = c;
		*++out2 = d;
	}
}

void mdaImage::processReplacing(float **inputs, float **outputs, int32_t sampleFrames)
{
	float *in1 = inputs[0];
	float *in2 = inputs[1];
	float *out1 = outputs[0];
	float *out2 = outputs[1];
	float a, b, c, d, ll=l2l, lr=l2r, rl=r2l, rr=r2r;

	--in1;
	--in2;
	--out1;
	--out2;

  while(--sampleFrames >= 0)
	{
		a = *++in1;
    b = *++in2;

    d = rr*b + lr*a;
    c = ll*a + rl*b;

    *++out1 = c;
		*++out2 = d;
	}
}
