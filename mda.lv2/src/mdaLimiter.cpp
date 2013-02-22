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

#include "mdaLimiter.h"

#include <math.h>

AudioEffect *createEffectInstance(audioMasterCallback audioMaster)
{
  return new mdaLimiter(audioMaster);
}

mdaLimiter::mdaLimiter(audioMasterCallback audioMaster)	: AudioEffectX(audioMaster, 1, 5)	// 1 program, 4 parameters
{
	fParam1 = (float)0.60; //thresh
  fParam2 = (float)0.60; //trim
  fParam3 = (float)0.15; //attack
  fParam4 = (float)0.50; //release
  fParam5 = (float)0.0; //knee

  setNumInputs(2);		    // stereo in
	setNumOutputs(2);		    // stereo out
	setUniqueID("mdaLimiter");    // identify
	DECLARE_LVZ_DEPRECATED(canMono) ();
	canProcessReplacing();	// supports both accumulating and replacing output
	strcpy(programName, "Limiter");	// default program name

  if(fParam5>0.0) //soft knee
  {
    thresh = (float)pow(10.0, 1.0 - (2.0 * fParam1));
  }
  else //hard knee
  {
    thresh = (float)pow(10.0, (2.0 * fParam1) - 2.0);
  }
  trim = (float)(pow(10.0, (2.0 * fParam2) - 1.0));
  att = (float)pow(10.0, -0.01 - 2.0 * fParam3);//wavelab overruns with zero???
  rel = (float)pow(10.0, -2.0 - (3.0 * fParam4));
  gain = 1.0;
}

mdaLimiter::~mdaLimiter()
{
	// nothing to do here
}

bool  mdaLimiter::getProductString(char* text) { strcpy(text, "MDA Limiter"); return true; }
bool  mdaLimiter::getVendorString(char* text)  { strcpy(text, "mda"); return true; }
bool  mdaLimiter::getEffectName(char* name)    { strcpy(name, "Limiter"); return true; }

void mdaLimiter::setProgramName(char *name)
{
	strcpy(programName, name);
}

void mdaLimiter::getProgramName(char *name)
{
	strcpy(name, programName);
}

bool mdaLimiter::getProgramNameIndexed (int32_t category, int32_t index, char* name)
{
	if (index == 0) 
	{
	    strcpy(name, programName);
	    return true;
	}
	return false;
}

void mdaLimiter::setParameter(int32_t index, float value)
{
	switch(index)
  {
    case 0: fParam1 = value; break;
    case 1: fParam2 = value; break;
    case 2: fParam4 = value; break;
    case 3: fParam3 = value; break;
    case 4: fParam5 = value; break;
  }
  //calcs here
  if(fParam5>0.0) //soft knee
  {
    thresh = (float)pow(10.0, 1.0 - (2.0 * fParam1));
  }
  else //hard knee
  {
    thresh = (float)pow(10.0, (2.0 * fParam1) - 2.0);
  }
  trim = (float)(pow(10.0, (2.0 * fParam2) - 1.0));
  att = (float)pow(10.0, -2.0 * fParam3);
  rel = (float)pow(10.0, -2.0 - (3.0 * fParam4));
}

float mdaLimiter::getParameter(int32_t index)
{
	float v=0;

  switch(index)
  {
    case 0: v = fParam1; break;
    case 1: v = fParam2; break;
    case 2: v = fParam4; break;
    case 3: v = fParam3; break;
    case 4: v = fParam5; break;
  }
  return v;
}

void mdaLimiter::getParameterName(int32_t index, char *label)
{
	switch(index)
  {
    case 0: strcpy(label, "Thresh"); break;
    case 1: strcpy(label, "Output"); break;
    case 3: strcpy(label, "Attack"); break;
    case 2: strcpy(label, "Release"); break;
    case 4: strcpy(label, "Knee"); break;
  }
}

#include <stdio.h>
static void int2strng(int32_t value, char *string) { sprintf(string, "%d", value); }

