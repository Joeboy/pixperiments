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

#include "mdaDubDelay.h"

#include <math.h>
#include <float.h>

AudioEffect *createEffectInstance(audioMasterCallback audioMaster)
{
  return new mdaDubDelay(audioMaster);
}

mdaDubDelay::mdaDubDelay(audioMasterCallback audioMaster)	: AudioEffectX(audioMaster, 1, 7)	// programs, parameters
{
  //inits here!
  fParam0 = 0.30f; //delay
  fParam1 = 0.70f; //feedback
  fParam2 = 0.40f; //tone
  fParam3 = 0.00f; //lfo depth
  fParam4 = 0.50f; //lfo speed
  fParam5 = 0.33f; //wet mix
  fParam6 = 0.50f; //output
         ///CHANGED///too long?
  size = 323766; //705600; //95998; //32766;  //set max delay time at max sample rate
	buffer = new float[size + 2]; //spare just in case!
  ipos = 0;
  fil0 = 0.0f;
  env  = 0.0f;
  phi  = 0.0f;
  dlbuf= 0.0f;

  setNumInputs(2);
	setNumOutputs(2);
	setUniqueID("mdaDubDelay");  //identify here
	DECLARE_LVZ_DEPRECATED(canMono) ();
	canProcessReplacing();
	strcpy(programName, "Dub Feedback Delay");

  suspend();		//flush buffer
  setParameter(0, 0.5);
}

bool  mdaDubDelay::getProductString(char* text) { strcpy(text, "MDA DubDelay"); return true; }
bool  mdaDubDelay::getVendorString(char* text)  { strcpy(text, "mda"); return true; }
bool  mdaDubDelay::getEffectName(char* name)    { strcpy(name, "DubDelay"); return true; }

void mdaDubDelay::setParameter(int32_t index, float value)
{
  float fs=getSampleRate();
  if(fs < 8000.0f) fs = 44100.0f; //??? bug somewhere!

	switch(index)
  {
    case 0: fParam0 = value; break;
    case 1: fParam1 = value; break;
    case 2: fParam2 = value; break;
    case 3: fParam3 = value; break;
    case 4: fParam4 = value; break;
    case 5: fParam5 = value; break;
    case 6: fParam6 = value; break;
 }
  //calcs here
  ///CHANGED///del = fParam0 * fParam0 * fParam0 * (float)size;
  del = fParam0 * fParam0 * (float)size;
  mod = 0.049f * fParam3 * del;

  fil = fParam2;
  if(fParam2>0.5f)  //simultaneously change crossover frequency & high/low mix
  {
    fil = 0.5f * fil - 0.25f;
    lmix = -2.0f * fil;
    hmix = 1.0f;
  }
  else
  {
    hmix = 2.0f * fil;
    lmix = 1.0f - hmix;
  }
  fil = (float)exp(-6.2831853f * pow(10.0f, 2.2f + 4.5f * fil) / fs);

  fbk = (float)fabs(2.2f * fParam1 - 1.1f);
  if(fParam1>0.5f) rel=0.9997f; else rel=0.8f; //limit or clip

  wet = 1.0f - fParam5;
  wet = fParam6 * (1.0f - wet * wet); //-3dB at 50% mix
  dry = fParam6 * 2.0f * (1.0f - fParam5 * fParam5);

  dphi = 628.31853f * (float)pow(10.0f, 3.0f * fParam4 - 2.0f) / fs; //100-sample steps
}

mdaDubDelay::~mdaDubDelay()
{
	if(buffer) delete [] buffer;
}

void mdaDubDelay::suspend()
{
	memset(buffer, 0, size * sizeof(float));
}

void mdaDubDelay::setProgramName(char *name)
{
	strcpy(programName, name);
}

void mdaDubDelay::getProgramName(char *name)
{
	strcpy(name, programName);
}

bool mdaDubDelay::getProgramNameIndexed (int32_t category, int32_t index, char* name)
{
	if (index == 0) 
	{
	    strcpy(name, programName);
	    return true;
	}
	return false;
}

float mdaDubDelay::getParameter(int32_t index)
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
  }
  return v;
}

void mdaDubDelay::getParameterName(int32_t index, char *label)
{
	switch(index)
  {
    case 0: strcpy(label, "Delay"); break;
    case 1: strcpy(label, "Feedback"); break;
    case 2: strcpy(label, "Fb Tone"); break;
    case 3: strcpy(label, "LFO Depth"); break;
    case 4: strcpy(label, "LFO Rate"); break;
    case 5: strcpy(label, "FX Mix"); break;
    case 6: strcpy(label, "Output"); break;
  }
}

#include <stdio.h>
static void int2strng(int32_t value, char *string) { sprintf(string, "%d", value); }
static void float2strng(float value, char *string) { sprintf(string, "%.2f", value); }

void mdaDubDelay::getParameterDisplay(int32_t index, char *text)
{
	switch(index)
  {
    case 0: int2strng((int32_t)(del * 1000.0f / getSampleRate()), text); break;
    case 1: int2strng((int32_t)(220 * fParam1 - 110), text); break;
    case 2: int2strng((int32_t)(200 * fParam2 - 100), text); break;
    case 3: int2strng((int32_t)(100 * fParam3), text); break;
    case 4: float2strng((float)pow(10.0f, 2.0f - 3.0f * fParam4), text); break;
    case 5: int2strng((int32_t)(100 * fParam5), text); break;
    case 6: int2strng((int32_t)(20 * log10(2.0 * fParam6)), text); break;
  }
}

