###########################################################################################
###########################################################################################
#  MASTER MAKEFILE
###########################################################################################
###########################################################################################

include Makefile-help
include Makefile-avrdude

all: help
linux-all: linux-shell linux-hex
windows-all: windows-shell windows-hex

###########################################################################################
###########################################################################################
# LINUX
linux-shell:
	make -f XenoShell/Makefile-LINUX

linux-hex:
	make -f XenoAT/Makefile-LINUX

linux-extra:
	make -f XenoShell/Makefile-LINUX
	make extra -f XenoAT/Makefile-LINUX

###########################################################################################
###########################################################################################
# WINDOWS
windows-shell:
	$(MAKE) -f XenoShell/Makefile-WIN

windows-hex:
	$(MAKE) -f XenoAT/Makefile-WIN

windows-extra:
	$(MAKE) -f XenoShell/Makefile-WIN
	$(MAKE) extra -f XenoAT/Makefile-WIN

###########################################################################################
###########################################################################################
# Clean WINDOWS / LINUX
clean:
	make clean -f XenoAT/Makefile-LINUX
	make clean -f XenoShell/Makefile-LINUX

###########################################################################################
###########################################################################################
# AVRDUDE
flash: 
	$(AVRDUDE) $(FLAGS) -v -U flash:w:$(TARGET)/$(TARGET).hex

readfuse:
	uisp -dprog=dasa2 -dserial=$(PORT) --rd_fuses

writefuse:
	$(AVRDUDE) $(FLAGS) -v -U lfuse:w:0xC4:m -U hfuse:w:0xD9:m

###########################################################################################
###########################################################################################
# HELP
help:
