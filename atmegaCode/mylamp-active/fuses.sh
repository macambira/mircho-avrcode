#!/bins/bash
avrdude -p m8 -c usbasp -U lfuse:w:0xd4:m -U hfuse:w:0xd9:m
