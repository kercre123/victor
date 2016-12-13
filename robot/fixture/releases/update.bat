@echo Copy .bin files from various build folders
@echo DO NOT COMMIT .bin FILES YOU ARE NOT INTENDING TO SHIP TO FACTORY!
@echo.
@echo See binaries.s for more details on these files.
@echo.
cd %~dp0
copy ..\..\binaries\sys_boot.bin .
copy ..\..\syscon\build\app\syscon.bin .
copy ..\..\binaries\robot_boot.bin .
copy ..\..\build\robot.fcc.bin .
copy ..\..\build\robot.bin .
copy ..\..\accessories\fcc\build\xsfcc.bin .
rem copy ..\..\accessories\boot\build\xsboot.bin .
copy ..\..\staging\factory.safe esp.safe.bin
copy ..\..\staging\esp.factory.bin .
copy ..\..\espressif_bootloader\firmware\cboot.bin esp.boot.bin
@echo.
@echo ===========================================
@echo REMEMBER TO:
@echo 1) Manually build syscon and robot41 first - batch build doesn't work 
@echo 2) make force_stage_binaries ^&^& make release ^&^& make -C espressif clean ^&^& make esp_factory ^&^& make factory_upgrade ^&^& cp staging/factory_upgrade.safe releases/factory_upgrade.safe ^&^& rm build/calc*
@rem make force_stage_binaries && make release && make -C espressif clean && make esp_factory && make factory_upgrade && cp staging/factory_upgrade.safe releases/factory_upgrade.safe && rm build/calc*
@echo ===========================================
@echo.
@pause
