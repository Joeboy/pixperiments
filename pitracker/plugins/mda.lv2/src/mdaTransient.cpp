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

#include "mdaTransient.h"

#include <math.h>

AudioEffect *createEffectInstance(audioMasterCallback audioMaster)
{
  return new mdaTransient(audioMaster);
}

mdaTransient::mdaTransient(audioMasterCallback audioMaster)	: AudioEffectX(audioMaster, 1, 6)
{
	fParam1 = (float)0.50; //attack
  fParam2 = (float)0.50; //release
  fParam3 = (float)0.50; //output
  fParam4 = (float)0.49; //filter
  fParam5 = (float)0.35; //att-rel
  fParam6 = (float)0.35; //rel-att

  setNumInputs(2);		    // stereo in
	setNumOutputs(2);		    // stereo out
	setUniqueID("mdaTransient");    // identify
	DECLARE_LVZ_DEPRECATED(canMono) ();
	canProcessReplacing();	// supports both accumulating and replacing output
	strcpy(programName, "Transient Processor");	// default program name

	dry = att1 = att2 = rel12 = att34 = rel3 = rel4 = 0.0f;
	env1 = env2 = env3 = env4 = fili = filo = filx = fbuf1 = fbuf2 = 0.0f;
  setParameter(0, 0.5f);
}

mdaTransient::~mdaTransient()
{
	// nothing to do here
}

bool  mdaTransient::getProductString(char* text) { strcpy(text, "MDA Transient"); return true; }
bool  mdaTransient::getVendorString(char* text)  { strcpy(text, "mda"); return true; }
bool  mdaTransient::getEffectName(char* name)    { strcpy(name, "Transient"); return true; }

void mdaTransient::setProgramName(char *name)
{
	strcpy(programName, name);
}

void mdaTransient::getProgramName(char *name)
{
	strcpy(name, programName);
}

bool mdaTransient::getProgramNameIndexed (int32_t category, int32_t index, char* name)
{
	if (index == 0) 
	{
	    strcpy(name, programName);
	    return true;
	}
	return false;
}

void mdaTransient::setParameter(int32_t index, float value)
{
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
  dry = (float)(pow(10.0, (2.0 * fParam3) - 1.0));
  if(fParam4>0.50)
  {
    fili = 0.8f - 1.6f*fParam4;
    filo = 1.f + fili;
    filx = 1.f;
  }
  else
  {
    fili = 0.1f + 1.8f*fParam4;
    filo = 1.f - fili;
    filx = 0.f;
  }

  if(fParam1>0.5)
  {
    att1 = (float)pow(10.0, -1.5);
    att2 = (float)pow(10.0, 1.0 - 5.0 * fParam1);
  }
  else
  {
    att1 = (float)pow(10.0, -4.0 + 5.0 * fParam1);
    att2 = (float)pow(10.0, -1.5);
  }
  rel12 = 1.f - (float)pow(10.0, -2.0 - 4.0 * fParam5);

  if(fParam2>0.5)
  {
    rel3 = 1.f - (float)pow(10.0, -4.5);
    rel4 = 1.f - (float)pow(10.0, -5.85 + 2.7 * fParam2);
  }
  else
  {
    rel3 = 1.f - (float)pow(10.0, -3.15 - 2.7 * fParam2);
    rel4 = 1.f - (float)pow(10.0, -4.5);
  }
  att34 = (float)pow(10.0, - 4.0 * fParam6);
}

float mdaTransient::getParameter(int32_t index)
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

void mdaTransient::getParameterName(int32_t index, char *label)
{
	switch(index)
  {
    case 0: strcpy(label, "Attack"); break;
    case 1: strcpy(label, "Release"); break;
    case 2: strcpy(label, "Output"); break;
    case 3: strcpy(label, "Filter"); break;
    case 4: strcpy(label, "Att Hold"); break;
    case 5: strcpy(label, "Rel Hold"); break;
  }
}

#include <stdio.h>
static void int2strng(int32_t value, char *string) { sprintf(string, "%d", value); }

