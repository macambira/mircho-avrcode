#PRG            = oenc_adc
PRG            = oenc_bin
OBJ            = $(PRG).o
MCU_TARGET     = attiny13
# -O2 increases binary file
OPTIMIZE       = -O1
#OPTIMIZE       = -Os

DEFS           = -D__ATtiny13__
LIBS           =

CC            = avr-gcc
#CFLAGS        = -g -Wall $(OPTIMIZE) -mmcu=$(MCU_TARGET) $(DEFS)
CFLAGS        = -Wall $(OPTIMIZE) -mmcu=$(MCU_TARGET) $(DEFS)
LDFLAGS       = -Wl,-Map,$(PRG).map
OBJCOPY        = avr-objcopy
OBJDUMP        = avr-objdump


all: $(PRG).hex $(PRG).bin

clean:
#	rm -f *.o
	rm -f *.o $(PRG).elf $(PRG).map \
		$(PRG).bin $(PRG).hex

backup:
	tar cvzf ../backup/$(PRG).tgz .

asm:
	$(CC) $(CFLAGS) -S oenc.c -o x.s

expanded:
	$(CC) $(CFLAGS) -E oenc.c -o x

disasm:
	avr-objdump -d $(PRG).elf

#main.o:	main.c 

$(PRG).hex: $(PRG).elf
	$(OBJCOPY) -j .text -j .data -O ihex $(PRG).elf $(PRG).hex

$(PRG).bin: $(PRG).elf
	$(OBJCOPY) -j .text -j .data -O binary $(PRG).elf $(PRG).bin

$(PRG).elf:	$(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(PRG).elf $(OBJ)
	./checksize $(PRG).elf

