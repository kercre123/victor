                AREA    |.text|, DATA, READONLY
                ALIGN

                EXPORT  CUBE_UPDATE
                EXPORT  CUBE_FIRMWARE_LENGTH

CUBE_UPDATE
                INCBIN  ../../accessories/releases/xs6-p2.safe
CUBE_FIRMWARE_LENGTH
                DCD     CUBE_FIRMWARE_LENGTH - CUBE_UPDATE

                END
