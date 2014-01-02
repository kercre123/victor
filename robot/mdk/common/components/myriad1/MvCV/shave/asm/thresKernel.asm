; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief
; ///

.version 00.51.04

.data .data.thresholdKernel

.code .text.thresholdKernel
;void threshKernel(u8** in(i18), u8** out(i17), u32 width(i16), u32 height(i15), u8 thresh(i14), u32 thresh_type(i13))

thresholdKernel_asm:
	LSU0.LD32 i18 i18     ;input
	LSU0.LD32 i17 i17     ;output
	LSU1.LDIL i7 0xFF   || IAU.SUB i6 i6 i6
	LSU1.LDIL i3 1        ;Thresh_type = Threshold_To_Zero_Inv
	LSU1.LDIL i4 2        ;Thresh_type = Threshold_To_Binary
	LSU1.LDIL i5 3        ;Thresh_type = Threshold_To_Binary_Inv
	LSU1.LDIL i10 4       ;Thresh_type = Threshold_Trunc
	LSU0.LDIL i2 0x000f   || IAU.SHR.u32 i8 i16 4  || CMU.CPIVR.x8 v5 i14   ;copies i14=thresh into v5(transmitted by usr)
	CMU.CPIVR.x8 v7 i7    || IAU.AND i11 i16 i2    ;last 4 remaining pixels

CMU.CMII.u8 i13 i6 || LSU0.LDIL i1 ___thresholdToZero_loop   || LSU1.LDIH i1 ___thresholdToZero_loop
PEU.PC1C EQ || BRU.BRA ___thresholdToZero_loop_head
nop 5

CMU.CMII.u8 i13 i3 || LSU0.LDIL i1 ___thresholdToZeroInv_loop   || LSU1.LDIH i1 ___thresholdToZeroInv_loop
PEU.PC1C EQ   || BRU.BRA ___thresholdToZeroInv_loop_head
nop 5

CMU.CMII.u8 i13 i4 || LSU0.LDIL i1 ___thresholdToBinary_loop    || LSU1.LDIH i1 ___thresholdToBinary_loop
PEU.PC1C EQ  || BRU.BRA ___thresholdToBinary_loop_head
nop 5

CMU.CMII.u8 i13 i5 || LSU0.LDIL i1 ___thresholdToBinaryInv_loop || LSU1.LDIH i1 ___thresholdToBinaryInv_loop
PEU.PC1C EQ  || BRU.BRA ___thresholdToBinaryInv_loop_head
nop 5

CMU.CMII.u8 i13 i10 || LSU0.LDIL i1 ___thresholdTrunc_loop      || LSU1.LDIH i1 ___thresholdTrunc_loop
PEU.PC1C GTE || BRU.BRA ___thresholdTrunc_loop_head
nop 5


___thresholdToZero_loop_head:
.lalign
CMU.CMZ.i8 i8
PEU.PC1C EQ || BRU.BRA ___thresholdToZero_compensation
nop 5
LSU0.LDO64.l v1 i18 0 || LSU1.LDO64.h v1 i18 8 || IAU.ADD i18 i18 16 || BRU.RPL i1 i8
nop 4
___thresholdToZero_loop:
CMU.CPIVR.x8 v6 i6
CMU.CMVV.u8 v1 v5
PEU.PVV8 GT || VAU.ADD.i8 v6 v6 v1
nop 2
LSU0.STO64.l v6 i17 0 || LSU1.STO64.h v6 i17 8 || IAU.ADD i17 i17 16

;LAST UP TO 15 PIXELS!
___thresholdToZero_compensation:
LSU0.LDO64.l v1 i18 0 || LSU1.LDO64.h v1 i18 8 || IAU.ADD i18 i18 16
CMU.CPIVR.x8 v6 i6
nop 4
CMU.CMVV.u8 v1 v5
PEU.PVV8 GT || VAU.ADD.i8 v6 v6 v1

