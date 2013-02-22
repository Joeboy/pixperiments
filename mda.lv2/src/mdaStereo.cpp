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

#include "mdaStereo.h"

#include <math.h>
#include <float.h>

AudioEffect *createEffectInstance(audioMasterCallback audioMaster)
{
  return new mdaStereo(audioMaster);
}

mdaStereo::mdaStereo(audioMasterCallback audioMaster)	: AudioEffectX(audioMaster, 1, 5)	// programs, parameters
	, fParam6(0.0f)
{
  //inits here!
  fParam1 = (float)0.78; //Haas/Comb width
  fParam2 = (float)0.43; //delay
  fParam3 = (float)0.50; //balance
  fParam4 = (float)0.00; //mod
  fParam5 = (float)0.50; //rate

  size = 4800;
  bufpos = 0;
	buffer = new float[size];

  setNumInputs(2);
	setNumOutputs(2);
	setUniqueID("mdaStereo");    // identify here
	DECLARE_LVZ_DEPRECATED(canMono) ();
	canProcessReplacing();
	strcpy(programName, "Stereo Simulator");
	suspend();		// flush buffer

  //calcs here!
  phi=0;
  dphi=(float)(3.141 * pow(10.0,-2.0 + 3.0 * fParam5) / getSampleRate());
  mod=(float)(2100.0 * pow(fParam4, 2));

  if(fParam1<0.5)
  {
    fli = (float)(0.25 + (1.5 * fParam1));
    fld = 0.0;
    fri = (float)(2.0 * fParam1);
    frd = (float)(1.0 - fri);
  }
  else
  {
    fli = (float)(1.5 - fParam1);
    fld = (float)(fParam1 - 0.5);
    fri = fli;
    frd = -fld;
  }
  fdel = (float)(20.0 + 2080.0 * pow(fParam2, 2));
  if(fParam3>0.5)
  {
    fli *= (float)((1.0 - fParam3) * 2.0);
    fld *= (float)((1.0 - fParam3) * 2.0);
  }
  else
  {
    fri *= (2 * fParam3);
    frd *= (2 * fParam3);
  }
  fri *= (float)(0.5 + fabs(fParam1 - 0.5));
  frd *= (float)(0.5 + fabs(fParam1 - 0.5));
  fli *= (float)(0.5 + fabs(fParam1 - 0.5));
  fld *= (float)(0.5 + fabs(fParam1 - 0.5));
}

//still do something sensible with stereo inputs? Haas?

bool  mdaStereo::getProductString(char* text) { strcpy(text, "MDA Stereo"); return true; }
bool  mdaStereo::getVendorString(char* text)  { strcpy(text, "mda"); return true; }
bool  mdaStereo::getEffectName(char* name)    { strcpy(name, "Stereo"); return true; }

void mdaStereo::setParameter(int32_t index, float value)
{
	switch(index)
  {
    case 0: fParam1 = value; break;
    case 1: fParam2 = value; break;
    case 2: fParam3 = value; break;
    case 3: fParam4 = value; break;
    case 4: fParam5 = value; break;
  }
  //calcs here
  dphi=(float)(3.141 * pow(10.0,-2.0 + 3.0 * fParam5) / getSampleRate());
  mod=(float)(2100.0 * pow(fParam4, 2));

  if(fParam1<0.5)
  {
    fli = (float)(0.25 + (1.5 * fParam1));
    fld = 0.0;
    fri = (float)(2.0 * fParam1);
    frd = (float)(1.0 - fri);
  }
  else
  {
    fli = (float)(1.5 - fParam1);
    fld = (float)(fParam1 - 0.5);
    fri = fli;
    frd = -fld;
  }
  fdel = (float)(20.0 + 2080.0 * pow(fParam2, 2));
  if(fParam3>0.5)
  {
    fli *= (float)((1.0 - fParam3) * 2.0);
    fld *= (float)((1.0 - fParam3) * 2.0);
  }
  else
  {
    fri *= (2 * fParam3);
    frd *= (2 * fParam3);
  }
  fri *= (float)(0.5 + fabs(fParam1 - 0.5));
  frd *= (float)(0.5 + fabs(fParam1 - 0.5));
  fli *= (float)(0.5 + fabs(fParam1 - 0.5));
  fld *= (float)(0.5 + fabs(fParam1 - 0.5));
}

mdaStereo::~mdaStereo()
{
	if(buffer) delete [] buffer;
}

void mdaStereo::suspend()
{
	memset(buffer, 0, size * sizeof(float));
}

void mdaStereo::setProgramName(char *name)
{
	strcpy(programName, name);
}

void mdaStereo::getProgramName(char *name)
{
	strcpy(name, programName);
}

bool mdaStereo::getProgramNameIndexed (int32_t category, int32_t index, char* name)
{
	if (index == 0) 
	{
	    strcpy(name, programName);
	    return true;
	}
	return false;
}

float mdaStereo::getParameter(int32_t index)
{
	float v=0;

  switch(index)
  {
    case 0: v = fParam1; break;
    case 1: v = fParam2; break;
    case 2: v = fParam3; break;
    case 3: v = fParam4; break;
    case 4: v = fParam5; break;
  }
  return v;
}

