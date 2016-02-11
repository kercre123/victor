                AREA    |.text|, DATA, READONLY
                ALIGN

                EXPORT  BOOTLOADER_UPDATE
                EXPORT  BOOTLOADER_LENGTH

BOOTLOADER_UPDATE
                INCBIN  build/boot/sys_boot.bin
__EMERGENCY_BOOTLOADER_END
BOOTLOADER_LENGTH
                DCD     __EMERGENCY_BOOTLOADER_END - BOOTLOADER_UPDATE

                END
