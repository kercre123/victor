LoadAddress     EQU   0x20002000

                AREA    |.text|, CODE, READONLY
                EXPORT  JumpToOTA

JumpToOTA       PROC
                LDR     r0, =LoadAddress
                LDR     r0, [r0]
                BX      r0
                ENDP

                ALIGN

                AREA    |.text|, DATA, READONLY

                EXPORT  OTA_IMAGE

OTA_IMAGE       DCD     LoadAddress
                DCD     (OTA_IMAGE_SIZE - OTA_IMAGE_START)
OTA_IMAGE_START 
                ;INCBIN  build/ota/ota_bin.raw
OTA_IMAGE_SIZE

                END
