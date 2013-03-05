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

#include "mdaBeatBox.h"

#include <math.h>
#include <stdlib.h>

AudioEffect *createEffectInstance(audioMasterCallback audioMaster)
{
  return new mdaBeatBox(audioMaster);
}

mdaBeatBox::mdaBeatBox(audioMasterCallback audioMaster)	: AudioEffectX(audioMaster, 1, 12)	// programs, parameters
{
  fParam1 = 0.30f; //hat thresh
  fParam2 = 0.45f; //hat rate
  fParam3 = 0.50f; //hat mix
   fParam4 = 0.46f; //kick thresh
   fParam5 = 0.15f; //kick key
   fParam6 = 0.50f; //kick mix
  fParam7 = 0.50f; //snare thresh
  fParam8 = 0.70f; //snare key
  fParam9 = 0.50f; //snare mix
   fParam10 = 0.00f; //dynamics
   fParam11 = 0.00f; //record
   fParam12 = 0.00f; //thru mix

  hbuflen = 20000;
  kbuflen = 20000;
  sbuflen = 60000;
  if(getSampleRate()>49000) { hbuflen*=2; kbuflen*=2; sbuflen*=2; }

  hbufpos = 0;
  kbufpos = 0;
  sbufpos = 0;
  hfil = 0;
  sb1 = 0;
  sb2 = 0;
  ksb1 = 0;
  ksb2 = 0;
  wwx = 0;

  hbuf = new float[hbuflen];
	sbuf = new float[sbuflen]; sbuf2 = new float[sbuflen];
	kbuf = new float[kbuflen];

  setNumInputs(2);		    // stereo in
	setNumOutputs(2);		    // stereo out
	setUniqueID("mdaBBox");    // identify
	DECLARE_LVZ_DEPRECATED(canMono) ();
	canProcessReplacing();	// supports both accumulating and replacing output
	strcpy(programName, "BeatBox - Drum Replacer");	// default program name
  synth();

  //calcs here
  hthr = (float)pow(10.f, 2.f * fParam1 - 2.f);
  hdel = (int32_t)((0.04 + 0.20 * fParam2) * getSampleRate());
  sthr = (float)(40.0 * pow(10.f, 2.f * fParam7 - 2.f));
  sdel = (int32_t)(0.12 * getSampleRate());
  kthr = (float)(220.0 * pow(10.f, 2.f * fParam4 - 2.f));
  kdel = (int32_t)(0.10 * getSampleRate());

  hlev = (float)(0.0001f + fParam3 * fParam3 * 4.f);
  klev = (float)(0.0001f + fParam6 * fParam6 * 4.f);
  slev = (float)(0.0001f + fParam9 * fParam9 * 4.f);

  kww = (float)pow(10.0,-3.0 + 2.2 * fParam5);
  ksf1 = (float)cos(3.1415927 * kww);     //p
  ksf2 = (float)sin(3.1415927 * kww);     //q

  ww = (float)pow(10.0,-3.0 + 2.2 * fParam8);
  sf1 = (float)cos(3.1415927 * ww);     //p
  sf2 = (float)sin(3.1415927 * ww);     //q
  sf3 = 0.991f; //r
  sfx = 0; ksfx = 0;
  rec=0; recx=0; recpos=0;

  mix = fParam12;
  dyna = (float)pow(10.0,-1000.0/getSampleRate());
  dynr = (float)pow(10.0,-6.0/getSampleRate());
  dyne = 0.f; dynm = fParam10;
}

bool  mdaBeatBox::getProductString(char* text) { strcpy(text, "MDA BeatBox"); return true; }
bool  mdaBeatBox::getVendorString(char* text)  { strcpy(text, "mda"); return true; }
bool  mdaBeatBox::getEffectName(char* name)    { strcpy(name, "BeatBox"); return true; }

void mdaBeatBox::setProgramName(char *name)
{
	strcpy(programName, name);
}

