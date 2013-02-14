A baremetal midi file player and lv2 plugin host for the raspberry pi
=====================================================================

This is a simple midi file player that runs directly on the rpi hardware,
without the need for an OS. It mostly serves the purpose of allowing me to
relive my misspent childhood, but as far as I know it's also the first working
baremetal audio code with source for the pi so hopefully it'll help somebody
else get started with bigger and better things.

The synth engine is implemented as an LV2 plugin, although it departs from the
spec in that it's statically linked and doesn't use .ttl files due to the lack
of a working filesystem. Somebody on the rpi baremetal forum was supposed to
be working on the filesystem stuff, so those things might change when they and
I get our shit together.

Many thanks to David Welch for https://github.com/dwelch67/raspberrypi , which
was invaluable in getting started, to Remo Dentato for his midi file code, and
to Dave Robillard for his LV2 work.

To make it go, just copy the files in the SD_Card directory onto a blank SD
card and stick it in your Pi, and stick some headphones in the Pi's headphone
socket.

Things to do:
-------------
 * Read hardware MIDI input. We have uarts so shouldn't be too hard. I predict
   the main challenge will be digging out the obsolescent hardware.
 * Implement LV2 plugins via dynamic linkage, so you can just drop plugins
   onto the SD.
 * Implement reading files from the SD card, so that the previous two things
   become possible.
 * Port to other platforms, eg. TI Stellaris. Make Linux port work properly.
 * Unbodge stereo / sample rate.
 * Implement more midi stuff like pitch bending, channels etc.
 * Effects plugins.

