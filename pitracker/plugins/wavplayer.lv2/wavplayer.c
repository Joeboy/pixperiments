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

extern uint8_t _binary_sample_wav_start;

static LV2_Descriptor *synthDescriptor = NULL;

enum voice_state {on, off};

typedef struct  WAV_HEADER {
     uint8_t       RIFF[4];        /* RIFF Header      */ //Magic header
     uint32_t      ChunkSize;      /* RIFF Chunk Size  */
     uint8_t       WAVE[4];        /* WAVE Header      */
     uint8_t       fmt[4];         /* FMT header       */
     uint32_t      Subchunk1Size;  /* Size of the fmt chunk                                */
     uint16_t      AudioFormat;    /* Audio format 1=PCM,6=mulaw,7=alaw, 257=IBM Mu-Law, 258=IBM A-Law, 259=ADPCM */
     uint16_t      NumOfChan;      /* Number of channels 1=Mono 2=Sterio                   */
     uint32_t      SamplesPerSec;  /* Sampling Frequency in Hz                             */
     uint32_t      bytesPerSec;    /* bytes per second */
     uint16_t      blockAlign;     /* 2=16-bit mono, 4=16-bit stereo */
     uint16_t      bitsPerSample;  /* Number of bits per sample      */
     uint8_t       Subchunk2ID[4]; /* "data"  string   */
     uint32_t      Subchunk2Size;  /* Sampled data length    */
}wav_hdr;

typedef struct {
    enum voice_state state;
    uint32_t note_no;
    float freq;
    uint32_t time;
} voice;


static voice voices[NUM_VOICES];

typedef struct {
    double sample_rate;
    LV2_Atom_Sequence* midi_in;
    float *output_left;
    float *output_right;
    LV2_URID midi_Event;
    int16_t *audiodata;
    uint32_t audiodata_len;
} Plugin;


static LV2_Handle instantiate(const LV2_Descriptor *descriptor,
        double s_rate, const char *path, const LV2_Feature * const* features) {

    Plugin *plugin = (Plugin *)malloc(sizeof(Plugin));
    for (int i=0;i<NUM_VOICES;i++) voices[i].state = off;
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

    wav_hdr *hdr = (wav_hdr*)&_binary_sample_wav_start;
    plugin->audiodata = (int16_t*)(hdr + 1);
    plugin->audiodata_len = hdr->Subchunk2Size / sizeof(int16_t);

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
    float multipliers[12] = { 1.0, 1.05946309436, 1.12246204831, 1.189207115, 1.25992104989,
                              1.33483985417, 1.41421356237, 1.49830707688, 1.58740105197,
                              1.68179283051, 1.78179743628, 1.88774862536 };
    float multiplier, freq;
    int octave = note_no/12;
    multiplier = multipliers[note_no - 12*octave];
    freq = 440.0 / 32.0;
    while (octave-- > 0) freq *= 2;
    return freq * multiplier / 8;
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
//            voices[i].state = off;
        }
    }
}

static float waveform(Plugin *plugin, voice *v, double sample_rate) {
    float base_freq = 110; // Guess
    float time_shift = v->freq / base_freq;
    int index = time_shift * v->time;
    if (index > plugin->audiodata_len) {
        v->state = off;
        return 0;
    } else return (float)plugin->audiodata[index] / 32768.0 / 4;
}


static void run(LV2_Handle instance, uint32_t sample_count) {
    static uint32_t t=0;
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
//                dump_int_hex(msg[2]); // velocity
                break;
            case LV2_MIDI_MSG_NOTE_OFF:
                note_off(msg[1]);
                break;
            default:
                break;
            }
        }
    }

    for (unsigned int i=0;i<sample_count;i++) {
        out=0;
        for (int v=0;v<NUM_VOICES;v++) {
            if (voices[v].state == off) continue;
            out += VOICE_CLAMPER * waveform(plugin, &voices[v], plugin->sample_rate);
            voices[v].time++;
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

        synthDescriptor->URI = "http://www.joebutton.co.uk/software/pitracker/plugins/wavplayer";
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

