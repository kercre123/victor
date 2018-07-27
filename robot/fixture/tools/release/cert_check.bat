@echo OFF

where adb >nul 2>nul
if %ERRORLEVEL% NEQ 0 ( echo "adb command not found" & goto :END )

adb shell "echo shell connection established"
if %ERRORLEVEL% NEQ 0 ( echo adb not connected to a device e=%ERRORLEVEL% & goto :END )

adb shell "unzip -l /anki/certs.zip | grep -Eo '(00[0-9a-f]{2})1234'"

:END
pause
exit 0

