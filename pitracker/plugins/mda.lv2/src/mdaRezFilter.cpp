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

#include "mdaRezFilter.h"

#include <math.h>
#include <float.h>
#include <stdlib.h>

AudioEffect *createEffectInstance(audioMasterCallback audioMaster)
{
  return new mdaRezFilter(audioMaster);
}

mdaRezFilter::mdaRezFilter(audioMasterCallback audioMaster)	: AudioEffectX(audioMaster, 1, 10)	// programs, parameters
{
  //inits here!
  fParam0 = 0.33f; //f
  fParam1 = 0.70f; //q
  fParam2 = 0.50f; //a
  fParam3 = 0.85f; //fenv
  fParam4 = 0.00f; //att
  fParam5 = 0.50f; //rel
  fParam6 = 0.70f; //lfo
  fParam7 = 0.40f; //rate
  fParam8 = 0.00f; //trigger
  fParam9 = 0.75f; //max freq

  fff = fq = fg = fmax = env = fenv = att = rel = 0.0f;
  flfo = phi = dphi = bufl = buf0 = buf1 = buf2 = tthr = env2 = 0.0f;
  lfomode = ttrig = tatt = 0;

  setNumInputs(2);
	setNumOutputs(2);
	setUniqueID("mdaRezFilter");
	DECLARE_LVZ_DEPRECATED(canMono) ();
	canProcessReplacing();
	strcpy(programName, "Resonant Filter");

  suspend();		// flush buffer
  setParameter(2, 0.5f); //go and set initial values!
}

void mdaRezFilter::setParameter(int32_t index, float value)
{
	switch(index)
  {
    case 0: fParam0 = value; break;
    case 1: fParam1 = value; break;
    case 2: fParam2 = value; break;
    case 3: fParam3 = value; break;
    case 4: fParam4 = value; break;
    case 5: fParam5 = value; break;
    case 6: fParam6 = value; break;
    case 7: fParam7 = value; break;
    case 8: fParam8 = value; break;
    case 9: fParam9 = value; break;
  }
  //calcs here
  fff = 1.5f * fParam0 * fParam0 - 0.15f;
  fq = 0.99f * (float)pow(fParam1,0.3f); //was 0.99f *
  fg = 0.5f * (float)pow(10.0f, 2.f * fParam2 - 1.f);

  fmax = 0.99f + 0.3f * fParam1;
  if(fmax>(1.3f * fParam9)) fmax=1.3f*fParam9;
  //fmax = 1.0f;
  //fq *= 1.0f + 0.2f * fParam9;

  fenv = 2.f*(0.5f - fParam3)*(0.5f - fParam3);
  fenv = (fParam3>0.5f)? fenv : -fenv;
  att = (float)pow(10.0, -0.01 - 4.0 * fParam4);
  rel = 1.f - (float)pow(10.0, -2.00 - 4.0 * fParam5);

  lfomode=0;
  flfo = 2.f * (fParam6 - 0.5f)*(fParam6 - 0.5f);
  dphi = (float)(6.2832f * (float)pow(10.0f, 3.f * fParam7 - 1.5f) / getSampleRate());
  if(fParam6<0.5) { lfomode=1; dphi *= 0.15915f; flfo *= 0.001f; } //S&H

  if(fParam8<0.1f) tthr=0.f; else tthr = 3.f * fParam8 * fParam8;
}

mdaRezFilter::~mdaRezFilter()
{

}

bool  mdaRezFilter::getProductString(char* text) { strcpy(text, "MDA RezFilter"); return true; }
bool  mdaRezFilter::getVendorString(char* text)  { strcpy(text, "mda"); return true; }
bool  mdaRezFilter::getEffectName(char* name)    { strcpy(name, "RezFilter"); return true; }

void mdaRezFilter::suspend()
{
  buf0=0.f;
  buf1=0.f;
  buf2=0.f;
}

void mdaRezFilter::setProgramName(char *name)
{
	strcpy(programName, name);
}

void mdaRezFilter::getProgramName(char *name)
{
	strcpy(name, programName);
}

bool mdaRezFilter::getProgramNameIndexed (int32_t category, int32_t index, char* name)
{
	if (index == 0) 
	{
	    strcpy(name, programName);
	    return true;
	}
	return false;
}

float mdaRezFilter::getParameter(int32_t index)
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
    case 6: v = fParam6; break;
    case 7: v = fParam7; break;
    case 8: v = fParam8; break;
    case 9: v = fParam9; break;
  }
  return v;
}

