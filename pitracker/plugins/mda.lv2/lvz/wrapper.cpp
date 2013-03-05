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

#ifndef PLUGIN_CLASS
#error "This file requires PLUGIN_CLASS to be defined"
#endif
#ifndef URI_PREFIX
#error "This file requires URI_PREFIX to be defined"
#endif
#ifndef PLUGIN_URI_SUFFIX
#error "This file requires PLUGIN_URI_SUFFIX to be defined"
#endif
#ifndef PLUGIN_HEADER
#error "This file requires PLUGIN_HEADER to be defined"
#endif

#include <stdlib.h>
#include "audioeffectx.h"
#include "lv2.h"
#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/midi/midi.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include PLUGIN_HEADER

extern "C" {

/* Plugin */

typedef struct {
	PLUGIN_CLASS* effect;
	float*        controls;
	float**       control_buffers;
	float**       inputs;
	float**       outputs;
} LVZPlugin;

static void
lvz_cleanup(LV2_Handle instance)
{
	LVZPlugin* plugin = (LVZPlugin*)instance;
	free(plugin->controls);
	free(plugin->control_buffers);
	free(plugin->inputs);
	free(plugin->outputs);
	delete plugin->effect;
	free(plugin);
}

static void
lvz_connect_port(LV2_Handle instance, uint32_t port, void* data)
{
	LVZPlugin*     plugin      = (LVZPlugin*)instance;
	const uint32_t num_params  = plugin->effect->getNumParameters();
	const uint32_t num_inputs  = plugin->effect->getNumInputs();
	const uint32_t num_outputs = plugin->effect->getNumOutputs();

	if (port < num_params) {
		plugin->control_buffers[port] = (float*)data;
	} else if (port < num_params + num_inputs) {
		plugin->inputs[port - num_params] = (float*)data;
	} else if (port < num_params + num_inputs + num_outputs) {
		plugin->outputs[port - num_params - num_inputs] = (float*)data;
	} else if (port == num_params + num_inputs + num_outputs) {
		plugin->effect->setEventInput((LV2_Atom_Sequence*)data);
	}
}

static int
master_callback(int, int ver, int, int, int, int)
{
	return 0;
}

static LV2_Handle
lvz_instantiate(const LV2_Descriptor*    descriptor,
                double                   rate,
                const char*              bundle_path,
                const LV2_Feature*const* features)
{
	PLUGIN_CLASS* effect = new PLUGIN_CLASS(master_callback);
	effect->setURI(URI_PREFIX PLUGIN_URI_SUFFIX);
	effect->setSampleRate(rate);

	uint32_t num_params  = effect->getNumParameters();
	uint32_t num_inputs  = effect->getNumInputs();
	uint32_t num_outputs = effect->getNumOutputs();

	LVZPlugin* plugin = (LVZPlugin*)malloc(sizeof(LVZPlugin));
	plugin->effect = effect;

	for (int i = 0; features[i]; ++i) {
		if (!strcmp(features[i]->URI, LV2_URID__map)) {
			LV2_URID_Map* map = (LV2_URID_Map*)features[i]->data;
			plugin->effect->setMidiEventType(
				map->map(map->handle, LV2_MIDI__MidiEvent));
			break;
		}
	}

	if (num_params > 0) {
		plugin->controls        = (float*)malloc(sizeof(float) * num_params);
		plugin->control_buffers = (float**)malloc(sizeof(float*) * num_params);
		for (uint32_t i = 0; i < num_params; ++i) {
			plugin->controls[i] = effect->getParameter(i);
			plugin->control_buffers[i] = NULL;
		}
	} else {
		plugin->controls        = NULL;
		plugin->control_buffers = NULL;
	}

	if (num_inputs > 0) {
		plugin->inputs = (float**)malloc(sizeof(float*) * num_inputs);
		for (uint32_t i = 0; i < num_inputs; ++i) {
			plugin->inputs[i] = NULL;
		}
	} else {
		plugin->inputs = NULL;
	}

	if (num_outputs > 0) {
		plugin->outputs = (float**)malloc(sizeof(float*) * num_outputs);
		for (uint32_t i = 0; i < num_outputs; ++i) {
			plugin->outputs[i] = NULL;
		}
	} else {
		plugin->outputs = NULL;
	}

	return (LV2_Handle)plugin;
}

static void
lvz_run(LV2_Handle instance, uint32_t sample_count)
{
	LVZPlugin* plugin = (LVZPlugin*)instance;

	for (int32_t i = 0; i < plugin->effect->getNumParameters(); ++i) {
		float val = plugin->control_buffers[i][0];
		if (val != plugin->controls[i]) {
			plugin->effect->setParameter(i, val);
			plugin->controls[i] = val;
		}
	}

	plugin->effect->processReplacing(plugin->inputs, plugin->outputs, sample_count);
}

static const void*
lvz_extension_data(const char* uri)
{
	return NULL;
}

static void
lvz_deactivate(LV2_Handle instance)
{
	LVZPlugin* plugin = (LVZPlugin*)instance;
	plugin->effect->suspend();
}

/* Library */

static LV2_Descriptor descriptor;
static bool           initialised = false;

LV2_SYMBOL_EXPORT
const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
	if (!initialised) {
		descriptor.URI            = URI_PREFIX PLUGIN_URI_SUFFIX;
		descriptor.instantiate    = lvz_instantiate;
		descriptor.connect_port   = lvz_connect_port;
		descriptor.activate       = NULL;
		descriptor.run            = lvz_run;
		descriptor.deactivate     = lvz_deactivate;
		descriptor.cleanup        = lvz_cleanup;
		descriptor.extension_data = lvz_extension_data;
		initialised = true;
	}

	switch (index) {
	case 0:
		return &descriptor;
	default:
		return NULL;
	}
}

/** Entry point for LVZ gendata */
LV2_SYMBOL_EXPORT
AudioEffectX*
lvz_new_audioeffectx()
{
	PLUGIN_CLASS* effect = new PLUGIN_CLASS(master_callback);
	effect->setURI(URI_PREFIX PLUGIN_URI_SUFFIX);
	return effect;
}

} // extern "C"
