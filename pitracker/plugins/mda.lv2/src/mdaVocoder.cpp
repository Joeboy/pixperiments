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

#include "mdaVocoder.h"

#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <math.h>

AudioEffect *createEffectInstance(audioMasterCallback audioMaster)
{
  return new mdaVocoder(audioMaster);
}

mdaVocoderProgram::mdaVocoderProgram() ///default program settings
{
  param[0] = 0.0f;   //input select
  param[1] = 0.50f;  //output dB
  param[2] = 0.40f;  //hi thru
  param[3] = 0.40f;  //hi band
  param[4] = 0.16f;  //envelope
  param[5] = 0.55f;  //filter q
  param[6] = 0.6667f;//freq range
  param[7] = 0.0f;  //num bands
  strcpy(name, "Vocoder");
}


mdaVocoder::mdaVocoder(audioMasterCallback audioMaster): AudioEffectX(audioMaster, NPROGS, NPARAMS)
{
  setNumInputs(2);
  setNumOutputs(2);
  setUniqueID("mdaVocoder");  ///identify plug-in here
  //canMono();
  canProcessReplacing();

  programs = new mdaVocoderProgram[NPROGS];
  setProgram(0);

  ///differences from default program...
  programs[1].param[7] = 1.0f;
  strcpy(programs[1].name,"16 Band Vocoder");
  programs[2].param[2] = 0.00f;
  programs[2].param[3] = 0.00f;
  programs[2].param[6] = 0.50f;
  strcpy(programs[2].name,"Old Vocoder");
  programs[3].param[3] = 0.00f;
  programs[3].param[5] = 0.70f;
  programs[3].param[6] = 0.50f;
  strcpy(programs[3].name,"Choral Vocoder");
  programs[4].param[4] = 0.78f;
  programs[4].param[6] = 0.30f;
  strcpy(programs[4].name,"Pad Vocoder");

  suspend();
}

bool  mdaVocoder::getProductString(char* text) { strcpy(text, "MDA Vocoder"); return true; }
bool  mdaVocoder::getVendorString(char* text)  { strcpy(text, "mda"); return true; }
bool  mdaVocoder::getEffectName(char* name)    { strcpy(name, "Vocoder"); return true; }

void mdaVocoder::resume() ///update internal parameters...
{
  float * param = programs[curProgram].param;
  double tpofs = 6.2831853/getSampleRate();
  double rr, th; //, re;
  float sh;
  int32_t i;

  swap = 1; if(param[0]>0.5f) swap = 0;
  gain = (float)pow(10.0f, 2.0f * param[1] - 3.0f * param[5] - 2.0f);

  thru = (float)pow(10.0f, 0.5f + 2.0f * param[1]);
  high =  param[3] * param[3] * param[3] * thru;
  thru *= param[2] * param[2] * param[2];

  if(param[7]<=0.0f)
  {
    nbnd=8;
    //re=0.003f;
    f[1][2] = 3000.0f;
    f[2][2] = 2200.0f;
    f[3][2] = 1500.0f;
    f[4][2] = 1080.0f;
    f[5][2] = 700.0f;
    f[6][2] = 390.0f;
    f[7][2] = 190.0f;
  }
  else
  {
    nbnd=16;
    //re=0.0015f;
    f[ 1][2] = 5000.0f; //+1000
    f[ 2][2] = 4000.0f; //+750
    f[ 3][2] = 3250.0f; //+500
    f[ 4][2] = 2750.0f; //+450
    f[ 5][2] = 2300.0f; //+300
    f[ 6][2] = 2000.0f; //+250
    f[ 7][2] = 1750.0f; //+250
    f[ 8][2] = 1500.0f; //+250
    f[ 9][2] = 1250.0f; //+250
    f[10][2] = 1000.0f; //+250
    f[11][2] =  750.0f; //+210
    f[12][2] =  540.0f; //+190
    f[13][2] =  350.0f; //+155
    f[14][2] =  195.0f; //+100
    f[15][2] =   95.0f;
  }

  if(param[4]<0.05f) //freeze
  {
    for(i=0;i<nbnd;i++) f[i][12]=0.0f;
  }
  else
  {
    f[0][12] = (float)pow(10.0, -1.7 - 2.7f * param[4]); //envelope speed

    rr = 0.022f / (float)nbnd; //minimum proportional to frequency to stop distortion
    for(i=1;i<nbnd;i++)
    {
      f[i][12] = (float)(0.025 - rr * (double)i);
      if(f[0][12] < f[i][12]) f[i][12] = f[0][12];
    }
    f[0][12] = 0.5f * f[0][12]; //only top band is at full rate
  }

  rr = 1.0 - pow(10.0f, -1.0f - 1.2f * param[5]);
  sh = (float)pow(2.0f, 3.0f * param[6] - 1.0f); //filter bank range shift

  for(i=1;i<nbnd;i++)
  {
    f[i][2] *= sh;
    th = acos((2.0 * rr * cos(tpofs * f[i][2])) / (1.0 + rr * rr));
    f[i][0] = (float)(2.0 * rr * cos(th)); //a0
    f[i][1] = (float)(-rr * rr);           //a1
                //was .98
    f[i][2] *= 0.96f; //shift 2nd stage slightly to stop high resonance peaks
    th = acos((2.0 * rr * cos(tpofs * f[i][2])) / (1.0 + rr * rr));
    f[i][2] = (float)(2.0 * rr * cos(th));
  }
}


