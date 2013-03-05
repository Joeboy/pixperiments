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

#include "mdaBandisto.h"

#include <stdio.h>
#include <float.h>
#include <math.h>

AudioEffect *createEffectInstance(audioMasterCallback audioMaster)
{
  return new mdaBandisto(audioMaster);
}

mdaBandisto::mdaBandisto(audioMasterCallback audioMaster)	: AudioEffectX(audioMaster, 1, 10)	// programs, parameters
{
  //inits here!
  fParam1 = (float)1.00; //Listen: L/M/H/out
  fParam2 = (float)0.40; //xover1
  fParam3 = (float)0.50; //xover2
  fParam4 = (float)0.50; //L drive    (1)
  fParam5 = (float)0.50; //M drive
  fParam6 = (float)0.50; //H drive
  fParam7 = (float)0.50; //L trim     (2)
  fParam8 = (float)0.50; //M trim
  fParam9 = (float)0.50; //H trim
  fParam10 = (float)0.0; //unipolar/bipolar

  setNumInputs(2);
	setNumOutputs(2);
	setUniqueID("mdaBand");    // identify here
	DECLARE_LVZ_DEPRECATED(canMono) ();
	canProcessReplacing();
	strcpy(programName, "Multi-Band Distortion");

  //calcs here!
  driv1 = (float)pow(10.0,(6.0 * fParam4 * fParam4) - 1.0);
  driv2 = (float)pow(10.0,(6.0 * fParam5 *fParam5) - 1.0);
  driv3 = (float)pow(10.0,(6.0 * fParam6 *fParam6) - 1.0);

  valve = int(fParam10 > 0.0);
  if(valve)
  {
    trim1 = (float)(0.5);
    trim2 = (float)(0.5);
    trim3 = (float)(0.5);
  }
  else
  {
    trim1 = 0.3f*(float)pow(10.0,(4.0 * pow(fParam4,3.f)));//(0.5 + 500.0 * pow(fParam4,6.0));
    trim2 = 0.3f*(float)pow(10.0,(4.0 * pow(fParam5,3.f)));
    trim3 = 0.3f*(float)pow(10.0,(4.0 * pow(fParam6,3.f)));
  }
  trim1 = (float)(trim1 * pow(10.0, 2.0 * fParam7 - 1.0));
  trim2 = (float)(trim2 * pow(10.0, 2.0 * fParam8 - 1.0));
  trim3 = (float)(trim3 * pow(10.0, 2.0 * fParam9 - 1.0));

  switch(int(fParam1*3.9))
  {
    case 0: trim2=0.0; trim3=0.0; slev=0.0; break;
    case 1: trim1=0.0; trim3=0.0; slev=0.0; break;
    case 2: trim1=0.0; trim2=0.0; slev=0.0; break;
    default: slev=0.5; break;
  }
  fi1 = (float)pow(10.0,fParam2 - 1.70); fo1=(float)(1.0 - fi1);
  fi2 = (float)pow(10.0,fParam3 - 1.05); fo2=(float)(1.0 - fi2);
  fb1 = fb2 = fb3 = 0.0f;
}

mdaBandisto::~mdaBandisto()
{

}

bool  mdaBandisto::getProductString(char* text) { strcpy(text, "MDA Bandisto"); return true; }
bool  mdaBandisto::getVendorString(char* text)  { strcpy(text, "mda"); return true; }
bool  mdaBandisto::getEffectName(char* name)    { strcpy(name, "Bandisto"); return true; }

void mdaBandisto::setProgramName(char *name)
{
	strcpy(programName, name);
}

void mdaBandisto::getProgramName(char *name)
{
	strcpy(name, programName);
}

bool mdaBandisto::getProgramNameIndexed (int32_t category, int32_t index, char* name)
{
	if (index == 0) 
	{
	    strcpy(name, programName);
	    return true;
	}
	return false;
}

void mdaBandisto::setParameter(int32_t index, float value)
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
  }
  //calcs here
  driv1 = (float)pow(10.0,(6.0 * fParam4 * fParam4) - 1.0);
  driv2 = (float)pow(10.0,(6.0 * fParam5 *fParam5) - 1.0);
  driv3 = (float)pow(10.0,(6.0 * fParam6 *fParam6) - 1.0);

  valve = int(fParam10 > 0.0);
  if(valve)
  {
    trim1 = (float)(0.5);
    trim2 = (float)(0.5);
    trim3 = (float)(0.5);
  }
  else
  {
    trim1 = 0.3f*(float)pow(10.0,(4.0 * pow(fParam4,3.f)));//(0.5 + 500.0 * pow(fParam4,6.0));
    trim2 = 0.3f*(float)pow(10.0,(4.0 * pow(fParam5,3.f)));
    trim3 = 0.3f*(float)pow(10.0,(4.0 * pow(fParam6,3.f)));
  }
  trim1 = (float)(trim1 * pow(10.0, 2.0 * fParam7 - 1.0));
  trim2 = (float)(trim2 * pow(10.0, 2.0 * fParam8 - 1.0));
  trim3 = (float)(trim3 * pow(10.0, 2.0 * fParam9 - 1.0));

  switch(int(fParam1*3.9))
  {
    case 0: trim2=0.0; trim3=0.0; slev=0.0; break;
    case 1: trim1=0.0; trim3=0.0; slev=0.0; break;
    case 2: trim1=0.0; trim2=0.0; slev=0.0; break;
   default: slev=0.5; break;
  }
  fi1 = (float)pow(10.0,fParam2 - 1.70); fo1=(float)(1.0 - fi1);
  fi2 = (float)pow(10.0,fParam3 - 1.05); fo2=(float)(1.0 - fi2);
}

float mdaBandisto::getParameter(int32_t index)
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
  }
  return v;
}

