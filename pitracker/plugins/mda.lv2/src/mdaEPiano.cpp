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

#include "mdaEPianoData.h"
#include "mdaEPiano.h"

#include "lv2/lv2plug.in/ns/ext/atom/util.h"

#include <stdio.h>
#include <math.h>

//#include "AEffEditor.hpp" ////for GUI

AudioEffect *createEffectInstance(audioMasterCallback audioMaster)
{
  return new mdaEPiano(audioMaster);
}

mdaEPiano::mdaEPiano(audioMasterCallback audioMaster) : AudioEffectX(audioMaster, NPROGS, NPARAMS)
{
	Fs = 44100.0f;  iFs = 1.0f/Fs;  //just in case...

  programs = new mdaEPianoProgram[NPROGS];
	if(programs)
  {
    //fill patches...
    int32_t i=0;
    fillpatch(i++, "Default", 0.500f, 0.500f, 0.500f, 0.500f, 0.500f, 0.650f, 0.250f, 0.500f, 0.50f, 0.500f, 0.146f, 0.000f);
    fillpatch(i++, "Bright", 0.500f, 0.500f, 1.000f, 0.800f, 0.500f, 0.650f, 0.250f, 0.500f, 0.50f, 0.500f, 0.146f, 0.500f);
    fillpatch(i++, "Mellow", 0.500f, 0.500f, 0.000f, 0.000f, 0.500f, 0.650f, 0.250f, 0.500f, 0.50f, 0.500f, 0.246f, 0.000f);
    fillpatch(i++, "Autopan", 0.500f, 0.500f, 0.500f, 0.500f, 0.250f, 0.650f, 0.250f, 0.500f, 0.50f, 0.500f, 0.246f, 0.000f);
    fillpatch(i++, "Tremolo", 0.500f, 0.500f, 0.500f, 0.500f, 0.750f, 0.650f, 0.250f, 0.500f, 0.50f, 0.500f, 0.246f, 0.000f);
    setProgram(0);
  }

  setUniqueID("mdaEPiano");

  if(audioMaster)
	{
		setNumInputs(0);
		setNumOutputs(NOUTS);
		canProcessReplacing();
		isSynth();
	}

  waves = epianoData;

  //Waveform data and keymapping
  kgrp[ 0].root = 36;  kgrp[ 0].high = 39; //C1
  kgrp[ 3].root = 43;  kgrp[ 3].high = 45; //G1
  kgrp[ 6].root = 48;  kgrp[ 6].high = 51; //C2
  kgrp[ 9].root = 55;  kgrp[ 9].high = 57; //G2
  kgrp[12].root = 60;  kgrp[12].high = 63; //C3
  kgrp[15].root = 67;  kgrp[15].high = 69; //G3
  kgrp[18].root = 72;  kgrp[18].high = 75; //C4
  kgrp[21].root = 79;  kgrp[21].high = 81; //G4
  kgrp[24].root = 84;  kgrp[24].high = 87; //C5
  kgrp[27].root = 91;  kgrp[27].high = 93; //G5
  kgrp[30].root = 96;  kgrp[30].high =999; //C6

  kgrp[0].pos = 0;        kgrp[0].end = 8476;     kgrp[0].loop = 4400;
  kgrp[1].pos = 8477;     kgrp[1].end = 16248;    kgrp[1].loop = 4903;
  kgrp[2].pos = 16249;    kgrp[2].end = 34565;    kgrp[2].loop = 6398;
  kgrp[3].pos = 34566;    kgrp[3].end = 41384;    kgrp[3].loop = 3938;
  kgrp[4].pos = 41385;    kgrp[4].end = 45760;    kgrp[4].loop = 1633; //was 1636;
  kgrp[5].pos = 45761;    kgrp[5].end = 65211;    kgrp[5].loop = 5245;
  kgrp[6].pos = 65212;    kgrp[6].end = 72897;    kgrp[6].loop = 2937;
  kgrp[7].pos = 72898;    kgrp[7].end = 78626;    kgrp[7].loop = 2203; //was 2204;
  kgrp[8].pos = 78627;    kgrp[8].end = 100387;   kgrp[8].loop = 6368;
  kgrp[9].pos = 100388;   kgrp[9].end = 116297;   kgrp[9].loop = 10452;
  kgrp[10].pos = 116298;  kgrp[10].end = 127661;  kgrp[10].loop = 5217; //was 5220;
  kgrp[11].pos = 127662;  kgrp[11].end = 144113;  kgrp[11].loop = 3099;
  kgrp[12].pos = 144114;  kgrp[12].end = 152863;  kgrp[12].loop = 4284;
  kgrp[13].pos = 152864;  kgrp[13].end = 173107;  kgrp[13].loop = 3916;
  kgrp[14].pos = 173108;  kgrp[14].end = 192734;  kgrp[14].loop = 2937;
  kgrp[15].pos = 192735;  kgrp[15].end = 204598;  kgrp[15].loop = 4732;
  kgrp[16].pos = 204599;  kgrp[16].end = 218995;  kgrp[16].loop = 4733;
  kgrp[17].pos = 218996;  kgrp[17].end = 233801;  kgrp[17].loop = 2285;
  kgrp[18].pos = 233802;  kgrp[18].end = 248011;  kgrp[18].loop = 4098;
  kgrp[19].pos = 248012;  kgrp[19].end = 265287;  kgrp[19].loop = 4099;
  kgrp[20].pos = 265288;  kgrp[20].end = 282255;  kgrp[20].loop = 3609;
  kgrp[21].pos = 282256;  kgrp[21].end = 293776;  kgrp[21].loop = 2446;
  kgrp[22].pos = 293777;  kgrp[22].end = 312566;  kgrp[22].loop = 6278;
  kgrp[23].pos = 312567;  kgrp[23].end = 330200;  kgrp[23].loop = 2283;
  kgrp[24].pos = 330201;  kgrp[24].end = 348889;  kgrp[24].loop = 2689;
  kgrp[25].pos = 348890;  kgrp[25].end = 365675;  kgrp[25].loop = 4370;
  kgrp[26].pos = 365676;  kgrp[26].end = 383661;  kgrp[26].loop = 5225;
  kgrp[27].pos = 383662;  kgrp[27].end = 393372;  kgrp[27].loop = 2811;
  kgrp[28].pos = 383662;  kgrp[28].end = 393372;  kgrp[28].loop = 2811; //ghost
  kgrp[29].pos = 393373;  kgrp[29].end = 406045;  kgrp[29].loop = 4522;
  kgrp[30].pos = 406046;  kgrp[30].end = 414486;  kgrp[30].loop = 2306;
  kgrp[31].pos = 406046;  kgrp[31].end = 414486;  kgrp[31].loop = 2306; //ghost
  kgrp[32].pos = 414487;  kgrp[32].end = 422408;  kgrp[32].loop = 2169;

  //extra xfade looping...
  for(int32_t k=0; k<28; k++)
  {
    int32_t p0 = kgrp[k].end;
    int32_t p1 = kgrp[k].end - kgrp[k].loop;

    float xf = 1.0f;
    float dxf = -0.02f;

    while(xf > 0.0f)
    {
      waves[p0] = (short)((1.0f - xf) * (float)waves[p0] + xf * (float)waves[p1]);
      p0--;
      p1--;
      xf += dxf;
    }
  }

  //initialise...
  for(int32_t v=0; v<NVOICES; v++) 
  {
    voice[v].env = 0.0f;
    voice[v].dec = 0.99f; //all notes off
  }
  volume = 0.2f;
  muff = 160.0f;
  sustain = activevoices = 0;
  tl = tr = lfo0 = dlfo = 0.0f;
  lfo1 = 1.0f;

  guiUpdate = 0;

  update();
	suspend();
}

