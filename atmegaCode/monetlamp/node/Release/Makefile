################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init
SHELL := cmd.exe
RM := rm -rf

USER_OBJS :=

LIBS := 
PROJ := 

O_SRCS := 
C_SRCS := 
S_SRCS := 
S_UPPER_SRCS := 
OBJ_SRCS := 
ASM_SRCS := 
PREPROCESSING_SRCS := 
OBJS := 
OBJS_AS_ARGS := 
C_DEPS := 
C_DEPS_AS_ARGS := 
EXECUTABLES := 
OUTPUT_FILE_PATH :=
OUTPUT_FILE_PATH_AS_ARGS :=
HEX_FLASH_FILE_PATH :=
HEX_FLASH_FILE_PATH_AS_ARGS :=
HEX_EEPROM_FILE_PATH :=
HEX_EEPROM_FILE_PATH_AS_ARGS :=
LSS_FILE_PATH :=
LSS_FILE_PATH_AS_ARGS :=
MAP_FILE_PATH :=
MAP_FILE_PATH_AS_ARGS :=
AVR_APP_PATH :=C:/Program Files/Atmel/AVR Studio 5.0/extensions/Application/Avr ToolChain/bin/
QUOTE := "
ADDITIONAL_DEPENDENCIES:=

# Every subdirectory with source files must be described here
SUBDIRS := 


# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS +=  \
../event.c \
../eventcodes.c \
../monetlamp.c \
../spi.c \
../spilcd.c \
../tick.c


PREPROCESSING_SRCS += 


ASM_SRCS += 


OBJS +=  \
event.o \
eventcodes.o \
monetlamp.o \
spi.o \
spilcd.o \
tick.o


OBJS_AS_ARGS +=  \
event.o \
eventcodes.o \
monetlamp.o \
spi.o \
spilcd.o \
tick.o


C_DEPS +=  \
event.d \
eventcodes.d \
monetlamp.d \
spi.d \
spilcd.d \
tick.d


C_DEPS_AS_ARGS +=  \
event.d \
eventcodes.d \
monetlamp.d \
spi.d \
spilcd.d \
tick.d


OUTPUT_FILE_PATH +=monetlamp.elf

OUTPUT_FILE_PATH_AS_ARGS +=monetlamp.elf

HEX_FLASH_FILE_PATH +=monetlamp.hex

HEX_FLASH_FILE_PATH_AS_ARGS +=monetlamp.hex

HEX_EEPROM_FILE_PATH +=monetlamp.eep

HEX_EEPROM_FILE_PATH_AS_ARGS +=monetlamp.eep

LSS_FILE_PATH +=monetlamp.lss

LSS_FILE_PATH_AS_ARGS +=monetlamp.lss

MAP_FILE_PATH =monetlamp.map

MAP_FILE_PATH_AS_ARGS =monetlamp.map

ADDITIONAL_DEPENDENCIES:= $(HEX_FLASH_FILE_PATH) $(LSS_FILE_PATH) size

# AVR/GNU C Compiler













./%.o: .././%.c ../monetlamp.avrgccproj
	@echo Building file: $<
	@echo Invoking: AVR/GNU C Compiler
	$(QUOTE)$(AVR_APP_PATH)avr-gcc.exe$(QUOTE) -funsigned-char -funsigned-bitfields -Os -ffunction-sections -fpack-struct -fshort-enums -Wall -c -std=gnu99  -mmcu=atmega8   -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo Finished building: $<



# AVR/GNU Assembler




ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: $(OUTPUT_FILE_PATH) $(ADDITIONAL_DEPENDENCIES)

# AVR/GNU C Linker
$(OUTPUT_FILE_PATH): $(OBJS) $(USER_OBJS)
	@echo Building target: $@
	@echo Invoking: AVR/GNU C Linker
	$(QUOTE)$(AVR_APP_PATH)avr-gcc.exe$(QUOTE) -Wl,--gc-sections  -mmcu=atmega8   -o$(OUTPUT_FILE_PATH_AS_ARGS) $(OBJS_AS_ARGS) $(USER_OBJS) $(LIBS)
	@echo Finished building target: $@



$(HEX_FLASH_FILE_PATH): $(OUTPUT_FILE_PATH)
	$(QUOTE)$(AVR_APP_PATH)avr-objcopy.exe$(QUOTE) -O ihex -R .eeprom -R .fuse -R .lock -R .signature  $(QUOTE)$<$(QUOTE) $(QUOTE)$@$(QUOTE)

$(HEX_EEPROM_FILE_PATH): $(OUTPUT_FILE_PATH)
	-$(QUOTE)$(AVR_APP_PATH)avr-objcopy.exe$(QUOTE) -j .eeprom --set-section-flags=.eeprom=alloc,load --change-section-lma .eeprom=0 --no-change-warnings -O ihex $(QUOTE)$<$(QUOTE) $(QUOTE)$@$(QUOTE) || exit 0

$(LSS_FILE_PATH): $(OUTPUT_FILE_PATH)
	$(QUOTE)$(AVR_APP_PATH)avr-objdump.exe$(QUOTE) -h -S $(QUOTE)$<$(QUOTE) > $(QUOTE)$@$(QUOTE)

size: $(OUTPUT_FILE_PATH)
	@$(QUOTE)$(AVR_APP_PATH)avr-size.exe$(QUOTE) -C --mcu=atmega8 $(OUTPUT_FILE_PATH_AS_ARGS)

# Other Targets
clean:
	-$(RM) $(OBJS_AS_ARGS)$(C_DEPS_AS_ARGS) $(QUOTE)$(EXECUTABLES)$(QUOTE) $(OUTPUT_FILE_PATH_AS_ARGS) $(HEX_FLASH_FILE_PATH_AS_ARGS) $(HEX_EEPROM_FILE_PATH_AS_ARGS) $(LSS_FILE_PATH_AS_ARGS) $(MAP_FILE_PATH_AS_ARGS)

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets