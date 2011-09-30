@call gctools.bat 

@REM ***********************************************************************
 @SET QCODE_IMGBASE=0x0040C600
 @SET COMPILE_QCODE=1
@REM ***********************************************************************


@echo off
@cls
@PROMPT [.]

@del upload.bin

@SETLOCAL
@SET PATH=%PATH%;%CYGPUB%;

@echo *************************
@echo *    Assembling...      *
@echo *************************
@echo .
@echo .

@as_mn10200 -L upload.S -o upload.o

@echo .
@echo .
@echo *************************
@echo *    Linking...         *
@echo *************************
@echo .
@echo .

@ld-mn10200 -Ttext 0x8674 -O mn10200-elf upload.o -o upload.elf
@objcopy-all -O binary upload.elf upload.bin

@call delfile upload.o
@call delfile upload.elf

@ENDLOCAL
