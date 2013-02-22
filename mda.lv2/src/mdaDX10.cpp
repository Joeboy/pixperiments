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

#include "mdaDX10.h"

#include "lv2/lv2plug.in/ns/ext/atom/util.h"

#include <stdio.h>
#include <stdlib.h> //rand()
#include <math.h>

AudioEffect *createEffectInstance(audioMasterCallback audioMaster)
{
  return new mdaDX10(audioMaster);
}


mdaDX10::mdaDX10(audioMasterCallback audioMaster) : AudioEffectX(audioMaster, NPROGS, NPARAMS)
{
  int32_t i=0;
  Fs = 44100.0f;

  programs = new mdaDX10Program[NPROGS];
	if(programs)
  {                                //Att     Dec     Rel   | Rat C   Rat F   Att     Dec     Sus     Rel     Vel   | Vib     Oct     Fine    Rich    Thru    LFO
    fillpatch(i++, "Bright E.Piano", 0.000f, 0.650f, 0.441f, 0.842f, 0.329f, 0.230f, 0.800f, 0.050f, 0.800f, 0.900f, 0.000f, 0.500f, 0.500f, 0.447f, 0.000f, 0.414f);
    fillpatch(i++, "Jazz E.Piano",   0.000f, 0.500f, 0.100f, 0.671f, 0.000f, 0.441f, 0.336f, 0.243f, 0.800f, 0.500f, 0.000f, 0.500f, 0.500f, 0.178f, 0.000f, 0.500f);
    fillpatch(i++, "E.Piano Pad",    0.000f, 0.700f, 0.400f, 0.230f, 0.184f, 0.270f, 0.474f, 0.224f, 0.800f, 0.974f, 0.250f, 0.500f, 0.500f, 0.428f, 0.836f, 0.500f);
    fillpatch(i++, "Fuzzy E.Piano",  0.000f, 0.700f, 0.400f, 0.320f, 0.217f, 0.599f, 0.670f, 0.309f, 0.800f, 0.500f, 0.263f, 0.507f, 0.500f, 0.276f, 0.638f, 0.526f);
    fillpatch(i++, "Soft Chimes",    0.400f, 0.600f, 0.650f, 0.760f, 0.000f, 0.390f, 0.250f, 0.160f, 0.900f, 0.500f, 0.362f, 0.500f, 0.500f, 0.401f, 0.296f, 0.493f);
    fillpatch(i++, "Harpsichord",    0.000f, 0.342f, 0.000f, 0.280f, 0.000f, 0.880f, 0.100f, 0.408f, 0.740f, 0.000f, 0.000f, 0.600f, 0.500f, 0.842f, 0.651f, 0.500f);
    fillpatch(i++, "Funk Clav",      0.000f, 0.400f, 0.100f, 0.360f, 0.000f, 0.875f, 0.160f, 0.592f, 0.800f, 0.500f, 0.000f, 0.500f, 0.500f, 0.303f, 0.868f, 0.500f);
    fillpatch(i++, "Sitar",          0.000f, 0.500f, 0.704f, 0.230f, 0.000f, 0.151f, 0.750f, 0.493f, 0.770f, 0.500f, 0.000f, 0.400f, 0.500f, 0.421f, 0.632f, 0.500f);
    fillpatch(i++, "Chiff Organ",    0.600f, 0.990f, 0.400f, 0.320f, 0.283f, 0.570f, 0.300f, 0.050f, 0.240f, 0.500f, 0.138f, 0.500f, 0.500f, 0.283f, 0.822f, 0.500f);
    fillpatch(i++, "Tinkle",         0.000f, 0.500f, 0.650f, 0.368f, 0.651f, 0.395f, 0.550f, 0.257f, 0.900f, 0.500f, 0.300f, 0.800f, 0.500f, 0.000f, 0.414f, 0.500f);
    fillpatch(i++, "Space Pad",      0.000f, 0.700f, 0.520f, 0.230f, 0.197f, 0.520f, 0.720f, 0.280f, 0.730f, 0.500f, 0.250f, 0.500f, 0.500f, 0.336f, 0.428f, 0.500f);
    fillpatch(i++, "Koto",           0.000f, 0.240f, 0.000f, 0.390f, 0.000f, 0.880f, 0.100f, 0.600f, 0.740f, 0.500f, 0.000f, 0.500f, 0.500f, 0.526f, 0.480f, 0.500f);
    fillpatch(i++, "Harp",           0.000f, 0.500f, 0.700f, 0.160f, 0.000f, 0.158f, 0.349f, 0.000f, 0.280f, 0.900f, 0.000f, 0.618f, 0.500f, 0.401f, 0.000f, 0.500f);
    fillpatch(i++, "Jazz Guitar",    0.000f, 0.500f, 0.100f, 0.390f, 0.000f, 0.490f, 0.250f, 0.250f, 0.800f, 0.500f, 0.000f, 0.500f, 0.500f, 0.263f, 0.145f, 0.500f);
    fillpatch(i++, "Steel Drum",     0.000f, 0.300f, 0.507f, 0.480f, 0.730f, 0.000f, 0.100f, 0.303f, 0.730f, 1.000f, 0.000f, 0.600f, 0.500f, 0.579f, 0.000f, 0.500f);
    fillpatch(i++, "Log Drum",       0.000f, 0.300f, 0.500f, 0.320f, 0.000f, 0.467f, 0.079f, 0.158f, 0.500f, 0.500f, 0.000f, 0.400f, 0.500f, 0.151f, 0.020f, 0.500f);
    fillpatch(i++, "Trumpet",        0.000f, 0.990f, 0.100f, 0.230f, 0.000f, 0.000f, 0.200f, 0.450f, 0.800f, 0.000f, 0.112f, 0.600f, 0.500f, 0.711f, 0.000f, 0.401f);
    fillpatch(i++, "Horn",           0.280f, 0.990f, 0.280f, 0.230f, 0.000f, 0.180f, 0.400f, 0.300f, 0.800f, 0.500f, 0.000f, 0.400f, 0.500f, 0.217f, 0.480f, 0.500f);
    fillpatch(i++, "Reed 1",         0.220f, 0.990f, 0.250f, 0.170f, 0.000f, 0.240f, 0.310f, 0.257f, 0.900f, 0.757f, 0.000f, 0.500f, 0.500f, 0.697f, 0.803f, 0.500f);
    fillpatch(i++, "Reed 2",         0.220f, 0.990f, 0.250f, 0.450f, 0.070f, 0.240f, 0.310f, 0.360f, 0.900f, 0.500f, 0.211f, 0.500f, 0.500f, 0.184f, 0.000f, 0.414f);
    fillpatch(i++, "Violin",         0.697f, 0.990f, 0.421f, 0.230f, 0.138f, 0.750f, 0.390f, 0.513f, 0.800f, 0.316f, 0.467f, 0.678f, 0.500f, 0.743f, 0.757f, 0.487f);
    fillpatch(i++, "Chunky Bass",    0.000f, 0.400f, 0.000f, 0.280f, 0.125f, 0.474f, 0.250f, 0.100f, 0.500f, 0.500f, 0.000f, 0.400f, 0.500f, 0.579f, 0.592f, 0.500f);
    fillpatch(i++, "E.Bass",         0.230f, 0.500f, 0.100f, 0.395f, 0.000f, 0.388f, 0.092f, 0.250f, 0.150f, 0.500f, 0.200f, 0.200f, 0.500f, 0.178f, 0.822f, 0.500f);
    fillpatch(i++, "Clunk Bass",     0.000f, 0.600f, 0.400f, 0.230f, 0.000f, 0.450f, 0.320f, 0.050f, 0.900f, 0.500f, 0.000f, 0.200f, 0.500f, 0.520f, 0.105f, 0.500f);
    fillpatch(i++, "Thick Bass",     0.000f, 0.600f, 0.400f, 0.170f, 0.145f, 0.290f, 0.350f, 0.100f, 0.900f, 0.500f, 0.000f, 0.400f, 0.500f, 0.441f, 0.309f, 0.500f);
    fillpatch(i++, "Sine Bass",      0.000f, 0.600f, 0.490f, 0.170f, 0.151f, 0.099f, 0.400f, 0.000f, 0.900f, 0.500f, 0.000f, 0.400f, 0.500f, 0.118f, 0.013f, 0.500f);
    fillpatch(i++, "Square Bass",    0.000f, 0.600f, 0.100f, 0.320f, 0.000f, 0.350f, 0.670f, 0.100f, 0.150f, 0.500f, 0.000f, 0.200f, 0.500f, 0.303f, 0.730f, 0.500f);
    fillpatch(i++, "Upright Bass 1", 0.300f, 0.500f, 0.400f, 0.280f, 0.000f, 0.180f, 0.540f, 0.000f, 0.700f, 0.500f, 0.000f, 0.400f, 0.500f, 0.296f, 0.033f, 0.500f);
    fillpatch(i++, "Upright Bass 2", 0.300f, 0.500f, 0.400f, 0.360f, 0.000f, 0.461f, 0.070f, 0.070f, 0.700f, 0.500f, 0.000f, 0.400f, 0.500f, 0.546f, 0.467f, 0.500f);
    fillpatch(i++, "Harmonics",      0.000f, 0.500f, 0.500f, 0.280f, 0.000f, 0.330f, 0.200f, 0.000f, 0.700f, 0.500f, 0.000f, 0.500f, 0.500f, 0.151f, 0.079f, 0.500f);
    fillpatch(i++, "Scratch",        0.000f, 0.500f, 0.000f, 0.000f, 0.240f, 0.580f, 0.630f, 0.000f, 0.000f, 0.500f, 0.000f, 0.600f, 0.500f, 0.816f, 0.243f, 0.500f);
    fillpatch(i++, "Syn Tom",        0.000f, 0.355f, 0.350f, 0.000f, 0.105f, 0.000f, 0.000f, 0.200f, 0.500f, 0.500f, 0.000f, 0.645f, 0.500f, 1.000f, 0.296f, 0.500f);

    setProgram(0);
  }

  setUniqueID("mdaDX10");

  if(audioMaster)
	{
		setNumInputs(0);
		setNumOutputs(NOUTS);
		canProcessReplacing();
		isSynth();
	}

 	//initialise...
  for(i=0; i<NVOICES; i++)
  {
    voice[i].env = 0.0f;
    voice[i].car = voice[i].dcar = 0.0f;
    voice[i].mod0 = voice[i].mod1 = voice[i].dmod = 0.0f;
    voice[i].cdec = 0.99f; //all notes off
  }
  lfo0 = dlfo = modwhl = 0.0f;
  lfo1 = pbend = 1.0f;
  volume = 0.0035f;
  sustain = activevoices = 0;
  K = 0;

  update();
  suspend();
}


