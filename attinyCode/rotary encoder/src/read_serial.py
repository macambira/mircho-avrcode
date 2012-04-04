#!/usr/bin/env python
#
# Simple python program to read serial port.
# FreeBSD version

import sys
import serial

# Main
serport = serial.Serial(
    port="/dev/ttyu0",
    baudrate=1200,
    bytesize=serial.EIGHTBITS,
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_ONE,
    timeout=0.1,
    xonxoff=0,
    rtscts=0,
    interCharTimeout=None
)

# Read serial port
while True:
    c = serport.read()
    if ord(c) < ord(' '):
	print "%02x" % ord(c[0])
    sys.stdout.write(c)
#    if len(c) == 0:
#	break