void mdaTransient::getParameterDisplay(int32_t index, char *text)
{
	switch(index)
  {
    case 0: int2strng((int32_t)(200*fParam1 - 100),text); break;
    case 1: int2strng((int32_t)(200*fParam2 - 100),text); break;
    case 2: int2strng((int32_t)(40.0*fParam3 - 20.0),text); break;
    case 3: int2strng((int32_t)(20*fParam4 - 10),text); break;
    case 4: int2strng((int32_t)(100*fParam5),text); break;
    case 5: int2strng((int32_t)(100*fParam6),text); break;
  }

}

void mdaTransient::getParameterLabel(int32_t index, char *label)
{
	switch(index)
  {
    case 0: strcpy(label, "%"); break;
    case 1: strcpy(label, "%"); break;
    case 2: strcpy(label, "dB"); break;
    case 3: strcpy(label, "Lo <> Hi"); break;
    case 4: strcpy(label, "%"); break;
    case 5: strcpy(label, "%"); break;
  }
}

//--------------------------------------------------------------------------------
// process

void mdaTransient::process(float **inputs, float **outputs, int32_t sampleFrames)
{
	float *in1 = inputs[0];
	float *in2 = inputs[1];
	float *out1 = outputs[0];
	float *out2 = outputs[1];
	float a, b, c, d, e, f, g, i;
  float e1=env1, e2=env2, e3=env3, e4=env4, y=dry;
  float a1=att1, a2=att2, r12=rel12, a34=att34, r3=rel3, r4=rel4;
  float fi=fili, fo=filo, fx=filx, fb1=fbuf1, fb2=fbuf2;

  --in1;
	--in2;
	--out1;
	--out2;

 	while(--sampleFrames >= 0)
	{
    a = *++in1;
    b = *++in2;
    c = out1[1];
    d = out1[2];

    fb1 = fo*fb1 + fi*a;
    fb2 = fo*fb2 + fi*b;
    e = fb1 + fx*a;
    f = fb2 + fx*b;

    i = a + b; i = (i>0)? i : -i;
    e1 = (i>e1)? e1 + a1 * (i-e1) : e1 * r12;
    e2 = (i>e2)? e2 + a2 * (i-e2) : e2 * r12;
    e3 = (i>e3)? e3 + a34 * (i-e3) : e3 * r3;
    e4 = (i>e4)? e4 + a34 * (i-e4) : e4 * r4;
    g = (e1 - e2 + e3 - e4);

    *++out1 = c + y * (a + e * g);
		*++out2 = d + y * (b + f * g);
	}
  if(e1<1.0e-10) { env1=0.f; env2=0.f; env3=0.f; env4=0.f; fbuf1=0.f; fbuf2=0.f; }
            else { env1=e1;  env2=e2;  env3=e3;  env4=e4;  fbuf1=fb1; fbuf2=fb2; }
}

void mdaTransient::processReplacing(float **inputs, float **outputs, int32_t sampleFrames)
{
	float *in1 = inputs[0];
	float *in2 = inputs[1];
	float *out1 = outputs[0];
	float *out2 = outputs[1];
	float a, b, e, f, g, i;
  float e1=env1, e2=env2, e3=env3, e4=env4, y=dry;
  float a1=att1, a2=att2, r12=rel12, a34=att34, r3=rel3, r4=rel4;
  float fi=fili, fo=filo, fx=filx, fb1=fbuf1, fb2=fbuf2;

	--in1;
	--in2;
	--out1;
	--out2;

 	while(--sampleFrames >= 0)
	{
    a = *++in1;
    b = *++in2;

    fb1 = fo*fb1 + fi*a;
    fb2 = fo*fb2 + fi*b;
    e = fb1 + fx*a;
    f = fb2 + fx*b;

    i = a + b; i = (i>0)? i : -i;
    e1 = (i>e1)? e1 + a1 * (i-e1) : e1 * r12;
    e2 = (i>e2)? e2 + a2 * (i-e2) : e2 * r12;
    e3 = (i>e3)? e3 + a34 * (i-e3) : e3 * r3;
    e4 = (i>e4)? e4 + a34 * (i-e4) : e4 * r4;
    g = (e1 - e2 + e3 - e4);

    *++out1 = y * (a + e * g);
		*++out2 = y * (b + f * g);
	}
  if(e1<1.0e-10) { env1=0.f; env2=0.f; env3=0.f; env4=0.f; fbuf1=0.f; fbuf2=0.f; }
            else { env1=e1;  env2=e2;  env3=e3;  env4=e4;  fbuf1=fb1; fbuf2=fb2; }
}
