#!/usr/bin/env python

import sys
from time import sleep

PIMIDI_DEV = "/dev/pimidi1"


def test_receive():
    """ Read from the pimidi device and print out any data received """

    f = open(PIMIDI_DEV, "rb")
    print "Listening...."
    while True:
        for c in f.read():
            if ord(c) == 0xfe: continue   # Ignore active sense bytes
            print ord(c)


def test_send():
    """ Write a few note on events to the pimidi device """

    print "Sending Note Ons...."
    f = open(PIMIDI_DEV, "wb")
    for note_no in xrange(0x3c, 0x48, 2):
        for j in (chr(0x90), chr(note_no), chr(0x60)):
            f.write(j)
        sleep(1)
    f.close()


if __name__ == '__main__':
    if len(sys.argv) > 1 and sys.argv[1] == 'send':
        test_send()
    else: test_receive()
