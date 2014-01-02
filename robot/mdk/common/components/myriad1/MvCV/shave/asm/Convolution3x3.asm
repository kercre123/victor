; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief
; ///

.version 00.51.04

.data .data.convolution3x3

.salign 16
    ___clampVal:
        .float16 255.0


.code .text.convolution3x3

;void Convolution3x3_asm(u8** in(i18), u8** out(i17), f16 conv[3*3](i16), u32 inWidth(i15))
Convolution3x3_asm: 
    LSU0.LD32  i1  i18 || IAU.ADD i18 i18 4 
    lsu0.ldil i4, ___clampVal     ||  lsu1.ldih i4, ___clampVal
    LSU0.LD64.l v15 i16 || IAU.ADD i16 i16 6
    LSU0.LD32  i2  i18 || IAU.ADD i18 i18 4
    LSU0.LD64.l v16 i16  || IAU.ADD i16 i16 6
    LSU0.LD32  i3  i18 || IAU.ADD i18 i18 4 || LSU1.LD32  i17  i17
    LSU0.LD64.l v17 i16  || IAU.ADD i16 i16 6
    LSU0.LDIL i10 ___forLoop || LSU1.LDIH i10 ___forLoop ; loads the label for the loop in i0
    lsu0.ld16r v20, i4
    IAU.SHR.u32 i11 i15 3   ; i1 = i15/8 because we process 8 pixels at once
    nop 1

    LSU0.LD128.u8.f16 v0 i1  || IAU.ADD i1 i1 8
    LSU0.LD128.u8.f16 v1 i2  || IAU.ADD i2 i2 8
    LSU0.LD128.u8.f16 v2 i3  || IAU.ADD i3 i3 8
    LSU0.LD128.u8.f16 v3 i1
    LSU0.LD128.u8.f16 v4 i2
    LSU0.LD128.u8.f16 v5 i3
    nop 2

    nop 8
    VAU.ALIGNVEC v6 v0 v3 2
    VAU.ALIGNVEC v7 v0 v3 4
    VAU.ALIGNVEC v8 v1 v4 2 
    VAU.ALIGNVEC v9 v1 v4 4
    VAU.ALIGNVEC v10 v2 v5 2
    VAU.ALIGNVEC v11 v2 v5 4

___forLoopHead:
    VAU.MACPZ.f16       v0  v15 || lsu0.swzv8 [00000000] || bru.rpl i10 i11 ; i1 = i18[0][0]
    VAU.MACP.f16        v6  v15 || lsu0.swzv8 [11111111]
    VAU.MACP.f16        v7  v15 || lsu0.swzv8 [22222222]
    VAU.MACP.f16        v1  v16 || lsu0.swzv8 [00000000]
    VAU.MACP.f16        v8  v16 || lsu0.swzv8 [11111111]
    VAU.MACP.f16        v9  v16 || lsu0.swzv8 [22222222]
    VAU.MACP.f16        v2  v17 || lsu0.swzv8 [00000000]
    VAU.MACP.f16        v10 v17 || lsu0.swzv8 [11111111]
    VAU.MACPW.f16 v19   v11 v17 || lsu0.swzv8 [22222222]

    LSU0.LD128.u8.f16 v0 i1  || IAU.ADD i1 i1 8
    LSU0.LD128.u8.f16 v1 i2  || IAU.ADD i2 i2 8
    LSU0.LD128.u8.f16 v2 i3  || IAU.ADD i3 i3 8
    LSU0.LD128.u8.f16 v3 i1
    LSU0.LD128.u8.f16 v4 i2
    LSU0.LD128.u8.f16 v5 i3
    nop

    nop 6

    VAU.ALIGNVEC v6 v0 v3 2
    VAU.ALIGNVEC v7 v0 v3 4

    ___forLoop:
        VAU.ALIGNVEC v8 v1 v4 2
        VAU.ALIGNVEC v9 v1 v4 4
        VAU.ALIGNVEC v10 v2 v5 2
        CMU.CLAMP0.F16 v19 v19 v20
        VAU.ALIGNVEC v11 v2 v5 4
        LSU0.ST128.f16.u8 v19 i17 || IAU.ADD i17 i17 8

    BRU.jmp i30
    nop 5

.end
 
 
