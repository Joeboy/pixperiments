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

#include "mdaMultiBand.h"

#include <math.h>
#include <float.h>

AudioEffect *createEffectInstance(audioMasterCallback audioMaster)
{
  return new mdaMultiBand(audioMaster);
}

mdaMultiBand::mdaMultiBand(audioMasterCallback audioMaster)	: AudioEffectX(audioMaster, 1, 13)	// programs, parameters
{
  //inits here!
  fParam1 = (float)1.00; //Listen: L/M/H/out
  fParam2 = (float)0.103; //xover1
  fParam3 = (float)0.878; //xover2
  fParam4 = (float)0.54; //L drive    (1)
  fParam5 = (float)0.00; //M drive
  fParam6 = (float)0.60; //H drive
  fParam7 = (float)0.45; //L trim     (2)
  fParam8 = (float)0.50; //M trim
  fParam9 = (float)0.50; //H trim
  fParam10 = (float)0.22; //attack    (3)
  fParam11 = (float)0.602; //release   (4)
  fParam12 = (float)0.55; //width
  fParam13 = (float)0.00; //MS swap
/*  fParam1 = (float)1.00; //Listen: L/M/H/out
  fParam2 = (float)0.50; //xover1
  fParam3 = (float)0.50; //xover2
  fParam4 = (float)0.45; //L drive    (1)
  fParam5 = (float)0.45; //M drive
  fParam6 = (float)0.45; //H drive
  fParam7 = (float)0.50; //L trim     (2)
  fParam8 = (float)0.50; //M trim
  fParam9 = (float)0.50; //H trim
  fParam10 = (float)0.22; //attack    (3)
  fParam11 = (float)0.60; //release   (4)
  fParam12 = (float)0.50; //width
  fParam13 = (float)0.40; //MS swap*/

  setNumInputs(2);
	setNumOutputs(2);
	setUniqueID("mdaMultiBand");    // identify here
	DECLARE_LVZ_DEPRECATED(canMono) ();
	canProcessReplacing();
	strcpy(programName, "Multi-Band Compressor");

  //calcs here!
  gain1 = 1.0;
  driv1 = (float)pow(10.0,(2.5 * fParam4) - 1.0);
  trim1 = (float)(0.5 + (4.0 - 2.0 * fParam10) * (fParam4 * fParam4 * fParam4));
  trim1 = (float)(trim1 * pow(10.0, 2.0 * fParam7 - 1.0));
  att1 = (float)pow(10.0, -0.05 -(2.5 * fParam10));
  rel1 = (float)pow(10.0, -2.0 - (3.5 * fParam11));

  gain2 = 1.0;
  driv2 = (float)pow(10.0,(2.5 * fParam5) - 1.0);
  trim2 = (float)(0.5 + (4.0 - 2.0 * fParam10) * (fParam5 * fParam5 * fParam5));
  trim2 = (float)(trim2 * pow(10.0, 2.0 * fParam8 - 1.0));
  att2 = (float)pow(10.0, -0.05 -(2.0 * fParam10));
  rel2 = (float)pow(10.0, -2.0 - (3.0 * fParam11));

  gain3 = 1.0;
  driv3 = (float)pow(10.0,(2.5 * fParam6) - 1.0);
  trim3 = (float)(0.5 + (4.0 - 2.0 * fParam10) * (fParam6 * fParam6 * fParam6));
  trim3 = (float)(trim3 * pow(10.0, 2.0 * fParam9 - 1.0));
  att3 = (float)pow(10.0, -0.05 -(1.5 * fParam10));
  rel3 = (float)pow(10.0, -2.0 - (2.5 * fParam11));

  switch(int(fParam1*3.9))
  {
    case 0: trim2=0.0; trim3=0.0; slev=0.0; break;
    case 1: trim1=0.0; trim3=0.0; slev=0.0; break;
    case 2: trim1=0.0; trim2=0.0; slev=0.0; break;
    default: slev=fParam12; break;
  }
  fi1 = (float)pow(10.0,fParam2 - 1.70); fo1=(float)(1.0 - fi1);
  fi2 = (float)pow(10.0,fParam3 - 1.05); fo2=(float)(1.0 - fi2);
  fb1 = fb2 = fb3 = 0.0f;
  mswap = 0;
}

