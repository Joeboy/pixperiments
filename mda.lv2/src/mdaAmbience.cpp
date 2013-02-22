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

#include "mdaAmbience.h"

#include <stdio.h>
#include <float.h>
#include <math.h>

AudioEffect *createEffectInstance(audioMasterCallback audioMaster)
{
  return new mdaAmbience(audioMaster);
}

mdaAmbience::mdaAmbience(audioMasterCallback audioMaster) : AudioEffectX(audioMaster, 1, 4)	// programs, parameters
{
  //inits here!
  fParam0 = 0.7f; //size
  fParam1 = 0.7f; //hf
  fParam2 = 0.9f; //mix
  fParam3 = 0.5f; //output

  size = 0;

	buf1 = new float[1024];
	buf2 = new float[1024];
	buf3 = new float[1024];
	buf4 = new float[1024];

  fil = 0.0f;
  den = pos=0;

  setNumInputs(2);
	setNumOutputs(2);
	setUniqueID("mdaAmb");    // identify here
	DECLARE_LVZ_DEPRECATED(canMono) ();
	canProcessReplacing();
	strcpy(programName, "Small Space Ambience");

  suspend();		// flush buffer
  setParameter(0, 0.7f); //go and set initial values!
}

void mdaAmbience::setParameter(int32_t index, float value)
{
	float tmp;

  switch(index)
  {
    case 0: fParam0 = value; break;
    case 1: fParam1 = value; break;
    case 2: fParam2 = value; break;
    case 3: fParam3 = value; break;
 }
  //calcs here
  fbak = 0.8f;
  damp = 0.05f + 0.9f * fParam1;
  tmp = (float)pow(10.0f, 2.0f * fParam3 - 1.0f);
  dry = tmp - fParam2 * fParam2 * tmp;
  wet = (0.4f + 0.4f) * fParam2 * tmp;

  tmp = 0.025f + 2.665f * fParam0;
  if(size!=tmp) rdy=0;  //need to flush buffer
  size = tmp;
}

mdaAmbience::~mdaAmbience()
{
	if(buf1) delete [] buf1;
	if(buf2) delete [] buf2;
	if(buf3) delete [] buf3;
	if(buf4) delete [] buf4;
}

bool  mdaAmbience::getProductString(char* text) { strcpy(text, "MDA Ambience"); return true; }
bool  mdaAmbience::getVendorString(char* text)  { strcpy(text, "mda"); return true; }
bool  mdaAmbience::getEffectName(char* name)    { strcpy(name, "Ambience"); return true; }

void mdaAmbience::suspend()
{
	memset(buf1, 0, 1024 * sizeof(float));
	memset(buf2, 0, 1024 * sizeof(float));
	memset(buf3, 0, 1024 * sizeof(float));
	memset(buf4, 0, 1024 * sizeof(float));
  rdy = 1;
}

void mdaAmbience::setProgramName(char *name)
{
	strcpy(programName, name);
}

void mdaAmbience::getProgramName(char *name)
{
	strcpy(name, programName);
}

bool mdaAmbience::getProgramNameIndexed (int32_t category, int32_t index, char* name)
{
	if (index == 0) 
	{
	    strcpy(name, programName);
	    return true;
	}
	return false;
}

float mdaAmbience::getParameter(int32_t index)
{
	float v=0;

  switch(index)
  {
    case 0: v = fParam0; break;
    case 1: v = fParam1; break;
    case 2: v = fParam2; break;
    case 3: v = fParam3; break;
  }
  return v;
}

void mdaAmbience::getParameterName(int32_t index, char *label)
{
	switch(index)
  {
    case 0: strcpy(label, "Size"); break;
    case 1: strcpy(label, "HF Damp"); break;
    case 2: strcpy(label, "Mix"); break;
    case 3: strcpy(label, "Output"); break;
  }
}

void mdaAmbience::getParameterDisplay(int32_t index, char *text)
{
	switch(index)
  {
    case 0: sprintf(text, "%.0f",  10.0f * fParam0); break;
    case 1: sprintf(text, "%.0f", 100.0f * fParam1); break;
    case 2: sprintf(text, "%.0f", 100.0f * fParam2); break;
    case 3: sprintf(text, "%.0f",  40.0f * fParam3 - 20.0f); break;
  }
}

