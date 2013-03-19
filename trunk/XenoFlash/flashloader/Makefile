############################################################################################################
############################################################################################################
# Makefile - Flashloader.bin LINUX
############################################################################################################
############################################################################################################

# TARGET  =  Target File to Compile
# MNDIR   =  Path to MN10200 ELF
# MNDIR1  =  Path to MN10200 Binutils
# MNDIR2  =  Path to MN10200 Binutils ELF


TARGET   = flashloader

MNDIR    = /opt/XenoTools/mn10200/bin
MNDIR1   = /opt/XenoTools/mn10200_binutils/bin
MNDIR2   = /opt/XenoTools/mn10200_binutils-elf/bin

GCC      = $(MNDIR)/mn10200-elf-gcc
AS       = $(MNDIR1)/mn10200-as
LD       = $(MNDIR1)/mn10200-ld
OBJCOPY  = $(MNDIR2)/mn10200-elf-objcopy

############################################################################################################
# Build rules
all: clean $(TARGET).bin move

move:
	cp flashloader.bin ../data/flashloader.bin

clean:
	rm -rf ../data/flashloader.bin
	rm -rf flashloader.bin
	rm -rf flashloader.elf
	rm -rf flashloader.o

flashloader.o: flashloader.S
	$(AS) -L flashloader.S -o flashloader.o

flashloader.elf: flashloader.o
	$(LD) -Ttext 0x40D000 --section-start absolute=0x00 -O $(GCC) flashloader.o -o flashloader.elf

flashloader.bin: flashloader.elf
	$(OBJCOPY) -O binary flashloader.elf flashloader.bin