void mdaEPiano::update()  //parameter change
{
  float * param = programs[curProgram].param;
  size = (int32_t)(12.0f * param[2] - 6.0f);

  treb = 4.0f * param[3] * param[3] - 1.0f; //treble gain
  if(param[3] > 0.5f) tfrq = 14000.0f; else tfrq = 5000.0f; //treble freq
  tfrq = 1.0f - (float)exp(-iFs * tfrq);

  rmod = lmod = param[4] + param[4] - 1.0f; //lfo depth
  if(param[4] < 0.5f) rmod = -rmod;

  dlfo = 6.283f * iFs * (float)exp(6.22f * param[5] - 2.61f); //lfo rate

  velsens = 1.0f + param[6] + param[6];
  if(param[6] < 0.25f) velsens -= 0.75f - 3.0f * param[6];

  width = 0.03f * param[7];
  poly = 1 + (int32_t)(31.9f * param[8]);
  fine = param[9] - 0.5f;
  random = 0.077f * param[10] * param[10];
  stretch = 0.0f; //0.000434f * (param[11] - 0.5f); parameter re-used for overdrive!
  overdrive = 1.8f * param[11];
}


void mdaEPiano::resume()
{
  Fs = getSampleRate();
  iFs = 1.0f / Fs;
  dlfo = 6.283f * iFs * (float)exp(6.22f * programs[curProgram].param[5] - 2.61f); //lfo rate

  DECLARE_LVZ_DEPRECATED (wantEvents) ();
}