void mdaDX10::update()  //parameter change //if multitimbral would have to move all this...
{
  float ifs = 1.0f / Fs;
  float * param = programs[curProgram].param;

  tune = (float)(8.175798915644 * ifs * pow(2.0, floor(param[11] * 6.9) - 2.0));

  rati = param[3];
  rati = (float)floor(40.1f * rati * rati);
  if(param[4]<0.5f)
    ratf = 0.2f * param[4] * param[4];
  else
    switch((int32_t)(8.9f * param[4]))
    {
      case  4: ratf = 0.25f;       break;
      case  5: ratf = 0.33333333f; break;
      case  6: ratf = 0.50f;       break;
      case  7: ratf = 0.66666667f; break;
      default: ratf = 0.75f;
    }
  ratio = 1.570796326795f * (rati + ratf);

  depth = 0.0002f * param[5] * param[5];
  dept2 = 0.0002f * param[7] * param[7];

  velsens = param[9];
  vibrato = 0.001f * param[10] * param[10];

  catt = 1.0f - (float)exp(-ifs * exp(8.0 - 8.0 * param[0]));
  if(param[1]>0.98f) cdec = 1.0f; else
  cdec =        (float)exp(-ifs * exp(5.0 - 8.0 * param[1]));
  crel =        (float)exp(-ifs * exp(5.0 - 5.0 * param[2]));
  mdec = 1.0f - (float)exp(-ifs * exp(6.0 - 7.0 * param[6]));
  mrel = 1.0f - (float)exp(-ifs * exp(5.0 - 8.0 * param[8]));

  rich = 0.50f - 3.0f * param[13] * param[13];
  //rich = -1.0f + 2 * param[13];
  modmix = 0.25f * param[14] * param[14];
  dlfo = 628.3f * ifs * 25.0f * param[15] * param[15]; //these params not in original DX10
}


