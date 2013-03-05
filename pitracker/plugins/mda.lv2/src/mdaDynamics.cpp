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

#include "mdaDynamics.h"

#include <math.h>

AudioEffect *createEffectInstance(audioMasterCallback audioMaster)
{
  return new mdaDynamics(audioMaster);
}

mdaDynamics::mdaDynamics(audioMasterCallback audioMaster)	: AudioEffectX(audioMaster, 1, 10)	// 1 program, 4 parameters
{
	fParam1 = (float)0.60; //thresh 		///Note : special version for ardislarge
  fParam2 = (float)0.40; //ratio
  fParam3 = (float)0.10; //level      ///was 0.6
  fParam4 = (float)0.18; //attack
  fParam5 = (float)0.55; //release
  fParam6 = (float)1.00; //Limiter
  fParam7 = (float)0.00; //gate thresh
  fParam8 = (float)0.10; //gate attack
  fParam9 = (float)0.50; //gate decay
  fParam10= (float)1.00; //fx mix
  thr = rat = env = env2 = att = rel = trim = lthr = xthr = xrat = dry = 0.0f;
  genv = gatt = irel = 0.0f;
  mode = 0;

  setNumInputs(2);		    // stereo in
	setNumOutputs(2);		    // stereo out
	setUniqueID("mdaDynamics");    // identify
	DECLARE_LVZ_DEPRECATED(canMono) ();
	canProcessReplacing();	// supports both accumulating and replacing output
	strcpy(programName, "Dynamics");	// default program name

  setParameter(6, 0.f); //initial settings
}

bool  mdaDynamics::getProductString(char* text) { strcpy(text, "MDA Dynamics"); return true; }
bool  mdaDynamics::getVendorString(char* text)  { strcpy(text, "mda"); return true; }
bool  mdaDynamics::getEffectName(char* name)    { strcpy(name, "Dynamics"); return true; }

mdaDynamics::~mdaDynamics()
{
	// nothing to do here
}

void mdaDynamics::setProgramName(char *name)
{
	strcpy(programName, name);
}

void mdaDynamics::getProgramName(char *name)
{
	strcpy(name, programName);
}

bool mdaDynamics::getProgramNameIndexed (int32_t category, int32_t index, char* name)
{
	if (index == 0) 
	{
	    strcpy(name, programName);
	    return true;
	}
	return false;
}

void mdaDynamics::setParameter(int32_t index, float value)
{
	switch(index)
  {
    case 0: fParam1 = value; break;
    case 1: fParam2 = value; break;
    case 2: fParam3 = value; break;
    case 3: fParam4 = value; break;
    case 4: fParam5 = value; break;
    case 5: fParam6 = value; break;
    case 6: fParam7 = value; break;
    case 7: fParam8 = value; break;
    case 8: fParam9 = value; break;
    case 9: fParam10= value; break;
  }
  //calcs here
  mode=0;
  thr = (float)pow(10.f, 2.f * fParam1 - 2.f);
  rat = 2.5f * fParam2 - 0.5f;
  if(rat>1.0) { rat = 1.f + 16.f*(rat-1.f) * (rat - 1.f); mode = 1; }
  if(rat<0.0) { rat = 0.6f*rat; mode=1; }
  trim = (float)pow(10.f, 2.f * fParam3); //was  - 1.f);
  att = (float)pow(10.f, -0.002f - 2.f * fParam4);
  rel = (float)pow(10.f, -2.f - 3.f * fParam5);

  if(fParam6>0.98) lthr=0.f; //limiter
  else { lthr=0.99f*(float)pow(10.0f,int(30.0*fParam6 - 20.0)/20.f);
         mode=1; }

  if(fParam7<0.02) { xthr=0.f; } //expander
  else { xthr=(float)pow(10.f,3.f * fParam7 - 3.f); mode=1; }
  xrat = 1.f - (float)pow(10.f, -2.f - 3.3f * fParam9);
  irel = (float)pow(10.0,-2.0/getSampleRate());
  gatt = (float)pow(10.f, -0.002f - 3.f * fParam8);

  if(rat<0.0f && thr<0.1f) rat *= thr*15.f;

  dry = 1.0f - fParam10;  trim *= fParam10; //fx mix
}

