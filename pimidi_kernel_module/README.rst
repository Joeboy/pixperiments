This is a Linux kernel module that makes it possible to read/write the
Raspberry Pi's built-in uart at the nonstandard midi baud rate of 31250Hz.

Unfortunately if you ask Linux to talk to a serial port at 31250Hz it
rounds it up to the next "standard" baud rate. Various people have suggested
ways of hacking round that limitation, eg:
http://m0xpd.blogspot.co.uk/2013/01/midi-controller-on-rpi.html

This kernel model arguably provides a more straightforward way to get midi IO
from the pi's uart. It would be much simpler to deploy if it were packaged,
but I don't have time right now. If anybody wants to package it, that would be
very welcome.

Instructions
------------

::

  cd pimidi_kernel_module

  make

  sudo insmod pimidi.ko

  sudo mknod /dev/pimidi u 248 1

  sudo chmod 666 /dev/pimidi

Note that the "248" in the mknod command above may need to be some other number
on your system. If you look in the output of 'dmesg', It should tell you the
correct number.

Anyway, after you've run that lot you should have a midi serial device at
/dev/pimidi. I haven't been able to test writing to it as I don't have any
equipment with a midi in. Please let me know if it works! And if it doesn't,
please fix it!

Midi is transmitted on pin 8 and received on pin 10. You need to convert
between the pi's 3.3v IO and MIDI's 5v, normally via an optocoupler.
