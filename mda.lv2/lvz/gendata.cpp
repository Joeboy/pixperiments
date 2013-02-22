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

#include <cassert>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <string>

#include <dlfcn.h>

#include "audioeffectx.h"

using namespace std;

// VST is so incredibly awful.  Just.. wow.
#define MAX_NAME_LENGTH 1024
char name_buf[MAX_NAME_LENGTH];

struct Record {
	Record(const string& n) : base_name(n) {}
	string base_name;
	typedef list<string> UIs;
	UIs uis;
};

typedef std::map<string, Record> Manifest;
Manifest manifest;

string
symbolify(const char* name, char space_char='_')
{
	string str(name);

	// Like This -> Like_This
	for (size_t i=0; i < str.length(); ++i)
		if (str[i] == ' ')
			str[i] = space_char;

	str[0] = std::tolower(str[0]);

	// LikeThis -> like_this
	for (size_t i=1; i < str.length(); ++i)
		if (str[i] >= 'A' && str[i] <= 'Z'
				&& str[i-1] >= 'a' && str[i-1] <= 'z'
				&& ((i == str.length() - 1) || (str[i+1] <= 'a' && str[i+1] >= 'Z'))
				&& (!(str[i-1] == 'd' && str[i] == 'B'))
				&& (!(str[i-1] == 'F' && str[i] == 'X'))
				&& (!(str[i-1] == 'D' && str[i] == 'C')))
			str = str.substr(0, i) + space_char + str.substr(i);

	// To lowercase, and skip invalids
	for (size_t i=1; i < str.length(); ) {
		if (std::isalpha(str[i]) || std::isdigit(str[i])) {
			str[i] = std::tolower(str[i]);
			++i;
		} else if (str[i-1] != space_char) {
			str[i] = space_char;
			++i;
		} else {
			str = str.substr(0, i) + str.substr(i+1);
		}
	}

	return str;
}

void
write_plugin(AudioEffectX* effect, const string& lib_file_name)
{
	const string base_name = lib_file_name.substr(0, lib_file_name.find_last_of("."));
	const string data_file_name = base_name + ".ttl";

	fstream os(data_file_name.c_str(), ios::out);
	effect->getProductString(name_buf);

	os << "@prefix lv2: <http://lv2plug.in/ns/lv2core#> ." << endl;
	os << "@prefix doap: <http://usefulinc.com/ns/doap#> ." << endl << endl;
	os << "<" << effect->getURI() << ">" << endl;
	os << "\tlv2:symbol \"" << effect->getUniqueID() << "\" ;" << endl;
	os << "\tdoap:name \"" << name_buf << "\" ;" << endl;
	os << "\tdoap:license <http://usefulinc.com/doap/licenses/gpl> ;" << endl;
	os << "\tlv2:pluginProperty lv2:hardRTCapable";

	uint32_t num_params     = effect->getNumParameters();
	uint32_t num_audio_ins  = effect->getNumInputs();
	uint32_t num_audio_outs = effect->getNumOutputs();
	uint32_t num_ports      = num_params + num_audio_ins + num_audio_outs;

	if (num_ports > 0)
		os << " ;" << endl << "\tlv2:port [" << endl;
	else
		os << " ." << endl;

	uint32_t idx = 0;

	for (uint32_t i = idx; i < num_params; ++i, ++idx) {
		effect->getParameterName(i, name_buf);
		os << "\t\ta lv2:InputPort, lv2:ControlPort ;" << endl;
		os << "\t\tlv2:index" << " " << idx << " ;" << endl;
		os << "\t\tlv2:name \"" << name_buf << "\" ;" << endl;
		os << "\t\tlv2:symbol \"" << symbolify(name_buf) << "\" ;" << endl;
		os << "\t\tlv2:default " << effect->getParameter(i) << " ;" << endl;
		os << "\t\tlv2:minimum 0.0 ;" << endl;
		os << "\t\tlv2:maximum 1.0 ;" << endl;
		os << ((idx == num_ports - 1) ? "\t] ." : "\t] , [") << endl;
	}

	for (uint32_t i = 0; i < num_audio_ins; ++i, ++idx) {
		os << "\t\ta lv2:InputPort, lv2:AudioPort ;" << endl;
		os << "\t\tlv2:index" << " " << idx << " ;" << endl;
		os << "\t\tlv2:symbol \"in" << i+1 << "\" ;" << endl;
		os << "\t\tlv2:name \"Input " << i+1 << "\" ;" << endl;
		os << ((idx == num_ports - 1) ? "\t] ." : "\t] , [") << endl;
	}

	for (uint32_t i = 0; i < num_audio_outs; ++i, ++idx) {
		os << "\t\ta lv2:OutputPort, lv2:AudioPort ;" << endl;
		os << "\t\tlv2:index " << idx << " ;" << endl;
		os << "\t\tlv2:symbol \"out" << i+1 << "\" ;" << endl;
		os << "\t\tlv2:name \"Output " << i+1 << "\" ;" << endl;
		os << ((idx == num_ports - 1) ? "\t] ." : "\t] , [") << endl;
	}

	os.close();

	Manifest::iterator i = manifest.find(effect->getURI());
	if (i != manifest.end()) {
		i->second.base_name = base_name;
	} else {
		manifest.insert(std::make_pair(effect->getURI(), Record(base_name)));
	}

	if (effect->getNumPrograms() > 1) {
		std::string preset_file = base_name + "-presets.ttl";

		fstream pos(preset_file.c_str(), ios::out);
		pos << "@prefix lv2: <http://lv2plug.in/ns/lv2core#> ." << endl;
		pos << "@prefix pset: <http://lv2plug.in/ns/ext/presets#> ." << endl;
		pos << "@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> ." << endl << endl;
		for (int32_t i = 0; i < effect->getNumPrograms(); ++i) {
			effect->setProgram(i);
			effect->getProgramName(name_buf);

			std::string preset_uri = string(effect->getURI())
				+ "#pset-" + symbolify(name_buf, '-');
			
			// Write wanifest entry
			std::cout << "<" << preset_uri << ">"
			          << "\n\ta pset:Preset ;\n\tlv2:appliesTo <"
			          << effect->getURI() << "> ;\n\t"
			          << "rdfs:seeAlso <" << preset_file << "> .\n" << std::endl;

			// Write preset file
			pos << "<" << preset_uri << ">"
			    << "\n\ta pset:Preset ;\n\tlv2:appliesTo <"
			    << effect->getURI() << "> ;\n\t"
			    << "rdfs:label \"" << name_buf << "\"";
			for (uint32_t i = 0; i < num_params; ++i) {
				effect->getParameterName(i, name_buf);
				pos << " ;\n\tlv2:port [" << endl;
				pos << "\t\tlv2:symbol \"" << symbolify(name_buf) << "\" ;" << endl;
				pos << "\t\tpset:value " << effect->getParameter(i) << " ;" << endl;
				pos << "\t]";
			}
			pos << " .\n" << endl;
		}
		pos.close();
	}
}

