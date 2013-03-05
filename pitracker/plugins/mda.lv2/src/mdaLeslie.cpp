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

//Remove optimization /Og else wavelab crashes!!!

#include "mdaLeslie.h"

#include <stdlib.h>
#include <math.h>
#include <float.h>

AudioEffect *createEffectInstance(audioMasterCallback audioMaster)
{
  return new mdaLeslie(audioMaster);
}

mdaLeslieProgram::mdaLeslieProgram()
{
  param[0] = 0.5f;
  param[1] = 0.50f;
  param[2] = 0.48f;
  param[3] = 0.70f;
  param[4] = 0.60f;
  param[5] = 0.70f;
  param[6] = 0.50f;
  param[7] = 0.50f;
  param[8] = 0.60f;
  strcpy(name, "Leslie Simulator");
}

mdaLeslie::mdaLeslie(audioMasterCallback audioMaster)	: AudioEffectX(audioMaster, 3, NPARAMS)	// programs, parameters 
{
  size = 256; hpos = 0;
	hbuf = new float[size];
  fbuf1 = fbuf2 = 0.0f;
  twopi = 6.2831853f;

  setNumInputs(2);
	setNumOutputs(2);
	setUniqueID("mdaLeslie");  // identify here
	DECLARE_LVZ_DEPRECATED(canMono) ();
	canProcessReplacing();
	suspend();

  programs = new mdaLeslieProgram[numPrograms];
  if(programs)
  {
    programs[1].param[0] = 0.5f;
    programs[1].param[4] = 0.75f;
    programs[1].param[5] = 0.57f;
    strcpy(programs[1].name,"Slow");
    programs[2].param[0] = 1.0f;
    programs[2].param[4] = 0.60f;
    programs[2].param[5] = 0.70f;
    strcpy(programs[2].name,"Fast");
    setProgram(0);
  }

  chp = dchp = clp = dclp = shp = dshp = slp = dslp = 0.0f;

  lspd = 0.0f; hspd = 0.0f;
  lphi = 0.0f; hphi = 1.6f;
  setParameter(0, 0.66f);
}

bool  mdaLeslie::getProductString(char* text) { strcpy(text, "MDA Leslie"); return true; }
bool  mdaLeslie::getVendorString(char* text)  { strcpy(text, "mda"); return true; }
bool  mdaLeslie::getEffectName(char* name)    { strcpy(name, "Leslie"); return true; }

void mdaLeslie::setParameter(int32_t index, float value)
{
  float * param = programs[curProgram].param;

  switch(index)
  {
    case 0: param[0] = value; break;
    case 1: param[6] = value; break;
    case 2: param[8] = value; break;
    case 3: param[3] = value; break;
    case 4: param[4] = value; break;
    case 5: param[5] = value; break;
    case 6: param[2] = value; break;
    case 7: param[1] = value; break;
    case 8: param[7] = value; break;
  }
  update();
}

mdaLeslie::~mdaLeslie()
{
  if(hbuf) delete [] hbuf;
  if(programs) delete [] programs; 
}

void mdaLeslie::setProgram(int32_t program) 
{
	curProgram = program;
    update();
}

void mdaLeslie::update()
{
  float ifs = 1.0f / getSampleRate();
  float * param = programs[curProgram].param;
  float spd = twopi * ifs * 2.0f * param[7];

  //calcs here!
  filo = 1.f - (float)pow(10.0f, param[2] * (2.27f - 0.54f * param[2]) - 1.92f);

  if(param[0]<0.50f)
  {
     if(param[0]<0.1f) //stop
     {
       lset = 0.00f; hset = 0.00f;
       lmom = 0.12f; hmom = 0.10f;
     }
     else //low speed
     {
       lset = 0.49f; hset = 0.66f;
       lmom = 0.27f; hmom = 0.18f;
     }
  }
  else //high speed
  {
    lset = 5.31f; hset = 6.40f;
    lmom = 0.14f; hmom = 0.09f;
  }
  hmom = (float)pow(10.0f, -ifs / hmom);
  lmom = (float)pow(10.0f, -ifs / lmom);
  hset *= spd;
  lset *= spd;

  gain = 0.4f * (float)pow(10.0f, 2.0f * param[1] - 1.0f);
  lwid = param[6] * param[6];
  llev = gain * 0.9f * param[8] * param[8];
  hwid = param[3] * param[3];
  hdep = param[4] * param[4] * getSampleRate() / 760.0f;
  hlev = gain * 0.9f * param[5] * param[5];
}

void mdaLeslie::suspend()
{
	memset(hbuf, 0, size * sizeof(float));
}

void mdaLeslie::setProgramName(char *name)
{
	strcpy(programs[curProgram].name, name);
}

