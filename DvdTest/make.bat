@cls
@ECHO OFF
@call gctools.bat

@SETLOCAL

REM *********************************************
REM	Setup devkitcube Variables
REM *********************************************
SET DEVKITCUBE=%GCTOOLSDIR%\dev\DevkitCube
SET PATH=%DEVKITCUBE%\bin;%PATH%;


REM *********************************************
REM	Setup GcLib Variables
REM *********************************************
SET GCLIBDIR=%GCTOOLSDIR%\dev\source\GcLib
SET PROGNAME=DvdTest
SET BINDIR=Bin


REM *********************************************
REM	Delete temporary files
REM *********************************************

REM *********************************************
REM	Compile !
REM *********************************************

@rem echo.
@rem echo Compiling...
@rem echo .
@rem echo .
@rem echo .

SET GC_IMAGEBASE=0x80003100
@REM SET GC_IMAGEBASE=0x81300000
@REM SET GC_IMAGEBASE=0x81700000

@ngc_gcc -O2 -fno-exceptions -Wa,-mregnames -Wl,-Ttext,%GC_IMAGEBASE% -Wl,-Map,Bin/DvdTest.map -Wl,--omagic -I %GCLIBDIR% -o %BINDIR%/%PROGNAME%.elf %GCLIBDIR%/crt0.s drivecode.s %GCLIBDIR%/cache.s %GCLIBDIR%/GC_Debug.cpp %GCLIBDIR%/GC_Video.cpp %GCLIBDIR%/GC_String.cpp %GCLIBDIR%/GC_Random.cpp %GCLIBDIR%/GC_Pad.cpp %GCLIBDIR%/GC_Dvd.cpp %GCLIBDIR%/GC_Memory.cpp %GCLIBDIR%/GC_Time.cpp %GCLIBDIR%/GC_vsprintf.cpp %GCLIBDIR%/GC_EXI.cpp main.cpp Flash.cpp libogc.a
@REM ngc_gcc -O2 -fno-exceptions -Wa,-mregnames -Wl,-Ttext,%GC_IMAGEBASE% -Wl,-Map,Bin/DvdTest.map -Wl,--omagic -I %GCLIBDIR% -o %BINDIR%/%PROGNAME%.elf %GCLIBDIR%/crt0.s drivecode.s %GCLIBDIR%/cache.s %GCLIBDIR%/GC_Memory.cpp main.cpp 

@rem echo .
@rem echo .
@rem echo .

REM -Wl,--strip-all -Wl,--gc-sections -Wl,--discard-all				

if ERRORLEVEL==1 goto Error

@echo Success !

REM *********************************************
REM	Create DOL from ELF
REM *********************************************
@rem echo.

cd %BINDIR%
@ngc_objcopy -O binary %PROGNAME%.elf %PROGNAME%.bin
if ERRORLEVEL==1 goto Error

rem @doltool -d %PROGNAME%.elf
@bin2dol %PROGNAME%.bin %PROGNAME%.dol >> NUL
rem @doltool -c %PROGNAME%.bin < 2dol_8130.txt
rem @doltool -c %PROGNAME%.bin < 2dol_8170.txt

if ERRORLEVEL==1 goto Error
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
