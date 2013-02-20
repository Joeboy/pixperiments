#ifndef PITRACKER_LV2_H
#define PITRACKER_LV2_H
#include <lv2/lv2plug.in/ns/lv2core/lv2.h>
#include <lv2/lv2plug.in/ns/ext/atom/forge.h>

LV2_Atom_Forge forge;

LV2_URID_Map lv2_urid_map;

#define LV2_AUDIO_BUFFER_SIZE 0x40
#define LV2_ATOM_BUFFER_SIZE 256

enum lv2_port_type { lv2_audio_port, lv2_atom_port };

typedef struct {
    enum lv2_port_type type;
    uint32_t id;
    void *buffer;
    size_t buffer_sz;
} lv2_port;

lv2_port *new_lv2_port(enum lv2_port_type type, uint32_t id);

typedef struct Lv2Plugin {
    LV2_Descriptor *descriptor;
    LV2_Handle *handle;
    struct Lv2Plugin* next;
} Lv2Plugin;

typedef struct {
    unsigned int sample_rate;
    unsigned int num_plugins;
    const LV2_Feature *lv2_features[3];
    const Lv2Plugin* plugin_list;
} Lv2World;

Lv2World *lv2_init(uint32_t sample_rate);


#endif
