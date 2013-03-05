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

#include "mdaTestTone.h"

#include <math.h>
#include <stdlib.h>

AudioEffect *createEffectInstance(audioMasterCallback audioMaster)
{
  return new mdaTestTone(audioMaster);
}

mdaTestTone::mdaTestTone(audioMasterCallback audioMaster)	: AudioEffectX(audioMaster, 1, 8)
{
  fParam0 = 0.0f; //mode
  fParam1 = 0.71f; //level dB
  fParam2 = 0.50f; //pan dB
  fParam3 = 0.57f; //freq1 B
  fParam4 = 0.50f; //freq2 Hz
  fParam5 = 0.00f; //thru dB
  fParam6 = 0.30f; //sweep ms
  fParam7 = 1.00f; //cal dBFS

  setNumInputs(2);
	setNumOutputs(2);
	setUniqueID("mdaTestTone");
	DECLARE_LVZ_DEPRECATED(canMono) ();
	canProcessReplacing();
	strcpy(programName, "Signal Generator");

  updateTx = updateRx;

  suspend();
  setParameter(6, 0.f);
}

bool  mdaTestTone::getProductString(char* text) { strcpy(text, "MDA TestTone"); return true; }
bool  mdaTestTone::getVendorString(char* text)  { strcpy(text, "mda"); return true; }
bool  mdaTestTone::getEffectName(char* name)    { strcpy(name, "TestTone"); return true; }

mdaTestTone::~mdaTestTone()
{
}

void mdaTestTone::suspend()
{
  zz0 = zz1 = zz2 = zz3 = zz4 = zz5 = phi = 0.0f;
}

void mdaTestTone::setProgramName(char *name)
{
	strcpy(programName, name);
}

void mdaTestTone::getProgramName(char *name)
{
	strcpy(name, programName);
}

bool mdaTestTone::getProgramNameIndexed (int32_t category, int32_t index, char* name)
{
	if (index == 0) 
	{
	    strcpy(name, programName);
	    return true;
	}
	return false;
}

#include <stdio.h>
static void int2strng(int32_t value, char *string) { sprintf(string, "%d", value); }
static void float2strng(float value, char *string) { sprintf(string, "%.2f", value); }

void mdaTestTone::setParameter(int32_t index, float value)
{
	switch(index)
  {
    case 0: fParam0 = value; break;
    case 1: fParam1 = value; break;
    case 2: fParam2 = value; break;
    case 3: fParam3 = value; break;
    case 4: fParam4 = value; break;
    case 6: fParam5 = value; break;
    case 5: fParam6 = value; break;
    case 7: fParam7 = value; break;
  }


  //just update display text...
  mode = int(8.9 * fParam0);
  float f, df=0.0f;
  if(fParam4>0.6) df = 1.25f*fParam4 - 0.75f;
  if(fParam4<0.4) df = 1.25f*fParam4 - 0.50f;
  switch(mode)
  {
    case 0: //MIDI note
            f = (float)floor(128.f*fParam3);
            //int2strng((int32_t)f, disp1); //Semi
            midi2string(f, disp1); //Semitones
            int2strng((int32_t)(100.f*df), disp2); //Cents
            break;

    case 1: //no frequency display
    case 2: 
    case 3: 
    case 4: strcpy(disp1, "--");
            strcpy(disp2, "--"); break;
    
    case 5: //sine
            f = 13.f + (float)floor(30.f*fParam3);
            iso2string(f, disp1); //iso band freq
            f=(float)pow(10.0f, 0.1f*(f+df));
            float2strng(f, disp2); //Hz
            break;
    
    case 6: //log sweep & step        
    case 7: sw = 13.f + (float)floor(30.f*fParam3);
            swx = 13.f + (float)floor(30.f*fParam4);
            iso2string(sw, disp1); //start freq
            iso2string(swx, disp2); //end freq
            break; 
    
    case 8: //lin sweep
            sw = 200.f * (float)floor(100.f*fParam3);
            swx = 200.f * (float)floor(100.f*fParam4);
            int2strng((int32_t)sw, disp1); //start freq
            int2strng((int32_t)swx, disp2); //end freq
            break; 
  }

  updateTx++;
}