void mdaVocoder::suspend() ///clear any buffers...
{
  int32_t i, j;

  for(i=0; i<nbnd; i++) for(j=3; j<12; j++) f[i][j] = 0.0f; //zero band filters and envelopes
  kout = 0.0f;
  kval = 0;
}


mdaVocoder::~mdaVocoder() ///destroy any buffers...
{
  if(programs) delete [] programs;
}


void mdaVocoder::setProgram(int32_t program)
{
  curProgram = program;
  resume();
}


void  mdaVocoder::setParameter(int32_t index, float value)
{
  programs[curProgram].param[index] = value;
  resume();
}
float mdaVocoder::getParameter(int32_t index) { return programs[curProgram].param[index]; }
void  mdaVocoder::setProgramName(char *name) { strcpy(programs[curProgram].name, name); }
void  mdaVocoder::getProgramName(char *name) { strcpy(name, programs[curProgram].name); }
bool mdaVocoder::getProgramNameIndexed (int32_t category, int32_t index, char* name)
{
	if ((unsigned int)index < NPROGS) 
	{
	    strcpy(name, programs[index].name);
	    return true;
	}
	return false;
}

void mdaVocoder::getParameterName(int32_t index, char *label)
{
  switch(index)
  {
    case  0: strcpy(label, "Mod In"); break;
    case  1: strcpy(label, "Output"); break;
    case  2: strcpy(label, "Hi Thru"); break;
    case  3: strcpy(label, "Hi Band"); break;
    case  4: strcpy(label, "Envelope"); break;
    case  5: strcpy(label, "Filter Q"); break;
    case  6: strcpy(label, "Mid Freq"); break;
    default: strcpy(label, "Quality");
  }
}


void mdaVocoder::getParameterDisplay(int32_t index, char *text)
{
 	char string[16];
 	float * param = programs[curProgram].param;

  switch(index)
  {
    case  0: if(swap) strcpy(string, "RIGHT"); else strcpy(string, "LEFT"); break;
    case  1: sprintf(string, "%.1f", 40.0f * param[index] - 20.0f); break;
    case  4: if(param[index]<0.05f) strcpy(string, "FREEZE");
             else sprintf(string, "%.1f", (float)pow(10.0f, 1.0f + 3.0f * param[index])); break;
    case  6: sprintf(string, "%.0f", 800.0f * (float)pow(2.0f, 3.0f * param[index] - 2.0f)); break;
    case  7: if(nbnd==8) strcpy(string, "8 BAND"); else strcpy(string, "16 BAND"); break;

    default: sprintf(string, "%.0f", 100.0f * param[index]);
  }
	string[8] = 0;
	strcpy(text, (char *)string);
}


void mdaVocoder::getParameterLabel(int32_t index, char *label)
{
  switch(index)
  {
    case  7:
    case  0: strcpy(label, ""); break;
    case  1: strcpy(label, "dB"); break;
    case  4: strcpy(label, "ms"); break;
    case  6: strcpy(label, "Hz"); break;
    default: strcpy(label, "%");
  }
}