mdaMultiBand::~mdaMultiBand()
{

}

bool  mdaMultiBand::getProductString(char* text) { strcpy(text, "MDA MultiBand"); return true; }
bool  mdaMultiBand::getVendorString(char* text)  { strcpy(text, "mda"); return true; }
bool  mdaMultiBand::getEffectName(char* name)    { strcpy(name, "MultiBand"); return true; }

void mdaMultiBand::setProgramName(char *name)
{
	strcpy(programName, name);
}

void mdaMultiBand::getProgramName(char *name)
{
	strcpy(name, programName);
}

bool mdaMultiBand::getProgramNameIndexed (int32_t category, int32_t index, char* name)
{
	if (index == 0) 
	{
	    strcpy(name, programName);
	    return true;
	}
	return false;
}

void mdaMultiBand::setParameter(int32_t index, float value)
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
    case 9: fParam10 = value; break;
    case 10: fParam11 = value; break;
    case 11: fParam12 = value; break;
    case 12: fParam13 = value; break;
  }
  //calcs here
  driv1 = (float)pow(10.0,(2.5 * fParam4) - 1.0);
  trim1 = (float)(0.5 + (4.0 - 2.0 * fParam10) * (fParam4 * fParam4 * fParam4));
  trim1 = (float)(trim1 * pow(10.0, 2.0 * fParam7 - 1.0));
  att1 = (float)pow(10.0, -0.05 -(2.5 * fParam10));
  rel1 = (float)pow(10.0, -2.0 - (3.5 * fParam11));

  driv2 = (float)pow(10.0,(2.5 * fParam5) - 1.0);
  trim2 = (float)(0.5 + (4.0 - 2.0 * fParam10) * (fParam5 * fParam5 * fParam5));
  trim2 = (float)(trim2 * pow(10.0, 2.0 * fParam8 - 1.0));
  att2 = (float)pow(10.0, -0.05 -(2.0 * fParam10));
  rel2 = (float)pow(10.0, -2.0 - (3.0 * fParam11));

  driv3 = (float)pow(10.0,(2.5 * fParam6) - 1.0);
  trim3 = (float)(0.5 + (4.0 - 2.0 * fParam10) * (fParam6 * fParam6 * fParam6));
  trim3 = (float)(trim3 * pow(10.0, 2.0 * fParam9 - 1.0));
  att3 = (float)pow(10.0, -0.05 -(1.5 * fParam10));
  rel3 = (float)pow(10.0, -2.0 - (2.5 * fParam11));

  switch(int(fParam1*3.9))
  {
    case 0: trim2=0.0; trim3=0.0; slev=0.0; break;
    case 1: trim1=0.0; trim3=0.0; slev=0.0; break;
    case 2: trim1=0.0; trim2=0.0; slev=0.0; break;
    default: slev=fParam12; break;
  }
  fi1 = (float)pow(10.0,fParam2 - 1.70); fo1=(float)(1.0 - fi1);
  fi2 = (float)pow(10.0,fParam3 - 1.05); fo2=(float)(1.0 - fi2);

  if(fParam13>0.0) mswap=1; else mswap=0;
}

float mdaMultiBand::getParameter(int32_t index)
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
    case 9: v = fParam10; break;
    case 10: v = fParam11; break;
    case 11: v = fParam12; break;
    case 12: v = fParam13; break;
  }
  return v;
}

void mdaMultiBand::getParameterName(int32_t index, char *label)
{
	switch(index)
  {
    case 0: strcpy(label, "Listen"); break;
    case 1: strcpy(label, "L <> M"); break;
    case 2: strcpy(label, "M <> H"); break;
    case 3: strcpy(label, "L Comp"); break;
    case 4: strcpy(label, "M Comp"); break;
    case 5: strcpy(label, "H Comp"); break;
    case 6: strcpy(label, "L Out"); break;
    case 7: strcpy(label, "M Out"); break;
    case 8: strcpy(label, "H Out"); break;
    case 9: strcpy(label, "Attack"); break;
    case 10: strcpy(label, "Release"); break;
    case 11: strcpy(label, "Stereo"); break;
    case 12: strcpy(label, "Process"); break;
  }
}

