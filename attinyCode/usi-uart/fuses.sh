#!/bin/bash
avrdude -c usbasp -p t26 -U lfuse:w:0xff:m -U hfuse:w:0x17:m