void mdaLeslie::getProgramName(char *name)
{
	strcpy(name, programs[curProgram].name);
}

bool mdaLeslie::getProgramNameIndexed (int32_t category, int32_t index, char* name)
{
	if ((unsigned int)index < NPROGS) 
	{
	    strcpy(name, programs[index].name);
	    return true;
	}
	return false;
}

float mdaLeslie::getParameter(int32_t index)
{
  float v=0;
  float * param = programs[curProgram].param;

  switch(index)
  {
    case 0: v = param[0]; break;
    case 1: v = param[6]; break;
    case 2: v = param[8]; break;
    case 3: v = param[3]; break;
    case 4: v = param[4]; break;
    case 5: v = param[5]; break;
    case 6: v = param[2]; break;
    case 7: v = param[1]; break;
    case 8: v = param[7]; break;
  }
  return v;
}

void mdaLeslie::getParameterName(int32_t index, char *label)
{
	switch(index)
  {
    case 0: strcpy(label, "Mode"); break;
    case 1: strcpy(label, "Lo Width"); break;
    case 2: strcpy(label, "Lo Throb"); break;
    case 3: strcpy(label, "Hi Width"); break;
    case 4: strcpy(label, "Hi Depth"); break;
    case 5: strcpy(label, "Hi Throb"); break;
    case 6: strcpy(label, "X-Over"); break;
    case 7: strcpy(label, "Output"); break;
    case 8: strcpy(label, "Speed"); break;
  }
}

#include <stdio.h>
static void int2strng(int32_t value, char *string) { sprintf(string, "%d", value); }

void mdaLeslie::getParameterDisplay(int32_t index, char *text)
{
  float * param = programs[curProgram].param;
	switch(index)
  {
    case 0:
      if(param[0]<0.5f) 
      {
        if(param[0] < 0.1f) strcpy(text, "STOP"); 
                      else strcpy(text, "SLOW");
      }               else strcpy(text, "FAST");  break;
    case 1: int2strng((int32_t)(100 * param[6]), text); break;
    case 2: int2strng((int32_t)(100 * param[8]), text); break;
    case 3: int2strng((int32_t)(100 * param[3]), text); break;
    case 4: int2strng((int32_t)(100 * param[4]), text); break;
    case 5: int2strng((int32_t)(100 * param[5]), text); break;
    case 6: int2strng((int32_t)(10*int((float)pow(10.0f,1.179f + param[2]))), text); break; 
    case 7: int2strng((int32_t)(40 * param[1] - 20), text); break;
    case 8: int2strng((int32_t)(200 * param[7]), text); break;
  }
}

void mdaLeslie::getParameterLabel(int32_t index, char *label)
{
	switch(index)
  {
    case  0: strcpy(label, ""); break;
    case  6: strcpy(label, "Hz"); break;
    case  7: strcpy(label, "dB"); break;
    default: strcpy(label, "%"); break;
  }
}

//--------------------------------------------------------------------------------

void mdaLeslie::process(float **inputs, float **outputs, int32_t sampleFrames)
{
	float *in1 = inputs[0];
	float *in2 = inputs[1];
	float *out1 = outputs[0];
	float *out2 = outputs[1];
	float a, c, d, g=gain, h, l;
  float fo=filo, fb1=fbuf1, fb2=fbuf2;
  float hl=hlev, hs=hspd, ht, hm=hmom, hp=hphi, hw=hwid, hd=hdep;
  float ll=llev, ls=lspd, lt, lm=lmom, lp=lphi, lw=lwid;
  float hint, k0=0.03125f, k1=32.f;
  int32_t  hdd, hdd2, k=0, hps=hpos;

  ht=hset*(1.f-hm);
  lt=lset*(1.f-lm);

  chp = (float)cos(hp); chp *= chp * chp;
  clp = (float)cos(lp);
  shp = (float)sin(hp);
  slp = (float)sin(lp);

  --in1;
	--in2;
	--out1;
	--out2;
	while(--sampleFrames >= 0)
	{
		a = *++in1 + *++in2;
    c = out1[1];
		d = out2[1]; //see processReplacing() for comments

    if(k) k--; else
    {
      ls = (lm * ls) + lt;
      hs = (hm * hs) + ht;
      lp += k1 * ls;
      hp += k1 * hs;

      dchp = (float)cos(hp + k1*hs);
      dchp = k0 * (dchp * dchp * dchp - chp);
      dclp = k0 * ((float)cos(lp + k1*ls) - clp);
      dshp = k0 * ((float)sin(hp + k1*hs) - shp);
      dslp = k0 * ((float)sin(lp + k1*ls) - slp);

      k=(int32_t)k1;
    }

    fb1 = fo * (fb1 - a) + a;
    fb2 = fo * (fb2 - fb1) + fb1;
    h = (g - hl * chp) * (a - fb2);
    l = (g - ll * clp) * fb2;

    if(hps>0) hps--; else hps=200;
    hint = hps + hd * (1.0f + chp);
    hdd = (int)hint;
    hint = hint - hdd;
    hdd2 = hdd + 1;
    if(hdd>199) { if(hdd>200) hdd -= 201; hdd2 -= 201; }

    *(hbuf + hps) = h;
    a = *(hbuf + hdd);
    h += a + hint * ( *(hbuf + hdd2) - a);

    c += l + h;
    d += l + h;
    h *= hw * shp;
    l *= lw * slp;
    d += l - h;
    c += h - l;

    *++out1 = c;
		*++out2 = d;

    chp += dchp;
    clp += dclp;
    shp += dshp;
    slp += dslp;
	}
  lspd = ls;
  hspd = hs;
  hpos = hps;
  lphi = (float)fmod(lp+(k1-k)*ls,twopi);
  hphi = (float)fmod(hp+(k1-k)*hs,twopi);
  if(fabs(fb1)>1.0e-10) fbuf1=fb1; else fbuf1=0.f;
  if(fabs(fb2)>1.0e-10) fbuf2=fb2; else fbuf2=0.f;
}

