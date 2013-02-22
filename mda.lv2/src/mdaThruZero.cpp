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

#include "mdaThruZero.h"

#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <math.h>

AudioEffect *createEffectInstance(audioMasterCallback audioMaster)
{
  return new mdaThruZero(audioMaster);
}

mdaThruZeroProgram::mdaThruZeroProgram() ///default program settings
{
  param[0] = 0.30f;  //rate
  param[1] = 0.43f;  //depth
  param[2] = 0.47f;  //mix
  param[3] = 0.30f;  //feedback
  param[4] = 1.00f;  //minimum delay to stop LF buildup with feedback
  strcpy(name, "Thru-Zero Flanger");
}


mdaThruZero::mdaThruZero(audioMasterCallback audioMaster): AudioEffectX(audioMaster, NPROGS, NPARAMS)
{
  setNumInputs(2);
  setNumOutputs(2);
  setUniqueID("mdaThruZero");  ///identify plug-in here
	DECLARE_LVZ_DEPRECATED(canMono) ();
  canProcessReplacing();

  programs = new mdaThruZeroProgram[NPROGS]; ///////////////TODO: programs
  setProgram(0);

  ///differences from default program...
  programs[1].param[0] = 0.50f;
  programs[1].param[1] = 0.20f;
  programs[1].param[2] = 0.47f;
  strcpy(programs[1].name,"Phase Canceller");
  programs[2].param[0] = 0.60f;
  programs[2].param[1] = 0.60f;
  programs[2].param[2] = 0.35f;
  programs[2].param[4] = 0.70f;
  strcpy(programs[2].name,"Chorus Doubler");
  programs[3].param[0] = 0.75f;
  programs[3].param[1] = 1.00f;
  programs[3].param[2] = 0.50f;
  programs[3].param[3] = 0.75f;
  programs[3].param[4] = 1.00f;
  strcpy(programs[3].name,"Mad Modulator");



  ///initialise...
  bufpos  = 0;
  buffer  = new float[BUFMAX];
  buffer2 = new float[BUFMAX];
  phi = fb = fb1 = fb2 = deps = 0.0f;

  suspend();
}

bool  mdaThruZero::getProductString(char* text) { strcpy(text, "MDA ThruZero"); return true; }
bool  mdaThruZero::getVendorString(char* text)  { strcpy(text, "mda"); return true; }
bool  mdaThruZero::getEffectName(char* name)    { strcpy(name, "ThruZero"); return true; }

void mdaThruZero::resume() ///update internal parameters...
{
  float * param = programs[curProgram].param;
  rat = (float)(pow(10.0f, 3.f * param[0] - 2.f) * 2.f / getSampleRate());
  dep = 2000.0f * param[1] * param[1];
  dem = dep - dep * param[4];
  dep -= dem;

  wet = param[2];
  dry = 1.f - wet;
  if(param[0]<0.01f) { rat=0.0f; phi=(float)0.0f; }
  fb = 1.9f * param[3] - 0.95f;
}


void mdaThruZero::suspend() ///clear any buffers...
{
  if(buffer) memset(buffer , 0, BUFMAX * sizeof(float));
  if(buffer2) memset(buffer2, 0, BUFMAX * sizeof(float));
}//^^^being cautious as was crashing in AudioMulch when unloaded


mdaThruZero::~mdaThruZero() ///destroy any buffers...
{
  if(buffer) delete [] buffer;
  if(buffer2) delete [] buffer2;
  if(programs) delete[] programs;
}


void mdaThruZero::setProgram(int32_t program)
{
  curProgram = program;
  resume();
}


void  mdaThruZero::setParameter(int32_t index, float value)
{
  if(index==3) phi=0.0f; //reset cycle
  programs[curProgram].param[index] = value;
  resume(); 
}


