::**********************************************************************
 @SET QCODE_IMGBASE=0x0040C600

 @SET COMPILE_QCODE=1
 @SET COMPILE_QCODE_HI=0
 @SET COMPILE_QCODE_LO=0
::**********************************************************************

@echo off
@cls
@PROMPT [.]

@SETLOCAL
@call gctools
@powerpc-gekko-as -L -mregnames -mbig-endian "source/QLiteIPL.asm.c" -o bin/IPLBoot.o
@powerpc-gekko-ld -Ttext 0x81200000 -O elf32-powerpc bin/IPLBoot.o -o bin/IPLBoot.elf
@powerpc-gekko-objcopy -O binary bin/IPLBoot.elf bin/IPLBoot.bin

@powerpc-gekko-as -L -mregnames -mbig-endian "source/QLiteApploader.asm.c" -o bin/QLiteApploader.o
@powerpc-gekko-ld -Ttext 0x81200000 -O elf32-powerpc bin/QLiteApploader.o -o bin/QLiteApploader.elf
@powerpc-gekko-objcopy -O binary bin/QLiteApploader.elf bin/QLiteApploader.bin

@ENDLOCAL

@cd bin
@del qcode.bin
@cd ..

@SETLOCAL
@SET PATH=%PATH%;%CYGPUB%;

@echo *************************
@echo *    Assembling...      *
@echo *************************
@echo .
@echo .
@if %COMPILE_QCODE_LO% EQU 1	@as_mn10200 -L Source/qcode_lo.asm.c -o bin/qcode_lo.o
@if %COMPILE_QCODE_HI% EQU 1	@as_mn10200 -L Source/qcode_hi.asm.c -o bin/qcode_hi.o
@if %COMPILE_QCODE% EQU 1		@as_mn10200  --defsym QCODEImageBase=%QCODE_IMGBASE% -L "Source/QLite Main.asm.c" -o bin/qcode.o


@echo .
@echo .
@echo *************************
@echo *    Linking...         *
@echo *************************
@echo .
@echo .
@if %COMPILE_QCODE_LO% EQU 1	@ld-mn10200 -Ttext %QCODE_IMGBASE% --section-start absolute=0x00 -O mn10200-elf bin/qcode_lo.o -o bin/qcode_lo.elf
@if %COMPILE_QCODE_HI% EQU 1	@ld-mn10200 -Ttext %QCODE_IMGBASE% --section-start absolute=0x00 -O mn10200-elf bin/qcode_hi.o -o bin/qcode_hi.elf
@if %COMPILE_QCODE% EQU 1		@ld-mn10200 -Ttext %QCODE_IMGBASE% --section-start absolute=0x00 -O mn10200-elf bin/qcode.o -o bin/qcode.elf

@if %COMPILE_QCODE_LO% EQU 1	@objcopy-all -O binary bin/qcode_lo.elf bin/qcode_lo.bin
@if %COMPILE_QCODE_HI% EQU 1	@objcopy-all -O binary bin/qcode_hi.elf bin/qcode_hi.bin
@if %COMPILE_QCODE% EQU 1		@objcopy-all -O binary bin/qcode.elf bin/qcode.bin

@cd bin
@call delfile qcode_lo.o
@call delfile qcode_lo.elf
@call delfile qcode_hi.o
@call delfile qcode_hi.elf
@call delfile qcode.o
@call delfile qcode.elf
@call delfile IPLBoot.o

@copy qcode.bin ..\..\DvdTest\qcode.bin
@copy qcode.bin ..\..\XenoAt\qcode.bin
@copy qcode.bin ..\..\PortIo\bin\qcode.bin
:: @copy qcode_lo.bin ..\..\DvdTest\qcode_lo.bin
:: @copy qcode_hi.bin ..\..\DvdTest\qcode_hi.bin
:: @call disasm.bat
@cd..

@ENDLOCAL