void mdaRezFilter::getParameterName(int32_t index, char *label)
{
	switch(index)
  {
    case 0: strcpy(label, "Freq"); break;
    case 1: strcpy(label, "Res"); break;
    case 2: strcpy(label, "Output"); break;
    case 3: strcpy(label, "Env->VCF"); break;
    case 4: strcpy(label, "Attack"); break;
    case 5: strcpy(label, "Release"); break;
    case 6: strcpy(label, "LFO->VCF"); break;
    case 7: strcpy(label, "LFO Rate"); break;
    case 8: strcpy(label, "Trigger"); break;
    case 9: strcpy(label, "Max Freq"); break;
  }
}

#include <stdio.h>
static void int2strng(int32_t value, char *string) { sprintf(string, "%d", value); }
static void float2strng(float value, char *string) { sprintf(string, "%.2f", value); }

void mdaRezFilter::getParameterDisplay(int32_t index, char *text)
{
	switch(index)
  {
    case 0: int2strng((int32_t)(100 * fParam0), text); break;
    case 1: int2strng((int32_t)(100 * fParam1), text); break;
    case 2: int2strng((int32_t)(40 *fParam2 - 20),text); break;
    case 3: int2strng((int32_t)(200 * fParam3 - 100), text); break;
    case 4: float2strng((float)(-301.0301 / (getSampleRate() * log10(1.0 - att))),text); break;
    case 5: float2strng((float)(-301.0301 / (getSampleRate() * log10(rel))),text); break;
    case 6: int2strng((int32_t)(200 * fParam6 - 100), text); break;
    case 7: float2strng((float)pow(10.0f, 4.f*fParam7 - 2.f), text); break;
    case 8: if(tthr==0.f) strcpy(text, "FREE RUN");
            else int2strng((int32_t)(20*log10(0.5*tthr)), text); break;
    case 9: int2strng((int32_t)(100 * fParam9), text); break;
  }
}

void mdaRezFilter::getParameterLabel(int32_t index, char *label)
{
	switch(index)
  {
    case 0: strcpy(label, "%"); break;
    case 1: strcpy(label, "%"); break;
    case 2: strcpy(label, "dB"); break;
    case 3: strcpy(label, "%"); break;
    case 4: strcpy(label, "ms"); break;
    case 5: strcpy(label, "ms"); break;
    case 6: strcpy(label, "S+H<>Sin"); break;
    case 7: strcpy(label, "Hz"); break;
    case 8: strcpy(label, "dB"); break;
    case 9: strcpy(label, "%"); break;
  }
}

//--------------------------------------------------------------------------------
// process

void mdaRezFilter::process(float **inputs, float **outputs, int32_t sampleFrames)
{
	float *in1 = inputs[0];
	float *in2 = inputs[1];
	float *out1 = outputs[0];
	float *out2 = outputs[1];
	float a, c, d;
  float f, i, o, ff=fff, fe=fenv, q=fq, g=fg, e=env;
  float b0=buf0, b1=buf1, b2=buf2, at=att, re=rel, fm=fmax;
  float fl=flfo, dph=dphi, ph=phi, bl=bufl, th=tthr, e2=env2;
  int lm=lfomode, ta=tatt, tt=ttrig;

	--in1;
	--in2;
	--out1;
	--out2;

  if(th==0.f)
  {
	  while(--sampleFrames >= 0)
	  {
		  a = *++in1 + *++in2;
		  c = out1[1];
		  d = out2[1]; //process from here...

      i = (a>0)? a : -a; //envelope
      e = (i>e)?  e + at * (i - e) : e * re;

      if(lm==0) bl = fl * (float)sin(ph); //lfo
      else if(ph>1.f) { bl = fl*(rand() % 2000 - 1000); ph=0.f; }
      ph += dph;

      f = ff + fe * e + bl; //freq
      if(f<0.f) i=0.f; else i=(f>fm)? fm : f;
      o = 1.f - i;

      b0 = o * b0 + i * (g*a + q*(1.f + (1.f/o)) * (b0-b1) );
      b1 = o * b1 + i * b0; //filter
      b2 = o * b2 + i * b1;

      *++out1 = c + b2;
		  *++out2 = d + b2;
	  }
  }
  else
  {
  	while(--sampleFrames >= 0)
	  {
		  a = *++in1 + *++in2;
		  c = out1[1];
		  d = out2[1]; //process from here...

      i = (a>0)? a : -a; //envelope
      e = (i>e)? i : e * re;
      if(e>th) { if(tt==0) {ta=1; if(lm==1)ph=2.f; } tt=1; } else tt=0;
      if(ta==1) { e2 += at*(1.f-e2); if(e2>0.999f)ta=0; } else e2*=re;

      if(lm==0) bl = fl * (float)sin(ph); //lfo
      else if(ph>1.f) { bl = fl*(rand() % 2000 - 1000); ph=0.f; }
      ph += dph;

      f = ff + fe * e + bl; //freq
      if(f<0.f) i=0.f; else i=(f>fm)? fm : f;
      o = 1.f - i;

      b0 = o * b0 + i * (g*a + q*(1.f + (1.f/o)) * (b0-b1) );
      b1 = o * b1 + i * b0; //filter
      b2 = o * b2 + i * b1;

      *++out1 = c + b2;
		  *++out2 = d + b2;
	  }
  }
  if(fabs(b0)<1.0e-10) { buf0=0.f; buf1=0.f; buf2=0.f; }
  else { buf0=b0; buf1=b1; buf2=b2; }
  env=e; env2=e2; bufl=bl; tatt=ta; ttrig=tt;
  phi=(float)fmod(ph,6.2831853f);
}

