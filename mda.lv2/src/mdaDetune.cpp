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

#include "mdaDetune.h"

#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <math.h>

AudioEffect *createEffectInstance(audioMasterCallback audioMaster)
{
  return new mdaDetune(audioMaster);
}

bool  mdaDetune::getProductString(char* text) { strcpy(text, "mda Detune"); return true; }
bool  mdaDetune::getVendorString(char* text)  { strcpy(text, "mda"); return true; }
bool  mdaDetune::getEffectName(char* name)    { strcpy(name, "Detune"); return true; }

mdaDetune::mdaDetune(audioMasterCallback audioMaster): AudioEffectX(audioMaster, NPROGS, NPARAMS)
{
  setNumInputs(2);
  setNumOutputs(2);
  setUniqueID("mdaDetune");  ///identify mdaDetune-in here
	DECLARE_LVZ_DEPRECATED(canMono) ();
  canProcessReplacing();

  programs[0].param[0] = 0.20f; //fine
  programs[0].param[1] = 0.90f; //mix
  programs[0].param[2] = 0.50f; //output
  programs[0].param[3] = 0.50f; //chunksize
  strcpy(programs[0].name, "Stereo Detune");
  programs[1].param[0] = 0.20f;
  programs[1].param[1] = 0.90f;
  programs[1].param[2] = 0.50f;
  programs[1].param[3] = 0.50f;
  strcpy(programs[1].name,"Symphonic");
  programs[2].param[0] = 0.8f;
  programs[2].param[1] = 0.7f;
  programs[2].param[2] = 0.50f;
  programs[2].param[3] = 0.50f;
  strcpy(programs[2].name,"Out Of Tune");

  ///initialise...
  curProgram=0;
  suspend();
  
  semi = 3.0f * 0.20f * 0.20f * 0.20f;
  dpos2 = (float)pow(1.0594631f, semi);
  dpos1 = 1.0f / dpos2;
  wet = 1.0f;
  dry = wet - wet * 0.90f * 0.90f;
  wet = (wet + wet - wet * 0.90f) * 0.90f;
}

void mdaDetune::suspend() ///clear any buffers...
{
  memset(buf, 0, sizeof (buf));
  memset(win, 0, sizeof (win));
  pos0 = 0; pos1 = pos2 = 0.0f;
  
  //recalculate crossfade window
  buflen = 1 << (8 + (int32_t)(4.9f * programs[curProgram].param[3]));
	if (buflen > BUFMAX) buflen = BUFMAX;
  bufres = 1000.0f * (float)buflen / getSampleRate();

  int32_t i; //hanning half-overlap-and-add
  double p=0.0, dp=6.28318530718/buflen;
  for(i=0;i<buflen;i++) { win[i] = (float)(0.5 - 0.5 * cos(p)); p+=dp; }
}


void mdaDetune::setProgram(int32_t program)
{
	if ((unsigned int)program < NPROGS)
	{
		curProgram = program;
		
		// update
		float * param = programs[curProgram].param;
    semi = 3.0f * param[0] * param[0] * param[0];
    dpos2 = (float)pow(1.0594631f, semi);
    dpos1 = 1.0f / dpos2;
    
    wet = (float)pow(10.0f, 2.0f * param[2] - 1.0f);
    dry = wet - wet * param[1] * param[1];
    wet = (wet + wet - wet * param[1]) * param[1];
    
    int32_t tmp = 1 << (8 + (int32_t)(4.9f * param[3]));

    if(tmp!=buflen) //recalculate crossfade window
    {
      buflen = tmp;
	    if (buflen > BUFMAX) buflen = BUFMAX;
      bufres = 1000.0f * (float)buflen / getSampleRate();

      int32_t i; //hanning half-overlap-and-add
      double p=0.0, dp=6.28318530718/buflen;
      for(i=0;i<buflen;i++) { win[i] = (float)(0.5 - 0.5 * cos(p)); p+=dp; }
    }
	}
}


