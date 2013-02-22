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

///27 Mar 2001 - changed number of programs - was not remembering settings

#include "mdaLoudness.h"

#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <math.h>

AudioEffect *createEffectInstance(audioMasterCallback audioMaster)
{
  return new mdaLoudness(audioMaster);
}

float loudness[14][3] = { {402.f,  0.0025f,  0.00f},  //-60dB
                          {334.f,  0.0121f,  0.00f},
                          {256.f,  0.0353f,  0.00f},
                          {192.f,  0.0900f,  0.00f},
                          {150.f,  0.2116f,  0.00f},
                          {150.f,  0.5185f,  0.00f},
                          { 1.0f,     0.0f,  0.00f},  //0dB
                          {33.7f,     5.5f,  1.00f},
                          {92.0f,     8.7f,  0.62f},
                          {63.7f,    18.4f,  0.44f},
                          {42.9f,    48.2f,  0.30f},
                          {37.6f,   116.2f,  0.18f},
                          {22.9f,   428.7f,  0.09f},  //+60dB
                          { 0.0f,     0.0f,  0.00f}  };


mdaLoudnessProgram::mdaLoudnessProgram() ///default program settings
{
  param[0] = 0.70f;  //loudness
  param[1] = 0.50f;  //output
  param[2] = 0.35f;  //link
  strcpy(name, "Equal Loudness Contours");  //re. Stevens-Davis @ 100dB
}

bool  mdaLoudness::getProductString(char* text) { strcpy(text, "MDA Loudness"); return true; }
bool  mdaLoudness::getVendorString(char* text)  { strcpy(text, "mda"); return true; }
bool  mdaLoudness::getEffectName(char* name)    { strcpy(name, "Loudness"); return true; }

mdaLoudness::mdaLoudness(audioMasterCallback audioMaster): AudioEffectX(audioMaster, NPROGS, NPARAMS)
{
  setNumInputs(2);
  setNumOutputs(2);
  setUniqueID("mdaLoudness");
	DECLARE_LVZ_DEPRECATED(canMono) ();
  canProcessReplacing();

  programs = new mdaLoudnessProgram[NPROGS];
  setProgram(0);

  suspend();
}


void mdaLoudness::resume() ///update internal parameters...
{
  float * param = programs[curProgram].param;
  float f, tmp;
  int32_t  i;

  tmp = param[0] + param[0] - 1.0f;
  igain = 60.0f * tmp * tmp;
  if(tmp<0.0f) igain *= -1.0f;

  tmp = param[1] + param[1] - 1.0f;
  ogain = 60.0f * tmp * tmp;
  if(tmp<0.0f) ogain *= -1.0f;

  f = 0.1f * igain + 6.0f;  //coefficient index + fractional part
  i = (int32_t)f;
  f -= (float)i;

  tmp = loudness[i][0];  A0 = tmp + f * (loudness[i + 1][0] - tmp);
  tmp = loudness[i][1];  A1 = tmp + f * (loudness[i + 1][1] - tmp);
  tmp = loudness[i][2];  A2 = tmp + f * (loudness[i + 1][2] - tmp);

  A0 = 1.0f - (float)exp(-6.283153f * A0 / getSampleRate());

  if(igain>0)
  {
    //if(mode==0) suspend();  //don't click when switching mode
    mode=1;
  }
  else
  {
    //if(mode==1) suspend();
    mode=0;
  }

  tmp = ogain;
  if(param[2]>0.5f)  //linked gain
  {
    tmp -= igain;  if(tmp>0.0f) tmp = 0.0f;  //limit max gain
  }
  gain = (float)pow(10.0f, 0.05f * tmp);
}


void mdaLoudness::suspend() ///clear any buffers...
{
  Z0 = Z1 = Z2 = Z3 = 0.0f;
}


mdaLoudness::~mdaLoudness() ///destroy any buffers...
{
  if(programs) delete[] programs;
}

void mdaLoudness::setProgram(int32_t program)
{
  curProgram = program;
  resume();
}

void  mdaLoudness::setParameter(int32_t index, float value)
{
  programs[curProgram].param[index] = value; //bug was here!
  resume();
}

float mdaLoudness::getParameter(int32_t index) { return programs[curProgram].param[index]; }
void  mdaLoudness::setProgramName(char *name) { strcpy(programs[curProgram].name, name); }
void  mdaLoudness::getProgramName(char *name) { strcpy(name, programs[curProgram].name); }
bool mdaLoudness::getProgramNameIndexed (int32_t category, int32_t index, char* name)
{
	if ((unsigned int)index < NPROGS) 
	{
	    strcpy(name, programs[index].name);
	    return true;
	}
	return false;
}


