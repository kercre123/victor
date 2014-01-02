; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief
; ///

.version 00.51.04

.data .data.minMaxKernel

.code .text.minMaxKernel
;void minMaxLocat_asm(u8** in, u32 width, u32 height, u8* minVal, u8* maxVal, u8* maskAddr);
;                      i18      i17          i16        i15          i14           i13
minMaxKernel_asm:
LSU0.LD.32 i10 i18 || IAU.SUB i4 i4 i4; loads input img
LSU0.LDIL i0 ___minMax_loop || LSU1.LDIH i0 ___minMax_loop  || IAU.SUB i2 i2 i2
IAU.SUB i5 i5 i5
LSU0.LD.8 i5 i15   ;minVal
LSU0.LD.8 i4 i14  || IAU.SUB i7 i7 i7    ;maxVal
IAU.SHR.u32 i1 i17 4; nr of repetitions i1 = 17/16(processing 8 pixels at once)
LSU1.LDIL i8 0xFF || IAU.AND i11 i17 i2
CMU.CPIVR.x8 v4 i8
nop 5
;given the fact that minimum value should be kept in mind from one line to another, one will compute the diff between 0xff
;  and the previous computed minimum and retain it`s value in i7(for the first line it is not needed to proceed this step)
IAU.SUB i7 i8 i5
CMU.CPIVR.x8 v7 i4  ; maxVal copied in vec.
CMU.CPIVR.x8 v8 i7


;loads input data pixels in v3 & v1(one is used form minimum and one for maximum)
___minMax_loop_head:
; (Myriad2 COMPATIBILITY ISSUE): BRU.RPL: changed order of parameters at line number 35, in minMaxValue.asm
    LSU0.LDO.64.L v3 i10 0 || LSU1.LDO.64.H v3 i10 8 || BRU.RPL i0 i1
    LSU0.LDO.64.L v1 i10 0 || LSU1.LDO.64.H v1 i10 8 || IAU.ADD i10 i10 16
    LSU0.LDO.64.L v2 i13 0 || LSU1.LDO.64.H v2 i13 8 || IAU.ADD i13 i13 16
nop 5


___minMax_loop:
        ;for minimum calc, from 0xff one decreases the value of the input img
        VAU.SUB.i8 v3 v4 v3
        ;multiply pixel values with mask values
        VAU.MUL.i8 v1 v1 v2
        ;multiply pixel values with mask values
        VAU.MUL.i8 v3 v3 v2
nop
        ;max operation needed for finding out the maximum values
        CMU.MAX.u8 v7 v1 v7
        CMU.MAX.u8 v8 v3 v8
nop

___store_minMax:
        CMU.MAX.u8 v7, v7, v7 || LSU0.SWZMC4.BYTE [2301] [UUUU]
        CMU.MAX.u8 v8, v8, v8 || LSU1.SWZMC4.BYTE [2301] [UUUU]
        CMU.MAX.u8 v7, v7, v7 || LSU0.SWZMC4.BYTE [1032] [UUUU]
        CMU.MAX.u8 v8, v8, v8 || LSU1.SWZMC4.BYTE [1032] [UUUU]
        CMU.MAX.u8 v7, v7, v7 || LSU0.SWZMC4.WORD [2301] [UUUU]
        CMU.MAX.u8 v8, v8, v8 || LSU1.SWZMC4.WORD [2301] [UUUU]
        CMU.MAX.u8 v7, v7, v7 || LSU0.SWZMC4.WORD [1032] [UUUU]
        CMU.MAX.u8 v8, v8, v8 || LSU1.SWZMC4.WORD [1032] [UUUU] ;last performed swizzle operation leads to a vrf
                                                       ;                filled with max computed values
nop 2

        VAU.SUB.i8 v8 v4 v8 ;if from 0xff one decreases the max value previously obtained in v8, in fact,
                            ;                            will dispose of the minimum value from the image
nop

        CMU.CPVI i5 v8.3
        CMU.CPVI i4 v7.3 || IAU.SHR.u32 i5 i5 24
        IAU.SHR.u32 i4 i4 24

        LSU0.ST.8 i5 i15 ;stores the smallest value found in the img
        LSU1.ST.8 i4 i14 ; stores the highest value found in the img


bru.jmp i30
nop 6




.end
