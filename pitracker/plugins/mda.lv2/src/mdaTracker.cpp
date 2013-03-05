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

#include "mdaTracker.h"

#include <math.h>
#include <float.h>

AudioEffect *createEffectInstance(audioMasterCallback audioMaster)
{
  return new mdaTracker(audioMaster);
}

mdaTracker::mdaTracker(audioMasterCallback audioMaster)	: AudioEffectX(audioMaster, 1, 8)	// programs, parameters
{
  //inits here!
  fParam1 = (float)0.00; //Mode
  fParam2 = (float)1.00; //Dynamics
  fParam3 = (float)1.00; //Mix
  fParam4 = (float)0.97; //Tracking
  fParam5 = (float)0.50; //Trnspose
  fParam6 = (float)0.80; //Maximum Hz
  fParam7 = (float)0.50; //Trigger dB
  fParam8 = (float)0.50; //Output

  setNumInputs(2);
	setNumOutputs(2);
	setUniqueID("mdaTracker");    // identify here
	DECLARE_LVZ_DEPRECATED(canMono) ();
	canProcessReplacing();
	strcpy(programName, "Pitch Tracker");

  dphi = 100.f/getSampleRate(); //initial pitch
  min = (int32_t)(getSampleRate()/30.0); //lower limit
  res1 = (float)cos(0.01); //p
  res2 = (float)sin(0.01); //q

  fi = fo = thr = phi = ddphi = trans = buf1 = buf2 = dn = bold = wet = dry = 0.0f;
  dyn = env = rel = saw = dsaw = res1 = res2 = buf3 = buf4 = 0.0f;
  max = min = num = sig = mode = 0;

  setParameter(0, 0.0f);
}

void mdaTracker::setParameter(int32_t index, float value)
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
  }
  //calcs here
  mode = int(fParam1*4.9);
  fo = filterFreq(50.f); fi = (1.f - fo)*(1.f - fo);
  ddphi = fParam4 * fParam4;
  thr = (float)pow(10.0, 3.0*fParam7 - 3.8);
  max = (int32_t)(getSampleRate() / pow(10.0f, 1.6f + 2.2f * fParam6));
  trans = (float)pow(1.0594631,int(72.f*fParam5 - 36.f));
  wet = (float)pow(10.0, 2.0*fParam8 - 1.0);
  if(mode<4)
  {
    dyn = wet * 0.6f * fParam3 * fParam2;
    dry = wet * (float)sqrt(1.f - fParam3);
    wet = wet * 0.3f * fParam3 * (1.f - fParam2);
  }
  else
  {
    dry = wet * (1.f - fParam3);
    wet *= (0.02f*fParam3 - 0.004f);
    dyn = 0.f;
  }
  rel = (float)pow(10.0,-10.0/getSampleRate());
}

mdaTracker::~mdaTracker()
{
}

bool  mdaTracker::getProductString(char* text) { strcpy(text, "MDA Tracker"); return true; }
bool  mdaTracker::getVendorString(char* text)  { strcpy(text, "mda"); return true; }
bool  mdaTracker::getEffectName(char* name)    { strcpy(name, "Tracker"); return true; }

void mdaTracker::suspend()
{
}

float mdaTracker::filterFreq(float hz)
{
  float j, k, r=0.999f;

  j = r * r - 1;
  k = (float)(2.f - 2.f * r * r * cos(0.647f * hz / getSampleRate() ));
  return (float)((sqrt(k*k - 4.f*j*j) - k) / (2.f*j));
}

void mdaTracker::setProgramName(char *name)
{
	strcpy(programName, name);
}

void mdaTracker::getProgramName(char *name)
{
	strcpy(name, programName);
}

bool mdaTracker::getProgramNameIndexed (int32_t category, int32_t index, char* name)
{
	if (index == 0) 
	{
	    strcpy(name, programName);
	    return true;
	}
	return false;
}

float mdaTracker::getParameter(int32_t index)
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
  }
  return v;
}