void mdaLeslie::processReplacing(float **inputs, float **outputs, int32_t sampleFrames)
{
	float *in1 = inputs[0];
	float *in2 = inputs[1];
	float *out1 = outputs[0];
	float *out2 = outputs[1];
	float a, c, d, g=gain, h, l;
  float fo=filo, fb1=fbuf1, fb2=fbuf2;
  float hl=hlev, hs=hspd, ht, hm=hmom, hp=hphi, hw=hwid, hd=hdep;
  float ll=llev, ls=lspd, lt, lm=lmom, lp=lphi, lw=lwid;
  float hint, k0=0.03125f, k1=32.f; //k0 = 1/k1
  int32_t  hdd, hdd2, k=0, hps=hpos;

  ht=hset*(1.f-hm); //target speeds
  lt=lset*(1.f-lm);

  chp = (float)cos(hp); chp *= chp * chp; //set LFO values
  clp = (float)cos(lp);
  shp = (float)sin(hp);
  slp = (float)sin(lp);

	--in1;
	--in2;
	--out1;
	--out2;
	while(--sampleFrames >= 0)
	{
		a = *++in1 + *++in2; //mono input

    if(k) k--; else //linear piecewise approx to LFO waveforms
    {
      ls = (lm * ls) + lt; //tend to required speed
      hs = (hm * hs) + ht;
      lp += k1 * ls;
      hp += k1 * hs;

      dchp = (float)cos(hp + k1*hs);
      dchp = k0 * (dchp * dchp * dchp - chp); //sin^3 level mod
      dclp = k0 * ((float)cos(lp + k1*ls) - clp);
      dshp = k0 * ((float)sin(hp + k1*hs) - shp);
      dslp = k0 * ((float)sin(lp + k1*ls) - slp);

      k=(int32_t)k1;
    }

    fb1 = fo * (fb1 - a) + a; //crossover
    fb2 = fo * (fb2 - fb1) + fb1;
    h = (g - hl * chp) * (a - fb2); //volume
    l = (g - ll * clp) * fb2;

    if(hps>0) hps--; else hps=200;  //delay input pos
    hint = hps + hd * (1.0f + chp); //delay output pos
    hdd = (int)hint;
    hint = hint - hdd; //linear intrpolation
    hdd2 = hdd + 1;
    if(hdd>199) { if(hdd>200) hdd -= 201; hdd2 -= 201; }

    *(hbuf + hps) = h; //delay input
    a = *(hbuf + hdd);
    h += a + hint * ( *(hbuf + hdd2) - a); //delay output

    c = l + h;
    d = l + h;
    h *= hw * shp;
    l *= lw * slp;
    d += l - h;
    c += h - l;

    *++out1 = c; //output
		*++out2 = d;

    chp += dchp;
    clp += dclp;
    shp += dshp;
    slp += dslp;
  }
  lspd = ls;
  hspd = hs;
  hpos = hps;
  lphi = (float)fmod(lp+(k1-k)*ls,twopi);
  hphi = (float)fmod(hp+(k1-k)*hs,twopi);
  if(fabs(fb1)>1.0e-10) fbuf1=fb1; else fbuf1=0.0f; //catch denormals
  if(fabs(fb2)>1.0e-10) fbuf2=fb2; else fbuf2=0.0f;
}
