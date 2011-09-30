###########################################################################################
###########################################################################################
# Makefile 
###########################################################################################
###########################################################################################

TARGET = XenoShell
SOURCES = source
DIR = $(DEVKITPPC)/bin/
CC = $(DIR)/powerpc-eabi-gcc
OBJCOPY = $(DIR)/powerpc-eabi-objcopy
FLAGS = -nostdlib -Wall -Os -pipe -Wl,-s,-x,--gc-sections -Wa,-mregnames -fno-exceptions -mhard-float -Wl,-T,$(SOURCES)/first.lds -Wl,-Map=$(SOURCES)/$(TARGET).map



all: build move

build:
	$(CC) $(FLAGS) $(SOURCES)/boot.s $(SOURCES)/main.c -lgcc -o $(SOURCES)/$(TARGET).elf
	$(OBJCOPY) -O  binary $(SOURCES)/$(TARGET).elf $(SOURCES)/$(TARGET).bin

move:
	mv $(SOURCES)/$(TARGET).bin .

clean:
	rm -f $(TARGET).bin
	rm -f $(SOURCES)/$(TARGET).elf
	rm -f $(SOURCES)/$(TARGET).map