void mdaTracker::getParameterName(int32_t index, char *label)
{
	switch(index)
  {
    case 0: strcpy(label, "Mode"); break;
    case 1: strcpy(label, "Dynamics"); break;
    case 2: strcpy(label, "Mix"); break;
    case 3: strcpy(label, "Glide"); break;
    case 4: strcpy(label, "Transpose"); break;
    case 5: strcpy(label, "Maximum"); break;
    case 6: strcpy(label, "Trigger"); break;
    case 7: strcpy(label, "Output"); break;
  }
}

#include <stdio.h>
static void int2strng(int32_t value, char *string) { sprintf(string, "%d", value); }

void mdaTracker::getParameterDisplay(int32_t index, char *text)
{
	switch(index)
  {
    case 0: switch(mode)
            {
              case 0: strcpy(text, "SINE"); break;
              case 1: strcpy(text, "SQUARE"); break;
              case 2: strcpy(text, "SAW"); break;
              case 3: strcpy(text, "RING"); break;
              case 4: strcpy(text, "EQ"); break;
            } break;
    case 1: int2strng((int32_t)(100 * fParam2), text); break;
    case 2: int2strng((int32_t)(100 * fParam3), text); break;
    case 3: int2strng((int32_t)(100 * fParam4), text); break;
    case 4: int2strng((int32_t)(72*fParam5 - 36), text); break;
    case 5: int2strng((int32_t)(getSampleRate()/max), text); break;
    case 6: int2strng((int32_t)(60*fParam7 - 60), text); break;
    case 7: int2strng((int32_t)(40*fParam8 - 20), text); break;
  }
}

void mdaTracker::getParameterLabel(int32_t index, char *label)
{
	switch(index)
  {
    case 0: strcpy(label, ""); break;
    case 1: strcpy(label, "%"); break;
    case 2: strcpy(label, "%"); break;
    case 3: strcpy(label, "%"); break;
    case 4: strcpy(label, "semi"); break;
    case 5: strcpy(label, "Hz"); break;
    case 6: strcpy(label, "dB"); break;
  }
}

//--------------------------------------------------------------------------------
// process

void mdaTracker::process(float **inputs, float **outputs, int32_t sampleFrames)
{
	float *in1 = inputs[0];
	float *in2 = inputs[1];
	float *out1 = outputs[0];
	float *out2 = outputs[1];
  float a, b, c, d, x, t=thr, p=phi, dp=dphi, ddp=ddphi, tmp, tmp2;
  float o=fo, i=fi, b1=buf1, b2=buf2, twopi=6.2831853f;
  float we=wet, dr=dry, bo=bold, r1=res1, r2=res2, b3=buf3, b4=buf4;
  float sw=saw, dsw=dsaw, dy=dyn, e=env, re=rel;
  int32_t  m=max, n=num, s=sig, mn=min, mo=mode;

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
    x = a + b;

    tmp = (x>0.f)? x : -x; //dynamics envelope
    e = (tmp>e)? 0.5f*(tmp + e) : e * re;

    b1 = o*b1 + i*x;
    b2 = o*b2 + b1; //low-pass filter

    if(b2>t) //if >thresh
    {
      if(s<1) //and was <thresh
      {
        if(n<mn) //not long ago
        {
          tmp2 = b2 / (b2 - bo); //update period
          tmp = trans*twopi/(n + dn - tmp2);
          dp = dp + ddp * (tmp - dp);
          dn = tmp2;
          dsw = 0.3183098f * dp;
          if(mode==4)
          {
            r1 = (float)cos(4.f*dp); //resonator
            r2 = (float)sin(4.f*dp);
          }
        }
        n=0; //restart period measurement
      }
      s=1;
    }
    else
    {
      if(n>m) s=0; //now <thresh
    }
    n++;
    bo=b2;

    p = (float)fmod(p+dp,twopi);
  	switch(mo)
    { //sine
      case 0: x=(float)sin(p); break;
      //square
      case 1: x=(sin(p)>0.f)? 0.5f : -0.5f; break;
      //saw
      case 2: sw = (float)fmod(sw+dsw,2.0f); x = sw - 1.f; break;
      //ring
      case 3: x *= (float)sin(p); break;
      //filt
      case 4: x += (b3 * r1) - (b4 * r2);
              b4 = 0.996f * ((b3 * r2) + (b4 * r1));
              b3 = 0.996f * x; break;
    }
    x *= (we + dy * e);
		*++out1 = c + dr*a + x;
		*++out2 = d + dr*b + x;
	}
  if(fabs(b1)<1.0e-10) {buf1=0.f; buf2=0.f; buf3=0.f; buf4=0.f; }
  else {buf1=b1; buf2=b2; buf3=b3; buf4=b4;}
  phi=p; dphi=dp; sig=s; bold=bo;
  num=(n>100000)? 100000: n;
  env=e; saw=sw; dsaw=dsw; res1=r1; res2=r2;
}

