;-------------------------------------------------------------------------------
.version 00.51.05
;-------------------------------------------------------------------------------



.set __top_of_stack 0x1c010000


.data __STACK__sect 0x1c010000

    shave_sp:
        .label shave_sp


.code __MAIN__sect
.salign 16


    .lalign
    main:
        LSU1.LDIL i30 puts
            ||  LSU0.LDIL i18 ___str
        LSU1.LDIH i30 puts
            ||  LSU0.LDIH i18 ___str
        LSU0.LDIL i19 shave_sp
            ||  BRU.SWP i30
        LSU0.LDIH i19 shave_sp
        NOP 4
        LSU0.LDIL i18 0
        BRU.SWIH 0x001f
            ||  NOP
        NOP 4




.data __static_data__Peaffbb69_b7603ac6T_
.salign 16

    ___.str:

        .byte 68, 111, 105, 110, 103, 32, 110, 111, 116, 104, 105, 110, 103, 46, 46, 46
        .byte 10, 0

    ___str:

        .byte 68, 111, 105, 110, 103, 32, 110, 111, 116, 104, 105, 110, 103, 46, 46, 46
        .byte 0




.unset __top_of_stack

.end


