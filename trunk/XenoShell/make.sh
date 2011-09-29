#!/bin/sh

export PROGNAME=XenoShell
export BINDIR=Bin

powerpc-eabi-gcc -nostdlib -Wall -Os -pipe -Wl,-s,-x,--gc-sections -Wa,-mregnames -fno-exceptions -mhard-float -Wl,-T,first.lds -Wl,-Map,$BINDIR/$PROGNAME.map boot.s main.c -lgcc -o $BINDIR/$PROGNAME.elf
powerpc-eabi-objcopy -O binary $BINDIR/$PROGNAME.elf $BINDIR/$PROGNAME.bin