mdaEPiano::~mdaEPiano ()  //destroy any buffers...
{
  if(programs) delete [] programs;
}


void mdaEPiano::setProgram(int32_t program)
{
	curProgram = program;
    update();
}


void mdaEPiano::setParameter(int32_t index, float value)
{
  programs[curProgram].param[index] = value;
  update();

  //if(editor) editor->postUpdate(); ///For GUI

  guiUpdate = index + 0x100 + (guiUpdate & 0xFFFF00);
}


void mdaEPiano::fillpatch(int32_t p, const char *name, float p0, float p1, float p2, float p3, float p4,
                      float p5, float p6, float p7, float p8, float p9, float p10,float p11)
{
  strcpy(programs[p].name, name);
  programs[p].param[0] = p0;  programs[p].param[1] = p1;
  programs[p].param[2] = p2;  programs[p].param[3] = p3;
  programs[p].param[4] = p4;  programs[p].param[5] = p5;
  programs[p].param[6] = p6;  programs[p].param[7] = p7;
  programs[p].param[8] = p8;  programs[p].param[9] = p9;
  programs[p].param[10]= p10; programs[p].param[11] = p11;
}


float mdaEPiano::getParameter(int32_t index)     { return programs[curProgram].param[index]; }
void  mdaEPiano::setProgramName(char *name)   { strcpy(programs[curProgram].name, name); }
void  mdaEPiano::getProgramName(char *name)   { strcpy(name, programs[curProgram].name); }
void  mdaEPiano::setBlockSize(int32_t blockSize) {	AudioEffectX::setBlockSize(blockSize); }
bool  mdaEPiano::getEffectName(char* name)    { strcpy(name, "ePiano"); return true; }
bool  mdaEPiano::getVendorString(char* text)  {	strcpy(text, "mda"); return true; }
bool  mdaEPiano::getProductString(char* text) { strcpy(text, "MDA ePiano"); return true; }


bool mdaEPiano::getOutputProperties(int32_t index, LvzPinProperties* properties)
{
	if(index<NOUTS)
	{
		if(index) sprintf(properties->label, "ePiano");
         else sprintf(properties->label, "ePiano");
		properties->flags = kLvzPinIsActive;
		if(index<2) properties->flags |= kLvzPinIsStereo; //make channel 1+2 stereo
		return true;
	}
	return false;
}


bool mdaEPiano::getProgramNameIndexed(int32_t category, int32_t index, char* text)
{
	if ((unsigned int)index < NPROGS)
	{
		strcpy(text, programs[index].name);
		return true;
	}
	return false;
}


bool mdaEPiano::copyProgram(int32_t destination)
{
  if(destination<NPROGS)
  {
    programs[destination] = programs[curProgram];
    return true;
  }
	return false;
}


int32_t mdaEPiano::canDo(const char* text)
{
  if(strcmp(text, "receiveLvzEvents") == 0) return 1;
  if(strcmp(text, "receiveLvzMidiEvent") == 0) return 1;
  return -1;
}


void mdaEPiano::getParameterName(int32_t index, char *label)
{
	switch (index)
	{
		case  0: strcpy(label, "Envelope Decay"); break;
		case  1: strcpy(label, "Envelope Release"); break;
		case  2: strcpy(label, "Hardness"); break;

    case  3: strcpy(label, "Treble Boost"); break;
		case  4: strcpy(label, "Modulation"); break;
		case  5: strcpy(label, "LFO Rate"); break;

    case  6: strcpy(label, "Velocity Sense"); break;
    case  7: strcpy(label, "Stereo Width"); break;
    case  8: strcpy(label, "Polyphony"); break;

    case  9: strcpy(label, "Fine Tuning"); break;
		case 10: strcpy(label, "Random Tuning"); break;
		default: strcpy(label, "Overdrive"); break;
	}
}


