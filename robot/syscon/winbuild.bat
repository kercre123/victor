@ECHO OFF
REM Breif: cygwin sucks. Use window's ubuntu bash instead!

REM echo cleaning sysboot + syscon
REM start /wait C:\Keil_v5\UV4\UV4.exe -c sysboot.uvprojx -j0
REM start /wait C:\Keil_v5\UV4\UV4.exe -c syscon.uvprojx -j0

REM echo clean generated crypto header
REM del /q build\publickeys.h

echo cleaning projects
rmdir build /s /q

REM (alternate to getting cygwin & python3 working)
echo re-generate crypto header
IF not EXIST ".\build" mkdir build
start /wait bash -c "python3 ./tools/export.py build/publickeys.h"
IF not errorlevel 0 ( 
  echo --FAILED. exit code %errorlevel%
  REM exit 1
)

echo building syscon bootloader
start /wait C:\Keil_v5\UV4\UV4.exe -b sysboot.uvprojx -j0
IF not EXIST ".\build\sysboot.bin" (
  echo --FAILED. uv4 exit code %errorlevel%
  REM exit 2
)

REM syscon application larger than evaluation compiler can handle. convert to UV4
echo convert syscon uv5 to uv4
start /wait bash -c "python ../tools/convert_keil_uv5_to_uv4.py syscon.uvprojx syscon.uvproj"

echo building syscon application
start /wait C:\Keil\UV4\UV4.exe -b syscon.uvproj -j0
IF not EXIST ".\build\syscon\syscon.axf"  (
  echo --FAILED. uv4 exit code %errorlevel%
  REM exit 3
) ELSE (
  echo signing syscon application
  start /wait bash -c "python3 ./tools/sign.py -b build/syscon.bin build/syscon/syscon.axf"
  IF not errorlevel 0 (
    echo --FAILED. exit code %errorlevel%
    REM exit 4
  )
)

REM pause
REM exit 0