void mdaLimiter::getParameterDisplay(int32_t index, char *text)
{
	switch(index)
  {
    case 0: int2strng((int32_t)(40.0*fParam1 - 40.0),text); break;
    case 1: int2strng((int32_t)(40.0*fParam2 - 20.0),text); break;
    case 3: int2strng((int32_t)(-301030.1 / (getSampleRate() * log10(1.0 - att))),text); break;
    case 2: int2strng((int32_t)(-301.0301 / (getSampleRate() * log10(1.0 - rel))),text); break;
    case 4: if(fParam5>0.0) strcpy(text, "SOFT");
            else strcpy(text, "HARD"); break;
  }

}

void mdaLimiter::getParameterLabel(int32_t index, char *label)
{
	switch(index)
  {
    case 0: strcpy(label, "dB"); break;
    case 1: strcpy(label, "dB"); break;
    case 3: strcpy(label, "ms"); break; 
    case 2: strcpy(label, "ms"); break;
    case 4: strcpy(label, ""); break;
  }
}

//--------------------------------------------------------------------------------
// process

void mdaLimiter::process(float **inputs, float **outputs, int32_t sampleFrames)
{
	float *in1 = inputs[0];
	float *in2 = inputs[1];
	float *out1 = outputs[0];
	float *out2 = outputs[1];
  float g, c, d, at, re, tr, th, lev, ol, or_;

  th = thresh;
  g = gain;
  at = att;
  re = rel;
  tr = trim;

  --in1;
	--in2;
	--out1;
	--out2;

  if(fParam5>0.5) //soft knee
  {
	  while(--sampleFrames >= 0)
	  {
      ol = *++in1;
      or_ = *++in2;
      c = out1[1];
      d = out2[1];

      lev = (float)(1.0 / (1.0 + th * fabs(ol + or_)));
      if(g>lev) { g=g-at*(g-lev); } else { g=g+re*(lev-g); }

      c += (ol * tr * g);
      d += (or_ * tr * g);

      *++out1 = c;
		  *++out2 = d;
	  }
  }
  else
  {
    while(--sampleFrames >= 0)
	  {
      ol = *++in1;
      or_ = *++in2;
      c = out1[1];
      d = out2[1];

      lev = (float)(0.5 * g * fabs(ol + or_));

      if (lev > th)
      {
        g = g - (at * (lev - th));
      }
      else
      {
        g = g + (float)(re * (1.0 - g));
      }

      c += (ol * tr * g);
      d += (or_ * tr * g);

      *++out1 = c;
		  *++out2 = d;
	  }
  }
  gain = g;
}

void mdaLimiter::processReplacing(float **inputs, float **outputs, int32_t sampleFrames)
{
	float *in1 = inputs[0];
	float *in2 = inputs[1];
	float *out1 = outputs[0];
	float *out2 = outputs[1];
	float g, at, re, tr, th, lev, ol, or_;

  th = thresh;
  g = gain;
  at = att;
  re = rel;
  tr = trim;

	--in1;
	--in2;
	--out1;
	--out2;
  if(fParam5>0.5) //soft knee
  {
	  while(--sampleFrames >= 0)
	  {
      ol = *++in1;
      or_ = *++in2;

      lev = (float)(1.0 / (1.0 + th * fabs(ol + or_)));
      if(g>lev) { g=g-at*(g-lev); } else { g=g+re*(lev-g); }

      *++out1 = (ol * tr * g);
		  *++out2 = (or_ * tr * g);
	  }
  }
  else
  {
 	  while(--sampleFrames >= 0)
	  {
      ol = *++in1;
      or_ = *++in2;

      lev = (float)(0.5 * g * fabs(ol + or_));

      if (lev > th)
      {
        g = g - (at * (lev - th));
      }
      else //below threshold
      {
        g = g + (float)(re * (1.0 - g));
      }

      *++out1 = (ol * tr * g);
		  *++out2 = (or_ * tr * g);
	  }
  }
  gain = g;
}