LSU0.LDIL i2, 0xFFFF  || LSU1.LDIH i2, 0xFFFF  || IAU.SUB i0, i0, i0
IAU.SUB i12, i12, i12
IAU.FINSJ i12, i2, i11
CMU.CMASK v2, i12, 0
CMU.CMZ.i8 v2 || VAU.SWZWORD v2, v2, [3232]
PEU.PVL0S8  NEQ || LSU0.STO64.l v6 i17 0 || CMU.CMZ.i8 v2
PEU.PVL1S8  NEQ || LSU1.STO64.h v6 i17 8

BRU.jmp i30
nop 5

___thresholdToZeroInv_loop_head:
.lalign
CMU.CMZ.i8 i8
PEU.PC1C EQ || BRU.BRA ___thresholdToZeroInv_compensation
nop 5
LSU0.LDO64.l v1 i18 0 || LSU1.LDO64.h v1 i18 8 || IAU.ADD i18 i18 16 || BRU.RPL i1 i8
nop 4
___thresholdToZeroInv_loop:
CMU.CPIVR.x8 v6 i6
CMU.CMVV.u8 v1 v5
PEU.PVV8 LTE || VAU.ADD.i8 v6 v6 v1
nop 2
LSU0.STO64.l v6 i17 0 || LSU1.STO64.h v6 i17 8 || IAU.ADD i17 i17 16

;LAST UP TO 15 PIXELS!
___thresholdToZeroInv_compensation:
LSU0.LDO64.l v1 i18 0 || LSU1.LDO64.h v1 i18 8 || IAU.ADD i18 i18 16
CMU.CPIVR.x8 v6 i6
nop 4
CMU.CMVV.u8 v1 v5
PEU.PVV8 LTE || VAU.ADD.i8 v6 v6 v1

LSU0.LDIL i2, 0xFFFF || LSU1.LDIH i2, 0xFFFF || IAU.SUB i0, i0, i0
IAU.SUB i12, i12, i12
IAU.FINSJ i12, i2, i11
CMU.CMASK v2, i12, 0
CMU.CMZ.i8 v2 || VAU.SWZWORD v2, v2, [3232] ;swizzle for placing on lower half the sup half;
PEU.PVL0S8  NEQ || LSU0.STO64.l v6 i17 0 || CMU.CMZ.i8 v2
PEU.PVL1S8  NEQ || LSU1.STO64.h v6 i17 8

BRU.jmp i30
nop 5


___thresholdToBinary_loop_head:
.lalign
CMU.CMZ.i8 i8
PEU.PC1C EQ || BRU.BRA ___thresholdToBinary_compensation
nop 5
LSU0.LDO64.l v1 i18 0 || LSU1.LDO64.h v1 i18 8 || IAU.ADD i18 i18 16 || BRU.RPL i1 i8
nop 4

___thresholdToBinary_loop:
CMU.CPIVR.x8 v6 i6
CMU.CMVV.u8 v1 v5
PEU.PVV8 GT || VAU.ADD.i8 v6 v6 v7
nop 2
LSU0.STO64.l v6 i17 0 || LSU1.STO64.h v6 i17 8 || IAU.ADD i17 i17 16

;LAST UP TO 15 PIXELS!
___thresholdToBinary_compensation:
LSU0.LDO64.l v1 i18 0 || LSU1.LDO64.h v1 i18 8 || IAU.ADD i18 i18 16
CMU.CPIVR.x8 v6 i6
nop 4
CMU.CMVV.u8 v1 v5
PEU.PVV8 GT || VAU.ADD.i8 v6 v6 v7

LSU0.LDIL i2, 0xFFFF || LSU1.LDIH i2, 0xFFFF || IAU.SUB i0, i0, i0
IAU.SUB i12, i12, i12
IAU.FINSJ i12, i2, i11
CMU.CMASK v2, i12, 0
CMU.CMZ.i8 v2 || VAU.SWZWORD v2, v2, [3232]
PEU.PVL0S8  NEQ || LSU0.STO64.l v6 i17 0 || CMU.CMZ.i8 v2
PEU.PVL1S8  NEQ || LSU1.STO64.h v6 i17 8