float mdaThruZero::getParameter(int32_t index) { return programs[curProgram].param[index]; }
void  mdaThruZero::setProgramName(char *name) { strcpy(programs[curProgram].name, name); }
void  mdaThruZero::getProgramName(char *name) { strcpy(name, programs[curProgram].name); }
bool mdaThruZero::getProgramNameIndexed (int32_t category, int32_t index, char* name)
{
	if ((unsigned int)index < NPROGS) 
	{
	    strcpy(name, programs[index].name);
	    return true;
	}
	return false;
}

void mdaThruZero::getParameterName(int32_t index, char *label)
{
  switch(index)
  {
    case  0: strcpy(label, "Rate"); break;
    case  1: strcpy(label, "Depth"); break;
    case  2: strcpy(label, "Mix"); break;
    case  4: strcpy(label, "DepthMod"); break;
    default: strcpy(label, "Feedback");
  }
}


void mdaThruZero::getParameterDisplay(int32_t index, char *text)
{
 	char string[16];
 	float * param = programs[curProgram].param;

  switch(index)
  {
    case  0: if(param[0]<0.01f) strcpy (string, "-");
             else sprintf(string, "%.2f", (float)pow(10.0f ,2.0f - 3.0f * param[index])); break;
    case  1: sprintf(string, "%.2f", 1000.f * dep / getSampleRate()); break;
    case  3: sprintf(string, "%.0f", 200.0f * param[index] - 100.0f); break;
    default: sprintf(string, "%.0f", 100.0f * param[index]);
  }
	string[8] = 0;
	strcpy(text, (char *)string);
}


void mdaThruZero::getParameterLabel(int32_t index, char *label)
{
  switch(index)
  {
    case  0: strcpy(label, "sec"); break;
    case  1: strcpy(label, "ms"); break;
    default: strcpy(label, "%");
  }
}


void mdaThruZero::process(float **inputs, float **outputs, int32_t sampleFrames)
{
  float *in1 = inputs[0];
  float *in2 = inputs[1];
  float *out1 = outputs[0];
  float *out2 = outputs[1];
  float a, b, c, d;

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

    c += a; ///process here
    d += b;

    *++out1 = c;
    *++out2 = d;
  }
}


void mdaThruZero::processReplacing(float **inputs, float **outputs, int32_t sampleFrames)
{
  float *in1 = inputs[0];
  float *in2 = inputs[1];
  float *out1 = outputs[0];
  float *out2 = outputs[1];
	float a, b, f=fb, f1=fb1, f2=fb2, ph=phi;
  float ra=rat, de=dep, we=wet, dr=dry, ds=deps, dm=dem;
  int32_t  tmp, tmpi, bp=bufpos;
  float tmpf;

	--in1;
	--in2;
	--out1;
	--out2;
	while(--sampleFrames >= 0)
	{
		a = *++in1;
		b = *++in2;

    ph += ra;
    if(ph>1.0f) ph -= 2.0f;

    bp--; bp &= 0x7FF;
    *(buffer  + bp) = a + f * f1;
    *(buffer2 + bp) = b + f * f2;

    //ds = 0.995f * (ds - de) + de;          //smoothed depth change ...try inc not mult
    tmpf = dm + de * (1.0f - ph * ph); //delay mod shape
    tmp  = int(tmpf);
    tmpf -= tmp;
    tmp = (tmp + bp) & 0x7FF;
    tmpi = (tmp + 1) & 0x7FF;

    f1 = *(buffer  + tmp);  //try adding a constant to reduce denormalling
    f2 = *(buffer2 + tmp);
    f1 = tmpf * (*(buffer  + tmpi) - f1) + f1; //linear interpolation
    f2 = tmpf * (*(buffer2 + tmpi) - f2) + f2;

    a = a * dr - f1 * we;
		b = b * dr - f2 * we;

    *++out1 = a;
		*++out2 = b;
	}
  if(fabs(f1)>1.0e-10) { fb1 = f1; fb2 = f2; } else fb1 = fb2 = 0.0f; //catch denormals
  phi = ph;
  deps = ds;
  bufpos = bp;
}