void mdaBeatBox::getProgramName(char *name)
{
	strcpy(name, programName);
}

bool mdaBeatBox::getProgramNameIndexed (int32_t category, int32_t index, char* name)
{
	if (index == 0) 
	{
	    strcpy(name, programName);
	    return true;
	}
	return false;
}

void mdaBeatBox::setParameter(int32_t index, float value)
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
    case 8: fParam9 = value; break;
    case 9: fParam10= value; break;
    case 10:fParam11= value; break;
    case 11:fParam12= value; break;
  }
  //calcs here
  hthr = (float)pow(10.f, 2.f * fParam1 - 2.f);
  hdel = (int32_t)((0.04 + 0.20 * fParam2) * getSampleRate());
  sthr = (float)(40.0 * pow(10.f, 2.f * fParam7 - 2.f));
  kthr = (float)(220.0 * pow(10.f, 2.f * fParam4 - 2.f));

  hlev = (float)(0.0001f + fParam3 * fParam3 * 4.f);
  klev = (float)(0.0001f + fParam6 * fParam6 * 4.f);
  slev = (float)(0.0001f + fParam9 * fParam9 * 4.f);

  wwx=ww;
  ww = (float)pow(10.0,-3.0 + 2.2 * fParam8);
  sf1 = (float)cos(3.1415927 * ww);     //p
  sf2 = (float)sin(3.1415927 * ww);     //q
  //sfx = 0; ksfx = 0;

  kwwx=kww;
  kww = (float)pow(10.0,-3.0 + 2.2 * fParam5);
  ksf1 = (float)cos(3.1415927 * kww);     //p
  ksf2 = (float)sin(3.1415927 * kww);     //q

  if(wwx != ww) sfx = (int32_t)(2 * getSampleRate()); 
  if(kwwx != kww) ksfx = (int32_t)(2 * getSampleRate());

  rec = (int32_t)(4.9 * fParam11); 
  if ((rec!=recx) && (recpos>0)) //finish sample
  {
    switch(rec)
    {
       case 2: while(recpos<hbuflen) *(hbuf + recpos++) = 0.f; break;
       case 3: while(recpos<kbuflen) *(kbuf + recpos++) = 0.f; break;
       case 4: while(recpos<sbuflen) { *(sbuf  + recpos) = 0.f;
                           *(sbuf2 + recpos) = 0.f; recpos++; } break;
    }
  }
  recpos=0; recx=rec;
  mix = fParam12;
  dynm = fParam10;
}

mdaBeatBox::~mdaBeatBox()
{
  if(hbuf) delete [] hbuf;
  if(kbuf) delete [] kbuf;
  if(sbuf) delete [] sbuf;
  if(sbuf2) delete [] sbuf2;
}

void mdaBeatBox::suspend()
{
}

void mdaBeatBox::synth()
{
	int32_t t; 
  float e=0.00012f, de, o, o1=0.f, o2=0.f, p=0.2f, dp;

  memset(hbuf, 0, hbuflen * sizeof(float)); //generate hi-hat
  de = (float)pow(10.0,-36.0/getSampleRate());
  for(t=0;t<5000;t++)
  {
    o = (float)((rand() % 2000) - 1000);
    *(hbuf + t) =  e * ( 2.f * o1 - o2 - o);
    e *= de; o2=o1; o1=o;
  }

  memset(kbuf, 0, kbuflen * sizeof(float)); //generate kick
  de = (float)pow(10.0,-3.8/getSampleRate());
  e=0.5f; dp = 1588.f / getSampleRate();
  for(t=0;t<14000;t++)
  {
    *(kbuf + t) =  e * (float)sin(p);
    e *= de; p = (float)fmod(p + dp * e ,6.2831853f);
  }

  memset(sbuf, 0, sbuflen * sizeof(float)); //generate snare
  de = (float)pow(10.0,-15.0/getSampleRate());
  e=0.38f; //dp = 1103.f / getSampleRate();
  for(t=0;t<7000;t++)
  {
    o = (0.3f * o) + (float)((rand() % 2000) - 1000);
    *(sbuf + t) =  (float)(e * (sin(p) + 0.0004 * o));
    *(sbuf2 + t) = *(sbuf + t);
    e *= de; p = (float)fmod(p + 0.025,6.2831853);
  }
}