#include <stdio.h>
static void int2strng(int32_t value, char *string) { sprintf(string, "%d", value); }

void mdaMultiBand::getParameterDisplay(int32_t index, char *text)
{
	switch(index)
  {
    case 0: switch(int(fParam1*3.9))
    { case 0: strcpy(text, "Low"); break;
    case 1: strcpy(text, "Mid"); break;
    case 2: strcpy(text, "High"); break;
     default: strcpy(text, "Output"); break; } break;
    case 1: int2strng((int32_t)(getSampleRate() * fi1 * (0.098 + 0.09*fi1 + 0.5*(float)pow(fi1,8.2f))), text); break;
    case 2: int2strng((int32_t)(getSampleRate() * fi2 * (0.015 + 0.15*fi2 + 0.9*(float)pow(fi2,8.2f))), text); break;
    case 3: int2strng((int32_t)(30.0 * fParam4), text); break;
    case 4: int2strng((int32_t)(30.0 * fParam5), text); break;
    case 5: int2strng((int32_t)(30.0 * fParam6), text); break;
    case 6: int2strng((int32_t)(40.0 * fParam7 - 20.0), text); break;
    case 7: int2strng((int32_t)(40.0 * fParam8 - 20.0), text); break;
    case 8: int2strng((int32_t)(40.0 * fParam9 - 20.0), text); break;
    case 9: int2strng((int32_t)(-301030.1 / (getSampleRate() * log10(1.0 - att2))),text); break;
    case 10: int2strng((int32_t)(-301.0301 / (getSampleRate() * log10(1.0 - rel2))),text); break;
    case 11: int2strng((int32_t)(200.0 * fParam12), text); break;
    case 12: if(mswap) strcpy(text, "S");
                  else strcpy(text, "M"); break;
  }
}

void mdaMultiBand::getParameterLabel(int32_t index, char *label)
{
	switch(index)
  {
    case 0: strcpy(label, ""); break;
    case 1: strcpy(label, "Hz"); break;
    case 2: strcpy(label, "Hz"); break;
    case 3: strcpy(label, "dB"); break;
    case 4: strcpy(label, "dB"); break;
    case 5: strcpy(label, "dB"); break;
    case 6: strcpy(label, "dB"); break;
    case 7: strcpy(label, "dB"); break;
    case 8: strcpy(label, "dB"); break;
    case 9: strcpy(label, "�s"); break; 
    case 10: strcpy(label, "ms"); break;
    case 11: strcpy(label, "% Width"); break;
    case 12: strcpy(label, ""); break;
  }
}

//--------------------------------------------------------------------------------
// process

