                AREA    |.text|, DATA, READONLY
                ALIGN

                EXPORT  BOOTLOADER_UPDATE
                EXPORT  BOOTLOADER_LENGTH

BOOTLOADER_UPDATE
                INCBIN  fixture/releases/robot_boot.bin
                FILL    0x1000, 0xFF
__EMERGENCY_BOOTLOADER_END
BOOTLOADER_LENGTH
                DCD     __EMERGENCY_BOOTLOADER_END - BOOTLOADER_UPDATE

                END