float mdaBeatBox::getParameter(int32_t index)
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
    case 8: v = fParam9; break;
    case 9: v =fParam10; break;
    case 10:v =fParam11; break;
    case 11:v =fParam12; break;
  }
  return v;
}

void mdaBeatBox::getParameterName(int32_t index, char *label)
{
	switch(index)
  {
    case 0:  strcpy(label, "Hat Thr"); break;
    case 1:  strcpy(label, "Hat Rate"); break;
    case 2:  strcpy(label, "Hat Mix"); break;
    case 3:  strcpy(label, "Kik Thr"); break;
    case 4:  strcpy(label, "Kik Trig"); break;
    case 5:  strcpy(label, "Kik Mix"); break;
    case 6:  strcpy(label, "Snr Thr"); break;
    case 7:  strcpy(label, "Snr Trig"); break;
    case 8:  strcpy(label, "Snr Mix"); break;
    case 9:  strcpy(label, "Dynamics"); break;
    case 10: strcpy(label, "Record"); break;
    case 11: strcpy(label, "Thru Mix"); break;
  }
}

#include <stdio.h>
static void int2strng(int32_t value, char *string) { sprintf(string, "%d", value); }
static void float2strng(float value, char *string) { sprintf(string, "%.2f", value); }

void mdaBeatBox::getParameterDisplay(int32_t index, char *text)
{
	switch(index)
  {
    case 0: float2strng((float)(40.0*fParam1 - 40.0),text); break;
    case 1: int2strng((int32_t)(1000.f * hdel / getSampleRate()),text); break;
    case 2: int2strng((int32_t)(20.f * log10(hlev)),text); break;
    case 3: float2strng((float)(40.0*fParam4 - 40.0),text); break;
    case 4: int2strng((int32_t)(0.5 * kww * getSampleRate()), text); break;
    case 5: int2strng((int32_t)(20.f * log10(klev)),text); break;
    case 6: float2strng((float)(40.0*fParam7 - 40.0),text); break;
    case 7: int2strng((int32_t)(0.5 * ww * getSampleRate()), text); break;
    case 8: int2strng((int32_t)(20.f * log10(slev)),text); break;
    case 9: int2strng((int32_t)(100.f * fParam10),text); break; 
    case 11: int2strng((int32_t)(20.f * log10(fParam12)),text); break;

    case 10: switch(rec)
            { case 0: strcpy(text, "-"); break;
              case 1: strcpy(text, "MONITOR"); break;
              case 2: strcpy(text, "-> HAT"); break;
              case 3: strcpy(text, "-> KIK"); break;
              case 4: strcpy(text, "-> SNR"); break; } break;
  }
}

void mdaBeatBox::getParameterLabel(int32_t index, char *label)
{
	switch(index)
  {
    case 0: strcpy(label, "dB"); break;
    case 1: strcpy(label, "ms"); break;
    case 2: strcpy(label, "dB"); break;
    case 3: strcpy(label, "dB"); break;
    case 4: strcpy(label, "Hz"); break;
    case 5: strcpy(label, "dB"); break;
    case 6: strcpy(label, "dB"); break;
    case 7: strcpy(label, "Hz"); break;
    case 8: strcpy(label, "dB"); break;
    case 9: strcpy(label, "%"); break;
    case 10:strcpy(label, ""); break;
    case 11:strcpy(label, "dB"); break;
  }
}

//--------------------------------------------------------------------------------
// process

