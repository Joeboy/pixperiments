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

#include "mdaCombo.h"

#include <math.h>
#include <float.h>

AudioEffect *createEffectInstance(audioMasterCallback audioMaster)
{
  return new mdaCombo(audioMaster);
}

mdaCombo::mdaCombo(audioMasterCallback audioMaster)	: AudioEffectX(audioMaster, 1, 7)	// programs, parameters
{
  //inits here!
  fParam1 = 1.00f; //select
  fParam2 = 0.50f; //drive
  fParam3 = 0.50f; //bias
  fParam4 = 0.50f; //output
  fParam5 = 0.00f; //stereo
  fParam6 = 0.00f; //hpf freq
  fParam7 = 0.50f; //hpf reso

  size = 1024;
  bufpos = 0;
	buffer = new float[size];
	buffe2 = new float[size];

  setNumInputs(2);
	setNumOutputs(2);
	setUniqueID("mdaCombo");    // identify here
	DECLARE_LVZ_DEPRECATED(canMono) ();
	canProcessReplacing();
	strcpy(programName, "Amp & Speaker Simulator");
	suspend();		// flush buffer

  setParameter(0, 0.f); //go and set initial values!
}

bool  mdaCombo::getProductString(char* text) { strcpy(text, "MDA Combo"); return true; }
bool  mdaCombo::getVendorString(char* text)  { strcpy(text, "mda"); return true; }
bool  mdaCombo::getEffectName(char* name)    { strcpy(name, "Combo"); return true; }

void mdaCombo::setParameter(int32_t index, float value)
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
  }
  //calcs here
  ster=0; if(fParam5>0.0) ster=1;
  hpf = filterFreq(25.f);
	switch(int(fParam1*6.9))
  {
    case 0: trim = 0.5f; lpf = 0.f;                 //DI
            mix1 = (float)0.0; mix2 = (float)0.0;
            del1 = 0; del2 = 0;
            break;

    case 1: trim = 0.53f; lpf = filterFreq(2700.f); //speaker sim
            mix1 = (float)0.0; mix2 = (float)0.0;
            del1 = 0; del2 = 0;
            hpf = filterFreq(382.f);
            break;

    case 2: trim = 1.10f; lpf = filterFreq(1685.f); //radio
            mix1 = -1.70f; mix2 = 0.82f;
            del1 = int(getSampleRate() / 6546.f);
            del2 = int(getSampleRate() / 4315.f);
            break;

    case 3: trim = 0.98f; lpf = filterFreq(1385.f); //mesa boogie 1"
            mix1 = -0.53f; mix2 = 0.21f;
            del1 = int(getSampleRate() / 7345.f);
            del2 = int(getSampleRate() / 1193.f);
            break;

    case 4: trim = 0.96f; lpf = filterFreq(1685.f); //mesa boogie 8"
            mix1 = -0.85f; mix2 = 0.41f;
            del1 = int(getSampleRate() / 6546.f);
            del2 = int(getSampleRate() / 3315.f);
            break;

    case 5: trim = 0.59f; lpf = filterFreq(2795.f);
            mix1 = -0.29f; mix2 = 0.38f;          //Marshall 4x12" celestion
            del1 = int(getSampleRate() / 982.f);
            del2 = int(getSampleRate() / 2402.f);
            hpf = filterFreq(459.f);
            break;

    case 6: trim = 0.30f; lpf = filterFreq(1744.f); //scooped-out metal
            mix1 = -0.96f; mix2 = 1.6f;
            del1 = int(getSampleRate() / 356.f);
            del2 = int(getSampleRate() / 1263.f);
            hpf = filterFreq(382.f);
            break;
  }

  mode = (fParam2<0.5)? 1 : 0;
  if(mode) //soft clipping
  {
    drive = (float)pow(10.f, 2.f - 6.f * fParam2);
    trim *= 0.55f + 150.f * (float)pow(fParam2,4.0f);
  }
  else //hard clipping
  {
    drive = 1.f;
    clip = 11.7f - 16.f*fParam2;
    if(fParam2>0.7)
    {
      drive = (float)pow(10.0f, 7.f*fParam2 - 4.9f);
      clip = 0.5f;
    }
  }
  bias = 1.2f * fParam3 - 0.6f;
  if(fParam2>0.5) bias /= (1.f + 3.f * (fParam2-0.5f));
             else bias /= (1.f + 3.f * (0.5f-fParam2));
  trim *= (float)pow(10.f, 2.f*fParam4 - 1.f);
  if(ster) trim *=2.f;

  hhf = fParam6;
  hhq = 1.1f - fParam7;
  if(fParam6>0.05f) drive = drive * (1 + 0.1f * drive);
}

