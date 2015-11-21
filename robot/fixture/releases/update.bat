@echo Copy .bin files from various build folders
@echo DO NOT COMMIT .bin FILES YOU ARE NOT INTENDING TO SHIP TO FACTORY!
@echo.
@echo See binaries.s for more details on these files.
@echo.
cd %~dp0
copy ..\..\syscon\sys_boot\build\sys_boot.bin .
copy ..\..\syscon\build\syscon.bin .
copy ..\..\robot_boot\build\robot_boot.bin .
copy ..\..\build\robot.bin .