void  mdaDetune::setParameter(int32_t which, float value)
{
  float * param = programs[curProgram].param;
  param[which] = value;

  switch(which)
  {
    case  0:
      semi = 3.0f * param[0] * param[0] * param[0];
      dpos2 = (float)pow(1.0594631f, semi);
      dpos1 = 1.0f / dpos2;
      break;
    case  1:
    case  2:
      wet = (float)pow(10.0f, 2.0f * param[2] - 1.0f);
      dry = wet - wet * param[1] * param[1];
      wet = (wet + wet - wet * param[1]) * param[1];
      break;
    case 3:
    {
      int32_t tmp = 1 << (8 + (int32_t)(4.9f * param[3]));

      if(tmp!=buflen) //recalculate crossfade window
      {
        buflen = tmp;
	      if (buflen > BUFMAX) buflen = BUFMAX;
        bufres = 1000.0f * (float)buflen / getSampleRate();

        int32_t i; //hanning half-overlap-and-add
        double p=0.0, dp=6.28318530718/buflen;
        for(i=0;i<buflen;i++) { win[i] = (float)(0.5 - 0.5 * cos(p)); p+=dp; }
      }
      break;
    }
    default:
      break;
  }
}


float mdaDetune::getParameter(int32_t which) { return programs[curProgram].param[which]; }
void  mdaDetune::setProgramName(char *name) { strcpy(programs[curProgram].name, name); }
void  mdaDetune::getProgramName(char *name) { strcpy(name, programs[curProgram].name); }
bool mdaDetune::getProgramNameIndexed (int32_t category, int32_t which, char* name)
{
	if ((unsigned int)which < NPROGS)
	{
	    strcpy(name, programs[which].name);
	    return true;
	}
	return false;
}


void mdaDetune::getParameterName(int32_t which, char *label)
{
  switch(which)
  {
    case  0: strcpy(label, "Detune"); break;
    case  1: strcpy(label, "Mix"); break;
    case  2: strcpy(label, "Output"); break;
    default: strcpy(label, "Latency");
  }
}


void mdaDetune::getParameterDisplay(int32_t which, char *text)
{
 	char string[16];

  switch(which)
  {
    case  1: sprintf(string, "%.0f", 99.0f * programs[curProgram].param[which]); break;
    case  2: sprintf(string, "%.1f", 40.0f * programs[curProgram].param[which] - 20.0f); break;
    case  3: sprintf(string, "%.1f", bufres); break;
    default: sprintf(string, "%.1f", 100.0f * semi);
  }
	string[8] = 0;
	strcpy(text, (char *)string);
}


void mdaDetune::getParameterLabel(int32_t which, char *label)
{
  switch(which)
  {
    case  0: strcpy(label, "cents"); break;
    case  1: strcpy(label, "%"); break;
    case  2: strcpy(label, "dB"); break;
    default: strcpy(label, "ms");
  }
}