float mdaDynamics::getParameter(int32_t index)
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
    case 6: v = fParam7; break;
    case 7: v = fParam8; break;
    case 8: v = fParam9; break;
    case 9: v = fParam10; break;
  }
  return v;
}

void mdaDynamics::getParameterName(int32_t index, char *label)
{
	switch(index)
  {
    case 0: strcpy(label, "Thresh"); break;
    case 1: strcpy(label, "Ratio"); break;
    case 2: strcpy(label, "Output"); break;
    case 3: strcpy(label, "Attack"); break;
    case 4: strcpy(label, "Release"); break;
    case 5: strcpy(label, "Limiter"); break;
    case 6: strcpy(label, "Gate Thr"); break;
    case 7: strcpy(label, "Gate Att"); break;
    case 8: strcpy(label, "Gate Rel"); break;
    case 9: strcpy(label, "Mix"); break;
  }
}

#include <stdio.h>
static void int2strng(int32_t value, char *string) { sprintf(string, "%d", value); }
static void float2strng(float value, char *string) { sprintf(string, "%.2f", value); }

void mdaDynamics::getParameterDisplay(int32_t index, char *text)
{
	switch(index)
  {
    case 0: int2strng((int32_t)(40.0*fParam1 - 40.0),text); break;
    case 1: if(fParam2>0.58)
            { if(fParam2<0.62) strcpy(text, "Limit");
              else float2strng(-rat,text); }
            else
            { if(fParam2<0.2) float2strng(0.5f+2.5f*fParam2,text);
              else float2strng(1.f/(1.f-rat),text); } break;
    case 2: int2strng((int32_t)(40.0*fParam3 - 0.0),text); break; ///was -20.0
    case 3: int2strng((int32_t)(-301030.1 / (getSampleRate() * log10(1.0 - att))),text); break;
    case 4: int2strng((int32_t)(-301.0301 / (getSampleRate() * log10(1.0 - rel))),text); break;
    case 5: if(lthr==0.f) strcpy(text, "OFF");
            else int2strng((int32_t)(30.0*fParam6 - 20.0),text); break;
    case 6: if(xthr==0.f) strcpy(text, "OFF");
            else int2strng((int32_t)(60.0*fParam7 - 60.0),text); break;
    case 7: int2strng((int32_t)(-301030.1 / (getSampleRate() * log10(1.0 - gatt))),text); break;
    case 8: int2strng((int32_t)(-1806.0 / (getSampleRate() * log10(xrat))),text); break;
    case 9: int2strng((int32_t)(100.0*fParam10),text); break;

  }
}

void mdaDynamics::getParameterLabel(int32_t index, char *label)
{
	switch(index)
  {
    case 0: strcpy(label, "dB"); break;
    case 1: strcpy(label, ":1"); break;
    case 2: strcpy(label, "dB"); break;
    case 3: strcpy(label, "�s"); break; 
    case 4: strcpy(label, "ms"); break;
    case 5: strcpy(label, "dB"); break;
    case 6: strcpy(label, "dB"); break;
    case 7: strcpy(label, "�s"); break; 
    case 8: strcpy(label, "ms"); break;
    case 9: strcpy(label, "%"); break;
  }
}

//--------------------------------------------------------------------------------
// process

