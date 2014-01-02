; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief
; ///

.version 00.70.00

.code .text.Convolution1x7_asm
.salign 16

;void Convolution1x7_asm(u8 **in(i18), u8 **out(i17), half* filter(i16), u32 inWidth(i15))
Convolution1x7_asm:
    LSU0.LDIL i5, 255         || LSU1.LDIL i7, 0
    LSU0.LD32 i18, i18        || LSU1.LD32 i17, i17
    LSU0.LDIL i8 __loopEnd1x7 || LSU1.LDIH i8 __loopEnd1x7
    LSU0.LDO64.l v10 i16 0    || LSU1.LDO64.h v10 i16 8  ; load the filter in v10
    CMU.CPIVR.x16 v22, i5                              ; load clamp value in v22
    CMU.CPVV.i16.f16 v22, v22
    IAU.SHR.u32 i15, i15, 3                            ; divide line width by 8 (number of pixels processed in one batch)
    LSU0.LD128.u8.f16 v0, i18  || IAU.ADD i18, i18, 8   ; load first bacth of 8 pixels from linein
    LSU0.LD128.u8.f16 v15, i18 || IAU.ADD i18, i18, 8
    NOP 4

    ;-----------------------------------------------------------
    CMU.VROT v1, v0, 2         || BRU.RPL i8, i15
    CMU.CPVI.x32 i1, v15.0
    CMU.CPIV.x16 v1.7, i1.l
    CMU.VROT v2, v0, 4
    CMU.CPIV.x32 v2.3, i1
    CMU.CPVI.x32 i2, v15.1
    CMU.VROT v3, v2, 2
    CMU.CPIV.x16 v3.7, i2.l
    VAU.ALIGNVEC v4, v0, v15, 8
    VAU.ALIGNVEC v5, v0, v15, 10
    VAU.ALIGNVEC v6, v0, v15, 12 || LSU0.LD128.u8.f16 v15, i18 || IAU.ADD i18, i18, 8

    VAU.MACPZ.f16 v0, v10      || LSU1.SWZV8 [00000000]
    VAU.MACP.f16 v1, v10       || LSU1.SWZV8 [11111111]     || CMU.CPVV v0, v15
__loopEnd1x7:
    VAU.MACP.f16 v2, v10       || LSU1.SWZV8 [22222222]     || CMU.CLAMP0.f16 v20 v20 v22
    VAU.MACP.f16 v3, v10       || LSU1.SWZV8 [33333333]
    VAU.MACP.f16 v4, v10       || LSU1.SWZV8 [44444444]
    VAU.MACP.f16 v5, v10       || LSU1.SWZV8 [55555555]
    VAU.MACPW.f16 v20, v6, v10 || LSU1.SWZV8 [66666666]     || LSU0.ST128.f16.u8 v20, i17 || IAU.ADD i17, i17, i7
    LSU0.LDIL i7, 8
    NOP 6
    BRU.JMP i30
    NOP
    CMU.CLAMP0.f16 v20 v20 v22
    NOP
    LSU0.ST128.f16.u8 v20, i17
    NOP
.end

 
