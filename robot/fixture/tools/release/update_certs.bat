@echo OFF

where adb >nul 2>nul
if %ERRORLEVEL% NEQ 0 ( echo "adb command not found" & goto :END )

adb shell "echo shell connection established"
if %ERRORLEVEL% NEQ 0 ( echo adb not connected to a device e=%ERRORLEVEL% & goto :END )

echo scanning current directory for cloud cert bundles...
for %%f in (*) do (
  REM echo %%f , %%~nxf
  echo %%~nxf |findstr /r "factory_cdpb_....\.zip" >nul && (
    echo found %%~nxf
    adb shell "rm -rf /anki/*"
    adb push %%~nxf /anki/certs.zip
    adb shell "sync"
    pause
    exit 0
  )
)
echo no cloud cert bundles detected

:END
pause
exit 1