void mdaRezFilter::processReplacing(float **inputs, float **outputs, int32_t sampleFrames)
{
	float *in1 = inputs[0];
	float *in2 = inputs[1];
	float *out1 = outputs[0];
	float *out2 = outputs[1];
	float a;
  float f, i, ff=fff, fe=fenv, q=fq, g=fg, e=env, tmp;
  float b0=buf0, b1=buf1, b2=buf2, at=att, re=rel, fm=fmax;
  float fl=flfo, dph=dphi, ph=phi, bl=bufl, th=tthr, e2=env2;
  int lm=lfomode, ta=tatt, tt=ttrig;

	--in1;
	--in2;
	--out1;
	--out2;

  if(th==0.f)
  {
	  while(--sampleFrames >= 0)
	  {
		  a = *++in1 + *++in2;

      i = (a>0)? a : -a; //envelope
      e = (i>e)?  e + at * (i - e) : e * re;

      if(lm==0) bl = fl * (float)sin(ph); //lfo
      else if(ph>1.f) { bl = fl*(rand() % 2000 - 1000); ph=0.f; }
      ph += dph;

      f = ff + fe * e + bl; //freq
      if(f<0.f) i=0.f; else i=(f>fm)? fm : f;
 //     o = 1.f - i;

 //     tmp = g*a + q*(1.f + (1.f/o)) * (b0-b1);
 //     b0 = o * (b0 - tmp) + tmp;
 //     b1 = o * (b1 - b0) + b0;

      tmp = q + q * (1.0f + i * (1.0f + 1.1f * i));
      //tmp = q + q/(1.0008 - i);
      b0 += i * (g * a - b0 + tmp * (b0 - b1));
      b1 += i * (b0 - b1);


  //    b2 = o * (b2 - b1) + b1;

      *++out1 = b1;
		  *++out2 = b1;
	  }
  }
  else
  {
  	while(--sampleFrames >= 0)
	  {
		  a = *++in1 + *++in2;


      i = (a>0)? a : -a; //envelope
      e = (i>e)? i : e * re;
      if(e>th) { if(tt==0) {ta=1; if(lm==1)ph=2.f; } tt=1; } else tt=0;
      if(ta==1) { e2 += at*(1.f-e2); if(e2>0.999f)ta=0; } else e2*=re;

      if(lm==0) bl = fl * (float)sin(ph); //lfo
      else if(ph>1.f) { bl = fl*(rand() % 2000 - 1000); ph=0.f; }
      ph += dph;

      f = ff + fe * e + bl; //freq
      if(f<0.f) i=0.f; else i=(f>fm)? fm : f;

  //    o = 1.f - i;

  tmp = q + q * (1.0f + i * (1.0f + 1.1f * i));
  //tmp = q + q/(1.0008 - i);
  b0 += i * (g * a - b0 + tmp * (b0 - b1));
  b1 += i * (b0 - b1);


  //    tmp = g*a + q*(1.f + (1.f/o)) * (b0-b1);  //what about (q + q/f)*
  //    b0 = o * (b0 - tmp) + tmp;                // ^ what about div0 ?
  //    b1 = o * (b1 - b0) + b0;
  //    b2 = o * (b2 - b1) + b1;


      *++out1 = b1;
		  *++out2 = b1;
	  }
  }
  if(fabs(b0)<1.0e-10) { buf0=0.f; buf1=0.f; buf2=0.f; }
  else { buf0=b0; buf1=b1; buf2=b2; }
  env=e; env2=e2; bufl=bl; tatt=ta; ttrig=tt;
  phi=(float)fmod(ph,6.2831853f);
}
