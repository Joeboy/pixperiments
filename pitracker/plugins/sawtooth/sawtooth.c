#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <malloc.h>
#include <math.h>

#include <lv2/lv2plug.in/ns/lv2core/lv2.h>
#include <lv2/lv2plug.in/ns/ext/atom/util.h>
#include <lv2/lv2plug.in/ns/ext/midi/midi.h>
#include <lv2/lv2plug.in/ns/ext/urid/urid.h>

#define NUM_VOICES 6
#define VOICE_CLAMPER  (float)1/NUM_VOICES

#define OUTPUT_LEFT 1
#define OUTPUT_RIGHT 2
#define MIDI_IN 3

static LV2_Descriptor *synthDescriptor = NULL;

enum voice_state {on, released, off};

typedef struct {
    enum voice_state state;
    uint32_t note_no;
    float freq;
    float env;
    uint32_t time;
    uint32_t released_time;
} voice;


static voice voices[NUM_VOICES];

typedef struct {
    double sample_rate;
    LV2_Atom_Sequence* midi_in;
    float *output_left;
    float *output_right;
    LV2_URID midi_Event;
} Plugin;


static LV2_Handle instantiate(const LV2_Descriptor *descriptor,
        double s_rate, const char *path, const LV2_Feature * const* features) {

    Plugin *plugin = (Plugin *)malloc(sizeof(Plugin));
    uint32_t i;
    for (i=0;i<NUM_VOICES;i++) voices[i].state = off;
    plugin->sample_rate = s_rate;
    LV2_URID_Map *map = NULL;

    for (int i =0; features[i]; i++) {
        if (!strcmp(features[i]->URI, LV2_URID__map)) {
            map = (LV2_URID_Map*)features[i]->data;
        }
    }
    if (map == NULL) {
        printf("Error: Host does not support map feature\r\n");
    }

    plugin->midi_Event = map->map(NULL, LV2_MIDI__MidiEvent);

    return (LV2_Handle)plugin;
}

static void cleanup(LV2_Handle instance) {
    free(instance);
}

static void connect_port(LV2_Handle instance, uint32_t port, void *data) {
    Plugin *plugin = (Plugin *)instance;

    switch (port) {
    case MIDI_IN:
        plugin->midi_in = data;
        break;
    case OUTPUT_LEFT:
        plugin->output_left = data;
        break;
    case OUTPUT_RIGHT:
        plugin->output_right = data;
        break;
    default:
        break;
    }
}

static float noteno2freq(uint32_t note_no) {
    // This was simpler than implementing pow()
    float multipliers[12] = { 1.0, 1.05946309436, 1.12246204831, 1.189207115, 1.25992104989,
                              1.33483985417, 1.41421356237, 1.49830707688, 1.58740105197,
                              1.68179283051, 1.78179743628, 1.88774862536 };
    float multiplier, freq;
    uint32_t octave = note_no/12;
    multiplier = multipliers[note_no - 12*octave];
    freq = 440.0 / 32.0;
    while (octave-- > 0) freq *= 2;
    return freq * multiplier;
}


static void note_on(uint32_t note_no) {
    // find a voice that's not on, or otherwise the voice that was
    // started longest ago
    uint32_t i;
    uint32_t best_voice = 0;
    uint32_t best_voice_age = 0;
    for (i=1;i<NUM_VOICES;i++) {
        if (voices[i].state == off) {
            best_voice = i;
            break;
        }
        if (voices[i].time > best_voice_age) {
            best_voice_age = voices[i].time;
            best_voice = i;
        }
    }
    voices[best_voice].state = on;
    voices[best_voice].time = 0;
    voices[best_voice].note_no = note_no;
    voices[best_voice].freq = noteno2freq(note_no);
}

static void note_off(uint32_t note_no) {
    uint32_t i;
    for (i=0;i<NUM_VOICES;i++) {
        if (voices[i].note_no == note_no) {
            voices[i].state = released;
            voices[i].released_time = 0;
        }
    }
}

static float envelope(voice *vp) {
    float env;
    uint32_t attack_time = 1000;
    float attack = 0.5;
    uint32_t decay_time = 5000;
    float sustain = 0.4;
    uint32_t release_time = 3000;
    voice v = *vp;

    if (v.state == on) {
        if (v.time < attack_time) env = attack*((float)(v.time) / attack_time);
        else if (v.time < (attack_time + decay_time)) env = (float)attack - (float)(attack - sustain) * (float)(((float)v.time - attack_time) / (float)decay_time);
        else env = sustain;
        (*vp).env = env;
    } else if (v.state == released) {
        if (v.released_time > release_time) {
            vp->state = off;
            env = 0;
        } else {
            // Ramp down from whatever the last envelope value was (not necessarily the sustain value)
            env = (float)v.env - (float)v.env*(float)((float)v.released_time / (float)release_time);
        }
    } else env = 0; // should never happen
    return env;
}

static float waveform(voice v, double sample_rate) {
    static int32_t vibrato=1;
    static uint32_t vib_dir=0;
    uint32_t vibrato_period = 2000;
    float vibrato_depth = 0.01;
    float freq;
    freq = v.freq * (1.0 + vibrato_depth * (float)((float)vibrato - (float)vibrato_period/2.0)/(float)vibrato_period);
    vibrato += (vib_dir ? -1 : 1);
    if (vibrato > vibrato_period || vibrato ==0) vib_dir = !vib_dir;
    return sawtooth((256 * (uint32_t)freq * 2 * v.time) / sample_rate);
}

static void run(LV2_Handle instance, uint32_t sample_count) {
    static uint32_t t=1; // 0 gets optimized out
    uint32_t i;
    uint32_t v;
    float out;

    Plugin *plugin = (Plugin *)instance;

    LV2_ATOM_SEQUENCE_FOREACH(plugin->midi_in, ev) {
        if (!ev->body.type) continue;
        if (ev->body.type == plugin->midi_Event) {
//            TODO: use ev->time.frames;
            const uint8_t* const msg = (const uint8_t*)(ev + 1);
            switch (lv2_midi_message_type(msg)) {
            case LV2_MIDI_MSG_NOTE_ON:
                note_on(msg[1]);
                break;
            case LV2_MIDI_MSG_NOTE_OFF:
                note_off(msg[1]);
                break;
            default:
                break;
            }
        }
    }

    for (i=0;i<sample_count;i++) {
        out=0;
        for (v=0;v<NUM_VOICES;v++) {
            if (voices[v].state == off) continue;
            out += VOICE_CLAMPER * envelope(&(voices[v])) * waveform(voices[v], plugin->sample_rate);

            voices[v].time++;
            if (voices[v].state == released) voices[v].released_time++;
        }
        plugin->output_left[i] = out;
        plugin->output_right[i] = out;
        t++;
    }
}


LV2_SYMBOL_EXPORT
const LV2_Descriptor *lv2_descriptor(uint32_t index)
{
    if (!synthDescriptor) {
        synthDescriptor = (LV2_Descriptor *)malloc(sizeof(LV2_Descriptor));

        synthDescriptor->URI = "http://www.joebutton.co.uk/software/pitracker/plugins/sawtooth";
        synthDescriptor->activate = NULL;
        synthDescriptor->cleanup = cleanup;
        synthDescriptor->connect_port = connect_port;
        synthDescriptor->deactivate = NULL;
        synthDescriptor->instantiate = instantiate;
        synthDescriptor->run = run;
        synthDescriptor->extension_data = NULL;
    }

    if (index == 0) return synthDescriptor;
    return NULL;
}

