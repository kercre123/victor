                AREA    RESET, DATA, READONLY
                IMPORT  main
                IMPORT  deinit
                IMPORT  tick
                IMPORT  recv
                EXPORT  ble_send

                DCB "Current Version", 0
                DCD main
                DCD deinit
                DCD tick
                DCD recv
ble_send        DCD default_handler
                DCB 0x3d, 0x70, 0x7a, 0x86, 0x10, 0x3c, 0xef, 0x4f, 0x83, 0x2f, 0xa8, 0xa6, 0x64, 0x20, 0xba, 0xdf ; Nonce

                AREA    |.text|, CODE, READONLY
                EXPORT  __initial_sp

default_handler PROC
__initial_sp    BX LR
                ENDP

                END