void mdaTestTone::update()
{
  updateRx = updateTx;

  float f, df, twopi=6.2831853f;

  //calcs here!
  mode = int(8.9 * fParam0);
  left = 0.05f * (float)int(60.f*fParam1);
  left = (float)pow(10.0f, left - 3.f);
  if(mode==2) left*=0.0000610f; //scale white for RAND_MAX = 32767
  if(mode==3) left*=0.0000243f; //scale pink for RAND_MAX = 32767
  if(fParam2<0.3f) right=0.f; else right=left;
  if(fParam2>0.6f) left=0.f;
  len = 1.f + 0.5f*(float)int(62*fParam6);
  swt=(int32_t)(len*getSampleRate());

  if(fParam7>0.8) //output level trim
  {
    if(fParam7>0.96) cal = 0.f;
    else if(fParam7>0.92) cal = -0.01000001f;
    else if(fParam7>0.88) cal = -0.02000001f;
    else if(fParam7>0.84) cal = -0.1f;
    else cal = -0.2f;

    calx = (float)pow(10.0f, 0.05f*cal);
    left*=calx; right*=calx;
    calx = 0.f;
  }
  else //output level calibrate
  {
    cal = (float)int(25.f*fParam7 - 21.1f);
    calx = cal;
  }

  df=0.f;
  if(fParam4>0.6) df = 1.25f*fParam4 - 0.75f;
  if(fParam4<0.4) df = 1.25f*fParam4 - 0.50f;

  switch(mode)
  {
        case 0: //MIDI note
                f = (float)floor(128.f*fParam3);
                //int2strng((int32_t)f, disp1); //Semi
                midi2string(f, disp1); //Semitones
                int2strng((int32_t)(100.f*df), disp2); //Cents
                dphi = 51.37006f*(float)pow(1.0594631f,f+df)/getSampleRate();
                break;

        case 1: //no frequency display
        case 2:
        case 3:
        case 4: strcpy(disp1, "--");
                strcpy(disp2, "--"); break;

        case 5: //sine
                f = 13.f + (float)floor(30.f*fParam3);
                iso2string(f, disp1); //iso band freq
                f=(float)pow(10.0f, 0.1f*(f+df));
                float2strng(f, disp2); //Hz
                dphi=twopi*f/getSampleRate();
                break;

        case 6: //log sweep & step
        case 7: sw = 13.f + (float)floor(30.f*fParam3);
                swx = 13.f + (float)floor(30.f*fParam4);
                iso2string(sw, disp1); //start freq
                iso2string(swx, disp2); //end freq
                if(sw>swx) { swd=swx; swx=sw; sw=swd; } //only sweep up
                if(mode==7) swx += 1.f;
                swd = (swx-sw) / (len*getSampleRate());
                swt= 2 * (int32_t)getSampleRate();
                break;

        case 8: //lin sweep
                sw = 200.f * (float)floor(100.f*fParam3);
                swx = 200.f * (float)floor(100.f*fParam4);
                int2strng((int32_t)sw, disp1); //start freq
                int2strng((int32_t)swx, disp2); //end freq
                if(sw>swx) { swd=swx; swx=sw; sw=swd; } //only sweep up
                sw = twopi*sw/getSampleRate();
                swx = twopi*swx/getSampleRate();
                swd = (swx-sw) / (len*getSampleRate());
                swt= 2 * (int32_t)getSampleRate();
                break;
  }
  thru = (float)pow(10.0f, (0.05f * (float)int(40.f*fParam5)) - 2.f);
  if(fParam5==0.0f) thru=0.0f;
  fscale = twopi/getSampleRate();
}

void mdaTestTone::midi2string(float n, char *text)
{
  char t[8];
  int nn, o, s, p=0;

  nn = int(n);
  if(nn>99) t[p++] = 48 + (int(0.01*n)%10);
  if(nn>9)  t[p++] = 48 + (int(0.10*n)%10);
            t[p++] = 48 + (int(n)%10);
  t[p++] = ' ';

  o = int(nn/12.f); s = nn-(12*o); o -= 2;

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

  if(o<0) { t[p++]='-'; o = -o; }
  t[p++]= 48 + (o%10);

  t[p]=0;
  strcpy(text, t);
}

void mdaTestTone::iso2string(float b, char *text)
{
  switch(int(b))
  {
    case 13: strcpy(text, "20 Hz"); break;
    case 14: strcpy(text, "25 Hz"); break;
    case 15: strcpy(text, "31 Hz"); break;
    case 16: strcpy(text, "40 Hz"); break;
    case 17: strcpy(text, "50 Hz"); break;
    case 18: strcpy(text, "63 Hz"); break;
    case 19: strcpy(text, "80 Hz"); break;
    case 20: strcpy(text, "100 Hz"); break;
    case 21: strcpy(text, "125 Hz"); break;
    case 22: strcpy(text, "160 Hz"); break;
    case 23: strcpy(text, "200 Hz"); break;
    case 24: strcpy(text, "250 Hz"); break;
    case 25: strcpy(text, "310 Hz"); break;
    case 26: strcpy(text, "400 Hz"); break;
    case 27: strcpy(text, "500 Hz"); break;
    case 28: strcpy(text, "630 Hz"); break;
    case 29: strcpy(text, "800 Hz"); break;
    case 30: strcpy(text, "1 kHz"); break;
    case 31: strcpy(text, "1.25 kHz"); break;
    case 32: strcpy(text, "1.6 kHz"); break;
    case 33: strcpy(text, "2.0 kHz"); break;
    case 34: strcpy(text, "2.5 kHz"); break;
    case 35: strcpy(text, "3.1 kHz"); break;
    case 36: strcpy(text, "4 kHz"); break;
    case 37: strcpy(text, "5 kHz"); break;
    case 38: strcpy(text, "6.3 kHz"); break;
    case 39: strcpy(text, "8 kHz"); break;
    case 40: strcpy(text, "10 kHz"); break;
    case 41: strcpy(text, "12.5 kHz"); break;
    case 42: strcpy(text, "16 kHz"); break;
    case 43: strcpy(text, "20 kHz"); break;
    default: strcpy(text, "--"); break;
  }
}

