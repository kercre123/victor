                IF :DEF: __STACK_SIZE
Stack_Size      EQU     __STACK_SIZE
                ELSE
Stack_Size      EQU     0x1000
                ENDIF

                AREA    STACK, NOINIT, READWRITE, ALIGN=3
                EXPORT  __initial_sp
Stack_Mem       SPACE   Stack_Size
__initial_sp

                PRESERVE8
                THUMB

                AREA    RESET, CODE, READONLY
                DCD     Reset_Handler

; Reset Handler
Reset_Handler   PROC
                IMPORT  main

                LDR     r0, =__initial_sp  ; Reset the stack pointer
                MOV     sp, r0

                CPSID   I
                LDR     R0, =main
                BX      R0
                ENDP

                ALIGN

                END
