A baremetal midi file player and lv2 plugin host for the raspberry pi
=====================================================================

This is an LV2 audio plugin host that runs directly on the rpi hardware,
without the need for an OS. At present it can host synth plugins, and play midi
input from either a standard MIDI file or from a hardware MIDI device connected
to the pi's UART.

The synth engine is implemented as an LV2 plugin, although it departs from the
spec in that it's statically linked and doesn't use .ttl files due to the lack
of a working filesystem at the time I wrote the code. Now I have a working
filesystem I will try to write a dynamic linker (I may fail).

Many thanks to David Welch for https://github.com/dwelch67/raspberrypi , which
was invaluable in getting started, and to Dave Robillard for his LV2 work.
Thanks to John Cronin and Arjan Van Vught for the SD card reading code.

To make it go, just copy the files in the SD_Card directory onto a blank SD
card and stick it in your Pi, and stick some headphones in the Pi's headphone
socket. If you get bored of Fur Elise, replace the tune.mid file on the card
with another midi file.

Things to do:
-------------
 * Implement LV2 plugins via dynamic linkage, so you can just drop plugins
   onto the SD.
 * Port to other platforms, eg. TI Stellaris. Make Linux port work properly.
 * Unbodge stereo / sample rate / timing.
 * Implement more midi stuff like pitch bending, channels etc.
 * Effects plugins.