float mdaTestTone::getParameter(int32_t index)
{
	float v=0;

  switch(index)
  {
    case 0: v = fParam0; break;
    case 1: v = fParam1; break;
    case 2: v = fParam2; break;
    case 3: v = fParam3; break;
    case 4: v = fParam4; break;
    case 6: v = fParam5; break;
    case 5: v = fParam6; break;
    case 7: v = fParam7; break;
  }
  return v;
}

void mdaTestTone::getParameterName(int32_t index, char *label)
{
	switch(index)
  {
    case 0: strcpy(label, "Mode"); break;
    case 1: strcpy(label, "Level"); break;
    case 2: strcpy(label, "Channel"); break;
    case 3: strcpy(label, "F1"); break;
    case 4: strcpy(label, "F2"); break;
    case 6: strcpy(label, "Thru"); break;
    case 5: strcpy(label, "Sweep"); break;
    case 7: strcpy(label, "Zero dB"); break;
  }
}

void mdaTestTone::getParameterDisplay(int32_t index, char *text)
{
	switch(index)
  {
    case 0:
      switch(mode)
      {
        case 0: strcpy(text, "MIDI #"); break;
        case 1: strcpy(text, "IMPULSE"); break;
        case 2: strcpy(text, "WHITE"); break;
        case 3: strcpy(text, "PINK"); break;
        case 4: strcpy(text, "---"); break;
        case 5: strcpy(text, "SINE"); break;
        case 6: strcpy(text, "LOG SWP."); break;
        case 7: strcpy(text, "LOG STEP"); break;
        case 8: strcpy(text, "LIN SWP."); break;
      } break;
    case 1: int2strng((int32_t)(int(60.f * fParam1) - 60.0 - calx), text); break;
    case 2: if(fParam2>0.3f)
            { if(fParam2>0.7f) strcpy(text, "RIGHT");
              else strcpy(text, "CENTRE"); }
            else strcpy(text, "LEFT"); break;
    case 3: strcpy(text, disp1); break;
    case 4: strcpy(text, disp2); break;
    case 6: if(fParam5==0) strcpy(text, "OFF");
            else int2strng((int32_t)(40 * fParam5 - 40), text); break;
    case 5: int2strng(1000 + 500*int(62*fParam6), text); break;
    case 7: float2strng(cal, text); break;
  }
}

void mdaTestTone::getParameterLabel(int32_t index, char *label)
{
	switch(index)
  {
    case 0: strcpy(label, ""); break;
    case 1: strcpy(label, "dB"); break;
    case 2: strcpy(label, "L <> R"); break;
    case 3: strcpy(label, ""); break;
    case 4: strcpy(label, ""); break;
    case 6: strcpy(label, "dB"); break;
    case 5: strcpy(label, "ms"); break;
    case 7: strcpy(label, "dBFS"); break;
  }
}

//--------------------------------------------------------------------------------
// process

