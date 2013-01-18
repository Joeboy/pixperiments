Baremetal chiptunes on the raspberry pi
=======================================

This is a simple chiptune type player that runs directly on the rpi hardware,
without the need for an OS. It mostly serves the purpose of allowing me to
relive my misspent childhood, but as far as I know it's also the first working
baremetal audio code with source for the pi so hopefully it'll help somebody
else get started with bigger and better things.

It uses LV2 for the synth engine.

Many thanks to dwelch for https://github.com/dwelch67/raspberrypi , which was
invaluable in getting started, and to Remo Dentato for his midi file code.

Things to do:
-------------
 * Under linux, the pwm seems to be driven from a much faster clock ('plla')
   than the one I'm using, which would allow for much better quality audio. I
   can't work out how to make it work with plla though.
 * Read MIDI input. We have uarts so shouldn't be too hard. I predict the
   main challenge will be digging out the obsolescent hardware.
 * Play MIDI files from the SD card, rather than linking them into the kernel.
 * Implement LV2 plugins via dynamic linkage, so you can just drop a plugin
   onto the SD.
 * Implement reading files from the SD card, so that the previous two things
   become possible.
 * Port to other platforms, eg. TI Stellaris