void mdaEPiano::getParameterDisplay(int32_t index, char *text)
{
	char string[16];
	float * param = programs[curProgram].param;

  switch(index)
  {
    case  2:
    case  3:
    case  9: sprintf(string, "%.0f", 100.0f * param[index] - 50.0f); break;

    case  4: if(param[index] > 0.5f)
               sprintf(string, "Trem %.0f", 200.0f * param[index] - 100.0f);
             else
               sprintf(string, "Pan %.0f", 100.0f - 200.0f * param[index]); break;

    case  5: sprintf(string, "%.2f", (float)exp(6.22f * param[5] - 2.61f)); break; //LFO Hz
    case  7: sprintf(string, "%.0f", 200.0f * param[index]); break;
    case  8: sprintf(string, "%d", poly); break;
    case 10: sprintf(string, "%.1f",  50.0f * param[index] * param[index]); break;
    case 11: sprintf(string, "%.1f", 100.0f * param[index]); break;
    default: sprintf(string, "%.0f", 100.0f * param[index]);
  }
	string[8] = 0;
	strcpy(text, (char *)string);
}


void mdaEPiano::getParameterLabel(int32_t index, char *label)
{
  switch(index)
  {
    case  5: strcpy(label, "Hz"); break;
    case  8: strcpy(label, "voices"); break;
    case  9:
    case 10: strcpy(label, "cents"); break;
    default: strcpy(label, "%");
  }
}


void mdaEPiano::guiGetDisplay(int32_t index, char *label)
{
  getParameterName(index,  label);
  strcat(label, " = ");
  getParameterDisplay(index, label + strlen(label));
  getParameterLabel(index, label + strlen(label));
}


void mdaEPiano::processReplacing(float **inputs, float **outputs, int32_t sampleFrames)
{
	float* out0 = outputs[0];
	float* out1 = outputs[1];
	int32_t frame=0, frames, v;
  float x, l, r, od=overdrive;
  int32_t i;

  LV2_Atom_Event* ev = lv2_atom_sequence_begin(&eventInput->body);
  while(frame<sampleFrames)
  {
    bool end = lv2_atom_sequence_is_end(&eventInput->body, eventInput->atom.size, ev);
    frames = end ? sampleFrames : ev->time.frames;
    frames -= frame;
    frame += frames;

    while(--frames>=0)
    {
      VOICE *V = voice;
      l = r = 0.0f;

      for(v=0; v<activevoices; v++)
      {
        V->frac += V->delta;  //integer-based linear interpolation
        V->pos += V->frac >> 16;
        V->frac &= 0xFFFF;
        if(V->pos > V->end) V->pos -= V->loop;
        //i = waves[V->pos];
        //i = (i << 7) + (V->frac >> 9) * (waves[V->pos + 1] - i) + 0x40400000;  //not working on intel mac !?!
		//x = V->env * (*(float *)&i - 3.0f);  //fast int->float
  //x = V->env * (float)i / 32768.0f;
  i = waves[V->pos] + ((V->frac * (waves[V->pos + 1] - waves[V->pos])) >> 16);
  x = V->env * (float)i / 32768.0f;

		V->env = V->env * V->dec;  //envelope

        if(x>0.0f) { x -= od * x * x;  if(x < -V->env) x = -V->env; } //+= 0.5f * x * x; } //overdrive

        l += V->outl * x;
        r += V->outr * x;

        V++;
      }
      tl += tfrq * (l - tl);  //treble boost
      tr += tfrq * (r - tr);
      r  += treb * (r - tr);
      l  += treb * (l - tl);

      lfo0 += dlfo * lfo1;  //LFO for tremolo and autopan
      lfo1 -= dlfo * lfo0;
      l += l * lmod * lfo1;
      r += r * rmod * lfo1;  //worth making all these local variables?

      *out0++ = l;
      *out1++ = r;
    }

    if(frame<sampleFrames)
    {
      if(activevoices == 0 && programs[curProgram].param[4] > 0.5f) 
        { lfo0 = -0.7071f;  lfo1 = 0.7071f; } //reset LFO phase - good idea?

      if (!end) {
        processEvent(ev);
        ev = lv2_atom_sequence_next(ev);
      }

    }
  }
  if(fabs(tl)<1.0e-10) tl = 0.0f; //anti-denormal
  if(fabs(tr)<1.0e-10) tr = 0.0f;

  for(v=0; v<activevoices; v++) if(voice[v].env < SILENCE) voice[v] = voice[--activevoices];
}