void mdaTestTone::process(float **inputs, float **outputs, int32_t sampleFrames)
{
	if(updateRx != updateTx) update();

  float *in1 = inputs[0];
	float *in2 = inputs[1];
	float *out1 = outputs[0];
	float *out2 = outputs[1];
	float a, b, c, d, x=0.0f, twopi=6.2831853f;
  float z0=zz0, z1=zz1, z2=zz2, z3=zz3, z4=zz4, z5=zz5;
  float ph=phi, dph=dphi, l=left, r=right, t=thru;
  float s=sw, sx=swx, ds=swd, fsc=fscale;
  int32_t st=swt;
  int m=mode;

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

  	switch(m)
    {
      case 1: if(st>0) { st--; x=0.f; } else //impulse
              {
                x=1.f;
                st=(int32_t)(len*getSampleRate());
              }
              break;

      case 2: //noise
    #ifdef _WIN32
      case 3: x = (float)(rand() - 16384); //for RAND_MAX = 32767
    #else //mac/gcc
      case 3: x = (float)((rand() & 0x7FFF) - 16384);
    #endif
              if(m==3)
              {
                 z0 = 0.997f * z0 + 0.029591f * x; //pink filter
                 z1 = 0.985f * z1 + 0.032534f * x;
                 z2 = 0.950f * z2 + 0.048056f * x;
                 z3 = 0.850f * z3 + 0.090579f * x;
                 z4 = 0.620f * z4 + 0.108990f * x;
                 z5 = 0.250f * z5 + 0.255784f * x;
                 x = z0 + z1 + z2 + z3 + z4 + z5;
              }
              break;

      case 4: x=0.f; break; //mute

      case 0: //tones
      case 5:
      case 9: ph = (float)fmod(ph+dph,twopi);
              x = (float)sin(ph);
              break;

      case 6: //log sweep & step
      case 7: if(st>0) { st--; ph=0.f; } else
              {
                s += ds;
                if(m==7) dph = fsc * (float)pow(10.0f, 0.1f * (float)int(s));
                else dph = fsc * (float)pow(10.0f, 0.1f * s);
                x = (float)sin(ph);
                ph = (float)fmod(ph+dph,twopi);
                if(s>sx) { l=0.f; r=0.f; }
              }
              break;

      case 8: //lin sweep
              if(st>0) { st--; ph=0.f; } else
              {
                s += ds;
                x = (float)sin(ph);
                ph = (float)fmod(ph+s,twopi);
                if(s>sx) { l=0.f; r=0.f; }
              }
              break;
    }
    *++out1 = c + t*a + l*x;
		*++out2 = d + t*b + r*x;
	}
  zz0=z0; zz1=z1; zz2=z2; zz3=z3, zz4=z4; zz5=z5;
  phi=ph; sw=s; swt=st;
  if(s>sx) setParameter(0, fParam0); //retrigger sweep
}

void mdaTestTone::processReplacing(float **inputs, float **outputs, int32_t sampleFrames)
{
	if(updateRx != updateTx) update();

	float *in1 = inputs[0];
	float *in2 = inputs[1];
	float *out1 = outputs[0];
	float *out2 = outputs[1];
	float a, b, x=0.0f, twopi=6.2831853f;
  float z0=zz0, z1=zz1, z2=zz2, z3=zz3, z4=zz4, z5=zz5;
  float ph=phi, dph=dphi, l=left, r=right, t=thru;
  float s=sw, sx=swx, ds=swd, fsc=fscale;
  int32_t st=swt;
  int m=mode;

	--in1;
	--in2;
	--out1;
	--out2;
	while(--sampleFrames >= 0)
	{
		a = *++in1;
		b = *++in2;

  	switch(m)
    {
      case 1: if(st>0) { st--; x=0.f; } else //impulse
              {
                x=1.f;
                st=(int32_t)(len*getSampleRate());
              }
              break;

      case 2: //noise
    #ifdef _WIN32
      case 3: x = (float)(rand() - 16384); //for RAND_MAX = 32767
    #else //mac/gcc
      case 3: x = (float)((rand() & 0x7FFF) - 16384);
    #endif
              if(m==3)
              {
                 z0 = 0.997f * z0 + 0.029591f * x; //pink filter
                 z1 = 0.985f * z1 + 0.032534f * x;
                 z2 = 0.950f * z2 + 0.048056f * x;
                 z3 = 0.850f * z3 + 0.090579f * x;
                 z4 = 0.620f * z4 + 0.108990f * x;
                 z5 = 0.250f * z5 + 0.255784f * x;
                 x = z0 + z1 + z2 + z3 + z4 + z5;
              }
              break;

      case 4: x=0.f; break; //mute

      case 0: //tones
      case 5:
      case 9: ph = (float)fmod(ph+dph,twopi);
              x = (float)sin(ph);
              break;

      case 6: //log sweep & step
      case 7: if(st>0) { st--; ph=0.f; } else
              {
                s += ds;
                if(m==7) dph = fsc * (float)pow(10.0f, 0.1f * (float)int(s));
                else dph = fsc * (float)pow(10.0f, 0.1f * s);
                x = (float)sin(ph);
                ph = (float)fmod(ph+dph,twopi);
                if(s>sx) { l=0.f; r=0.f; }
              }
              break;

      case 8: //lin sweep
              if(st>0) { st--; ph=0.f; } else
              {
                s += ds;
                x = (float)sin(ph);
                ph = (float)fmod(ph+s,twopi);
                if(s>sx) { l=0.f; r=0.f; }
              }
              break;
    }

    *++out1 = t*a + l*x;
		*++out2 = t*b + r*x;
	}
  zz0=z0; zz1=z1; zz2=z2; zz3=z3, zz4=z4; zz5=z5;
  phi=ph; sw=s; swt=st;
  if(s>sx) setParameter(0, fParam0); //retrigger sweep
}