void mdaDX10::setSampleRate(float rate)
{
	AudioEffectX::setSampleRate(rate);
  Fs = rate;
}


void mdaDX10::resume()
{
  DECLARE_LVZ_DEPRECATED (wantEvents) ();
  lfo0 = 0.0f;
  lfo1 = 1.0f; //reset LFO phase
}


mdaDX10::~mdaDX10 ()  //destroy any buffers...
{
  if(programs) delete [] programs;
}


void mdaDX10::setProgram(int32_t program)
{
	curProgram = program;
    update();
}


void mdaDX10::setParameter(int32_t index, float value)
{
  programs[curProgram].param[index] = value;
  update();
}


void mdaDX10::fillpatch(int32_t p, const char *name,
                     float p0,  float p1,  float p2,  float p3,  float p4,  float p5,
                     float p6,  float p7,  float p8,  float p9,  float p10, float p11,
                     float p12, float p13, float p14, float p15)
{
  strcpy(programs[p].name, name);
  programs[p].param[0] = p0;    programs[p].param[1] = p1;
  programs[p].param[2] = p2;    programs[p].param[3] = p3;
  programs[p].param[4] = p4;    programs[p].param[5] = p5;
  programs[p].param[6] = p6;    programs[p].param[7] = p7;
  programs[p].param[8] = p8;    programs[p].param[9] = p9;
  programs[p].param[10] = p10;  programs[p].param[11] = p11;
  programs[p].param[12] = p12;  programs[p].param[13] = p13;
  programs[p].param[14] = p14;  programs[p].param[15] = p15;
}


