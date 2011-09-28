cls
@ECHO OFF
call gctools.bat

SETLOCAL

REM *********************************************
REM	Setup devkitcube Variables
REM *********************************************
SET DEVKITCUBE=%GCTOOLSDIR%\dev\DevkitCube
SET PATH=%DEVKITCUBE%\bin;%PATH%;


REM *********************************************
REM	Setup GcLib Variables
REM *********************************************
SET GCLIBDIR=%GCTOOLSDIR%\dev\source\GcLib
SET PROGNAME=XenoShell
SET BINDIR=Bin


REM *********************************************
REM	Delete temporary files
REM *********************************************

REM *********************************************
REM	Compile !
REM *********************************************

echo.
echo Compiling XENOSHELL(!)...
echo .
echo .
echo .

SET GC_IMAGEBASE=0x81700000
@REM SET GC_IMAGEBASE=0x81300000


powerpc-gekko-gcc-3.4.4 -nostdlib -Os -pipe -Wl,-s,-x,--gc-sections -Wa,-mregnames -fno-exceptions -mhard-float -Wl,-T,first.lds -Wl,-Map,Bin/%PROGNAME%.map boot.s main.c -o %BINDIR%/%PROGNAME%.elf 
powerpc-gekko-objcopy -O binary %BINDIR%/%PROGNAME%.elf %BINDIR%/%PROGNAME%.bin

@rem bin2dol %BINDIR%/%PROGNAME%.bin %BINDIR%/%PROGNAME%2.dol >> NUL    -maltivec -Wl,-G,1000

rem powerpc-gekko-gcc-3.4.4
rem powerpc-gekko-gcc  -ffreestanding -Wl,--omagic -Os -nostdlib %GCLIBDIR%\crt0.s  debug.c ray.c -o %BINDIR%/%PROGNAME%.elf -Wl,-T -Wl,first.lds
rem powerpc-gekko-gcc -nostdlib -O2 -ffreestanding %GCLIBDIR%/crt0.sdebug.c ray.c -o %BINDIR%/%PROGNAME%.elf  -Wl,-T -Wl,first.lds
rem ngc_gcc -nostdlib  -ffreestanding -Os debug.c ray.c -o %BINDIR%/%PROGNAME%.elf -Wl,first.lds 
rem -Wl,--omagic   -Wl,-Ttext,0x8000   
rem powerpc-gekko-gcc-3.4.4 powerpc-gekko-gcc.exe powerpc-gekko-gcc.exe  ngc_gcc -Wl,--omagic -ffreestanding -nostdlib -O2 -Wl,-Ttext,%GC_IMAGEBASE% -Wl,-Map,Bin/%PROGNAME%.map -I %GCLIBDIR% -o %BINDIR%/%PROGNAME%.elf ray.c debug.c
rem  ngc_gcc -Os -fno-exceptions -Wa,-mregnames -ffreestanding -Wl,-Ttext,%GC_IMAGEBASE% -Wl,-Map,Bin/%PROGNAME%.map -Wl,--omagic -I %GCLIBDIR% -o %BINDIR%/%PROGNAME%.elf ray.c debug.c
rem ngc_gcc -O2 -fno-exceptions -ffreestanding -Wa,-mregnames -Wl,-Ttext,%GC_IMAGEBASE% -Wl,-Map,Bin/%PROGNAME%.map -Wl,--omagic -I %GCLIBDIR% -o %BINDIR%/%PROGNAME%.elf  %GCLIBDIR%/cache.s %GCLIBDIR%/GC_Debug.cpp %GCLIBDIR%/GC_Video.cpp %GCLIBDIR%/GC_String.cpp %GCLIBDIR%/GC_Random.cpp %GCLIBDIR%/GC_Pad.cpp %GCLIBDIR%/GC_Dvd.cpp %GCLIBDIR%/GC_Memory.cpp %GCLIBDIR%/GC_Time.cpp %GCLIBDIR%/GC_vsprintf.cpp ray.c






echo .
echo .
echo .

REM -Wl,--strip-all -Wl,--gc-sections -Wl,--discard-all				

if ERRORLEVEL==1 goto Error

echo Success !

REM *********************************************
REM	Create DOL from ELF
REM *********************************************
echo.

cd %BINDIR%
@REM ngc_objcopy -O binary %PROGNAME%.elf %PROGNAME%.bin
@REM if ERRORLEVEL==1 goto Error


doltool -d %PROGNAME%.elf
if ERRORLEVEL==1 goto Error

@copy XenoShell.bin ..\..\DvdTest\XenoShell.bin
@copy XenoShell.bin ..\..\XenoAT\XenoShell.bin
@size %PROGNAME%.elf
@rem dir /s /b XenoShell.bin > dir.txt
@rem type dir.txt


cd..

REM *********************************************
REM	Done !
REM *********************************************

goto End



REM *********************************************
REM	Error -> delete previous DOL file
REM *********************************************
:Error
echo.
echo.
echo [ERROR]
REM del /Q %PROGNAME%.dol
goto ErrorEnd


:End

REM *********************************************
REM	Delete temporary files
REM *********************************************
cd %BINDIR%
rem del /Q %PROGNAME%.elf
rem del /Q %PROGNAME%.bin
cd..

ENDLOCAL
:ErrorEnd
@rem pause
