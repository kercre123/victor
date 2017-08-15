                AREA    |.text|, DATA, READONLY
                ALIGN

                EXPORT  CUBE_UPDATE
                EXPORT  CUBE_FIRMWARE_LENGTH

CUBE_UPDATE
                INCBIN  ../../accessories/releases/xs6-p4.safe  ; TOMY test firmware
CUBE_FIRMWARE_LENGTH
                DCD     CUBE_FIRMWARE_LENGTH - CUBE_UPDATE

                END