BRU.jmp i30
nop 5


___thresholdToBinaryInv_loop_head:
.lalign
CMU.CMZ.i8 i8
PEU.PC1C EQ || BRU.BRA ___thresholdToBinaryInv_compensation
nop 5
LSU0.LDO64.l v1 i18 0 || LSU1.LDO64.h v1 i18 8 || IAU.ADD i18 i18 16 || BRU.RPL i1 i8
nop 4

___thresholdToBinaryInv_loop:
CMU.CPIVR.x8 v6 i6
CMU.CMVV.u8 v1 v5
PEU.PVV8 LTE || VAU.ADD.i8 v6 v6 v7
nop 2
LSU0.STO64.l v6 i17 0 || LSU1.STO64.h v6 i17 8 || IAU.ADD i17 i17 16

;LAST UP TO 15 PIXELS!
___thresholdToBinaryInv_compensation:
LSU0.LDO64.l v1 i18 0 || LSU1.LDO64.h v1 i18 8 || IAU.ADD i18 i18 16
CMU.CPIVR.x8 v6 i6
nop 4
CMU.CMVV.u8 v1 v5
PEU.PVV8 LTE || VAU.ADD.i8 v6 v6 v7

LSU0.LDIL i2, 0xFFFF || LSU1.LDIH i2, 0xFFFF || IAU.SUB i0, i0, i0
IAU.SUB i12, i12, i12
IAU.FINSJ i12, i2, i11
CMU.CMASK v2, i12, 0
CMU.CMZ.i8 v2 || VAU.SWZWORD v2, v2, [3232]
PEU.PVL0S8  NEQ || LSU0.STO64.l v6 i17 0 || CMU.CMZ.i8 v2
PEU.PVL1S8  NEQ || LSU1.STO64.h v6 i17 8

BRU.jmp i30
nop 5


___thresholdTrunc_loop_head:
.lalign
CMU.CMZ.i8 i8
PEU.PC1C EQ || BRU.BRA ___thresholdTrunc_compensation
nop 5
LSU0.LDO64.l v1 i18 0 || LSU1.LDO64.h v1 i18 8 || IAU.ADD i18 i18 16 || BRU.RPL i1 i8
nop 4
___thresholdTrunc_loop:
CMU.CPIVR.x8 v6 i6
CMU.CMVV.u8 v1 v5
PEU.PVV8 GT  || VAU.ADD.i8 v6 v6 v5
PEU.PVV8 LTE || VAU.ADD.i8 v6 v6 v1
nop
LSU0.STO64.l v6 i17 0 || LSU1.STO64.h v6 i17 8 || IAU.ADD i17 i17 16

;LAST UP TO 15 PIXELS!
___thresholdTrunc_compensation:
LSU0.LDO64.l v1 i18 0 || LSU1.LDO64.h v1 i18 8 || IAU.ADD i18 i18 16
CMU.CPIVR.x8 v6 i6
nop 4
CMU.CMVV.u8 v1 v5
PEU.PVV8 GT  || VAU.ADD.i8 v6 v6 v5
PEU.PVV8 LTE || VAU.ADD.i8 v6 v6 v1

LSU0.LDIL i2, 0xFFFF ||LSU1.LDIH i2, 0xFFFF || IAU.SUB i0, i0, i0
IAU.SUB i12, i12, i12
IAU.FINSJ i12, i2, i11
CMU.CMASK v2, i12, 0
CMU.CMZ.i8 v2 || VAU.SWZWORD v2, v2, [3232]
PEU.PVL0S8  NEQ || LSU0.STO64.l v6 i17 0 || CMU.CMZ.i8 v2
PEU.PVL1S8  NEQ || LSU1.STO64.h v6 i17 8

BRU.jmp i30
nop 5

.end