void mdaAmbience::getParameterLabel(int32_t index, char *label)
{
	switch(index)
  {
    case 0: strcpy(label, "m"); break;
    case 1: strcpy(label, "%"); break;
    case 2: strcpy(label, "%"); break;
    case 3: strcpy(label, "dB"); break;
  }
}

//--------------------------------------------------------------------------------
// process

void mdaAmbience::process(float **inputs, float **outputs, int32_t sampleFrames)
{
	float *in1 = inputs[0];
	float *in2 = inputs[1];
	float *out1 = outputs[0];
	float *out2 = outputs[1];
	float a, b, c, d, r;
	float t, f=fil, fb=fbak, dmp=damp, y=dry, w=wet;
  int32_t  p=pos, d1, d2, d3, d4;

  if(rdy==0) suspend();

  d1 = (p + (int32_t)(107 * size)) & 1023;
  d2 = (p + (int32_t)(142 * size)) & 1023;
  d3 = (p + (int32_t)(277 * size)) & 1023;
  d4 = (p + (int32_t)(379 * size)) & 1023;

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

    f += dmp * (w * (a + b) - f); //HF damping
    r = f;

    t = *(buf1 + p);
    r -= fb * t;
    *(buf1 + d1) = r; //allpass
    r += t;

    t = *(buf2 + p);
    r -= fb * t;
    *(buf2 + d2) = r; //allpass
    r += t;

    t = *(buf3 + p);
    r -= fb * t;
    *(buf3 + d3) = r; //allpass
    r += t;
    c += y * a + r - f; //left output

    t = *(buf4 + p);
    r -= fb * t;
    *(buf4 + d4) = r; //allpass
    r += t;
    d += y * b + r - f; //right output

    ++p  &= 1023;
    ++d1 &= 1023;
    ++d2 &= 1023;
    ++d3 &= 1023;
    ++d4 &= 1023;

    *++out1 = c;
		*++out2 = d;
  }
  pos=p;
  if(fabs(f)>1.0e-10) { fil=f;  den=0; }  //catch denormals
                 else { fil=0.0f;  if(den==0) { den=1;  suspend(); } }
}

void mdaAmbience::processReplacing(float **inputs, float **outputs, int32_t sampleFrames)
{
	float *in1 = inputs[0];
	float *in2 = inputs[1];
	float *out1 = outputs[0];
	float *out2 = outputs[1];
	float a, b, r;
  float t, f=fil, fb=fbak, dmp=damp, y=dry, w=wet;
  int32_t  p=pos, d1, d2, d3, d4;

  if(rdy==0) suspend();

  d1 = (p + (int32_t)(107 * size)) & 1023;
  d2 = (p + (int32_t)(142 * size)) & 1023;
  d3 = (p + (int32_t)(277 * size)) & 1023;
  d4 = (p + (int32_t)(379 * size)) & 1023;

	--in1;
	--in2;
	--out1;
	--out2;

	while(--sampleFrames >= 0)
	{
		a = *++in1;
    b = *++in2;

    f += dmp * (w * (a + b) - f); //HF damping
    r = f;

    t = *(buf1 + p);
    r -= fb * t;
    *(buf1 + d1) = r; //allpass
    r += t;

    t = *(buf2 + p);
    r -= fb * t;
    *(buf2 + d2) = r; //allpass
    r += t;

    t = *(buf3 + p);
    r -= fb * t;
    *(buf3 + d3) = r; //allpass
    r += t;
    a = y * a + r - f; //left output

    t = *(buf4 + p);
    r -= fb * t;
    *(buf4 + d4) = r; //allpass
    r += t;
    b = y * b + r - f; //right output

    ++p  &= 1023;
    ++d1 &= 1023;
    ++d2 &= 1023;
    ++d3 &= 1023;
    ++d4 &= 1023;

    *++out1 = a;
		*++out2 = b;
	}
  pos=p;
  if(fabs(f)>1.0e-10) { fil=f;  den=0; }  //catch denormals
                 else { fil=0.0f;  if(den==0) { den=1;  suspend(); } }
}