void mdaDubDelay::getParameterLabel(int32_t index, char *label)
{
	switch(index)
  {
    case 0:  strcpy(label, "ms"); break;
    case 1:  strcpy(label, "Sat<>Lim"); break;
    case 2:  strcpy(label, "Lo <> Hi"); break;
    case 4:  strcpy(label, "sec."); break;
    case 6:  strcpy(label, "dB"); break;
    default: strcpy(label, "%"); break;
  }
}

//--------------------------------------------------------------------------------
// process

void mdaDubDelay::process(float **inputs, float **outputs, int32_t sampleFrames)
{
	float *in1 = inputs[0];
	float *in2 = inputs[1];
	float *out1 = outputs[0];
	float *out2 = outputs[1];
	float a, b, c, d, ol, w=wet, y=dry, fb=fbk, dl=dlbuf, db=dlbuf, ddl=0.0f;
  float lx=lmix, hx=hmix, f=fil, f0=fil0, tmp;
  float e=env, g, r=rel; //limiter envelope, gain, release
  float twopi=6.2831853f;
  int32_t  i=ipos, l, s=size, k=0;

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

    if(k==0) //update delay length at slower rate (could be improved!)
    {
      db += 0.01f * (del - db - mod - mod * (float)sin(phi)); //smoothed delay+lfo
      ddl = 0.01f * (db - dl); //linear step
      phi+=dphi; if(phi>twopi) phi-=twopi;
      k=100;
    }
    k--;
    dl += ddl; //lin interp between points

    i--; if(i<0) i=s;                         //delay positions

    l = (int32_t)dl;
    tmp = dl - (float)l; //remainder
    l += i; if(l>s) l-=(s+1);

    ol = *(buffer + l);                       //delay output

    l++; if(l>s) l=0;
    ol += tmp * (*(buffer + l) - ol); //lin interp

    tmp = a + fb * ol;                        //mix input (left only!) & feedback

    f0 = f * (f0 - tmp) + tmp;                //low-pass filter
    tmp = lx * f0 + hx * tmp;

    g =(tmp<0.0f)? -tmp : tmp;                //simple limiter
    e *= r; if(g>e) e = g;
    if(e>1.0f) tmp /= e;

    *(buffer + i) = tmp;                      //delay input

    ol *= w;                                  //wet

    *++out1 = c + y * a + ol;                 //dry
		*++out2 = d + y * b + ol;
	}
  ipos = i;
  dlbuf=dl;
  if(fabs(f0)<1.0e-10) { fil0=0.0f; env=0.0f; } else { fil0=f0; env=e; } //trap denormals
}

void mdaDubDelay::processReplacing(float **inputs, float **outputs, int32_t sampleFrames)
{
	float *in1 = inputs[0];
	float *in2 = inputs[1];
	float *out1 = outputs[0];
	float *out2 = outputs[1];
	float a, b, ol, w=wet, y=dry, fb=fbk, dl=dlbuf, db=dlbuf, ddl=0.0f;
  float lx=lmix, hx=hmix, f=fil, f0=fil0, tmp;
  float e=env, g, r=rel; //limiter envelope, gain, release
  float twopi=6.2831853f;
  int32_t  i=ipos, l, s=size, k=0;

	--in1;
	--in2;
	--out1;
	--out2;
  while(--sampleFrames >= 0)
	{
		a = *++in1;
    b = *++in2;

    if(k==0) //update delay length at slower rate (could be improved!)
    {
      db += 0.01f * (del - db - mod - mod * (float)sin(phi)); //smoothed delay+lfo
      ddl = 0.01f * (db - dl); //linear step
      phi+=dphi; if(phi>twopi) phi-=twopi;
      k=100;
    }
    k--;
    dl += ddl; //lin interp between points

    i--; if(i<0) i=s;                         //delay positions

    l = (int32_t)dl;
    tmp = dl - (float)l; //remainder
    l += i; if(l>s) l-=(s+1);

    ol = *(buffer + l);                       //delay output

    l++; if(l>s) l=0;
    ol += tmp * (*(buffer + l) - ol); //lin interp

    tmp = a + fb * ol;                        //mix input (left only!) & feedback

    f0 = f * (f0 - tmp) + tmp;                //low-pass filter
    tmp = lx * f0 + hx * tmp;

    g =(tmp<0.0f)? -tmp : tmp;                //simple limiter
    e *= r; if(g>e) e = g;
    if(e>1.0f) tmp /= e;

    *(buffer + i) = tmp;                      //delay input

    ol *= w;                                  //wet

    *++out1 = y * a + ol;                     //dry
		*++out2 = y * b + ol;
	}
  ipos = i;
  dlbuf=dl;
  if(fabs(f0)<1.0e-10) { fil0=0.0f; env=0.0f; } else { fil0=f0; env=e; } //trap denormals
}

