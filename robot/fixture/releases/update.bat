@echo Copy .bin files from various build folders
@echo DO NOT COMMIT .bin FILES YOU ARE NOT INTENDING TO SHIP TO FACTORY!
@echo.
@echo See binaries.s for more details on these files.
@echo.
cd %~dp0
copy ..\..\syscon\build\boot\sys_boot.bin .
copy ..\..\syscon\build\app\syscon.bin .
copy ..\..\syscon\build\radio\radio.bin .
copy ..\..\build\robot_boot.bin .
copy ..\..\build\robot.bin .
copy ..\..\espressif\bin\upgrade\user1.2048.new.3.bin esp.user.bin
