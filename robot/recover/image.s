                AREA    |.text|, DATA, READONLY
                ALIGN

                EXPORT  __BOOTLOADER
                EXPORT  __BOOTLOADER_SIZE

__BOOTLOADER
                INCBIN  ../binaries/robot_boot.bin
__BOOTLOADER_SIZE
__BOOTLOADER_END
                DCD     __BOOTLOADER_END - __BOOTLOADER

                END
