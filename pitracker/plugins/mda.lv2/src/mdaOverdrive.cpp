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

#include "mdaOverdrive.h"

#include <math.h>


AudioEffect *createEffectInstance(audioMasterCallback audioMaster)
{
  return new mdaOverdrive(audioMaster);
}

mdaOverdrive::mdaOverdrive(audioMasterCallback audioMaster)	: AudioEffectX(audioMaster, 1, 3)	// 1 program, 3 parameters
{
  fParam1 = 0.0f;
  fParam2 = 0.0f;
  fParam3 = 0.5f;

  setNumInputs(2);
	setNumOutputs(2);
	setUniqueID("mdaOverdrive");    // identify
	DECLARE_LVZ_DEPRECATED(canMono) ();
	canProcessReplacing();
	strcpy(programName, "Soft Overdrive");

  filt1 = filt2 = 0.0f;
  setParameter(0, 0.0f);
}

mdaOverdrive::~mdaOverdrive()
{

}

bool  mdaOverdrive::getProductString(char* text) { strcpy(text, "MDA Overdrive"); return true; }
bool  mdaOverdrive::getVendorString(char* text)  { strcpy(text, "mda"); return true; }
bool  mdaOverdrive::getEffectName(char* name)    { strcpy(name, "Overdrive"); return true; }

void mdaOverdrive::setProgramName(char *name)
{
	strcpy(programName, name);
}

void mdaOverdrive::getProgramName(char *name)
{
	strcpy(name, programName);
}

bool mdaOverdrive::getProgramNameIndexed (int32_t category, int32_t index, char* name)
{
	if (index == 0) 
	{
	    strcpy(name, programName);
	    return true;
	}
	return false;
}

void mdaOverdrive::setParameter(int32_t index, float value)
{
	switch(index)
  {
    case 0: fParam1 = value; break;
    case 1: fParam2 = value; break;
    case 2: fParam3 = value; break;
  }
  filt = (float)pow(10.0,-1.6 * fParam2);
  gain = (float)pow(10.0f, 2.0f * fParam3 - 1.0f);
}

float mdaOverdrive::getParameter(int32_t index)
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

void mdaOverdrive::getParameterName(int32_t index, char *label)
{
	switch(index)
  {
    case 0: strcpy(label, "Drive"); break;
    case 1: strcpy(label, "Muffle"); break;
    case 2: strcpy(label, "Output"); break;
  }
}

#include <stdio.h>
static void int2strng(int32_t value, char *string) { sprintf(string, "%d", value); }

void mdaOverdrive::getParameterDisplay(int32_t index, char *text)
{
	switch(index)
  {
    case 0: int2strng((int32_t)(100 * fParam1     ), text); break;
    case 1: int2strng((int32_t)(100 * fParam2     ), text); break;
    case 2: int2strng((int32_t)( 40 * fParam3 - 20), text); break;
  }

}

void mdaOverdrive::getParameterLabel(int32_t index, char *label)
{
	switch(index)
  {
    case 0: strcpy(label, "%"); break;
    case 1: strcpy(label, "%"); break;
    case 2: strcpy(label, "dB"); break;
  }
}

//--------------------------------------------------------------------------------
// process

void mdaOverdrive::process(float **inputs, float **outputs, int32_t sampleFrames)
{
	float *in1 = inputs[0];
	float *in2 = inputs[1];
	float *out1 = outputs[0];
	float *out2 = outputs[1];
	float a, b, c, d;
  float i=fParam1, g=gain, aa, bb;
  float f=filt, fa=filt1, fb=filt2;

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

    aa = (a>0.0f)? (float)sqrt(a) : (float)-sqrt(-a); //overdrive
    bb = (b>0.0f)? (float)sqrt(b) : (float)-sqrt(-b);

    fa = fa + f * (i*(aa-a) + a - fa);                //filter
    fb = fb + f * (i*(bb-b) + b - fb);

    c += fa * g;
		d += fb * g;

    *++out1 = c;
		*++out2 = d;
	}
  if(fabs(fa)>1.0e-10) filt1 = fa; else filt1 = 0.0f; //catch denormals
  if(fabs(fb)>1.0e-10) filt2 = fb; else filt2 = 0.0f;
}

void mdaOverdrive::processReplacing(float **inputs, float **outputs, int32_t sampleFrames)
{
	float *in1 = inputs[0];
	float *in2 = inputs[1];
	float *out1 = outputs[0];
	float *out2 = outputs[1];
	float a, b, c, d;
  float i=fParam1, g=gain, aa, bb;
  float f=filt, fa=filt1, fb=filt2;

	--in1;
	--in2;
	--out1;
	--out2;
	while(--sampleFrames >= 0)
	{
		a = *++in1;
		b = *++in2;

    aa = (a>0.0f)? (float)sqrt(a) : (float)-sqrt(-a); //overdrive
    bb = (b>0.0f)? (float)sqrt(b) : (float)-sqrt(-b);

    fa = fa + f * (i*(aa-a) + a - fa);                //filter
    fb = fb + f * (i*(bb-b) + b - fb);

    c = fa * g;
		d = fb * g;

    *++out1 = c;
		*++out2 = d;
	}
  if(fabs(fa)>1.0e-10) filt1 = fa; else filt1 = 0.0f; //catch denormals
  if(fabs(fb)>1.0e-10) filt2 = fb; else filt2 = 0.0f;
}
