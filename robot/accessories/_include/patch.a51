$NOMOD51
; Bare minimum patch startup - we need to set our stack pointer past our reserved data
; There's no returning to the boot loader

                NAME    ?C_STARTUP
          
?C_C51STARTUP   SEGMENT   CODE
?STACK          SEGMENT   IDATA

                RSEG    ?STACK
                DS      1
          
        ; Set aside space for HAL LED/Timer ISR
        DSEG AT 8h
        DS 8
        ; Set aside space for LED temporaries
        DSEG AT 70h
        DS 4

                EXTRN CODE (?C_START)
                PUBLIC  ?C_STARTUP

                CSEG    AT      0
?C_STARTUP:     LJMP    STARTUP1

                RSEG    ?C_C51STARTUP

STARTUP1:
; This MUST be the first instruction
                MOV     SP,#?STACK-1
                AJMP    ?C_START

                END
