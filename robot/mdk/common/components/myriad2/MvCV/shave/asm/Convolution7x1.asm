; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief
; ///

.version 00.70.00

.set in0      i0
.set in1      i1
.set in2      i2
.set in3      i3
.set in4      i4
.set in5      i5
.set in6      i6
.set inWidth  i15

.set P0       v0
.set P1       v1
.set P2       v2
.set P3       v3
.set P4       v4
.set P5       v5
.set P6       v6

.code .text.Convolution7x1_asm
.salign 16

;void Convolution7x1_asm(u8 **in(i18), u8 **out(i17), half* filter(i16), u32 inWidth(i15))
Convolution7x1_asm:

    ; Manage data
    IAU.SUB i19 i19 8
    LSU0.ST.64.L v20 i19 || IAU.SUB i19 i19 8
    LSU0.ST.64.H v20 i19 || IAU.SUB i19 i19 8
    LSU0.ST.64.L v22 i19 || IAU.SUB i19 i19 8
    LSU0.ST.64.H V22 i19
    
    LSU0.LDIL i2, 255          || LSU1.LDIL i10, 0
    LSU0.LDIL i11 __loopEnd7x1 || LSU1.LDIH i11 __loopEnd7x1
    LSU0.LDO.64.L v15, i16, 0   || LSU1.LDO.64.H v15, i16, 8  ; load the filter in v15
    ; Initialize lines pointers
    ; lines[n] = *(in + n);
    LSU0.LD.32 in0, i18 || IAU.ADD i18, i18, 4     || CMU.CPIVR.x16 v22, i2 ; load clamp value in v22
    LSU0.LD.32 in1, i18 || IAU.ADD i18, i18, 4     || CMU.CPVV.i16.f16 v22, v22
    LSU0.LD.32 in2, i18 || IAU.ADD i18, i18, 4
    LSU0.LD.32 in3, i18 || IAU.ADD i18, i18, 4
    LSU0.LD.32 in4, i18 || IAU.ADD i18, i18, 4
    LSU0.LD.32 in5, i18 || IAU.ADD i18, i18, 4
    LSU0.LD.32 in6, i18 || IAU.SHR.u32 i15, i15, 3 || LSU1.LD.32 i17, i17

    ;-----------------------------------------------------------
    LSU0.LDI.128.U8.F16 P0, in0 || BRU.RPL i11, i15
    LSU0.LDI.128.U8.F16 P1, in1
    LSU0.LDI.128.U8.F16 P2, in2
    LSU0.LDI.128.U8.F16 P3, in3
    LSU0.LDI.128.U8.F16 P4, in4
    LSU0.LDI.128.U8.F16 P5, in5
    LSU0.LDI.128.U8.F16 P6, in6
    VAU.MACPZ.f16 v15, P0        || LSU1.SWZV8 [00000000]
    VAU.MACP.f16  v15, P1        || LSU1.SWZV8 [11111111]
__loopEnd7x1:
    VAU.MACP.f16  v15, P2        || LSU1.SWZV8 [22222222]
    VAU.MACP.f16  v15, P3        || LSU1.SWZV8 [33333333]
    VAU.MACP.f16  v15, P4        || LSU1.SWZV8 [44444444] || CMU.CLAMP0.f16 v20 v20 v22
    VAU.MACP.f16  v15, P5        || LSU1.SWZV8 [55555555]
    VAU.MACPW.f16 v20, v15, P6   || LSU1.SWZV8 [66666666] || LSU0.ST.128.F16.U8 v20, i17 || IAU.ADD i17, i17, i10
    LSU0.LDIL i10, 8
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
