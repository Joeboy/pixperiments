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

#ifndef __mdaJX10__
#define __mdaJX10__

#include <string.h>

#include "audioeffectx.h"

#define NPARAMS  24      //number of parameters
#define NPROGS   64      //number of programs
#define NOUTS    2       //number of outputs
#define NVOICES  8       //max polyphony
#define SILENCE  0.001f  //voice choking
#define PI       3.1415926535897932f
#define TWOPI    6.2831853071795864f
#define ANALOG   0.002f  //oscillator drift

class mdaJX10Program
{
  friend class mdaJX10;
public:
	mdaJX10Program();
private:
  float param[NPARAMS];
  char  name[24];
};


struct VOICE  //voice state
{
  float  period;
  float  p;    //sinc position
  float  pmax; //loop length
  float  dp;   //delta
  float  sin0; //sine osc
  float  sin1;
  float  sinx;
  float  dc;   //dc offset
  
  float  detune;
  float  p2;    //sinc position
  float  pmax2; //loop length
  float  dp2;   //delta
  float  sin02; //sine osc
  float  sin12;
  float  sinx2;
  float  dc2;   //dc offset
  
  float  fc;  //filter cutoff root
  float  ff;  //filter cutoff
  float  f0;  //filter buffers
  float  f1;
  float  f2;
  
  float  saw;
  //float  vca;  //current level  ///eliminate osc1 level when separate amp & filter envs?
  //float  env;  //envelope
  //float  att;  //attack
  //float  dec;  //decay
  float  env;
  float  envd;
  float  envl;
  float  fenv;
  float  fenvd;
  float  fenvl;
  
  float  lev;  //osc levels
  float  lev2;
  float  target; //period target
  int32_t   note; //remember what note triggered this
};


class mdaJX10 : public AudioEffectX
{
public:
	mdaJX10(audioMasterCallback audioMaster);
	~mdaJX10();

	virtual void processReplacing(float **inputs, float **outputs, int32_t sampleframes);

	virtual void setProgram(int32_t program);
	virtual void setProgramName(char *name);
	virtual void getProgramName(char *name);
	virtual void setParameter(int32_t index, float value);
	virtual float getParameter(int32_t index);
	virtual void getParameterLabel(int32_t index, char *label);
	virtual void getParameterDisplay(int32_t index, char *text);
	virtual void getParameterName(int32_t index, char *text);
	virtual void setSampleRate(float sampleRate);
	virtual void setBlockSize(int32_t blockSize);
  virtual void suspend();
  virtual void resume();

	virtual bool getOutputProperties (int32_t index, LvzPinProperties* properties);
	virtual bool getProgramNameIndexed (int32_t category, int32_t index, char* text);
	virtual bool copyProgram (int32_t destination);
	virtual bool getEffectName (char* name);
	virtual bool getVendorString (char* text);
	virtual bool getProductString (char* text);
	virtual int32_t getVendorVersion () {return 1;}
	virtual int32_t canDo (const char* text);

private:
	int32_t processEvent(const LV2_Atom_Event* ev);
	void update();  //my parameter update
  void noteOn(int32_t note, int32_t velocity);
  void fillpatch(int32_t p, const char *name,
                 float p0,  float p1,  float p2,  float p3,  float p4,  float p5, 
                 float p6,  float p7,  float p8,  float p9,  float p10, float p11,
                 float p12, float p13, float p14, float p15, float p16, float p17, 
                 float p18, float p19, float p20, float p21, float p22, float p23);
  
  mdaJX10Program* programs;
  float Fs;

  #define KMAX 32

  ///global internal variables
  #define SUSTAIN -1
  int32_t sustain, activevoices;
  VOICE voice[NVOICES];

  float semi, cent;
  float tune, detune;
  float filtf, fzip, filtq, filtlfo, filtenv, filtvel, filtwhl;
  float oscmix, noisemix;
  float att, dec, sus, rel, fatt, fdec, fsus, frel;
  float lfo, dlfo, modwhl, press, pbend, ipbend, rezwhl;
  float velsens, volume, voltrim;
  float vibrato, pwmdep, lfoHz, glide, glidedisp;
  int32_t  K, lastnote, veloff, mode;
  unsigned int noise;
};

#endif
