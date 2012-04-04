TARGET		= usi_uart_project
VPATH		= .

CSRC		= main.c
CSRC		+= USI_UART.c


#common dir
#COMMON_DIR 	= ../common
COMMON_DIR 	= .
CSRC		+= hd44780.c 

ASRC		=
#ASRC		+= $(COMMON_DIR)/hd44780/adelay.S


# List any extra directories to look for include files here.
#     Each directory must be seperated by a space.
EXTRAINCDIRS 	= $(COMMON_DIR)

MCU			= attiny26
OPTIMIZE	= -Os -mcall-prologues 
DEFS		= -DF_CPU=3686400UL -DMODE=1
LIBS		= 
DEBUG		= dwarf-2

CC		= avr-gcc


ASFLAGS		= -Wa,-adhlns=$(<:.S=.lst),-gstabs $(DEFS)
ALL_ASFLAGS	= -mmcu=$(MCU) -I. -x assembler-with-cpp $(ASFLAGS)

CFLAGS		= -Wall $(OPTIMIZE) -mmcu=$(MCU) $(DEFS)
CFLAGS		+= -fdata-sections
CFLAGS		+= -ffunction-sections
#CFLAGS		+= -fno-inline-small-functions
#CFLAGS		+= --combine
#CFLAGS		+= -fwhole-program
CFLAGS		+= -ffreestanding
CFLAGS		+= -fpack-struct
CFLAGS		+= -Wa,-adhlns=$(<:.c=.lst) $(patsubst %,-I%,$(EXTRAINCDIRS))

#CFLAGS		+= -g$(DEBUG) 

LDFLAGS		= -Wl,-Map,$(TARGET).map
#LDFLAGS		+= --gc-sections
LDFLAGS		+= -Wl,--relax
#LDFLAGS		+= -Wl,-I$(EXTRAINCDIRS)/hd44780/
OBJ		= $(CSRC:.c=.o) $(ASRC:.S=.o)

OBJCOPY		= avr-objcopy
OBJDUMP		= avr-objdump
SIZE		= avr-size

AVRDUDE		= avrdude

HEXSIZE = $(SIZE) --target=ihex $(TARGET).hex
ELFSIZE = $(SIZE) -A $(TARGET).elf

# Programming support using avrdude. Settings and variables.
AVRDUDE_PROGRAMMER = usbasp    # official name of
AVRDUDE_PORT = lpt1		       # windows

AVRDUDE_WRITE_FLASH = -U flash:w:$(TARGET).hex
#AVRDUDE_WRITE_EEPROM = -U eeprom:w:$(TARGET).eep

AVRDUDE_FLAGS = -p $(MCU) -P $(AVRDUDE_PORT) -c $(AVRDUDE_PROGRAMMER) -F

# Uncomment the following if you do /not/ wish a verification to be
# performed after programming the device.
#AVRDUDE_FLAGS += -V

# Increase verbosity level.  Please use this when submitting bug
# reports about avrdude. See <http://savannah.nongnu.org/projects/avrdude>
# to submit bug reports.
#AVRDUDE_FLAGS += -v -v

#Run while cable attached or don't
AVRDUDE_FLAGS += -E reset #keep chip disabled while cable attached
#AVRDUDE_FLAGS += -E noreset
# ---------------------------------------------------------------------------


all: clean $(TARGET).elf lst text size

#all:	clean
#	@echo $(OBJ) 

$(TARGET).elf: $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)
	
clean:
	rm -rf *.o $(TARGET).elf *.eps *.eep *.bak *.a
	rm -rf *.lst *.map $(EXTRA_CLEAN_FILES)
	rm -rf $(TARGET).hex

size: $(TARGET).elf
	$(ELFSIZE)
	$(HEXSIZE)
#	$(SIZE) -C --mcu=$(MCU) $(TARGET).elf

# Program the device.
#program: $(TARGET).hex $(TARGET).eep
program: all
	$(AVRDUDE) $(AVRDUDE_FLAGS) $(AVRDUDE_WRITE_FLASH) $(AVRDUDE_WRITE_EEPROM)	

lst:  $(TARGET).lst
%.lst: %.elf
	$(OBJDUMP) -s -l -h -S $< > $@

%.o : %.S
	$(CC) -c $(ALL_ASFLAGS) $< -o $@

text: hex eep
hex:  $(TARGET).hex
eep:  $(TARGET).eep

%.hex: %.elf
	$(OBJCOPY) -j .text -j .data -O ihex -R eeprom $< $@ 
	
%.eep: %.elf
	$(OBJCOPY) -j .eeprom --set-section-flags=.eeprom="alloc,load" \
	--change-section-lma .eeprom=0 -O ihex $< $@

# Compile: create assembler files from C source files.
%.s : %.c
	$(CC) -S -fverbose-asm $(ALL_CFLAGS) $< -o $@ 
   
	
# Automatically generate C source code dependencies.
# (Code originally taken from the GNU make user manual and modified
# (See README.txt Credits).)
#
# Note that this will work with sh (bash) and sed that is shipped with WinAVR
# (see the SHELL variable defined above).
# This may not work with other shells or other seds.
#
%.d: %.c
	set -e; $(CC) -MM $(ALL_CFLAGS) $< \
	| sed 's,\(.*\)\.o[ :]*,\1.o \1.d : ,g' > $@; \
	[ -s $@ ] || rm -f $@


# Remove the '-' if you want to see the dependency files generated.
-include $(SRC:.c=.d)
