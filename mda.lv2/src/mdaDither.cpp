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

#include "mdaDither.h"

#include <math.h>
#include <stdlib.h>
#include <float.h>

AudioEffect *createEffectInstance(audioMasterCallback audioMaster)
{
  return new mdaDither(audioMaster);
}

mdaDither::mdaDither(audioMasterCallback audioMaster)	: AudioEffectX(audioMaster, 1, 5)	// programs, parameters
{
  //inits here!
  fParam0 = 0.50f; //bits
  fParam1 = 0.88f; //dither (off, rect, tri, hp-tri, hp-tri-2ndOrderShaped)
  fParam2 = 0.50f; //dither level
  fParam3 = 0.50f; //dc trim
  fParam4 = 0.00f; //zoom (8 bit dither and fade audio out)

  sh1 = sh2 = sh3 = sh4 = 0.0f;
  rnd1 = rnd3 = 0;

  setNumInputs(2);
	setNumOutputs(2);
	setUniqueID("mdaDither");    // identify here
	DECLARE_LVZ_DEPRECATED(canMono) ();
	canProcessReplacing();
	strcpy(programName, "Dither & Noise Shaping");

  setParameter(0, 0.5f);
}

bool  mdaDither::getProductString(char* text) { strcpy(text, "MDA Dither"); return true; }
bool  mdaDither::getVendorString(char* text)  { strcpy(text, "mda"); return true; }
bool  mdaDither::getEffectName(char* name)    { strcpy(name, "Dither"); return true; }

void mdaDither::setParameter(int32_t index, float value)
{
  switch(index)
  {
    case 0: fParam0 = value; break;
    case 1: fParam1 = value; break;
    case 2: fParam2 = value; break;
    case 3: fParam3 = value; break;
    case 4: fParam4 = value; break;
 }
  //calcs here
  gain = 1.0f;
  bits = 8.0f + 2.0f * (float)floor(8.9f * fParam0);

  if(fParam4>0.1f) //zoom to 6 bit & fade out audio
  {
    wlen = 32.0f;
    gain = (1.0f - fParam4); gain*=gain;
  }
  else wlen = (float)pow(2.0f, bits - 1.0f); //word length in quanta

  //Using WaveLab 2.01 (unity gain) as a reference:
  //  16-bit output is (int)floor(floating_point_value*32768.0f)

  offs = (4.0f * fParam3 - 1.5f) / wlen; //DC offset (plus 0.5 to round dither not truncate)
  dith = 2.0f * fParam2 / (wlen * (float)32767);
  shap=0.0f;

  switch((int32_t)(fParam1*3.9)) //dither mode
  {
     case 0: dith = 0.0f; break; //off
     case 3: shap = 0.5f; break; //noise shaping
    default: break; //tri, hp-tri
  }
}

mdaDither::~mdaDither()
{
}

void mdaDither::suspend()
{
  //no need to zero buffers here as effect so small
}

void mdaDither::setProgramName(char *name)
{
	strcpy(programName, name);
}

void mdaDither::getProgramName(char *name)
{
	strcpy(name, programName);
}

bool mdaDither::getProgramNameIndexed (int32_t category, int32_t index, char* name)
{
	if (index == 0) 
	{
	    strcpy(name, programName);
	    return true;
	}
	return false;
}

float mdaDither::getParameter(int32_t index)
{
	float v=0;

  switch(index)
  {
    case 0: v = fParam0; break;
    case 1: v = fParam1; break;
    case 2: v = fParam2; break;
    case 3: v = fParam3; break;
    case 4: v = fParam4; break;
  }
  return v;
}

void mdaDither::getParameterName(int32_t index, char *label)
{
	switch(index)
  {
    case 0: strcpy(label, "Word Len"); break;
    case 1: strcpy(label, "Dither"); break;
    case 2: strcpy(label, "Dith Amp"); break;
    case 3: strcpy(label, "DC Trim"); break;
    case 4: strcpy(label, "Zoom"); break;
  }
}

#include <stdio.h>
static void int2strng(int32_t value, char *string) { sprintf(string, "%d", value); }
static void float2strng(float value, char *string) { sprintf(string, "%.2f", value); }

void mdaDither::getParameterDisplay(int32_t index, char *text)
{
	switch(index)
  {
    case 0: int2strng((int32_t)bits, text); break;
    case 1: switch((int32_t)(fParam1*3.9))
            {  case 0: strcpy(text, "OFF"); break;
               case 1: strcpy(text, "TRI"); break;
               case 2: strcpy(text, "HP-TRI"); break;
              default: strcpy(text, "N.SHAPE"); break;
            } break;
    case 2: float2strng(4.0f * fParam2, text); break;
    case 3: float2strng(4.0f * fParam3 - 2.0f, text); break;
    case 4: if(fParam4>0.1f)
            if(gain<0.0001f) strcpy(text, "-80");
                        else int2strng((int32_t)(20.0 * log10(gain)), text);
                        else strcpy(text, "OFF"); break;
  }
}

