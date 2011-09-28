cls
@ECHO OFF
call gctools.bat

SETLOCAL

::========================================
:: Setup devkitcube Variables
::========================================
SET DEVKITCUBE=%GCTOOLSDIR%\dev\DevkitCube
SET PATH=%DEVKITCUBE%\bin;%PATH%;

::========================================
::  Setup GcLib Variables
::========================================
SET GCLIBDIR=%GCTOOLSDIR%\dev\source\GcLib
SET PROGNAME=Credits
SET BINDIR=Bin


::========================================
:: del temp files
::========================================

::========================================
:: compile 
::========================================

echo.
echo Compiling...
echo .
echo .
echo .

SET GC_IMAGEBASE=0x81200000
:: SET GC_IMAGEBASE=0x81300000


powerpc-gekko-gcc-3.4.4 -nostdlib -pipe -Wl,-s,-x,--gc-sections  -fno-exceptions -mhard-float -Os -Wl,-T,first.lds -Wl,-Map,Bin/%PROGNAME%.map boot.s ray.c debug.c -o %BINDIR%/%PROGNAME%.elf 
if ERRORLEVEL==1 goto Error
powerpc-gekko-objcopy -O binary %BINDIR%/%PROGNAME%.elf %BINDIR%/%PROGNAME%.bin
if ERRORLEVEL==1 goto Error

echo .
echo .
echo .
echo Success !

:: -Wl,--strip-all -Wl,--gc-sections -Wl,--discard-all				

::========================================
:: DOL from ELF
::========================================
echo.

cd %BINDIR%
:: ngc_objcopy -O binary %PROGNAME%.elf %PROGNAME%.bin
:: if ERRORLEVEL==1 goto Error

doltool -d %PROGNAME%.elf
if ERRORLEVEL==1 goto Error

@copy credits.bin ..\..\DvdTest\credits.bin
@copy credits.bin ..\..\XenoAT\credits.bin
@size %PROGNAME%.elf
:: dir /s /b credits.bin > dir.txt
:: type dir.txt

cd..
goto End

::========================================
:: Error -> delete previous DOL file
::========================================
:Error
echo.
echo.
echo [ERROR]
:: del /Q %PROGNAME%.dol
goto ErrorEnd


:End

::========================================
::	Delete tmp files
::========================================
cd %BINDIR%
:: del /Q %PROGNAME%.elf
:: del /Q %PROGNAME%.bin
cd..

ENDLOCAL
:ErrorEnd

