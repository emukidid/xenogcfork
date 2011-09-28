#
# Main makefile
#

# I hope this doesn't break your build, but I've got different names for the tools. The contents 
# of Makefile.local on my machine are:
#  BINPATH=$(HOME)/gc/bin
#  CROSS=powerpc-eabi-elf
#  CC=$(BINPATH)/$(CROSS)-gcc
#  OBJCOPY=$(BINPATH)/$(CROSS)-objcopy
#  LD=$(BINPATH)/$(CROSS)-ld
#  AS=$(BINPATH)/$(CROSS)-as
#  
#  CFLAGS_LOCAL=-DSAMSON=1
# it is excluded from CVS, to not conflict with your settings.

APPNAME	= bin/xenoflash

############################################################################################################

INIT	=	call gctools.bat

CROSS	= 	@powerpc-gekko-
CC		=	$(CROSS)gcc
CXX		=	$(CROSS)g++
AR		=	$(CROSS)ar
OBJCOPY	=	$(CROSS)objcopy
AS		=	$(CROSS)as
LD		=	$(CXX)

DOLTOOL	=	@doltool


# Configuration
CFLAGS =		$(CFLAGS_LOCAL) -fno-exceptions -Wa,-mregnames \
				-I$(GCLIBDIR) -Iinclude -o2

AFLAGS	=	-Iinclude
LDFLAGS	=	--gc-sections --omagic -T first.lds
			#-Ttext=0x81400000


# Files
MAIN_OBJS	= main.o title.raw.o
BOOT_OBJS	= $(GCLIBDIR)/crt0.o 
GCLIB_OBJS	= $(GCLIBDIR)/cache.o $(GCLIBDIR)/GC_Debug.o $(GCLIBDIR)/GC_Video.o $(GCLIBDIR)/GC_String.o $(GCLIBDIR)/GC_Random.o $(GCLIBDIR)/GC_PAD.o $(GCLIBDIR)/GC_Dvd.o $(GCLIBDIR)/GC_Memory.o $(GCLIBDIR)/GC_Time.o $(GCLIBDIR)/GC_vsprintf.o $(GCLIBDIR)/GC_EXI.o

LIBRARIES =
# gdbstub/libgdbstub.a
# ./lwip/core/dhcp.o 

OBJS =   $(BOOT_OBJS) $(GCLIB_OBJS) $(LIBRARIES) $(MAIN_OBJS) 

# Build rules
all: $(APPNAME).dol 

clean:
	rm -rf $(OBJS)
	
cleanx:
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
# 
###################################################
main.o: flashloader.bin
main.o: ../XenoAT/bin/XenoAT.bin

$(APPNAME).elf: $(APPNAME)_tmp.elf
	$(OBJCOPY) -R .comment $< $@

$(APPNAME).bin: $(APPNAME).elf
	$(OBJCOPY) -O binary $< $@
			
$(APPNAME).dol: $(APPNAME).elf
	$(DOLTOOL) -d $(APPNAME).elf

$(APPNAME)_tmp.elf: $(OBJS)
	$(CXX) $(LDFLAGS) -Wl,-n -o $@ $(OBJS) -Wl,-Map -Wl,$(APPNAME).map

flashloader.o: flashloader.S
	@as_mn10200 -L flashloader.S -o flashloader.o

flashloader.elf: flashloader.o
	@ld-mn10200 -Ttext 0x40D000 --section-start absolute=0x00 -O mn10200-elf flashloader.o -o flashloader.elf

flashloader.bin: flashloader.elf
	@objcopy-all -O binary flashloader.elf flashloader.bin