mdaCombo::~mdaCombo()
{
	if(buffer) delete [] buffer;
	if(buffe2) delete [] buffe2;
}

void mdaCombo::suspend()
{
	memset(buffer, 0, size * sizeof(float));
	memset(buffe2, 0, size * sizeof(float));

  ff1=0.f; ff2=0.f; ff3=0.f; ff4=0.f; ff5=0.f;
  ff6=0.f; ff7=0.f; ff8=0.f; ff9=0.f; ff10=0.f;
  hh0 = hh1 = 0.0f;
}

float mdaCombo::filterFreq(float hz)
{
  float j, k, r=0.999f;

  j = r * r - 1;
  k = (float)(2.f - 2.f * r * r * cos(0.647f * hz / getSampleRate() ));
  return (float)((sqrt(k*k - 4.f*j*j) - k) / (2.f*j));
}

void mdaCombo::setProgramName(char *name)
{
	strcpy(programName, name);
}

void mdaCombo::getProgramName(char *name)
{
	strcpy(name, programName);
}

bool mdaCombo::getProgramNameIndexed (int32_t category, int32_t index, char* name)
{
	if (index == 0) 
	{
	    strcpy(name, programName);
	    return true;
	}
	return false;
}

float mdaCombo::getParameter(int32_t index)
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
  }
  return v;
}

void mdaCombo::getParameterName(int32_t index, char *label)
{
	switch(index)
  {
    case 0: strcpy(label, "Model"); break;
    case 1: strcpy(label, "Drive"); break;
    case 2: strcpy(label, "Bias"); break;
    case 3: strcpy(label, "Output"); break;
    case 4: strcpy(label, "Process"); break;
    case 5: strcpy(label, "HPF Freq"); break;
    case 6: strcpy(label, "HPF Reso"); break;
  }
}

#include <stdio.h>
static void int2strng(int32_t value, char *string) { sprintf(string, "%d", value); }

void mdaCombo::getParameterDisplay(int32_t index, char *text)
{
	switch(index)
  {
    case 0: switch(int(fParam1*6.9))
            {
              case 0: strcpy(text, "D.I."); break;
              case 1: strcpy(text, "Spkr Sim"); break;
              //case 2: strcpy(text, "Phone"); break;
              case 2: strcpy(text, "Radio"); break;
              case 3: strcpy(text, "MB 1\""); break;
              case 4: strcpy(text, "MB 8\""); break;
              case 5: strcpy(text, "4x12 ^"); break;
              case 6: strcpy(text, "4x12 >"); break;
           } break;

    case 1: int2strng((int32_t)(200 * fParam2 - 100), text); break;
    case 2: int2strng((int32_t)(200 * fParam3 - 100), text); break;
    case 3: int2strng((int32_t)(40 * fParam4 - 20), text); break;
    case 4: if(fParam5>0.0) strcpy(text, "STEREO");
                       else strcpy(text, "MONO"); break;
    case 5: int2strng((int32_t)(100 * fParam6), text); break;
    case 6: int2strng((int32_t)(100 * fParam7), text); break;
  }
}

void mdaCombo::getParameterLabel(int32_t index, char *label)
{
	switch(index)
  {
    case 0: strcpy(label, ""); break;
    case 1: strcpy(label, "S <> H"); break;
    case 2: strcpy(label, ""); break;
    case 3: strcpy(label, "dB"); break;
    case 4: strcpy(label, ""); break;
    case 5: strcpy(label, "%"); break;
    case 6: strcpy(label, "%"); break;
  }
}

//--------------------------------------------------------------------------------
// process

