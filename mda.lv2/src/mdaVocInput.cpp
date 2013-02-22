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

#include "mdaVocInput.h"

#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <math.h>

AudioEffect *createEffectInstance(audioMasterCallback audioMaster)
{
  return new mdaVocInput(audioMaster);
}

mdaVocInputProgram::mdaVocInputProgram() ///default program settings
{
  param[0] = 0.0f;  //Tracking Off / On / Quant
  param[1] = 0.50f;  //Pitch
  param[2] = 0.20f;  //Breath Noise
  param[3] = 0.50f;  //Voiced/Unvoiced Thresh
  param[4] = 0.35f;  //Max Freq
  strcpy(name, "Vocoder Carrier Signal");
}


mdaVocInput::mdaVocInput(audioMasterCallback audioMaster): AudioEffectX(audioMaster, NPROGS, NPARAMS)
{
  setNumInputs(2);
  setNumOutputs(2);
  setUniqueID("mdaVocInput");  ///identify plug-in here
	DECLARE_LVZ_DEPRECATED(canMono) ();
  canProcessReplacing();

  track = 0;
  pstep = pmult = sawbuf = noise = lenv = henv = 0.0f;
  lbuf0 = lbuf1 = lbuf2 = lbuf3 = lfreq = vuv = maxp = minp = 0.0f;
  root = 0.0;

  programs = new mdaVocInputProgram[numPrograms];
  setProgram(0);

  suspend();
}

bool  mdaVocInput::getProductString(char* text) { strcpy(text, "MDA VocInput"); return true; }
bool  mdaVocInput::getVendorString(char* text)  { strcpy(text, "mda"); return true; }
bool  mdaVocInput::getEffectName(char* name)    { strcpy(name, "VocInput"); return true; }

void mdaVocInput::resume() ///update internal parameters...
{
  float fs, ifs;
  float * param = programs[curProgram].param;
  fs = getSampleRate();
  ifs = 1.0f / fs;

  track = (int32_t)(2.99f * param[0]);
  pmult = (float)pow(1.0594631f, floor(48.0f * param[1] - 24.0f));
  if(track==0) pstep = 110.0f * pmult * ifs;

  noise = 6.0f * param[2];
  lfreq = 660.0f * ifs;
  minp = (float)pow(16.0f, 0.5f - param[4]) * fs / 440.0f;
  maxp = 0.03f * fs;
  root = log10(8.1757989f * ifs);
  vuv = param[3] * param[3];
}


void mdaVocInput::suspend() ///clear any buffers...
{
  lbuf0 = lbuf1 = lbuf2 = lbuf3 = 0.0f;
  pstep = sawbuf = lenv = 0.0f;
}


mdaVocInput::~mdaVocInput() ///destroy any buffers...
{
  if(programs) delete [] programs;
}


void mdaVocInput::setProgram(int32_t program)
{
  curProgram = program;
  resume();
}


void  mdaVocInput::setParameter(int32_t index, float value)
{
  programs[curProgram].param[index] = value;
  resume();
}
float mdaVocInput::getParameter(int32_t index) { return programs[curProgram].param[index]; }
void  mdaVocInput::setProgramName(char *name) { strcpy(programs[curProgram].name, name); }
void  mdaVocInput::getProgramName(char *name) { strcpy(name, programs[curProgram].name); }
bool mdaVocInput::getProgramNameIndexed (int32_t category, int32_t index, char* name)
{
	if ((unsigned int)index < NPROGS) 
	{
	    strcpy(name, programs[index].name);
	    return true;
	}
	return false;
}

void mdaVocInput::getParameterName(int32_t index, char *label)
{
  switch(index)
  {
    case  0: strcpy(label, "Tracking"); break;
    case  1: strcpy(label, "Pitch"); break;
    case  2: strcpy(label, "Breath"); break;
    case  3: strcpy(label, "S Thresh"); break;
    default: strcpy(label, "Max Freq");
  }
}


void mdaVocInput::getParameterDisplay(int32_t index, char *text)
{
 	char string[16];
 	float * param = programs[curProgram].param;

  switch(index)
  {
    case  0: switch(track) { case 0: strcpy(string, "OFF");   break;
                             case 1: strcpy(string, "FREE");   break;
                             case 2: strcpy(string, "QUANT"); } break;

    case  1: if(track) sprintf(string, "%d", (int32_t)(48.0f * param[1] - 24.0f)); 
                  else midi2string((int32_t)(48.0f * param[1] + 21.0f), string); break;

    case  4: midi2string((int32_t)(48.0f * param[4] + 45.0f), string); break;

    default: sprintf(string, "%.0f", 100.0f * param[index]);
  }
	string[8] = 0;
	strcpy(text, (char *)string);
}


void mdaVocInput::getParameterLabel(int32_t index, char *label)
{
  switch(index)
  {
    case  0: strcpy(label, ""); break;
    case  1: strcpy(label, ""); break;
    case  4: strcpy(label, "Hz"); break;
    default: strcpy(label, "%");
  }
}


void mdaVocInput::midi2string(int32_t n, char *text) //show name of MIDI note number (60=C3)
{
  char t[8];
  int32_t o, s, p=0;

  t[p++] = ' ';
  t[p++] = ' ';
  t[p++] = ' ';

  o = int(n/12.f); s = n-(12*o); o -= 2;

  switch(s)
  {
    case  0: t[p++]='C';             break;
    case  1: t[p++]='C'; t[p++]='#'; break;
    case  2: t[p++]='D';             break;
    case  3: t[p++]='D'; t[p++]='#'; break;
    case  4: t[p++]='E';             break;
    case  5: t[p++]='F';             break;
    case  6: t[p++]='F'; t[p++]='#'; break;
    case  7: t[p++]='G';             break;
    case  8: t[p++]='G'; t[p++]='#'; break;
    case  9: t[p++]='A';             break;
    case 10: t[p++]='A'; t[p++]='#'; break;
    default: t[p++]='B';
  }
  t[p++] = ' ';

  if(o < 0) { t[p++] = '-';  o = -o; }
  t[p++] = (char)(48 + (o % 10));

  t[p] = 0;
  strcpy(text, t);
}


