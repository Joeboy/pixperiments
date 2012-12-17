Baremetal chiptunes on the raspberry pi
=======================================

This is a simple chiptune type player that runs directly on the rpi hardware,
without the need for an OS. It mostly serves the purpose of allowing me to
relive my misspent childhood, but as far as I know it's also the first working
baremetal audio code with source for the pi so hopefully it'll help somebody
else get started with bigger and better things.

Many thanks to dwelch for https://github.com/dwelch67/raspberrypi , which was
invaluable in getting started.

Things to do:
-------------
 * Buffering would probably help eliminate the unpleasant crackling.
 * Under linux, the pwm seems to be driven from a much faster clock ('plla')
   than the one I'm using, which would allow for much better quality audio. I
   can't work out how to make it work with plla though.
 * Maybe write or import some actual music rather than the current nonsense
 * Implement a less rubbish synth engine.
 * Read MIDI input. We have uarts so shouldn't be too hard. I predict the
   main challenge will be digging out the obsolescent hardware.
 * Play MIDI files from the SD card.
 * Implement LV2 plugins.