void mdaCombo::process(float **inputs, float **outputs, int32_t sampleFrames)
{
	float *in1 = inputs[0];
	float *in2 = inputs[1];
	float *out1 = outputs[0];
	float *out2 = outputs[1];
	float a, b, c, d, trm, m1=mix1, m2=mix2, clp=clip;
  float o=lpf, i=1.f-lpf, o2=hpf, i2=1.f-hpf, bi=bias, drv=drive;
  float f1=ff1, f2=ff2, f3=ff3, f4=ff4, f5=ff5;
  float a2, b2, f6=ff6, f7=ff7, f8=ff8, f9=ff9, f10=ff10;
  float h0=hh0, h1=hh1;
  int32_t d1=del1, d2=del2, bp = bufpos;

  trm = trim * i * i * i * i;

  --in1;
	--in2;
	--out1;
	--out2;

  if(fParam5>0.0) //stereo
  {
    while(--sampleFrames >= 0)
	  {
		  a = drv * (*++in1 + bi); a2 = drv * (*++in2 + bi);
		  c = out1[1];
		  d = out2[1]; //process from here...

      if(mode)
      {
        b = (a>0.f)? a : -a;     b2 = (a2>0.f)? a2 : -a2;
        b = a / (1.f + b);       b2 = a2 / (1.f + b2);
      }
      else
      {
        b = (a> clp)?  clp : a;  b2 = (a2> clp)?  clp : a2; //distort
        b = (a<-clp)? -clp : b;  b2 = (a2<-clp)? -clp : b2;
      }

      *(buffer + bp) = b;        *(buffe2 + bp) = b2;
      b += (m1* *(buffer+((bp+d1)%1000))) + (m2* *(buffer+((bp+d2)%1000)));
      b2+= (m1* *(buffe2+((bp+d1)%1000))) + (m2* *(buffe2+((bp+d2)%1000)));

      f1 = o * f1 + trm * b;     f6 = o * f6 + trm * b2;
      f2 = o * f2 +  f1;      f7 = o * f7 +  f6;
      f3 = o * f3 +  f2;      f8 = o * f8 +  f7;
      f4 = o * f4 +  f3;      f9 = o * f9 +  f8; //-24dB/oct filter

      f5 = o2 * f5 + i2 * f4;    f10 = o2 * f10 + i2 * f9;//high pass
      b = f4 - f5;               b2 = f9 - f10;

      *++out1 = c + b;
		  *++out2 = d + b2;
      bp=bufpos;
	  }
  }
  else //mono
  {
    if(mode) //soft clip
    {
      while(--sampleFrames >= 0)
	    {
		    a = drv * (*++in1 + *++in2 + bi);
		    c = out1[1];
		    d = out2[1]; //process from here...

        b = (a>0.f)? a : -a;
        b = a / (1.f + b);

        *(buffer + bp) = b;
        b += (m1* *(buffer+((bp+d1)%1000))) + (m2* *(buffer+((bp+d2)%1000)));

        f1 = o * f1 + trm * b;
        f2 = o * f2 +  f1;
        f3 = o * f3 +  f2;
        f4 = o * f4 +  f3; //-24dB/oct filter

        f5 = o2 * f5 + i2 * f4; //high pass
        b = f4 - f5;

        bp = (bp==0)? 999 : bp - 1; //buffer position

        *++out1 = c + b;
		    *++out2 = d + b;
	    }
    }
    else //hard clip
    {
      while(--sampleFrames >= 0)
	    {
		    a = drv * (*++in1 + *++in2 + bi);
		    c = out1[1];
		    d = out2[1]; //process from here...

        b = (a>clp)? clp : a; //distort
        b = (a<-clp)? -clp : b;

        *(buffer + bp) = b;
        b += (m1* *(buffer+((bp+d1)%1000))) + (m2* *(buffer+((bp+d2)%1000)));

        f1 = o * f1 + trm * b;
        f2 = o * f2 +  f1;
        f3 = o * f3 +  f2;
        f4 = o * f4 +  f3; //-24dB/oct filter

        f5 = o2 * f5 + i2 * f4; //high pass
        b = f4 - f5;

        bp = (bp==0)? 999 : bp - 1; //buffer position

        *++out1 = c + b;
		    *++out2 = d + b;
	    }
    }
  }
  bufpos = bp;
  if(fabs(f1)<1.0e-10) { ff1=0.f; ff2=0.f; ff3=0.f; ff4=0.f; ff5=0.f;  }
                  else { ff1=f1;  ff2=f2;  ff3=f3;  ff4=f4;  ff5=f5;   }
  if(fabs(f6)<1.0e-10) { ff6=0.f; ff7=0.f; ff8=0.f; ff9=0.f; ff10=0.f; }
                  else { ff6=f6;  ff7=f7;  ff8=f8;  ff9=f9;  ff10=f10; }
  if(fabs(h0)<1.0e-10) { hh0 = hh1 = 0.0f; } else { hh0=h0; hh1=h1; }
}