void mdaStereo::getParameterName(int32_t index, char *label)
{
	switch(index)
  {
    case 0: strcpy(label, "Width"); break;
    case 1: strcpy(label, "Delay"); break;
    case 2: strcpy(label, "Balance"); break;
    case 3: strcpy(label, "Mod"); break;
    case 4: strcpy(label, "Rate"); break;
  }
}

#include <stdio.h>
static void int2strng(int32_t value, char *string) { sprintf(string, "%d", value); }
static void float2strng(float value, char *string) { sprintf(string, "%.2f", value); }

void mdaStereo::getParameterDisplay(int32_t index, char *text)
{
	switch(index)
  {
    case 0: int2strng((int32_t)(200.0 * fabs(fParam1 - 0.5)), text);break;
    case 1: float2strng((float)(1000.0 * fdel / getSampleRate()), text); break;
    case 2: int2strng((int32_t)(200.0 * (fParam3 - 0.5)), text); break;
    case 3: if(mod>0.f) float2strng((float)(1000.0 * mod / getSampleRate()), text);
            else strcpy(text, "OFF"); break;
    case 4: float2strng((float)pow(10.0,2.0 - 3.0 * fParam5), text); break;
  }
}

void mdaStereo::getParameterLabel(int32_t index, char *label)
{
	switch(index)
  {
    case 0: if(fParam1<0.5) { strcpy(label, "Haas"); }
            else { strcpy(label, "Comb"); } break;
    case 1: strcpy(label, "ms"); break;
    case 2: strcpy(label, ""); break;
    case 3: strcpy(label, "ms"); break;
    case 4: strcpy(label, "sec"); break;
  }
}

//--------------------------------------------------------------------------------
// process

void mdaStereo::process(float **inputs, float **outputs, int32_t sampleFrames)
{
	float *in1 = inputs[0];
	float *in2 = inputs[1];
	float *out1 = outputs[0];
	float *out2 = outputs[1];
	float a, b, c, d;
  float li, ld, ri, rd, del, ph=phi, dph=dphi, mo=mod;
  int32_t  tmp, bp = bufpos;

  li = fli;
  ld = fld;
  ri = fri;
  rd = frd;
  del = fdel;

  --in1;
	--in2;
	--out1;
	--out2;

  if(mo>0.f) //modulated delay
  {
 	  while(--sampleFrames >= 0)
	  {
		  a = *++in1 + *++in2; //sum to mono

      c = out1[1];
		  d = out2[1]; //process from here...

      *(buffer + bp) = a; //write

      tmp = (bp + (int)(del + fabs(mo * sin(ph)) ) ) % 4410;
      b = *(buffer + tmp);
      c += (a * li) - (b * ld); // output
		  d += (a * ri) - (b * rd);

      bp = (bp - 1); if(bp < 0) bp = 4410; //buffer position

      ph = ph + dph;

      *++out1 = c;
		  *++out2 = d;
	  }
  }
  else
  {
    while(--sampleFrames >= 0)
	  {
		  a = *++in1 + *++in2; //sum to mono

      c = out1[1];
		  d = out2[1]; //process from here...

      *(buffer + bp) = a; //write

      tmp = (bp + (int)(del) ) % 4410;
      b = *(buffer + tmp);
      c += (a * li) - (b * ld); // output
		  d += (a * ri) - (b * rd);

      bp = (bp - 1); if(bp < 0) bp = 4410; //buffer position

      *++out1 = c;
		  *++out2 = d;
	  }
  }
  bufpos = bp;
 phi = (float)fmod(ph,6.2831853f);
}

void mdaStereo::processReplacing(float **inputs, float **outputs, int32_t sampleFrames)
{
	float *in1 = inputs[0];
	float *in2 = inputs[1];
	float *out1 = outputs[0];
	float *out2 = outputs[1];
	float a, b, c, d;
  float li, ld, ri, rd, del, ph=phi, dph=dphi, mo=mod;
  int32_t tmp, bp = bufpos;

  li = fli;
  ld = fld;
  ri = fri;
  rd = frd;
  del = fdel;

	--in1;
	--in2;
	--out1;
	--out2;
	if(mo>0.f) //modulated delay
  {
    while(--sampleFrames >= 0)
	  {
		  a = *++in1 + *++in2; //sum to mono

      *(buffer + bp) = a; //write
      tmp = (bp + (int)(del + fabs(mo * sin(ph)) ) ) % 4410;
      b = *(buffer + tmp);

		  c = (a * li) - (b * ld); // output
		  d = (a * ri) - (b * rd);

      bp = (bp - 1); if(bp < 0) bp = 4410; //buffer position

      ph = ph + dph;

      *++out1 = c;
		  *++out2 = d;
	  }
  }
  else
  {
    while(--sampleFrames >= 0)
	  {
		  a = *++in1 + *++in2; //sum to mono

      *(buffer + bp) = a; //write
      tmp = (bp + (int)(del) ) % 4410;
      b = *(buffer + tmp);

		  c = (a * li) - (b * ld); // output
		  d = (a * ri) - (b * rd);

      bp = (bp - 1); if(bp < 0) bp = 4410; //buffer position

      *++out1 = c;
		  *++out2 = d;
	  }
  }
  bufpos = bp;
  phi = (float)fmod(ph,6.2831853f);
}
