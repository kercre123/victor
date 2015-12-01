                PRESERVE8
                THUMB

                AREA    RESET, CODE, READONLY

                EXPORT __initial_sp

__initial_sp EQU 0x1FFFFFF0

; Reset Handler
Reset_Handler   PROC
                EXPORT  Reset_Handler             [WEAK]
                    
                IMPORT  SystemInit
                IMPORT  __main

                CPSID   i
                LDR     SP, =__initial_sp
                LDR     R0, =SystemInit
                BLX     R0
                LDR     R0, =__main
                BX      R0
                ENDP

                END
