###########################################################################################
###########################################################################################
# USER DEFINE VARIABLES
# 
#
MCU = atmega8
TARGET = XenoAT

DIRAVR = c:/winavr
DIRAVRBIN = $(DIRAVR)/bin
DIRAVRUTILS = $(DIRAVR)/utils/bin
DIRINC = .
DIRLIB = $(DIRAVR)/avr/lib

AVRDUDE = avrdude
AVRDUDE_PROGRAMMER = stk500
AVRDUDE_PORT = com1	   # programmer connected to serial device
AVRDUDE_ERASE = -e
AVRDUDE_FLAGS = -p $(MCU) -P $(AVRDUDE_PORT) -c $(AVRDUDE_PROGRAMMER)
#
#
###########################################################################################
###########################################################################################

CC = avr-gcc
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
SIZE = avr-size
REMOVE = rm -f
COPY = cp
DIR = .
SOURCES	= source
SRC = $(SOURCES)/$(TARGET).c 
ASRC = $(SOURCES)/qcode.S
CFLAGS = -mmcu=$(MCU) -I. -g -Os \
	-funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums \
	-Wall -Wstrict-prototypes \
	-Wa,-ahlms=$(<:.c=.lst)
ASFLAGS = -mmcu=$(MCU) -I. -x assembler-with-cpp -Wa,-ahlms=$(<:.S=.lst),-gstabs 
LDFLAGS = -Wl,-Map=$(SOURCES)/$(TARGET).map,--cref
LDFLAGS += -lm
OBJ = $(SRC:.c=.o) $(ASRC:.S=.o) 
LST = $(ASRC:.S=.lst) $(SRC:.c=.lst)
SHELL = sh
HEXSIZE = $(SIZE) --target=hex $(SOURCES)/$(TARGET).hex
ELFSIZE = $(SIZE) -A $(SOURCES)/$(TARGET).elf
FINISH = echo Errors: none
BEGIN = echo -------- begin --------
END = echo --------  end  --------


###########################################################################################

# Default target.
all: begin gccversion sizebefore $(SOURCES)/$(TARGET).elf $(SOURCES)/$(TARGET).hex \
	$(SOURCES)/$(TARGET).bin $(SOURCES)/$(TARGET).eep $(SOURCES)/$(TARGET).lss \
	sizeafter finished move end

begin:
	@$(BEGIN)

finished:
	@$(FINISH)

end:
	@$(END)

gccversion : 
	$(CC) --version

sizebefore:
	@if [ -f $(SOURCES)/$(TARGET).elf ]; then echo Size before:; $(ELFSIZE);fi

sizeafter:
	@if [ -f $(SOURCES)/$(TARGET).elf ]; then echo Size after:; $(ELFSIZE);fi

move :
	@mv $(SOURCES)/$(TARGET).hex .
	@echo
	@echo md5sum:
	@md5sum $(TARGET).hex
	@echo
	@echo

###########################################################################################

# Convert ELF to COFF for use in debugging / simulating in
# AVR Studio or VMLAB.
COFFCONVERT=$(OBJCOPY) --debugging \
	--change-section-address .data-0x800000 \
	--change-section-address .bss-0x800000 \
	--change-section-address .noinit-0x800000 \
	--change-section-address .eeprom-0x810000 


coff: $(SOURCES)/$(TARGET).elf
	$(COFFCONVERT) -O coff-avr $< $(SOURCES)/$(TARGET).cof


extcoff: $(SOURCES)/$(TARGET).elf
	$(COFFCONVERT) -O coff-ext-avr $< $(SOURCES)/$(TARGET).cof

###########################################################################################

