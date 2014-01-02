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
.set in7      i7
.set in8      i8
.set inWidth  i15

.set P0       v0
.set P1       v1
.set P2       v2
.set P3       v3
.set P4       v4
.set P5       v5
.set P6       v6
.set P7       v7
.set P8       v8
.code .text.Convolution9x1_asm
.salign 16

;void Convolution9x1_asm(u8 **in(i18), u8 **out(i17), half* filter(i16), u32 inWidth(i15))
Convolution9x1_asm:
    LSU0.LDIL i2, 255          || LSU1.LDIL i10, 0
    LSU0.LDIL i11 __loopEnd9x1 || LSU1.LDIH i11 __loopEnd9x1
    LSU0.LDO64.l v15, i16, 0   || LSU1.LDO64.h v15, i16, 8  ; load the filter in v15
    LSU0.LDO64.l v16, i16, 16
    ; Initialize lines pointers
    ; lines[n] = *(in + n);
    LSU0.LD32 in0, i18 || IAU.ADD i18, i18, 4 || CMU.CPIVR.x16 v22, i2 ; load clamp value in v22
    LSU0.LD32 in1, i18 || IAU.ADD i18, i18, 4 || CMU.CPVV.i16.f16 v22, v22
    LSU0.LD32 in2, i18 || IAU.ADD i18, i18, 4
    LSU0.LD32 in3, i18 || IAU.ADD i18, i18, 4
    LSU0.LD32 in4, i18 || IAU.ADD i18, i18, 4
    LSU0.LD32 in5, i18 || IAU.ADD i18, i18, 4
    LSU0.LD32 in6, i18 || IAU.ADD i18, i18, 4
    LSU0.LD32 in7, i18 || IAU.ADD i18, i18, 4
    LSU0.LD32 in8, i18 || IAU.SHR.u32 i15, i15, 3 || LSU1.LD32 i17, i17

    ;-----------------------------------------------------------
    LSU0.LDI128.u8.f16 P0, in0 || BRU.RPL i11, i15
    LSU0.LDI128.u8.f16 P1, in1
    LSU0.LDI128.u8.f16 P2, in2
    LSU0.LDI128.u8.f16 P3, in3
    LSU0.LDI128.u8.f16 P4, in4
    LSU0.LDI128.u8.f16 P5, in5
    LSU0.LDI128.u8.f16 P6, in6
    LSU0.LDI128.u8.f16 P7, in7
    LSU0.LDI128.u8.f16 P8, in8
    VAU.MACPZ.f16 P0, v15      || LSU1.SWZV8 [00000000]
    VAU.MACP.f16 P1, v15       || LSU1.SWZV8 [11111111]
    VAU.MACP.f16 P2, v15       || LSU1.SWZV8 [22222222]
__loopEnd9x1:
    VAU.MACP.f16 P3, v15       || LSU1.SWZV8 [33333333]
    VAU.MACP.f16 P4, v15       || LSU1.SWZV8 [44444444]
    VAU.MACP.f16 P5, v15       || LSU1.SWZV8 [55555555] || CMU.CLAMP0.f16 v20 v20 v22
    VAU.MACP.f16 P6, v15       || LSU1.SWZV8 [66666666]
    VAU.MACP.f16 P7, v15       || LSU1.SWZV8 [77777777] || LSU0.ST128.f16.u8 v20, i17 || IAU.ADD i17, i17, i10
    VAU.MACPW.f16 v20, P8, v16 || LSU1.SWZV8 [00000000] || LSU0.LDIL i10, 8

    NOP 7
    BRU.JMP i30
    NOP
    CMU.CLAMP0.f16 v20 v20 v22
    NOP
    LSU0.ST128.f16.u8 v20, i17
    NOP
.end
