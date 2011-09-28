
#----------------------------------------------------------------------------------------------
# Attempted kinda generic gc makefile
#----------------------------------------------------------------------------------------------


#--------------------------
# Platform configuration
#--------------------------
CROSS			= powerpc-gekko-
APP_IMAGEBASE	= 0x80003100

#--------------------------
# Custom configuration
#--------------------------
POSTBUILD	= doltool


#--------------------------
# Generic configuration
#--------------------------
CC		=	$(CROSS)gcc
CXX		=	$(CROSS)g++
AR		=	$(CROSS)ar
OBJCOPY	=	$(CROSS)objcopy
AS		=	$(CROSS)as
LD		=	$(CXX)

#----------------------------------
# C configuration
#----------------------------------
CFLAGS =	-O2 -fno-exceptions -Wa,-mregnames -Wl,-Ttext, $(APP_IMAGEBASE) -nostdinc
e
#----------------------------------
# Asm configuration
#----------------------------------
AFLAGS	=	-Iinclude

#----------------------------------
# Linker configuration
#----------------------------------
LDFLAGS =	 -nostdlib




# Files
MAIN_OBJS = \
	main.o dvdboot.o drivecode.o


GCLIB_OBJS

ngc_gcc -O2 -fno-exceptions -Wa,-mregnames -Wl,-Ttext,%GC_IMAGEBASE% -Wl,-Map,Bin/DvdTest.map -Wl,--omagic 


-I %GCLIBDIR% -o %BINDIR%/%PROGNAME%.elf %GCLIBDIR%/crt0.s drivecode.s %GCLIBDIR%/cache.s %GCLIBDIR%/GC_Debug.cpp %GCLIBDIR%/GC_Video.cpp %GCLIBDIR%/GC_String.cpp  %GCLIBDIR%/GC_Random.cpp %GCLIBDIR%/GC_Pad.cpp %GCLIBDIR%/GC_Dvd.cpp %GCLIBDIR%/GC_Memory.cpp %GCLIBDIR%/GC_Time.cpp main.cpp


IO_OBJS = \
	io/video.o io/console.o io/console_font.o io/vsprintf.o \
	io/exi.o io/dvd.o  io/GC_Dvd.o io/network.o io/sram.o
# ../GcLib/GC_Dvd.o

LIBRARIES =
# gdbstub/libgdbstub.a


OBJS = lowlevel/crt0.o $(MAIN_OBJS) $(IPL_OBJS) $(LIBRARIES)


# Build rules
all: gcos.dol gcos.bin

clean:
	rm -rf $(OBJS)
	
realclean:
	find -name '*.o' | xargs -r rm
	find -name '*.a' | xargs -r rm
	find -name '*~' | xargs -r rm
	find -name '*.bin' | xargs -r rm
	find -name '*.dol' | xargs -r rm
	find -name '*.elf' | xargs -r rm
	find -name '*.map' | xargs -r rm
	find -name '*.tmp' | xargs -r rm

.c.o:
	$(CC) $(CFLAGS)  -c -o $@ $*.c

.cpp.o:
	$(CXX) $(CFLAGS) -c -o $@ $*.cpp

.s.o:
	$(CC) $(AFLAGS) -c -o $@ $*.s

.S.o:
	$(CC) $(AFLAGS) -c -o $@ $*.S


###################################################
# GcOs.dol .bin .elf  @ 0x81700000
###################################################
gcos.elf : main_tmp.elf
	$(OBJCOPY) -R .comment $< $@

gcos.bin: gcos.elf
	$(OBJCOPY) -O binary $< $@

gcos.dol: gcos.elf
	$(POSTBUILD) -d gcos.elf

main_tmp.elf: $(OBJS)
#	$(CC) $(LDFLAGS) -Wl,-n -o main_high.elf -Wl,-Ttext=0x81700000 $(OBJS) -Wl,-Map -Wl,main_high.map
#	$(CC) $(LDFLAGS) -Wl,-n -o $@ -Wl,-Ttext=0x80D00000 $(OBJS) -Wl,-Map -Wl,main.map
	$(CXX) $(LDFLAGS) -Wl,-n -o $@ -Wl,-Ttext=0x81300000 $(OBJS) -Wl,-Map -Wl,gcos.map


gdbstub/libgdbstub.a:
	$(MAKE) -C gdbstub

# TODO: dependencies