program: $(SOURCES)/$(TARGET).hex $(SOURCES)/$(TARGET).eep
	$(AVRDUDE) $(AVRDUDE_FLAGS) $(AVRDUDE_ERASE)
	$(AVRDUDE) $(AVRDUDE_FLAGS) -i $(SOURCES)/$(TARGET).hex
	@$(SIZE) --target=.hex $(SOURCES)/$(TARGET).eep | while read line; \
	do \
		set -- $$line; \
		if [ "x$$1" != "x0" ] ; then continue; fi; \
		if [ "$$2" -ne 0 ] ; then \
			echo $(AVRDUDE) $(AVRDUDE_FLAGS) -m eeprom -i $(SOURCES)/$(TARGET).eep; \
			$(AVRDUDE) $(AVRDUDE_FLAGS) -m eeprom -i $(SOURCES)/$(TARGET).eep; \
			break; \
		fi; \
	done

###########################################################################################

# Compile: create object files from C source files.
%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@


# Compile: create assembler files from C source files.
%.s : %.c
	$(CC) -S $(CFLAGS) $< -o $@


# Assemble: create object files from assembler source files.
%.o : %.S
	$(CC) -c $(ASFLAGS) $< -o $@


# Link: create ELF output file from object files.
.SECONDARY : $(SOURCES)/$(TARGET).elf
.PRECIOUS : $(OBJ)
%.elf: $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) --output $@ $(LDFLAGS)


# Create final output files (.hex, .eep) from ELF output file.
%.hex: %.elf
	$(OBJCOPY) -O ihex -R .eeprom $< $(DIR)/$@

%.eep: %.elf
	-$(OBJCOPY) -j .eeprom --set-section-flags=.eeprom="alloc,load" \
	--change-section-lma .eeprom=0 -O ihex $< $@


# Create extended listing file from ELF output file.
%.lss: %.elf
	$(OBJDUMP) -h -S $< > $@

%.bin: %.elf
	$(OBJCOPY) -j .text -j .data -O binary $< $(DIR)/$@


$(SOURCES)/upload.bin: $(SOURCES)/upload.S
	@as_mn10200 -L $(SOURCES)/upload.S -o $(SOURCES)/upload.o
	@ld-mn10200 -Ttext 0x40d000 -O mn10200-elf $(SOURCES)/upload.o -o $(SOURCES)/upload.elf
	@objcopy-all -O $(SOURCES)/binary $(SOURCES)/upload.elf $(SOURCES)/upload.bin

$(SOURCES)/qcode.o: $(SOURCES)/upload.bin

###########################################################################################
# Target: clean project.
clean: 
	$(REMOVE) $(SOURCES)/$(TARGET).bin
	$(REMOVE) $(TARGET).hex
	$(REMOVE) $(SOURCES)/$(TARGET).eep
	$(REMOVE) $(SOURCES)/$(TARGET).obj
	$(REMOVE) $(SOURCES)/$(TARGET).cof
	$(REMOVE) $(SOURCES)/$(TARGET).elf
	$(REMOVE) $(SOURCES)/$(TARGET).map
	$(REMOVE) $(SOURCES)/$(TARGET).obj
	$(REMOVE) $(SOURCES)/$(TARGET).a90
	$(REMOVE) $(SOURCES)/$(TARGET).sym
	$(REMOVE) $(SOURCES)/$(TARGET).lnk
	$(REMOVE) $(SOURCES)/$(TARGET).lss
	$(REMOVE) $(OBJ)
	$(REMOVE) $(LST)
	$(REMOVE) $(SRC:.c=.s)
	$(REMOVE) $(SRC:.c=.d)

###########################################################################################
# Note that this will work with sh (bash) and sed that is shipped with WinAVR
# (see the SHELL variable defined above).
# This may not work with other shells or other seds.
#
%.d: %.c
	set -e; $(CC) -MM $(CFLAGS) $< $(SOURCES)/\
	| sed 's,\(.*\)\.o[ :]*,\1.o \1.d : ,g' > $@; \
	[ -s $@ ] || rm -f $@


# Remove the '-' if you want to see the dependency files generated.
include $(SRC:.c=.d)



# Listing of phony targets.
.PHONY : all begin finish end sizebefore sizeafter gccversion coff extcoff \
	clean clean_list_full program