void mdaDither::getParameterLabel(int32_t index, char *label)
{
	switch(index)
  {
    case 0: strcpy(label, "Bits"); break;
    case 1: strcpy(label, ""); break;
    case 2: strcpy(label, "lsb"); break;
    case 3: strcpy(label, "lsb"); break;
    case 4: strcpy(label, "dB"); break;
  }
}

//--------------------------------------------------------------------------------
// process

void mdaDither::process(float **inputs, float **outputs, int32_t sampleFrames)
{
	float *in1 = inputs[0];
	float *in2 = inputs[1];
	float *out1 = outputs[0];
	float *out2 = outputs[1];
	float a, b, aa, bb, c, d;
  float sl=shap, s1=sh1, s2=sh2, s3=sh3, s4=sh4; //shaping level, buffers
  float dl=dith;                                 //dither level
  float o=offs, w=wlen, wi=1.0f/wlen;            //DC offset, word length & inverse
  float g=gain;                                  //gain for Zoom mode
  int32_t  r1=rnd1, r2, r3=rnd3, r4;                //random numbers for dither
  int32_t  m=1;                                     //dither mode
  if((int32_t)(fParam1 * 3.9f)==1) m=0;             //what is the fastest if(?)

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

    r2=r1; r4=r3;
    if(m==0) { r4=rand() & 0x7FFF; r2=(r4 & 0x7F)<<8; }
               r1=rand() & 0x7FFF; r3=(r1 & 0x7F)<<8;

    a  = g * a + sl * (s1 + s1 - s2);
    aa = a + o + dl * (float)(r1 - r2);
    if(aa<0.0f) aa-=wi;
    aa = wi * (float)(int32_t)(w * aa);
    s2 = s1;
    s1 = a - aa;

    b  = g * b + sl * (s3 + s3 - s4);
    bb = b + o + dl * (float)(r3 - r4);
    if(bb<0.0f) bb-=wi;
    bb = wi * (float)(int32_t)(w * bb);
    s4 = s3;
    s3 = b - bb;

    *++out1 = c + aa;
		*++out2 = d + bb;
	}

  sh1=s1; sh2=s2; sh3=s3; sh4=s4;
  rnd1=r1; rnd3=r3;
}

void mdaDither::processReplacing(float **inputs, float **outputs, int32_t sampleFrames)
{
	float *in1 = inputs[0];
	float *in2 = inputs[1];
	float *out1 = outputs[0];
	float *out2 = outputs[1];
	float a, b, aa, bb;
  float sl=shap, s1=sh1, s2=sh2, s3=sh3, s4=sh4; //shaping level, buffers
  float dl=dith;                                 //dither level
  float o=offs, w=wlen, wi=1.0f/wlen;            //DC offset, word length & inverse
  float g=gain;                                  //gain for Zoom mode
  int32_t  r1=rnd1, r2, r3=rnd3, r4;                //random numbers for dither
  int32_t  m=1;                                     //dither mode
  if((int32_t)(fParam1 * 3.9f)==1) m=0;             //what is the fastest if(?)

	--in1;
	--in2;
	--out1;
	--out2;

	while(--sampleFrames >= 0)
	{
		a = *++in1;
    b = *++in2;

    r2=r1; r4=r3; //HP-TRI dither (also used when noise shaping)
    if(m==0) { r4=rand() & 0x7FFF; r2=(r4 & 0x7F)<<8; } //TRI dither
               r1=rand() & 0x7FFF; r3=(r1 & 0x7F)<<8;   //Assumes RAND_MAX=32767?

    a  = g * a + sl * (s1 + s1 - s2);    //target level + error feedback
    aa = a + o + dl * (float)(r1 - r2);  //             + offset + dither
    if(aa<0.0f) aa-=wi;                 //(int32_t) truncates towards zero!
    aa = wi * (float)(int32_t)(w * aa);    //truncate 
    s2 = s1;
    s1 = a - aa;                        //error feedback: 2nd order noise shaping

    b  = g * b + sl * (s3 + s3 - s4);
    bb = b + o + dl * (float)(r3 - r4);
    if(bb<0.0f) bb-=wi;
    bb = wi * (float)(int32_t)(w * bb);
    s4 = s3;
    s3 = b - bb;

    *++out1 = aa;
		*++out2 = bb;
	}

  sh1=s1; sh2=s2; sh3=s3; sh4=s4; //doesn't actually matter if these are
  rnd1=r1; rnd3=r3;               //saved or not as effect is so small !
}
