#ifndef MF_READ_H
#define MF_READ_H
#include <lv2/lv2plug.in/ns/ext/atom/forge.h>

void init_midi_source(LV2_Atom_Forge *forge);
void forge_midi_input();
#endif