float mdaDX10::getParameter(int32_t index)     { return programs[curProgram].param[index]; }
void  mdaDX10::setProgramName(char *name)   { strcpy(programs[curProgram].name, name); }
void  mdaDX10::getProgramName(char *name)   { strcpy(name, programs[curProgram].name); }
void  mdaDX10::setBlockSize(int32_t blockSize) {	AudioEffectX::setBlockSize(blockSize); }
bool  mdaDX10::getEffectName(char* name)    { strcpy(name, "DX10"); return true; }
bool  mdaDX10::getVendorString(char* text)  {	strcpy(text, "mda"); return true; }
bool  mdaDX10::getProductString(char* text) { strcpy(text, "MDA DX10"); return true; }


bool mdaDX10::getOutputProperties(int32_t index, LvzPinProperties* properties)
{
	if(index<NOUTS)
	{
		sprintf(properties->label, "DX10");
		properties->flags = kLvzPinIsActive;
		if(index<2) properties->flags |= kLvzPinIsStereo; //make channel 1+2 stereo
		return true;
	}
	return false;
}


bool mdaDX10::getProgramNameIndexed(int32_t category, int32_t index, char* text)
{
	if ((unsigned int)index < NPROGS)
	{
		strcpy(text, programs[index].name);
		return true;
	}
	return false;
}


bool mdaDX10::copyProgram(int32_t destination)
{
  if(destination<NPROGS)
  {
    programs[destination] = programs[curProgram];
    return true;
  }
	return false;
}


int32_t mdaDX10::canDo(const char* text)
{
  if(strcmp(text, "receiveLvzEvents") == 0) return 1;
  if(strcmp(text, "receiveLvzMidiEvent") == 0) return 1;
  return -1;
}