void mdaLoudness::getParameterName(int32_t index, char *label)
{
  switch(index)
  {
    case  0: strcpy(label, "Loudness"); break;
    case  1: strcpy(label, "Output"); break;
    default: strcpy(label, "Link");
  }
}


void mdaLoudness::getParameterDisplay(int32_t index, char *text)
{
 	char string[16];

  switch(index)
  {
    case  0: sprintf(string, "%.1f", igain); break;
    case  2: if(programs[curProgram].param[index]>0.5f) strcpy (string, "ON");
                              else strcpy (string, "OFF"); break;
    default: sprintf(string, "%.1f", ogain); break;
  }
	string[8] = 0;
	strcpy(text, (char *)string);
}


void mdaLoudness::getParameterLabel(int32_t index, char *label)
{
  switch(index)
  {
    case  2: strcpy(label, ""); break;
    default: strcpy(label, "dB");
  }
}


void mdaLoudness::process(float **inputs, float **outputs, int32_t sampleFrames)
{
  float *in1 = inputs[0];
  float *in2 = inputs[1];
  float *out1 = outputs[0];
  float *out2 = outputs[1];
  float a, b, c, d;
  float z0=Z0, z1=Z1, z2=Z2, z3=Z3;
  float a0=A0, a1=A1, a2=A2, g=gain;

  --in1;
  --in2;
  --out1;
  --out2;

  if(mode==0) //cut
  {
    while(--sampleFrames >= 0)
    {
      a = *++in1;
      b = *++in2;
      c = out1[1];
      d = out2[1];

      z0 += a0 * (a - z0 + 0.3f * z1);  a -= z0;
      z1 += a0 * (a - z1);              a -= z1;
                                        a -= z0 * a1;
      z2 += a0 * (b - z2 + 0.3f * z1);  b -= z2;
      z3 += a0 * (b - z3);              b -= z3;
                                        b -= z2 * a1;
      c += a * g;
      d += b * g;

      *++out1 = c;
      *++out2 = d;
    }
  }
  else //boost
  {
    while(--sampleFrames >= 0)
    {
      a = *++in1;
      b = *++in2;
      c = out1[1];
      d = out2[1];

      z0 += a0 * (a  - z0);
      z1 += a0 * (z0 - z1);   a += a1 * (z1 - a2 * z0);

      z2 += a0 * (b  - z2);
      z3 += a0 * (z2 - z3);   b += a1 * (z3 - a2 * z2);

      c += a * g;
      d += b * g;

      *++out1 = c;
      *++out2 = d;
    }
  }
  if(fabs(z1)<1.0e-10 || fabs(z1)>100.0f) { Z0 = Z1 = 0.0f; } else { Z0 = z0; Z1 = z1; } //catch denormals
  if(fabs(z3)<1.0e-10 || fabs(z3)>100.0f) { Z2 = Z3 = 0.0f; } else { Z2 = z2; Z3 = z3; }
}


void mdaLoudness::processReplacing(float **inputs, float **outputs, int32_t sampleFrames)
{
  float *in1 = inputs[0];
  float *in2 = inputs[1];
  float *out1 = outputs[0];
  float *out2 = outputs[1];
  float a, b, c, d;
  float z0=Z0, z1=Z1, z2=Z2, z3=Z3;
  float a0=A0, a1=A1, a2=A2, g=gain;

  --in1;
  --in2;
  --out1;
  --out2;

  if(mode==0) //cut
  {
    while(--sampleFrames >= 0)
    {
      a = *++in1;
      b = *++in2;

      z0 += a0 * (a - z0 + 0.3f * z1);  a -= z0;
      z1 += a0 * (a - z1);              a -= z1;
                                        a -= z0 * a1;
      z2 += a0 * (b - z2 + 0.3f * z1);  b -= z2;
      z3 += a0 * (b - z3);              b -= z3;
                                        b -= z2 * a1;
      c = a * g;
      d = b * g;

      *++out1 = c;
      *++out2 = d;
    }
  }
  else //boost
  {
    while(--sampleFrames >= 0)
    {
      a = *++in1;
      b = *++in2;

      z0 += a0 * (a  - z0);
      z1 += a0 * (z0 - z1);   a += a1 * (z1 - a2 * z0);

      z2 += a0 * (b  - z2);
      z3 += a0 * (z2 - z3);   b += a1 * (z3 - a2 * z2);

      c = g * a;
      d = g * b;

      *++out1 = c;
      *++out2 = d;
    }
  }
  if(fabs(z1)<1.0e-10 || fabs(z1)>100.0f) { Z0 = Z1 = 0.0f; } else { Z0 = z0; Z1 = z1; } //catch denormals
  if(fabs(z3)<1.0e-10 || fabs(z3)>100.0f) { Z2 = Z3 = 0.0f; } else { Z2 = z2; Z3 = z3; }
}