void mdaMultiBand::process(float **inputs, float **outputs, int32_t sampleFrames)
{
	float *in1 = inputs[0];
	float *in2 = inputs[1];
	float *out1 = outputs[0];
	float *out2 = outputs[1];
	float a, b, c, d, l=fb3, m, h, s, sl=slev, tmp1, tmp2, tmp3;
  float f1i=fi1, f1o=fo1, f2i=fi2, f2o=fo2, b1=fb1, b2=fb2;
  float g1=gain1, /*d1=driv1,*/ t1=trim1, a1=att1, r1=1.f - rel1;
  float g2=gain2, d2=driv2, t2=trim2, a2=att2, r2=1.f - rel2;
  float g3=gain3, d3=driv3, t3=trim3, a3=att3, r3=1.f - rel3;
  int ms=mswap;

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

    b = (ms)? -b : b;

    s = (a - b) * sl; //keep stereo component for later
    a += b;
    b2 = (f2i * a) + (f2o * b2); //crossovers
    b1 = (f1i * b2) + (f1o * b1);
    l = (f1i * b1) + (f1o * l);
    m=b2-l; h=a-b2;

    tmp1 = (l>0)? l : -l;  //l
    g1 = (tmp1>g1)? g1+a1*(tmp1-g1) : g1*r1;
    //tmp1 = 1.f / (1.f + d1 * g1);

    tmp2 = (m>0)? m : -m;
    g2 = (tmp2>g2)? g2+a2*(tmp2-g2) : g2*r2;
    tmp2 = 1.f / (1.f + d2 * g2);

    tmp3 = (h>0)? h : -h;
    g3 = (tmp3>g3)? g3+a3*(tmp3-g3) : g3*r3;
    tmp3 = 1.f / (1.f + d3 * g3);

    a = (l*tmp3*t1) + (m*tmp2*t2) + (h*tmp3*t3);
		c += a + s; // output
		d += (ms)? s - a : a - s;

    *++out1 = c;
		*++out2 = d;
	}
  gain1=(g1<1.0e-10)? 0.f : g1;
  gain2=(g2<1.0e-10)? 0.f : g2;
  gain3=(g3<1.0e-10)? 0.f : g3;  // gain1=g1; gain2=g2; gain3=g3;
  if(fabs(b1)<1.0e-10) { fb1=0.f; fb2=0.f; fb3=0.f; }
                  else { fb1=b1;  fb2=b2;  fb3=l;   }
}

void mdaMultiBand::processReplacing(float **inputs, float **outputs, int32_t sampleFrames)
{
	float *in1 = inputs[0];
	float *in2 = inputs[1];
	float *out1 = outputs[0];
	float *out2 = outputs[1];
	float a, b, c, d, l=fb3, m, h, s, sl=slev, tmp1, tmp2, tmp3;
  float f1i=fi1, f1o=fo1, f2i=fi2, f2o=fo2, b1=fb1, b2=fb2;
  float g1=gain1, /*d1=driv1,*/ t1=trim1, a1=att1, r1=1.f - rel1;
  float g2=gain2, d2=driv2, t2=trim2, a2=att2, r2=1.f - rel2;
  float g3=gain3, d3=driv3, t3=trim3, a3=att3, r3=1.f - rel3;
  int ms=mswap;

	--in1;
	--in2;
	--out1;
	--out2;
	while(--sampleFrames >= 0)
	{
		a = *++in1;
		b = *++in2; //process from here...

    b = (ms)? -b : b;

    s = (a - b) * sl; //keep stereo component for later
    a += b;
    b2 = (f2i * a) + (f2o * b2); //crossovers
    b1 = (f1i * b2) + (f1o * b1);
    l = (f1i * b1) + (f1o * l);
    m=b2-l; h=a-b2;

    tmp1 = (l>0)? l : -l;  //l
    g1 = (tmp1>g1)? g1+a1*(tmp1-g1) : g1*r1;
    //tmp1 = 1.f / (1.f + d1 * g1);

    tmp2 = (m>0)? m : -m;
    g2 = (tmp2>g2)? g2+a2*(tmp2-g2) : g2*r2;
    tmp2 = 1.f / (1.f + d2 * g2);

    tmp3 = (h>0)? h : -h;
    g3 = (tmp3>g3)? g3+a3*(tmp3-g3) : g3*r3;
    tmp3 = 1.f / (1.f + d3 * g3);

    a = (l*tmp3*t1) + (m*tmp2*t2) + (h*tmp3*t3);
		c = a + s; // output
    d = (ms)? s - a : a - s;

    *++out1 = c;
		*++out2 = d;
	}
  gain1=(g1<1.0e-10)? 0.f : g1;
  gain2=(g2<1.0e-10)? 0.f : g2;
  gain3=(g3<1.0e-10)? 0.f : g3;  // gain1=g1; gain2=g2; gain3=g3;
  if(fabs(b1)<1.0e-10) { fb1=0.f; fb2=0.f; fb3=0.f; }
                  else { fb1=b1;  fb2=b2;  fb3=l;   }
}

    //g = (float)(1.0 / (1.0 + d1 * fabs(l)) ); //VCAs
    //if(g1>g) { g1=g1-a1*(g1-g); } else { g1=g1+r1*(g-g1); }