void mdaDX10::getParameterName(int32_t index, char *label)
{
	switch (index)
	{
		case  0: strcpy(label, "Attack"); break;
		case  1: strcpy(label, "Decay"); break;
		case  2: strcpy(label, "Release"); break;
		case  3: strcpy(label, "Coarse"); break;
		case  4: strcpy(label, "Fine"); break;
		case  5: strcpy(label, "Mod Init"); break;
		case  6: strcpy(label, "Mod Dec"); break;
		case  7: strcpy(label, "Mod Sus"); break;
		case  8: strcpy(label, "Mod Rel"); break;
		case  9: strcpy(label, "Mod Vel"); break;
 	  case 10: strcpy(label, "Vibrato"); break;
 	  case 11: strcpy(label, "Octave"); break;
 	  case 12: strcpy(label, "FineTune"); break;
 	  case 13: strcpy(label, "Waveform"); break;
 	  case 14: strcpy(label, "Mod Thru"); break;
    default: strcpy(label, "LFO Rate");
	}
}


void mdaDX10::getParameterDisplay(int32_t index, char *text)
{
	char string[16];
	float * param = programs[curProgram].param;

  switch(index)
  {
    case  3: sprintf(string, "%.0f", rati); break;
    case  4: sprintf(string, "%.3f", ratf); break;
    case 11: sprintf(string, "%d", (int32_t)(param[index] * 6.9f) - 3); break;
    case 12: sprintf(string, "%.0f", 200.0f * param[index] - 100.0f); break;
    case 15: sprintf(string, "%.2f", 25.0f * param[index] * param[index]); break;
    default: sprintf(string, "%.0f", 100.0f * param[index]);
  }
	string[8] = 0;
	strcpy(text, (char *)string);
}


void mdaDX10::getParameterLabel(int32_t index, char *label)
{
  switch(index)
  {
    case  3:
    case  4: strcpy(label, "ratio"); break;
    case 11: strcpy(label, ""); break;
    case 12: strcpy(label, "cents"); break;
    case 15: strcpy(label, "Hz"); break;
    default: strcpy(label, "%");
  }
}

void mdaDX10::processReplacing(float **inputs, float **outputs, int32_t sampleFrames)
{
	float* out1 = outputs[0];
	float* out2 = outputs[1];
	int32_t frame=0, frames, v;
  float o, x, e, mw=MW, w=rich, m=modmix;
  int32_t k=K;

  LV2_Atom_Event* ev = lv2_atom_sequence_begin(&eventInput->body);
  bool end = lv2_atom_sequence_is_end(&eventInput->body, eventInput->atom.size, ev);
  if(activevoices>0 || !end) //detect & bypass completely empty blocks
  {
    while(frame<sampleFrames)
    {
      end = lv2_atom_sequence_is_end(&eventInput->body, eventInput->atom.size, ev);
      frames = end ? sampleFrames : ev->time.frames;
      frames -= frame;
      frame += frames;

      while(--frames>=0)  //would be faster with voice loop outside frame loop!
      {                   //but then each voice would need it's own LFO...
        VOICE *V = voice;
        o = 0.0f;

        if(--k<0)
        {
          lfo0 += dlfo * lfo1; //sine LFO
          lfo1 -= dlfo * lfo0;
          mw = lfo1 * (modwhl + vibrato);
          k=100;
        }

        for(v=0; v<NVOICES; v++) //for each voice
        {
          e = V->env;
          if(e > SILENCE) //**** this is the synth ****
          {
            V->env = e * V->cdec; //decay & release
            V->cenv += V->catt * (e - V->cenv); //attack

            x = V->dmod * V->mod0 - V->mod1; //could add more modulator blocks like
            V->mod1 = V->mod0;               //this for a wider range of FM sounds
            V->mod0 = x;
            V->menv += V->mdec * (V->mlev - V->menv);

            x = V->car + V->dcar + x * V->menv + mw; //carrier phase
            while(x >  1.0f) x -= 2.0f;  //wrap phase
            while(x < -1.0f) x += 2.0f;
            V->car = x;
            o += V->cenv * (m * V->mod1 + (x + x * x * x * (w * x * x - 1.0f - w)));
          }      //amp env //mod thru-mix //5th-order sine approximation

         ///  xx = x * x;
         ///  x + x + x * xx * (xx - 3.0f);

          V++;
        }
        *out1++ = o;
        *out2++ = o;
      }

      if(!end) //next note on/off
      {
        processEvent(ev);
        ev = lv2_atom_sequence_next(ev);
      }
    }

    activevoices = NVOICES;
    for(v=0; v<NVOICES; v++)
    {
      if(voice[v].env < SILENCE)  //choke voices that have finished
      {
        voice[v].env = voice[v].cenv = 0.0f;
        activevoices--;
      }
      if(voice[v].menv < SILENCE) voice[v].menv = voice[v].mlev = 0.0f;
    }
  }
  else //completely empty block
  {
		while(--sampleFrames >= 0)
		{
			*out1++ = 0.0f;
			*out2++ = 0.0f;
		}
  }
  K=k; MW=mw; //remember these so vibrato speed not buffer size dependant!
}