void mdaBeatBox::process(float **inputs, float **outputs, int32_t sampleFrames)
{
	float *in1 = inputs[0];
	float *in2 = inputs[1];
	float *out1 = outputs[0];
	float *out2 = outputs[1];
  float a, b, c, d, e, o, hf=hfil, ht=hthr, mx3=0.f, mx1=mix;
  int32_t hp=hbufpos, hl=hbuflen-2, hd=hdel;
  float kt=kthr;
  int32_t kp=kbufpos, kl=kbuflen-2, kd=kdel;  
  float st=sthr, s, f1=sb1, f2=sb2, b1=sf1, b2=sf2, b3=sf3;
  float k, kf1=ksb1, kf2=ksb2, kb1=ksf1, kb2=ksf2;
  float hlv=hlev, klv=klev, slv=slev;
  int32_t sp=sbufpos, sl=sbuflen-2, sd=sdel;  
  float ya=dyna, yr=dynr, ye=dyne, ym=dynm, mx4;

  if(sfx>0) { mx3=0.08f; slv=0.f; klv=0.f; hlv=0.f; mx1=0.f; sfx-=sampleFrames;} //key listen (snare)
  if(ksfx>0) { mx3=0.03f; slv=0.f; klv=0.f; hlv=0.f; mx1=0.f; ksfx-=sampleFrames;
               b1=ksf1; b2=ksf2; } //key listen (kick)

  --in1;
	--in2;
	--out1;
	--out2;

  if(rec==0)
  {
	  while(--sampleFrames >= 0)
	  {
      a = *++in1; //input
      b = *++in2;
      e = a + b;

      ye = (e<ye)? ye * yr : e - ya * (e - ye); //dynamics envelope

      hf = e - hf; //high
      if((hp>hd) && (hf>ht)) hp=0; else { hp++; if(hp>hl)hp=hl; }
      o = hlv * *(hbuf + hp);

      k = e + (kf1 * kb1) - (kf2 * kb2);
      kf2 = b3 * ((kf1 * kb2) + (kf2 * kb1));
      kf1 = b3 * k;
      if((kp>kd) && (k>kt)) kp=0; else { kp++; if(kp>kl)kp=kl; }
      o += klv * *(kbuf + kp);

      s = hf + (0.3f * e) + (f1 * b1) - (f2 * b2);
      f2 = b3 * ((f1 * b2) + (f2 * b1));
      f1 = b3 * s;

      if((sp>sd) && (s>st)) sp=0; else { sp++; if(sp>sl)sp=sl; }

      mx4 = 1.f + ym * (ye + ye - 1.f); //dynamics

      c = out1[1] + mx1*a + mx3*s + mx4*(o + slv * *(sbuf  + sp));
		  d = out2[1] + mx1*b + mx3*s + mx4*(o + slv * *(sbuf2 + sp));
      *++out1 = c;
		  *++out2 = d;

      hf=e;
    }
  }
  else //record
  {
    while(--sampleFrames >= 0)
	  {

      a = *++in1;
      b = *++in2;
      e = 0.5f * (a + b);

      if((recpos==0) && (fabs(e) < 0.004)) e=0.f;
      else
      {
       	switch(rec)
        {
           case 1: break; //echo
           case 2: if(recpos<hl) *(hbuf + recpos++) = e; else e=0.f; break;
           case 3: if(recpos<kl) *(kbuf + recpos++) = e; else e=0.f; break;
           case 4: if(recpos<sl)
                   { *(sbuf+recpos)=a; *(sbuf2+recpos)=b; recpos++; }
                   else e=0.f; break;
        }
      }
      c = out1[1] + e;
      d = out2[1] + e;
      *++out1 = c;
	    *++out2 = d;
    }
  }
  hfil=hf; hbufpos=hp;
  sbufpos=sp; sb1 = f1; sb2 = f2;
  kbufpos=kp; ksb1 = f1; ksb2 = f2;
  dyne=ye;
}