void mdaEPiano::noteOn(int32_t note, int32_t velocity)
{
  float * param = programs[curProgram].param;
  float l=99.0f;
  int32_t  v, vl=0, k, s;

  if(velocity > 0)
  {
    if(activevoices < poly) //add a note
    {
      vl = activevoices;
      activevoices++;
      voice[vl].f0 = voice[vl].f1 = 0.0f;
    }
    else //steal a note
    {
      for(v=0; v<poly; v++)  //find quietest voice
      {
        if(voice[v].env < l) { l = voice[v].env;  vl = v; }
      }
    }

    k = (note - 60) * (note - 60);
    l = fine + random * ((float)(k % 13) - 6.5f);  //random & fine tune
    if(note > 60) l += stretch * (float)k; //stretch

    s = size;
    //if(velocity > 40) s += (int32_t)(sizevel * (float)(velocity - 40));  - no velocity to hardness in ePiano

    k = 0;
    while(note > (kgrp[k].high + s)) k += 3;  //find keygroup
    l += (float)(note - kgrp[k].root); //pitch
    l = 32000.0f * iFs * (float)exp(0.05776226505 * l);
    voice[vl].delta = (int32_t)(65536.0f * l);
    voice[vl].frac = 0;

    if(velocity > 48) k++; //mid velocity sample
    if(velocity > 80) k++; //high velocity sample
    voice[vl].pos = kgrp[k].pos;
    voice[vl].end = kgrp[k].end - 1;
    voice[vl].loop = kgrp[k].loop;

    voice[vl].env = (3.0f + 2.0f * velsens) * (float)pow(0.0078f * velocity, velsens); //velocity

    if(note > 60) voice[vl].env *= (float)exp(0.01f * (float)(60 - note)); //new! high notes quieter

    l = 50.0f + param[4] * param[4] * muff + muffvel * (float)(velocity - 64); //muffle
    if(l < (55.0f + 0.4f * (float)note)) l = 55.0f + 0.4f * (float)note;
    if(l > 210.0f) l = 210.0f;
    voice[vl].ff = l * l * iFs;

    voice[vl].note = note; //note->pan
    if(note <  12) note = 12;
    if(note > 108) note = 108;
    l = volume;
    voice[vl].outr = l + l * width * (float)(note - 60);
    voice[vl].outl = l + l - voice[vl].outr;

    if(note < 44) note = 44; //limit max decay length
    voice[vl].dec = (float)exp(-iFs * exp(-1.0 + 0.03 * (double)note - 2.0f * param[0]));
  }
  else //note off
  {
    for(v=0; v<NVOICES; v++) if(voice[v].note==note) //any voices playing that note?
    {
      if(sustain==0)
      {
        voice[v].dec = (float)exp(-iFs * exp(6.0 + 0.01 * (double)note - 5.0 * param[1]));
      }
      else voice[v].note = SUSTAIN;
    }
  }
}


int32_t mdaEPiano::processEvent(const LV2_Atom_Event* ev)
{
  float * param = programs[curProgram].param;

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
            modwhl = 0.0078f * (float)(midiData[2]);
            if(modwhl > 0.05f) //over-ride pan/trem depth
            {
              rmod = lmod = modwhl; //lfo depth
              if(param[4] < 0.5f) rmod = -rmod;
            }
            break;


          case 0x07:  //volume
            volume = 0.00002f * (float)(midiData[2] * midiData[2]);
            break;

          case 0x40:  //sustain pedal
          case 0x42:  //sustenuto pedal
            sustain = midiData[2] & 0x40;
            if(sustain==0)
            {
              noteOn(SUSTAIN, 0); //end all sustained notes  
            }
            break;

          default:  //all notes off
            if(midiData[1]>0x7A)
            {
              for(int32_t v=0; v<NVOICES; v++) voice[v].dec=0.99f;
              sustain = 0;
              muff = 160.0f;
            }
            break;
        }
        break;

      case 0xC0: //program change
        if(midiData[1]<NPROGS) setProgram(midiData[1]);
        break;

      default: break;
    }

	return 1;
}