void mdaVocoder::process(float **inputs, float **outputs, int32_t sampleFrames)
{
  float *in1 = inputs[0];
  float *in2 = inputs[1];
  float *out1 = outputs[0];
  float *out2 = outputs[1];
  float a, b, c, d, o=0.0f, aa, bb, oo=kout, g=gain, ht=thru, hh=high, tmp;
  int32_t i, k=kval, sw=swap, nb=nbnd;

  --in1;
  --in2;
  --out1;
  --out2;
  while(--sampleFrames >= 0)
  {
    a = *++in1; //speech
    b = *++in2; //synth
    c = out1[1];
    d = out2[1];
    if(sw==0) { tmp=a; a=b; b=tmp; } //swap channels

    tmp = a - f[0][7]; //integrate modulator for HF band and filter bank pre-emphasis
    f[0][7] = a;
    a = tmp;

    if(tmp<0.0f) tmp = -tmp;
    f[0][11] -= f[0][12] * (f[0][11] - tmp);      //high band envelope
    o = f[0][11] * (ht * a + hh * (b - f[0][3])); //high band + high thru

    f[0][3] = b; //integrate carrier for HF band

    if(++k & 0x1) //this block runs at half sample rate
    {
      oo = 0.0f;
      aa = a + f[0][9] - f[0][8] - f[0][8];  //apply zeros here instead of in each reson
               f[0][9] = f[0][8];  f[0][8] = a;
      bb = b + f[0][5] - f[0][4] - f[0][4];
               f[0][5] = f[0][4];  f[0][4] = b;

      for(i=1; i<nb; i++) //filter bank: 4th-order band pass
      {
        tmp = f[i][0] * f[i][3] + f[i][1] * f[i][4] + bb;
        f[i][4] = f[i][3];
        f[i][3] = tmp;
        tmp += f[i][2] * f[i][5] + f[i][1] * f[i][6];
        f[i][6] = f[i][5];
        f[i][5] = tmp;

        tmp = f[i][0] * f[i][7] + f[i][1] * f[i][8] + aa;
        f[i][8] = f[i][7];
        f[i][7] = tmp;
        tmp += f[i][2] * f[i][9] + f[i][1] * f[i][10];
        f[i][10] = f[i][9];
        f[i][9] = tmp;

        if(tmp<0.0f) tmp = -tmp;
        f[i][11] -= f[i][12] * (f[i][11] - tmp);
        oo += f[i][5] * f[i][11];
      }
    }
    o += oo * g; //effect of interpolating back up to Fs would be minimal (aliasing >16kHz)

    *++out1 = c + o;
    *++out2 = d + o;
  }

  kout = oo;
  kval = k & 0x1;
  if(fabs(f[0][11])<1.0e-10) f[0][11] = 0.0f; //catch HF envelope denormal

  for(i=1;i<nb;i++)
    if(fabs(f[i][3])<1.0e-10 || fabs(f[i][7])<1.0e-10)
      for(k=3; k<12; k++) f[i][k] = 0.0f; //catch reson & envelope denormals

  if(fabs(o)>10.0f) suspend(); //catch instability
}


void mdaVocoder::processReplacing(float **inputs, float **outputs, int32_t sampleFrames)
{
  float *in1 = inputs[0];
  float *in2 = inputs[1];
  float *out1 = outputs[0];
  float *out2 = outputs[1];
  float a, b, o=0.0f, aa, bb, oo=kout, g=gain, ht=thru, hh=high, tmp;
  int32_t i, k=kval, sw=swap, nb=nbnd;

  --in1;
  --in2;
  --out1;
  --out2;
  while(--sampleFrames >= 0)
  {
    a = *++in1; //speech
    b = *++in2; //synth
    if(sw==0) { tmp=a; a=b; b=tmp; } //swap channels

    tmp = a - f[0][7]; //integrate modulator for HF band and filter bank pre-emphasis
    f[0][7] = a;
    a = tmp;

    if(tmp<0.0f) tmp = -tmp;
    f[0][11] -= f[0][12] * (f[0][11] - tmp);      //high band envelope
    o = f[0][11] * (ht * a + hh * (b - f[0][3])); //high band + high thru

    f[0][3] = b; //integrate carrier for HF band

    if(++k & 0x1) //this block runs at half sample rate
    {
      oo = 0.0f;
      aa = a + f[0][9] - f[0][8] - f[0][8];  //apply zeros here instead of in each reson
               f[0][9] = f[0][8];  f[0][8] = a;
      bb = b + f[0][5] - f[0][4] - f[0][4];
               f[0][5] = f[0][4];  f[0][4] = b;

      for(i=1; i<nb; i++) //filter bank: 4th-order band pass
      {
        tmp = f[i][0] * f[i][3] + f[i][1] * f[i][4] + bb;
        f[i][4] = f[i][3];
        f[i][3] = tmp;
        tmp += f[i][2] * f[i][5] + f[i][1] * f[i][6];
        f[i][6] = f[i][5];
        f[i][5] = tmp;

        tmp = f[i][0] * f[i][7] + f[i][1] * f[i][8] + aa;
        f[i][8] = f[i][7];
        f[i][7] = tmp;
        tmp += f[i][2] * f[i][9] + f[i][1] * f[i][10];
        f[i][10] = f[i][9];
        f[i][9] = tmp;

        if(tmp<0.0f) tmp = -tmp;
        f[i][11] -= f[i][12] * (f[i][11] - tmp);
        oo += f[i][5] * f[i][11];
      }
    }
    o += oo * g; //effect of interpolating back up to Fs would be minimal (aliasing >16kHz)

    *++out1 = o;
    *++out2 = o;
  }

  kout = oo;
  kval = k & 0x1;
  if(fabs(f[0][11])<1.0e-10) f[0][11] = 0.0f; //catch HF envelope denormal

  for(i=1;i<nb;i++)
    if(fabs(f[i][3])<1.0e-10 || fabs(f[i][7])<1.0e-10)
      for(k=3; k<12; k++) f[i][k] = 0.0f; //catch reson & envelope denormals

  if(fabs(o)>10.0f) suspend(); //catch instability
}
