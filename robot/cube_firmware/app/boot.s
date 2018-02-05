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

                AREA    |.text|, CODE, READONLY
                EXPORT  __initial_sp

default_handler PROC
__initial_sp    BX LR
                ENDP

                END