void mdaVocInput::process(float **inputs, float **outputs, int32_t sampleFrames)
{
  float *in1 = inputs[0];
  float *in2 = inputs[1];
  float *out1 = outputs[0];
  float *out2 = outputs[1];
  float a, b, c, d;
  float ds=pstep, s=sawbuf, n=noise;
  float l0=lbuf0, l1=lbuf1, l2=lbuf2, l3=lbuf3;
  float le=lenv, he=henv, et=lfreq*0.1f, lf=lfreq, v=vuv, mn=minp, mx=maxp;
  float rootm=39.863137f;
  int32_t  tr=track;

  --in1;
  --in2;
  --out1;
  --out2;
  while(--sampleFrames >= 0)
  {
    a = *++in1;
    //b = *++in2;
    c = out1[1];
    d = out2[1];

    l0 -= lf * (l1 + a);       //fundamental filter (peaking 2nd-order 100Hz lpf)
    l1 -= lf * (l1 - l0);

    b = l0; if(b<0.0f) b = -b;
    le -= et * (le - b);       //fundamental level

    b = (a + 0.03f) * v;
    if(b<0.0f) b = -b;
    he -= et * (he - b);       //overall level (+ constant so >f0 when quiet)

    l3 += 1.0f;
    if(tr>0)                   //pitch tracking
    {
      if(l1>0.0f && l2<=0.0f)  //found +ve zero crossing
      {
        if(l3>mn && l3<mx)     //...in allowed range
        {
          mn = 0.6f * l3;       //new max pitch to discourage octave jumps!
          l2 = l1 / (l1 - l2);   //fractional period...
          ds = pmult / (l3 - l2); //new period

          if(tr==2)            //quantize pitch
          {
            ds = rootm * (float)(log10(ds) - root);
            ds = (float)pow(1.0594631, floor(ds + 0.5) + rootm * root);
          }
        }
        l3 = l2;               //restart period measurement
      }
      l2=l1;                   //remember previous sample
    }

    b = 0.00001f * (float)((rand() & 32767) - 16384);  //sibilance
    if(le>he) b *= s * n;                    //...or modulated breath noise
    b += s; s += ds; if(s>0.5f) s-=1.0f;     //badly aliased sawtooth!

    c += a;
    d += b;

    *++out1 = c;
    *++out2 = d;
  }
  sawbuf=s;

  if(fabs(he)>1.0e-10) henv = he; else henv=0.0f; //catch denormals
  if(fabs(l1)>1.0e-10) { lbuf0=l0; lbuf1=l1; lenv=le; } else { lbuf0 = lbuf1= lenv = 0.0f; }
  lbuf2=l2, lbuf3=l3;
  if(tr) pstep=ds;
}


void mdaVocInput::processReplacing(float **inputs, float **outputs, int32_t sampleFrames)
{
  float *in1 = inputs[0];
  float *in2 = inputs[1];
  float *out1 = outputs[0];
  float *out2 = outputs[1];
  float a, b;
  float ds=pstep, s=sawbuf, n=noise;
  float l0=lbuf0, l1=lbuf1, l2=lbuf2, l3=lbuf3;
  float le=lenv, he=henv, et=lfreq*0.1f, lf=lfreq, v=vuv, mn=minp, mx=maxp;
  float rootm=39.863137f;
  int32_t  tr=track;

  --in1;
  --in2;
  --out1;
  --out2;
  while(--sampleFrames >= 0)
  {
    a = *++in1;
    //b = *++in2;

    l0 -= lf * (l1 + a);       //fundamental filter (peaking 2nd-order 100Hz lpf)
    l1 -= lf * (l1 - l0);

    b = l0; if(b<0.0f) b = -b;
    le -= et * (le - b);       //fundamental level

    b = (a + 0.03f) * v;
    if(b<0.0f) b = -b;
    he -= et * (he - b);       //overall level (+ constant so >f0 when quiet)

    l3 += 1.0f;
    if(tr>0)                   //pitch tracking
    {
      if(l1>0.0f && l2<=0.0f)  //found +ve zero crossing
      {
        if(l3>mn && l3<mx)     //...in allowed range
        {
          mn = 0.6f * l3;       //new max pitch to discourage octave jumps!
          l2 = l1 / (l1 - l2);   //fractional period...
          ds = pmult / (l3 - l2); //new period

          if(tr==2)            //quantize pitch
          {
            ds = rootm * (float)(log10(ds) - root);
            ds = (float)pow(1.0594631, floor(ds + 0.5) + rootm * root);
          }
        }
        l3 = l2;               //restart period measurement
      }
      l2=l1;                   //remember previous sample
    }

    b = 0.00001f * (float)((rand() & 32767) - 16384);  //sibilance
    if(le>he) b *= s * n;                    //...or modulated breath noise
    b += s; s += ds; if(s>0.5f) s-=1.0f;     //badly aliased sawtooth!

    *++out1 = a;
    *++out2 = b;
  }
  sawbuf=s;

  if(fabs(he)>1.0e-10) henv = he; else henv=0.0f; //catch denormals
  if(fabs(l1)>1.0e-10) { lbuf0=l0; lbuf1=l1; lenv=le; } else { lbuf0 = lbuf1= lenv = 0.0f; }
  lbuf2=l2, lbuf3=l3;
  if(tr) pstep=ds;
}