void mdaBeatBox::processReplacing(float **inputs, float **outputs, int32_t sampleFrames)
{
	float *in1 = inputs[0];
	float *in2 = inputs[1];
	float *out1 = outputs[0];
	float *out2 = outputs[1];
  float a, b, e, o, hf=hfil, ht=hthr, mx3=0.f, mx1=mix;
  int32_t hp=hbufpos, hl=hbuflen-2, hd=hdel;
  float kt=kthr;
  int32_t kp=kbufpos, kl=kbuflen-2, kd=kdel;  
  float st=sthr, s, f1=sb1, f2=sb2, b1=sf1, b2=sf2, b3=sf3;
  float k, kf1=ksb1, kf2=ksb2, kb1=ksf1, kb2=ksf2;
  float hlv=hlev, klv=klev, slv=slev;
  int32_t sp=sbufpos, sl=sbuflen-2, sd=sdel;  
  float ya=dyna, yr=dynr, ye=dyne, ym=dynm, mx4;

  if(sfx>0) { mx3=0.08f; slv=0.f; klv=0.f; hlv=0.f; mx1=0.f; sfx-=sampleFrames;} //key listen (snare)
  if(ksfx>0) { mx3=0.03f; slv=0.f; klv=0.f; hlv=0.f; mx1=0.f; ksfx-=sampleFrames;
               b1=ksf1; b2=ksf2; } //key listen (kick)

  --in1;
	--in2;
	--out1;
	--out2;

  if(rec==0)
  {
    while(--sampleFrames >= 0)
	  {
      a = *++in1;
      b = *++in2;
      e = a + b;

      ye = (e<ye)? ye * yr : e - ya * (e - ye); //dynamics envelope

      hf = e - hf; //high filter
      if((hp>hd) && (hf>ht)) hp=0; else { hp++; if(hp>hl)hp=hl; }
      o = hlv * *(hbuf + hp); //hat

      k = e + (kf1 * kb1) - (kf2 * kb2); //low filter
      kf2 = b3 * ((kf1 * kb2) + (kf2 * kb1));
      kf1 = b3 * k;
      if((kp>kd) && (k>kt)) kp=0; else { kp++; if(kp>kl)kp=kl; }
      o += klv * *(kbuf + kp); //kick

      s = hf + (0.3f * e) + (f1 * b1) - (f2 * b2); //mid filter
      f2 = b3 * ((f1 * b2) + (f2 * b1));
      f1 = b3 * s;

      if((sp>sd) && (s>st)) sp=0; else { sp++; if(sp>sl)sp=sl; }

      mx4 = 1.f + ym * (ye + ye - 1.f); //dynamics

      *++out1 = mx1*a + mx3*s + mx4*(o + slv * *(sbuf  + sp));
		  *++out2 = mx1*a + mx3*s + mx4*(o + slv * *(sbuf2 + sp));

      hf=e;
    }
  }
  else //record
  {
    while(--sampleFrames >= 0)
	  {
      a = *++in1;
      b = *++in2;
      e = 0.5f * (a + b);

      if((recpos==0) && (fabs(e) < 0.004)) e=0.f;
      else
      {
       	switch(rec)
        {
           case 1: break; //echo
           case 2: if(recpos<hl) *(hbuf + recpos++) = e; else e=0.f; break;
           case 3: if(recpos<kl) *(kbuf + recpos++) = e; else e=0.f; break;
           case 4: if(recpos<sl)
                   { *(sbuf+recpos)=a; *(sbuf2+recpos)=b; recpos++; }
                   else e=0.f; break;
        }
      }
      *++out1 = e;
  	  *++out2 = e;
    }
  }
  hfil=hf; hbufpos=hp;
  sbufpos=sp; sb1 = f1; sb2 = f2;
  kbufpos=kp; ksb1 = kf1; ksb2 = kf2;
  dyne=ye;
}
