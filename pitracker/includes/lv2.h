#ifndef LV2_H
#define LV2_H

LV2_Atom_Forge forge;

#define MIDI_BUF_SZ 256
uint8_t atom_buffer[MIDI_BUF_SZ];

LV2_URID_Map lv2_urid_map;

const LV2_Feature* lv2_features[3];

LV2_Handle *lv2_handles[3];
LV2_Descriptor *lv2_descriptors[3];

void lv2_init();

#endif