void mdaBandisto::getParameterName(int32_t index, char *label)
{
	switch(index)
  {
    case 0: strcpy(label, "Listen"); break;
    case 1: strcpy(label, "L <> M"); break;
    case 2: strcpy(label, "M <> H"); break;
    case 3: strcpy(label, "L Dist"); break;
    case 4: strcpy(label, "M Dist"); break;
    case 5: strcpy(label, "H Dist"); break;
    case 6: strcpy(label, "L Out"); break;
    case 7: strcpy(label, "M Out"); break;
    case 8: strcpy(label, "H Out"); break;
    case 9: strcpy(label, "Mode"); break;
  }
}

void mdaBandisto::getParameterDisplay(int32_t index, char *text)
{
	switch(index)
  {
    case 0: switch(int(fParam1*3.9))
    { case 0: strcpy(text, "Low"); break;
      case 1: strcpy(text, "Mid"); break;
      case 2: strcpy(text, "High"); break;
     default: strcpy(text, "Output"); break; } break;
    case 1: sprintf(text, "%.0f", getSampleRate() * fi1 * (0.098 + 0.09*fi1 + 0.5*pow(fi1,8.2f))); break;
    case 2: sprintf(text, "%.0f", getSampleRate() * fi2 * (0.015 + 0.15*fi2 + 0.9*pow(fi2,8.2f))); break;
    case 3: sprintf(text, "%.0f", 60.0 * fParam4); break;
    case 4: sprintf(text, "%.0f", 60.0 * fParam5); break;
    case 5: sprintf(text, "%.0f", 60.0 * fParam6); break;
    case 6: sprintf(text, "%.0f", 40.0 * fParam7 - 20.0); break;
    case 7: sprintf(text, "%.0f", 40.0 * fParam8 - 20.0); break;
    case 8: sprintf(text, "%.0f", 40.0 * fParam9 - 20.0); break;
    case 9: if(fParam10>0.0) { strcpy(text, "Unipolar"); }
                        else { strcpy(text, "Bipolar"); } break;
  }
}

void mdaBandisto::getParameterLabel(int32_t index, char *label)
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
    case 9: strcpy(label, ""); break;
  }
}

//--------------------------------------------------------------------------------
// process

void mdaBandisto::process(float **inputs, float **outputs, int32_t sampleFrames)
{
	float *in1 = inputs[0];
	float *in2 = inputs[1];
	float *out1 = outputs[0];
	float *out2 = outputs[1];
	float a, b, c, d, g, l=fb3, m, h, s, sl=slev;
  float f1i=fi1, f1o=fo1, f2i=fi2, f2o=fo2, b1=fb1, b2=fb2;
  float g1, d1=driv1, t1=trim1;
  float g2, d2=driv2, t2=trim2;
  float g3, d3=driv3, t3=trim3;
  int v=valve;

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

    s = (a - b) * sl; //keep stereo component for later
    a += (float)(b + 0.00002); //dope filter at low level
    b2 = (f2i * a) + (f2o * b2); //crossovers
    b1 = (f1i * b2) + (f1o * b1);
    l = (f1i * b1) + (f1o * l);
    m=b2-l; h=a-b2;

    g = (l>0)? l : -l;
    g = (float)(1.0 / (1.0 + d1 * g) ); //distort
    g1=g;

    g = (m>0)? m : -m;
    g = (float)(1.0 / (1.0 + d2 * g) );
    g2=g;

    g = (h>0)? h : -h;
    g = (float)(1.0 / (1.0 + d3 * g) );
    g3=g;

    if(v) { if(l>0)g1=1.0; if(m>0)g2=1.0; if(h>0)g3=1.0; }

    a = (l*g1*t1) + (m*g2*t2) + (h*g3*t3);
		c += a + s; // output
		d += a - s;

    *++out1 = c;
		*++out2 = d;
	}
  fb1=b1; fb2=b2, fb3=l;
}

void mdaBandisto::processReplacing(float **inputs, float **outputs, int32_t sampleFrames)
{
	float *in1 = inputs[0];
	float *in2 = inputs[1];
	float *out1 = outputs[0];
	float *out2 = outputs[1];
	float a, b, c, d, g, l=fb3, m, h, s, sl=slev;
  float f1i=fi1, f1o=fo1, f2i=fi2, f2o=fo2, b1=fb1, b2=fb2;
  float g1, d1=driv1, t1=trim1;
  float g2, d2=driv2, t2=trim2;
  float g3, d3=driv3, t3=trim3;
  int v=valve;

	--in1;
	--in2;
	--out1;
	--out2;
	while(--sampleFrames >= 0)
	{
		a = *++in1;
		b = *++in2; //process from here...

    s = (a - b) * sl; //keep stereo component for later
    a += (float)(b + 0.00002); //dope filter at low level
    b2 = (f2i * a) + (f2o * b2); //crossovers
    b1 = (f1i * b2) + (f1o * b1);
    l = (f1i * b1) + (f1o * l);
    m=b2-l; h=a-b2;

    g = (l>0)? l : -l;
    g = (float)(1.0 / (1.0 + d1 * g) ); //distort
    g1=g;

    g = (m>0)? m : -m;
    g = (float)(1.0 / (1.0 + d2 * g) );
    g2=g;

    g = (h>0)? h : -h;
    g = (float)(1.0 / (1.0 + d3 * g) );
    g3=g;

    if(v) { if(l>0)g1=1.0; if(m>0)g2=1.0; if(h>0)g3=1.0; }

    a = (l*g1*t1) + (m*g2*t2) + (h*g3*t3);
		c = a + s; // output
		d = a - s;

    *++out1 = c;
		*++out2 = d;
	}
  fb1=b1; fb2=b2, fb3=l;
}