void mdaTracker::processReplacing(float **inputs, float **outputs, int32_t sampleFrames)
{
	float *in1 = inputs[0];
	float *in2 = inputs[1];
	float *out1 = outputs[0];
	float *out2 = outputs[1];
  float a, b, x, t=thr, p=phi, dp=dphi, ddp=ddphi, tmp, tmp2;
  float o=fo, i=fi, b1=buf1, b2=buf2, twopi=6.2831853f;
  float we=wet, dr=dry, bo=bold, r1=res1, r2=res2, b3=buf3, b4=buf4;
  float sw=saw, dsw=dsaw, dy=dyn, e=env, re=rel;
  int32_t  m=max, n=num, s=sig, mn=min, mo=mode;

	--in1;
	--in2;
	--out1;
	--out2;
	while(--sampleFrames >= 0)
	{
    a = *++in1;
    b = *++in2;
    x = a;// + b;

    tmp = (x>0.f)? x : -x; //dynamics envelope
    e = (tmp>e)? 0.5f*(tmp + e) : e * re;

    b1 = o*b1 + i*x;
    b2 = o*b2 + b1; //low-pass filter

    if(b2>t) //if >thresh
    {
      if(s<1) //and was <thresh
      {
        if(n<mn) //not long ago
        {
          tmp2 = b2 / (b2 - bo); //update period
          tmp = trans*twopi/(n + dn - tmp2);
          dp = dp + ddp * (tmp - dp);
          dn = tmp2;
          dsw = 0.3183098f * dp;
          if(mode==4)
          {
            r1 = (float)cos(4.f*dp); //resonator
            r2 = (float)sin(4.f*dp);
          }
        }
        n=0; //restart period measurement
      }
      s=1;
    }
    else
    {
      if(n>m) s=0; //now <thresh
    }
    n++;
    bo=b2;

    p = (float)fmod(p+dp,twopi);
  	switch(mo)
    { //sine
      case 0: x=(float)sin(p); break;
      //square
      case 1: x=(sin(p)>0.f)? 0.5f : -0.5f; break;
      //saw
      case 2: sw = (float)fmod(sw+dsw,2.0f); x = sw - 1.f; break;
      //ring
      case 3: x *= (float)sin(p); break;
      //filt
      case 4: x += (b3 * r1) - (b4 * r2);
              b4 = 0.996f * ((b3 * r2) + (b4 * r1));
              b3 = 0.996f * x; break;
    }
    x *= (we + dy * e);
		*++out1 = a;//dr*a + x;
		*++out2 = dr*b + x;
	}
  if(fabs(b1)<1.0e-10) {buf1=0.f; buf2=0.f; buf3=0.f; buf4=0.f; }
  else {buf1=b1; buf2=b2; buf3=b3; buf4=b4;}
  phi=p; dphi=dp; sig=s; bold=bo;
  num=(n>100000)? 100000: n;
  env=e; saw=sw; dsaw=dsw; res1=r1; res2=r2;
}
