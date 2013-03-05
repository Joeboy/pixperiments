/*
  LVZ - An ugly C++ interface for writing LV2 plugins.
  Copyright 2008-2012 David Robillard <http://drobilla.net>

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

#ifndef LVZ_AUDIOEFFECTX_H
#define LVZ_AUDIOEFFECTX_H

#include <stdint.h>
#include <string.h>

#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"

class AudioEffect;

typedef int (*audioMasterCallback)(int, int ver, int, int, int, int);

AudioEffect* createEffectInstance(audioMasterCallback audioMaster);

enum LvzPinFlags {
	kLvzPinIsActive = 1<<0,
	kLvzPinIsStereo = 1<<1
};

struct LvzPinProperties {
	LvzPinProperties() : label(NULL), flags(0) {}
	char* label;
	int   flags;
};

enum LvzEventTypes {
	kLvzMidiType = 0
};

struct LvzEvent {
	int type;
};

struct LvzMidiEvent : public LvzEvent {
	char*   midiData;
	int32_t deltaFrames;
};

struct LvzEvents {
	int32_t    numEvents;
	LvzEvent** events;
};

#define DECLARE_LVZ_DEPRECATED(x) x

class AudioEffect {
public:
	AudioEffect() {}
	virtual ~AudioEffect() {}

	virtual void  setParameter(int32_t index, float value) = 0;
	virtual void  setParameterAutomated(int32_t index, float value) {}
	virtual float getParameter(int32_t index)              = 0;

	virtual void masterIdle() {}
};

class AudioEffectX : public AudioEffect {
public:
	AudioEffectX(audioMasterCallback audioMaster, int32_t progs, int32_t params)
		: URI("NIL")
		, uniqueID("NIL")
		, eventInput(NULL)
		, sampleRate(44100)
		, curProgram(0)
		, numInputs(0)
		, numOutputs(0)
		, numParams(params)
		, numPrograms(progs)
	{
	}

	virtual void process         (float **inputs, float **outputs, int32_t nframes) {}
	virtual void processReplacing(float **inputs, float **outputs, int32_t nframes) = 0;

	void setMidiEventType(LV2_URID urid) { midiEventType = urid; }
	void setEventInput(const LV2_Atom_Sequence* seq) { eventInput = seq; }

	virtual int32_t processEvents(LvzEvents* ev) { return 0; }

	virtual const char* getURI()           { return URI; }
	virtual const char* getUniqueID()      { return uniqueID; }
	virtual float       getSampleRate()    { return sampleRate; }
	virtual int32_t     getNumInputs()     { return numInputs; }
	virtual int32_t     getNumOutputs()    { return numOutputs; }
	virtual int32_t     getNumParameters() { return numParams; }
	virtual int32_t     getNumPrograms()   { return numPrograms; }

	virtual void getParameterName(int32_t index, char *label) = 0;
	virtual bool getProductString(char* text)                 = 0;
	virtual void getProgramName(char *name) { name[0] = '\0'; }

	virtual int32_t canDo(const char* text) { return false; }
	virtual bool    canHostDo(const char* act) { return false; }
	virtual void    canMono()                  {}
	virtual void    canProcessReplacing()      {}
	virtual void    isSynth()                  {}
	virtual void    wantEvents()               {}

	virtual void setBlockSize(int32_t size)  {}
	virtual void setNumInputs(int32_t num)   { numInputs = num; }
	virtual void setNumOutputs(int32_t num)  { numOutputs = num; }
	virtual void setSampleRate(float rate)   { sampleRate = rate; }
	virtual void setProgram(int32_t prog)    { curProgram = prog; }
	virtual void setURI(const char* uri)     { URI = uri; }
	virtual void setUniqueID(const char* id) { uniqueID = id; }
	virtual void suspend()                   {}
	virtual void beginEdit(int32_t index)    {}
	virtual void endEdit(int32_t index)      {}

	virtual long dispatcher(long opCode, long index, long value, void *ptr, float opt) {
		return 0;
	}

protected:
	const char*              URI;
	const char*              uniqueID;
	const LV2_Atom_Sequence* eventInput;
	LV2_URID                 midiEventType;
	float                    sampleRate;
	int32_t                  curProgram;
	int32_t                  numInputs;
	int32_t                  numOutputs;
	int32_t                  numParams;
	int32_t                  numPrograms;
};

extern "C" {
AudioEffectX* lvz_new_audioeffectx();
}

#endif // LVZ_AUDIOEFFECTX_H