void mdaDetune::process(float **inputs, float **outputs, int32_t sampleFrames)
{
  float *in1 = inputs[0];
  float *in2 = inputs[1];
  float *out1 = outputs[0];
  float *out2 = outputs[1];
  float a, b, c, d;
  float x, w=wet, y=dry, p1=pos1, p1f, d1=dpos1;
  float                  p2=pos2,      d2=dpos2;
  int32_t  p0=pos0, p1i, p2i;
  int32_t  l=buflen-1, lh=buflen>>1;
  float lf = (float)buflen;

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

    c += y * a;
    d += y * b;

    --p0 &= l;
    *(buf + p0) = w * (a + b);      //input

    p1 -= d1;
    if(p1<0.0f) p1 += lf;           //output
    p1i = (int32_t)p1;
    p1f = p1 - (float)p1i;
    a = *(buf + p1i);
    ++p1i &= l;
    a += p1f * (*(buf + p1i) - a);  //linear interpolation

    p2i = (p1i + lh) & l;           //180-degree ouptut
    b = *(buf + p2i);
    ++p2i &= l;
    b += p1f * (*(buf + p2i) - b);  //linear interpolation

    p2i = (p1i - p0) & l;           //crossfade window
    x = *(win + p2i);
    //++p2i &= l;
    //x += p1f * (*(win + p2i) - x); //linear interpolation (doesn't do much)
    c += b + x * (a - b);

    p2 -= d2;                //repeat for downwards shift - can't see a more efficient way?
    if(p2<0.0f) p2 += lf;           //output
    p1i = (int32_t)p2;
    p1f = p2 - (float)p1i;
    a = *(buf + p1i);
    ++p1i &= l;
    a += p1f * (*(buf + p1i) - a);  //linear interpolation

    p2i = (p1i + lh) & l;           //180-degree ouptut
    b = *(buf + p2i);
    ++p2i &= l;
    b += p1f * (*(buf + p2i) - b);  //linear interpolation

    p2i = (p1i - p0) & l;           //crossfade window
    x = *(win + p2i);
    //++p2i &= l;
    //x += p1f * (*(win + p2i) - x); //linear interpolation (doesn't do much)
    d += b + x * (a - b);

    *++out1 = c;
    *++out2 = d;
  }
  pos0=p0; pos1=p1; pos2=p2;
}


void mdaDetune::processReplacing(float **inputs, float **outputs, int32_t sampleFrames)
{
  float *in1 = inputs[0];
  float *in2 = inputs[1];
  float *out1 = outputs[0];
  float *out2 = outputs[1];
  float a, b, c, d;
  float x, w=wet, y=dry, p1=pos1, p1f, d1=dpos1;
  float                  p2=pos2,      d2=dpos2;
  int32_t  p0=pos0, p1i, p2i;
  int32_t  l=buflen-1, lh=buflen>>1;
  float lf = (float)buflen;

  --in1;
  --in2;
  --out1;
  --out2;
  while(--sampleFrames >= 0)  //had to disable optimization /Og in MSVC++5!
  {
    a = *++in1;
    b = *++in2;

    c = y * a;
    d = y * b;

    --p0 &= l;
    *(buf + p0) = w * (a + b);      //input

    p1 -= d1;
    if(p1<0.0f) p1 += lf;           //output
    p1i = (int32_t)p1;
    p1f = p1 - (float)p1i;
    a = *(buf + p1i);
    ++p1i &= l;
    a += p1f * (*(buf + p1i) - a);  //linear interpolation

    p2i = (p1i + lh) & l;           //180-degree ouptut
    b = *(buf + p2i);
    ++p2i &= l;
    b += p1f * (*(buf + p2i) - b);  //linear interpolation

    p2i = (p1i - p0) & l;           //crossfade
    x = *(win + p2i);
    //++p2i &= l;
    //x += p1f * (*(win + p2i) - x); //linear interpolation (doesn't do much)
    c += b + x * (a - b);

    p2 -= d2;  //repeat for downwards shift - can't see a more efficient way?
    if(p2<0.0f) p2 += lf;           //output
    p1i = (int32_t)p2;
    p1f = p2 - (float)p1i;
    a = *(buf + p1i);
    ++p1i &= l;
    a += p1f * (*(buf + p1i) - a);  //linear interpolation

    p2i = (p1i + lh) & l;           //180-degree ouptut
    b = *(buf + p2i);
    ++p2i &= l;
    b += p1f * (*(buf + p2i) - b);  //linear interpolation

    p2i = (p1i - p0) & l;           //crossfade
    x = *(win + p2i);
    //++p2i &= l;
    //x += p1f * (*(win + p2i) - x); //linear interpolation (doesn't do much)
    d += b + x * (a - b);

    *++out1 = c;
    *++out2 = d;
  }
  pos0=p0; pos1=p1; pos2=p2;
}
