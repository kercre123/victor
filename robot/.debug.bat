cd %~dp0
if not %1==--version start ..\..\movidius-tools\tools\00.50.39.2\win32\bin\moviDebugServer.exe
..\..\movidius-tools\tools\00.50.39.2\win32\bin\moviDebug.exe %*
