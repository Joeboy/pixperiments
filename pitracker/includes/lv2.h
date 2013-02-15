#ifndef PITRACKER_LV2_H
#define PITRACKER_LV2_H
#include <lv2/lv2plug.in/ns/lv2core/lv2.h>

LV2_Atom_Forge forge;

LV2_URID_Map lv2_urid_map;

#define MAX_FEATURES 3
const LV2_Feature* lv2_features[MAX_FEATURES];

#define MAX_PLUGINS 20
LV2_Descriptor *lv2_descriptors[MAX_PLUGINS];
LV2_Handle *lv2_handles[MAX_PLUGINS];

void lv2_init();

#endif