void
write_manifest(ostream& os)
{
	os << "@prefix lv2: <http://lv2plug.in/ns/lv2core#> ." << endl;
	os << "@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> ." << endl;
	os << "@prefix uiext: <http://lv2plug.in/ns/extensions/ui#> ." << endl << endl;
	for (Manifest::iterator i = manifest.begin(); i != manifest.end(); ++i) {
		Record& r = i->second;
		os << "<" << i->first << ">\n\ta lv2:Plugin ;" << endl;
		os << "\trdfs:seeAlso <" << r.base_name << ".ttl> ;" << endl;
		os << "\tlv2:binary <" << r.base_name << ".so> ";
		for (Record::UIs::iterator j = r.uis.begin(); j != r.uis.end(); ++j)
			os << ";" << endl << "\tuiext:ui <" << *j << "> ";
		os << "." << endl << endl;
	}
}

int
main(int argc, char** argv)
{
	if (argc == 0) {
		cout << "Usage: gendata [PLUGINLIB1] [PLUGINLIB2]..." << endl;
		cout << "Each argument is a path to a LVZ plugin library." << endl;
		cout << "For each library an LV2 data file with the same name" << endl;
		cout << "will be output containing the data for that plugin." << endl;
		cout << "A manifest of the plugins found is written to stdout" << endl;
		return 1;
	}

	typedef AudioEffectX* (*new_effect_func)();
	typedef AudioEffectX* (*plugin_uri_func)();

	new_effect_func constructor = NULL;
	AudioEffectX*   effect      = NULL;

	for (int i = 1; i < argc; ++i) {
		void* handle = dlopen(argv[i], RTLD_LAZY);
		if (handle == NULL) {
			cerr << "ERROR: " << argv[i] << ": " << dlerror() << " (ignoring)" << endl;
			continue;
		}

		string lib_path = argv[i];
		size_t last_slash = lib_path.find_last_of("/");
		if (last_slash != string::npos)
			lib_path = lib_path.substr(last_slash + 1);

		constructor = (new_effect_func)dlsym(handle, "lvz_new_audioeffectx");
		if (constructor != NULL) {
			effect = constructor();
			write_plugin(effect, lib_path);
		}

		if (constructor == NULL) {
			cerr << "ERROR: " << argv[i] << ": not an LVZ plugin library, ignoring" << endl;
		}

		dlclose(handle);
	}

	write_manifest(cout);

	return 0;
}