void mdaDynamics::process(float **inputs, float **outputs, int32_t sampleFrames)
{
	float *in1 = inputs[0];
	float *in2 = inputs[1];
	float *out1 = outputs[0];
	float *out2 = outputs[1];
  float a, b, c, d, i, j, g, e=env, e2=env2, ra=rat, re=(1.f-rel), at=att, ga=gatt;
  float tr=trim, th=thr, lth=lthr, xth=xthr, ge=genv, y=dry;

  --in1;
	--in2;
	--out1;
	--out2;

  if(mode) //comp/gate/lim
  {
    if(lth==0.f) lth=1000.f;
	  while(--sampleFrames >= 0)
	  {
      a = *++in1;
      b = *++in2;
      c = out1[1];
      d = out2[1];

      i = (a<0.f)? -a : a;
      j = (b<0.f)? -b : b;
      i = (j>i)? j : i;

      e = (i>e)? e + at * (i - e) : e * re;
      e2 = (i>e)? i : e2 * re; //ir;
      g = (e>th)? tr / (1.f + ra * ((e/th) - 1.f)) : tr;

      if(g<0.f) g=0.f;
      if(g*e2>lth) g = lth/e2; //limit

      ge = (e>xth)? ge + ga - ga * ge : ge * xrat; //gate

      c += a * (g * ge + y);
      d += b * (g * ge + y);

      *++out1 = c;
		  *++out2 = d;
	  }
  }
  else //compressor only
  {
    while(--sampleFrames >= 0)
	  {
      a = *++in1;
      b = *++in2;
      c = out1[1];
      d = out2[1];

      i = (a<0.f)? -a : a;
      j = (b<0.f)? -b : b;
      i = (j>i)? j : i;

      e = (i>e)? e + at * (i - e) : e * re;
      g = (e>th)? tr / (1.f + ra * ((e/th) - 1.f)) : tr;

      c += a * (g + y);
      d += b * (g + y);

      *++out1 = c;
		  *++out2 = d;
	  }
  }
  if(e <1.0e-10) env =0.f; else env =e;
  if(e2<1.0e-10) env2=0.f; else env2=e2;
  if(ge<1.0e-10) genv=0.f; else genv=ge;
}

void mdaDynamics::processReplacing(float **inputs, float **outputs, int32_t sampleFrames)
{
	float *in1 = inputs[0];
	float *in2 = inputs[1];
	float *out1 = outputs[0];
	float *out2 = outputs[1];
  float a, b, i, j, g, e=env, e2=env2, ra=rat, re=(1.f-rel), at=att, ga=gatt;
  float tr=trim, th=thr, lth=lthr, xth=xthr, ge=genv, y=dry;
	--in1;
	--in2;
	--out1;
	--out2;

  if(mode) //comp/gate/lim
  {
    if(lth==0.f) lth=1000.f;
    while(--sampleFrames >= 0)
	  {
      a = *++in1;
      b = *++in2;

      i = (a<0.f)? -a : a;
      j = (b<0.f)? -b : b;
      i = (j>i)? j : i;

      e = (i>e)? e + at * (i - e) : e * re;
      e2 = (i>e)? i : e2 * re; //ir;

      g = (e>th)? tr / (1.f + ra * ((e/th) - 1.f)) : tr;

      if(g<0.f) g=0.f;
      if(g*e2>lth) g = lth/e2; //limit

      ge = (e>xth)? ge + ga - ga * ge : ge * xrat; //gate

      *++out1 = a * (g * ge + y);
		  *++out2 = b * (g * ge + y);
    }
  }
  else //compressor only
  {
    while(--sampleFrames >= 0)
	  {
      a = *++in1;
      b = *++in2;

      i = (a<0.f)? -a : a;
      j = (b<0.f)? -b : b;
      i = (j>i)? j : i; //get peak level

      e = (i>e)? e + at * (i - e) : e * re; //envelope
      g = (e>th)? tr / (1.f + ra * ((e/th) - 1.f)) : tr; //gain

      *++out1 = a * (g + y); //vca
		  *++out2 = b * (g + y);
    }
  }
  if(e <1.0e-10) env =0.f; else env =e;
  if(e2<1.0e-10) env2=0.f; else env2=e2;
  if(ge<1.0e-10) genv=0.f; else genv=ge;
}
