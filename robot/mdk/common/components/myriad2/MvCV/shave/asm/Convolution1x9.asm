; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief
; ///

.version 00.70.00

.code .text.Convolution1x9_asm
.salign 16

;void Convolution1x9_asm(u8 **in(i18), u8 **out(i17), half* filter(i16), u32 inWidth(i15))
Convolution1x9_asm:

    ; Manage data
    IAU.SUB i19 i19 8
    LSU0.ST.64.L v20 i19 || IAU.SUB i19 i19 8
    LSU0.ST.64.H v20 i19 || IAU.SUB i19 i19 8
    LSU0.ST.64.L v22 i19 || IAU.SUB i19 i19 8
    LSU0.ST.64.H V22 i19

    LSU0.LDIL i5, 255         || LSU1.LDIL i7, 0
    LSU0.LD.32 i18, i18       || LSU1.LD.32 i17, i17
    LSU0.LDIL i8 __loopEnd1x9 || LSU1.LDIH i8 __loopEnd1x9
    LSU0.LDO.64.L v10 i16 0   || LSU1.LDO.64.H v10 i16 8  ; load the filter in v10 and v11
    LSU0.LDO.64.L v11 i16 16
    CMU.CPIVR.x16 v22, i5                                ; load clamp value in v22
    CMU.CPVV.i16.f16 v22, v22
    IAU.SHR.u32 i15, i15, 3                              ; divide line width by 8 (number of pixels processed in one batch)
    LSU0.LD.128.U8.F16 v0, i18  || IAU.ADD i18, i18, 8   ; load first bacth of 8 pixels from linein
    LSU0.LD.128.U8.F16 v15, i18 || IAU.ADD i18, i18, 8
NOP 5

    ;-----------------------------------------------------------

    CMU.ALIGNVEC v1, V0, v0, 2         || BRU.RPL i8, i15
    CMU.CPVI.x32 i1, v15.0
    CMU.CPIV.x16 v1.7, i1.l
    CMU.ALIGNVEC v2, V0, v0, 4
    CMU.CPIV.x32 v2.3, i1
    CMU.CPVI.x32 i2, v15.1
    CMU.ALIGNVEC v3, V2, v2, 2
    CMU.CPIV.x16 v3.7, i2.l
    CMU.ALIGNVEC v4, v0, v15, 8
    CMU.ALIGNVEC v5, v0, v15, 10
    CMU.ALIGNVEC v6, v0, v15, 12
    CMU.CPVV v8, v15
    CMU.ALIGNVEC v7, v0, v15, 14 || LSU0.LD.128.U8.F16 v15, i18 || IAU.ADD i18, i18, 8
    VAU.MACPZ.f16 v10, v0        || LSU1.SWZV8 [00000000]
    VAU.MACP.f16 v10, v1         || LSU1.SWZV8 [11111111]      || CMU.CPVV v0, v15
    VAU.MACP.f16 v10, v2         || LSU1.SWZV8 [22222222]      || CMU.CLAMP0.f16 v20 v20 v22
__loopEnd1x9:
    VAU.MACP.f16 v10, v3         || LSU1.SWZV8 [33333333]
    VAU.MACP.f16 v10, v4         || LSU1.SWZV8 [44444444]
    VAU.MACP.f16 v10, v5         || LSU1.SWZV8 [55555555]
    VAU.MACP.f16 v10, v6         || LSU1.SWZV8 [66666666]
    VAU.MACP.f16 v10, v7         || LSU1.SWZV8 [77777777]
    VAU.MACPW.f16 v20, v11, v8   || LSU1.SWZV8 [00000000]      || LSU0.ST.128.F16.U8 v20, i17 || IAU.ADD i17, i17, i7
    LSU0.LDIL i7, 8
    
NOP 8
    CMU.CLAMP0.f16 v20 v20 v22
NOP
    LSU0.ST.128.F16.U8 v20, i17
    
    LSU0.LD.64.H v22 i19 || IAU.ADD i19 i19 8
    LSU0.LD.64.L v22 i19 || IAU.ADD i19 i19 8
    LSU0.LD.64.H v20 i19 || IAU.ADD i19 i19 8
    LSU0.LD.64.L v20 i19 || IAU.ADD i19 i19 8
    
    BRU.JMP i30
    NOP 6
.end
