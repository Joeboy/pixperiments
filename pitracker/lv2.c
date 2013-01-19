#ifndef LV2_C
#define LV2_C

#include <lv2/lv2plug.in/ns/ext/atom/forge.h>

typedef struct urid_map_entry {
    const char* uri;
    uint32_t id;
    struct urid_map_entry* next;
} urid_map_entry;

static urid_map_entry* urid_table;
LV2_URID_Map* lv2_urid_map;
LV2_URID_Unmap* lv2_urid_unmap;
LV2_Atom_Forge forge;

#define MIDI_BUF_SZ 256
uint8_t atom_buffer[MIDI_BUF_SZ];


LV2_URID urid_map_func(LV2_URID_Map_Handle handle, const char* uri) {
    static LV2_URID id = 1;
    urid_map_entry* next;
    if (urid_table == NULL) {
        urid_table = malloc(sizeof(urid_map_entry));
        urid_table->uri = "http://www.joebutton.co.uk/pitracker/urid-map-error";
        urid_table->id = 0;
        urid_table->next = NULL;
    }

    urid_map_entry* cur = urid_table;

    while (1) {
        if (!strcmp(cur->uri, uri)) {
            return cur->id;
        }
        if (cur->next == NULL) {
            next = malloc(sizeof(urid_map_entry));
            next->uri = uri;
            next->id = id++;
            next->next = NULL;
            cur->next = next;
            return next->id;
        } else cur = cur->next;
    }
}


const char *urid_unmap_func(LV2_URID_Unmap_Handle handle, LV2_URID urid) {
    urid_map_entry* cur = urid_table;
    while (1) {
        if (cur->id == urid) {
            return cur->uri;
        }
        if (cur->next == NULL) {
            return NULL;
        }
        cur = cur->next;
    }
}


void lv2_init() {
    urid_table = NULL;
    lv2_urid_map = malloc(sizeof(LV2_URID_Map));
    lv2_urid_unmap = malloc(sizeof(LV2_URID_Unmap));
    lv2_urid_map->handle = NULL; // Not sure what this should be set to..
    lv2_urid_map->map = urid_map_func;
    lv2_urid_unmap->handle = NULL;
    lv2_urid_unmap->unmap = urid_unmap_func;

	lv2_atom_forge_init(&forge, lv2_urid_map);
    lv2_atom_forge_set_buffer(&forge,
	                          (uint8_t*)atom_buffer,
	                          MIDI_BUF_SZ);
}

#endif
