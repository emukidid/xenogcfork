SET PROGNAME=XenoShell
SET BINDIR=Bin
SET GC_IMAGEBASE=0x81700000


powerpc-eabi-gcc -lgcc -nostdlib -Os -pipe -Wl,-s,-x,--gc-sections -Wa,-mregnames -fno-exceptions -mhard-float -Wl,-T,first.lds -Wl,-Map,Bin/%PROGNAME%.map boot.s main.c -lgcc -o %BINDIR%/%PROGNAME%.elf 
powerpc-eabi-objcopy -O binary %BINDIR%/%PROGNAME%.elf %BINDIR%/%PROGNAME%.bin
