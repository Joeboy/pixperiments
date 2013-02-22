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

#include "mdaRingMod.h"

#include <math.h>

AudioEffect *createEffectInstance(audioMasterCallback audioMaster)
{
  return new mdaRingMod(audioMaster);
}

mdaRingMod::mdaRingMod(audioMasterCallback audioMaster)	: AudioEffectX(audioMaster, 1, 3)	// 1 program, 3 parameter only
	, fParam4(0.0f)
	, nul(0.0f)
{
	fParam1 = (float)0.0625; //1kHz
  fParam2 = (float)0.0;
  fParam3 = (float)0.0;
	fPhi = 0.0;
	twoPi = (float)6.2831853;
  fdPhi = (float)(twoPi * 100.0 * (fParam2 + (160.0 * fParam1))/ getSampleRate());
  ffb = 0.f;
  fprev = 0.f;

  setNumInputs(2);		    // stereo in
	setNumOutputs(2);		    // stereo out
	setUniqueID("mdaRingMod");    // identify
	DECLARE_LVZ_DEPRECATED(canMono) ();
	canProcessReplacing();	// supports both accumulating and replacing output
	strcpy(programName, "Ring Modulator");	// default program name
}

mdaRingMod::~mdaRingMod()
{
	// nothing to do here
}

bool  mdaRingMod::getProductString(char* text) { strcpy(text, "MDA RingMod"); return true; }
bool  mdaRingMod::getVendorString(char* text)  { strcpy(text, "mda"); return true; }
bool  mdaRingMod::getEffectName(char* name)    { strcpy(name, "RingMod"); return true; }

void mdaRingMod::setProgramName(char *name)
{
	strcpy(programName, name);
}

void mdaRingMod::getProgramName(char *name)
{
	strcpy(name, programName);
}

bool mdaRingMod::getProgramNameIndexed (int32_t category, int32_t index, char* name)
{
	if (index == 0) 
	{
	    strcpy(name, programName);
	    return true;
	}
	return false;
}

void mdaRingMod::setParameter(int32_t index, float value)
{
	switch(index)
  {
    case 0: fParam1 = value; break;
    case 1: fParam2 = value; break;
    case 2: fParam3 = value; break;
  }
	fdPhi = (float) (twoPi * 100.0 * (fParam2 + (160.0 * fParam1))/ getSampleRate());
  ffb = 0.95f * fParam3;
}

float mdaRingMod::getParameter(int32_t index)
{
	float v=0;

  switch(index)
  {
    case 0: v = fParam1; break;
    case 1: v = fParam2; break;
    case 2: v = fParam3; break;
 }
  return v;
}

void mdaRingMod::getParameterName(int32_t index, char *label)
{
	switch(index)
  {
    case 0: strcpy(label, "Freq"); break;
    case 1: strcpy(label, "Fine"); break;
    case 2: strcpy(label, "Feedback"); break;
  }
}

#include <stdio.h>
static void int2strng(int32_t value, char *string) { sprintf(string, "%d", value); }

void mdaRingMod::getParameterDisplay(int32_t index, char *text)
{
	switch(index)
  {
    case 0: int2strng((int32_t)(100. * floor(160. * fParam1)), text); break;
    case 1: int2strng((int32_t)(100. * fParam2), text); break;
    case 2: int2strng((int32_t)(100. * fParam3), text); break;
  }

}

void mdaRingMod::getParameterLabel(int32_t index, char *label)
{
	switch(index)
  {
    case 0: strcpy(label, "Hz"); break;
    case 1: strcpy(label, "Hz"); break;
    case 2: strcpy(label, "%"); break;
  }
}

//--------------------------------------------------------------------------------
// process

void mdaRingMod::process(float **inputs, float **outputs, int32_t sampleFrames)
{
	float *in1 = inputs[0];
	float *in2 = inputs[1];
	float *out1 = outputs[0];
	float *out2 = outputs[1];
	float a, b, c, d, g;	// use registers in sample loops!
  float p, dp, tp = twoPi, fb, fp, fp2;

	p = fPhi;
  dp = fdPhi;
  fb = ffb;
  fp = fprev;

  --in1;	// pre-decrement so we can use pre-increment in the loop
	--in2;	// this is because pre-increment is fast on power pc
	--out1;
	--out2;
	while(--sampleFrames >= 0)
	{
		a = *++in1;		// try to do load operations first...
		b = *++in2;

    g = (float)sin(p);              //instantaneous gain
    p = (float)fmod( p + dp, tp );   //oscillator phase

    c = out1[1];	// get output, as we need to accumulate
		d = out2[1];

    fp =  (fb * fp + a) * g;
    fp2 = (fb * fp + b) * g;

    c += fp;		// accumulate to output buss
		d += fp2;

    *++out1 = c;	// ...and store operations at the end,
		*++out2 = d;	// as this uses the cache efficiently.
	}
  fPhi = p;
  fprev = fp;
}

void mdaRingMod::processReplacing(float **inputs, float **outputs, int32_t sampleFrames)
{
	float *in1 = inputs[0];
	float *in2 = inputs[1];
	float *out1 = outputs[0];
	float *out2 = outputs[1];
	float a, b, c, d, g;
  float p, dp, tp = twoPi, fb, fp, fp2;

  p = fPhi;
  dp = fdPhi;
  fb = ffb;
  fp = fprev;

	--in1;
	--in2;
	--out1;
	--out2;
	while(--sampleFrames >= 0)
	{
		a = *++in1;
		b = *++in2;

    g = (float)sin(p);
    p = (float)fmod( p + dp, tp );

    fp = (fb * fp + a) * g;
    fp2 = (fb * fp + b) * g;

		c = fp;
		d = fp2;

    *++out1 = c;
		*++out2 = d;
	}
  fPhi = p;
  fprev = fp;
}