void mdaCombo::processReplacing(float **inputs, float **outputs, int32_t sampleFrames)
{
	float *in1 = inputs[0];
	float *in2 = inputs[1];
	float *out1 = outputs[0];
	float *out2 = outputs[1];
	float a, b, trm, m1=mix1, m2=mix2, clp=clip;
  float o=lpf, i=1.f-lpf, o2=hpf, bi=bias, drv=drive;
  float f1=ff1, f2=ff2, f3=ff3, f4=ff4, f5=ff5;
  float a2, b2, f6=ff6, f7=ff7, f8=ff8, f9=ff9, f10=ff10;
  float hf=hhf, hq=hhq, h0=hh0, h1=hh1;
  int32_t d1=del1, d2=del2, bp = bufpos;

  trm = trim * i * i * i * i;

	--in1;
	--in2;
	--out1;
	--out2;

  if(ster) //stereo
  {
    while(--sampleFrames >= 0)
	  {
		  a = drv * (*++in1 + bi); a2 = drv * (*++in2 + bi);

      if(mode)
      {
        b = (a>0.f)? a : -a;     b2 = (a2>0.f)? a2 : -a2;
        b = a / (1.f + b);       b2 = a2 / (1.f + b2);
      }
      else
      {
        b = (a> clp)?  clp : a;  b2 = (a2> clp)?  clp : a2; //distort
        b = (a<-clp)? -clp : b;  b2 = (a2<-clp)? -clp : b2;
      }

      *(buffer + bp) = b;        *(buffe2 + bp) = b2;
      b += (m1* *(buffer+((bp+d1)%1000))) + (m2* *(buffer+((bp+d2)%1000)));
      b2+= (m1* *(buffe2+((bp+d1)%1000))) + (m2* *(buffe2+((bp+d2)%1000)));

      f1 = o * f1 + trm * b;     f6 = o * f6 + trm * b2;
      f2 = o * f2 + f1;      f7 = o * f7 + f6;
      f3 = o * f3 + f2;      f8 = o * f8 + f7;
      f4 = o * f4 + f3;      f9 = o * f9 + f8; //-24dB/oct filter

      f5 = o2 * (f5 - f4) + f4;  f10 = o2 * (f10 - f9) + f9;//high pass
      b = f4 - f5;               b2 = f9 - f10;

      if(bp==0) bufpos=999; else bufpos=bp-1;

      *++out1 = b;
		  *++out2 = b2;
	  }
  }
  else //mono
  {
    if(mode) //soft clip
    {
	    while(--sampleFrames >= 0)
	    {
		    a = drv * (*++in1 + *++in2 + bi);

        h0 += hf * (h1 + a); //resonant highpass (Chamberlin SVF)
        h1 -= hf * (h0 + hq * h1);
        a += h1;

        b = (a>0.f)? a : -a;
        b = a / (1.f + b);

        *(buffer + bp) = b;
        b += (m1* *(buffer+((bp+d1)%1000))) + (m2* *(buffer+((bp+d2)%1000)));

        f1 = o * f1 + trm * b;
        f2 = o * f2 + f1;
        f3 = o * f3 + f2;
        f4 = o * f4 + f3; //-24dB/oct filter

        f5 = o2 * (f5 - f4) + f4; //high pass
        b = f4 - f5;

        bp = (bp==0)? 999 : bp - 1; //buffer position

        *++out1 = b;
		    *++out2 = b;
	    }
    }
    else //hard clip
    {
      while(--sampleFrames >= 0)
	    {
		    a = drv * (*++in1 + *++in2 + bi);

        h0 += hf * (h1 + a); //resonant highpass (Chamberlin SVF)
        h1 -= hf * (h0 + hq * h1);
        a += h1;

        b = (a>clp)? clp : a; //distort
        b = (a<-clp)? -clp : b;

        *(buffer + bp) = b;
        b += (m1* *(buffer+((bp+d1)%1000))) + (m2* *(buffer+((bp+d2)%1000)));

        f1 = o * f1 + trm * b;
        f2 = o * f2 + f1;
        f3 = o * f3 + f2;
        f4 = o * f4 + f3; //-24dB/oct filter

        f5 = o2 * (f5 - f4) + f4; //high pass //also want smile curve here...
        b = f4 - f5;

        bp = (bp==0)? 999 : bp - 1; //buffer position

        *++out1 = b;
		    *++out2 = b;
	    }
    }
  }
  bufpos = bp;
  if(fabs(f1)<1.0e-10) { ff1=0.f; ff2=0.f; ff3=0.f; ff4=0.f; ff5=0.f;  }
                  else { ff1=f1;  ff2=f2;  ff3=f3;  ff4=f4;  ff5=f5;   }
  if(fabs(f6)<1.0e-10) { ff6=0.f; ff7=0.f; ff8=0.f; ff9=0.f; ff10=0.f; }
                  else { ff6=f6;  ff7=f7;  ff8=f8;  ff9=f9;  ff10=f10; }
  if(fabs(h0)<1.0e-10) { hh0 = hh1 = 0.0f; } else { hh0=h0; hh1=h1; }
}