void mdaDX10::noteOn(int32_t note, int32_t velocity)
{
  float * param = programs[curProgram].param;
  float l = 1.0f;
  int32_t  v, vl=0;

  if(velocity>0)
  {
    for(v=0; v<NVOICES; v++)  //find quietest voice
    {
      if(voice[v].env<l) { l=voice[v].env;  vl=v; }
    }

    l = (float)exp(0.05776226505f * ((float)note + param[12] + param[12] - 1.0f));
    voice[vl].note = note;                         //fine tuning
    voice[vl].car  = 0.0f;
    voice[vl].dcar = tune * pbend * l; //pitch bend not updated during note as a bit tricky...

    if(l>50.0f) l = 50.0f; //key tracking
    l *= (64.0f + velsens * (velocity - 64)); //vel sens
    voice[vl].menv = depth * l;
    voice[vl].mlev = dept2 * l;
    voice[vl].mdec = mdec;

    voice[vl].dmod = ratio * voice[vl].dcar; //sine oscillator
    voice[vl].mod0 = 0.0f;
    voice[vl].mod1 = (float)sin(voice[vl].dmod);
    voice[vl].dmod = 2.0f * (float)cos(voice[vl].dmod);
                     //scale volume with richness
    voice[vl].env  = (1.5f - param[13]) * volume * (velocity + 10);
    voice[vl].catt = catt;
    voice[vl].cenv = 0.0f;
    voice[vl].cdec = cdec;
  }
  else //note off
  {
    for(v=0; v<NVOICES; v++) if(voice[v].note==note) //any voices playing that note?
    {
      if(sustain==0)
      {
        voice[v].cdec = crel; //release phase
        voice[v].env  = voice[v].cenv;
        voice[v].catt = 1.0f;
        voice[v].mlev = 0.0f;
        voice[v].mdec = mrel;
      }
      else voice[v].note = SUSTAIN;
    }
  }
}


int32_t mdaDX10::processEvent(const LV2_Atom_Event* ev)
{
  if (ev->body.type != midiEventType)
      return 0;

  const uint8_t* midiData = (const uint8_t*)LV2_ATOM_BODY(&ev->body);

    switch(midiData[0] & 0xf0) //status byte (all channels)
    {
      case 0x80: //note off
        noteOn(midiData[1] & 0x7F, 0);
        break;

      case 0x90: //note on
        noteOn(midiData[1] & 0x7F, midiData[2] & 0x7F);
        break;

      case 0xB0: //controller
        switch(midiData[1])
        {
          case 0x01:  //mod wheel
            modwhl = 0.00000005f * (float)(midiData[2] * midiData[2]);
            break;

          case 0x07:  //volume
            volume = 0.00000035f * (float)(midiData[2] * midiData[2]);
            break;

          case 0x40:  //sustain
            sustain = midiData[2] & 0x40;
            if(sustain==0)
            {
              noteOn(SUSTAIN, 0);
            }
            break;

          default:  //all notes off
            if(midiData[1]>0x7A)
            {
              for(int32_t v=0; v<NVOICES; v++) voice[v].cdec=0.99f;
              sustain = 0;
            }
            break;
        }
        break;

      case 0xC0: //program change
        if(midiData[1]<NPROGS) setProgram(midiData[1]);
        break;

      case 0xE0: //pitch bend
        pbend = (float)(midiData[1] + 128 * midiData[2] - 8192);
        if(pbend>0.0f) pbend = 1.0f + 0.000014951f * pbend;
                  else pbend = 1.0f + 0.000013318f * pbend;
        break;

      default: break;
    }

	return 1;
}

