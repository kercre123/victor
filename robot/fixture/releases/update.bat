@echo Copy .bin files from various build folders
@echo DO NOT COMMIT .bin FILES YOU ARE NOT INTENDING TO SHIP TO FACTORY!
@echo.
@echo See binaries.s for more details on these files.
@echo.
cd %~dp0
copy ..\..\binaries\sys_boot.bin .
copy ..\..\syscon\build\app\syscon.bin .
copy ..\..\syscon\build\radio\radio.bin .
copy ..\..\binaries\robot_boot.bin .
copy ..\..\build\robot.fcc.bin .
copy ..\..\build\robot.bin .
copy ..\..\accessories\fcc\build\xsfcc.bin .
copy ..\..\accessories\boot\build\xsboot.bin .
copy ..\..\staging\factory.safe esp.safe.bin
copy ..\..\staging\esp.factory.bin .
copy ..\..\espressif_bootloader\firmware\cboot.bin esp.boot.bin
@echo.
@echo ===========================================
@echo Note:  Remember to:
@echo make release
@echo make esp_factory_safe
@echo make -C espressif clean ^&^& make esp_factory
@echo ===========================================
@echo.
@pause
