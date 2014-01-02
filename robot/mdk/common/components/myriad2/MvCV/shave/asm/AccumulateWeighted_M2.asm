; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief
; ///

.version 00.51.04

.data .rodata.AccumulateWeighted
___vect_mask:
		.float32    1, 1, 1, 1

.code .text.AccumulateWeighted

AccumulateWeighted_asm:
;void AccumulateWeighted(u8** srcAddr, u8** maskAddr, fp32** destAddr,u32 width, fp32 alpha)
;                             i18         i17              i16          i15         i113
IAU.SUB i19 i19 4
LSU0.ST.32  i20  i19 

LSU1.LDIL i7 0
     ||  LSU0.LDIH i7 0x3f80
; (Myriad2 COMPATIBILITY ISSUE): CMU.CPIS: SRF version no longer available at line number 23, in AccumulateWeighted.asm
CMU.CPII i0 i7
; (Myriad2 COMPATIBILITY ISSUE): SAU.SUB.F32: SRF version no longer available at line number 24, in AccumulateWeighted.asm
SAU.SUB.f32 i0 i0 i14
LSU0.ldil i1, ___vect_mask || LSU1.ldih i1, ___vect_mask
LSU0.LD.64.L   v0  i1 || IAU.ADD i1 i1 8    
LSU0.LD.64.H   v0  i1    ; vector full of ones
; (Myriad2 COMPATIBILITY ISSUE): CMU.CPSV: SRF version no longer available at line number 28, in AccumulateWeighted.asm
CMU.CPIVR.x32    v1 i14     ;vector with alpha value
LSU1.LD.32  i18  i18 || LSU0.LD.32  i17  i17
LSU0.LD.32  i16  i16
LSU0.ldil i2, ___loop   || LSU1.ldih i2, ___loop
LSU0.ldil i10, ___compensation_loop   || LSU1.ldih i10, ___compensation_loop


IAU.SHR.u32 i5  i15  5 ;calculate how many pixels will remain 
IAU.SHL  i9  i5  5
IAU.SUB i6 i15 i9   ;width mod 32
IAU.SUB i7 i7 i7 ;contor for loop

VAU.SUB.f32  v0 v0 v1 ; 1-alpha
LSU0.ldil i13, 0x20 || LSU1.ldih i13, 0x0




IAU.SUB i19 i19 8
LSU0.ST.64.L v24  i19  || IAU.SUB i19 i19 8
LSU0.ST.64.H v24  i19  || IAU.SUB i19 i19 8
LSU0.ST.64.L v25  i19  || IAU.SUB i19 i19 8
LSU0.ST.64.H v25  i19  || IAU.SUB i19 i19 8
LSU0.ST.64.L v26  i19  || IAU.SUB i19 i19 8
LSU0.ST.64.H v26  i19  || IAU.SUB i19 i19 8
LSU0.ST.64.L v27  i19  || IAU.SUB i19 i19 8
LSU0.ST.64.H v27  i19  || IAU.SUB i19 i19 8
LSU0.ST.64.L v28  i19  || IAU.SUB i19 i19 8
LSU0.ST.64.H v28  i19  || IAU.SUB i19 i19 8
LSU0.ST.64.L v29  i19  || IAU.SUB i19 i19 8
LSU0.ST.64.H v29  i19  || IAU.SUB i19 i19 8
LSU0.ST.64.L v30  i19  || IAU.SUB i19 i19 8
LSU0.ST.64.H v30  i19  || IAU.SUB i19 i19 8
LSU0.ST.64.L v31  i19  || IAU.SUB i19 i19 8
LSU0.ST.64.H v31  i19

CMU.CMII.i32 i15 i13
PEU.PC1C LT ||  BRU.BRA _____compensation || IAU.SUB  i6 i15 0
nop 6

___loop_begin:

; (Myriad2 COMPATIBILITY ISSUE): BRU.RPL: changed order of parameters at line number 70, in AccumulateWeighted.asm
	LSU0.LD.128.U8.F16 v2 i18  || IAU.ADD i18 i18 8 || bru.rpl i2, i5
	LSU0.LD.128.U8.F16 v3 i18  || IAU.ADD i18 i18 8
	LSU0.LD.128.U8.F16 v4 i18  || IAU.ADD i18 i18 8
	LSU0.LD.128.U8.F16 v5 i18  || IAU.ADD i18 i18 8


	LSU0.LD.128.U8.F16 v6 i17 || IAU.ADD i17 i17 8
    LSU0.LD.128.U8.F16 v7 i17 || IAU.ADD i17 i17 8
	nop
    LSU0.LD.128.U8.F16 v8 i17 || IAU.ADD i17 i17 8 || CMU.CPVV.f16l.f32 v10 v2
    LSU0.LD.128.U8.F16 v9 i17 || IAU.ADD i17 i17 8 || CMU.CPVV.f16h.f32 v11 v2 || vau.mul.f32 v10 v10 v1
    
	CMU.CPVV.f16l.f32 v12 v3 || vau.mul.f32 v11 v11 v1 || LSU0.LD.64.L  v26 i16         || LSU1.LDO.64.H v26 i16, (8*1)
    CMU.CPVV.f16h.f32 v13 v3 || vau.mul.f32 v12 v12 v1 || LSU0.LDO.64.L v27 i16, (8*2)  || LSU1.LDO.64.H v27 i16, (8*3)

	CMU.CPVV.f16l.f32 v14 v4 || vau.mul.f32 v13 v13 v1 || LSU0.LDO.64.L v28 i16  (8*4)  || LSU1.LDO.64.H v28 i16, (8*5) 
    CMU.CPVV.f16h.f32 v15 v4 || vau.mul.f32 v14 v14 v1 || LSU0.LDO.64.L v29 i16, (8*6)  || LSU1.LDO.64.H v29 i16, (8*7)
    CMU.CPVV.f16l.f32 v16 v5 || vau.mul.f32 v15 v15 v1 || LSU0.LDO.64.L v30 i16  (8*8)  || LSU1.LDO.64.H v30 i16, (8*9) 
    CMU.CPVV.f16h.f32 v17 v5 || vau.mul.f32 v16 v16 v1 ||  LSU0.LDO.64.L v31 i16, (8*10) || LSU1.LDO.64.H v31 i16, (8*11) 

    CMU.CPVV.f16l.f32 v18 v6 || vau.mul.f32 v17 v17 v1 || LSU0.LDO.64.L v2 i16   (8*12) || LSU1.LDO.64.H v2 i16,  (8*13) 
    CMU.CPVV.f16h.f32 v19 v6 || LSU0.LDO.64.L v3 i16,  (8*14) || LSU1.LDO.64.H v3 i16,  (8*15) || vau.mul.f32 v4 v26 v0
    CMU.CPVV.f16l.f32 v20 v7 || vau.mul.f32 v5 v27 v0
    CMU.CPVV.f16h.f32 v21 v7 || vau.mul.f32 v6 v28 v0

    CMU.CPVV.f16l.f32 v22 v8 || vau.mul.f32 v7 v29 v0
    CMU.CPVV.f16h.f32 v23 v8
    CMU.CPVV.f16l.f32 v24 v9 || vau.mul.f32 v8 v30 v0
    CMU.CPVV.f16h.f32 v25 v9
; (Myriad2 COMPATIBILITY ISSUE): CMU.CMZ.F32: SRF version no longer available at line number 98, in AccumulateWeighted.asm
	CMU.CMZ.f32 v18 || vau.mul.f32 v9 v31 v0
    PEU.PVV32 GT || VAU.ADD.f32 v26, v4, v10
; (Myriad2 COMPATIBILITY ISSUE): CMU.CMZ.F32: SRF version no longer available at line number 100, in AccumulateWeighted.asm
	CMU.CMZ.f32 v19 || vau.mul.f32 v10 v3 v0
    PEU.PVV32 GT || VAU.ADD.f32 v27, v5, v11
	LSU0.ST.64.L v26 i16  || LSU1.STO.64.H v26 i16 (8*1)
; (Myriad2 COMPATIBILITY ISSUE): CMU.CMZ.F32: SRF version no longer available at line number 103, in AccumulateWeighted.asm
	CMU.CMZ.f32 v20
    PEU.PVV32 GT || VAU.ADD.f32 v28, v6, v12
; (Myriad2 COMPATIBILITY ISSUE): CMU.CMZ.F32: SRF version no longer available at line number 105, in AccumulateWeighted.asm
	CMU.CMZ.f32 v21 || vau.mul.f32 v26 v2 v0 || LSU0.STO.64.L v27 i16 (8*2)  || LSU1.STO.64.H v27 i16 (8*3)
    PEU.PVV32 GT || VAU.ADD.f32 v29, v7, v13
; (Myriad2 COMPATIBILITY ISSUE): CMU.CMZ.F32: SRF version no longer available at line number 107, in AccumulateWeighted.asm
	CMU.CMZ.f32 v22 || LSU0.STO.64.L v28 i16 (8*4)  || LSU1.STO.64.H v28 i16 (8*5)
    PEU.PVV32 GT || VAU.ADD.f32 v30, v8, v14
; (Myriad2 COMPATIBILITY ISSUE): CMU.CMZ.F32: SRF version no longer available at line number 109, in AccumulateWeighted.asm
	CMU.CMZ.f32 v23 || LSU0.STO.64.L v29 i16 (8*6)  || LSU1.STO.64.H v29 i16 (8*7)
    PEU.PVV32 GT || VAU.ADD.f32 v31, v9, v15
; (Myriad2 COMPATIBILITY ISSUE): CMU.CMZ.F32: SRF version no longer available at line number 111, in AccumulateWeighted.asm
	CMU.CMZ.f32 v24 || LSU0.STO.64.L v30 i16 (8*8)  || LSU1.STO.64.H v30 i16 (8*9)
    PEU.PVV32 GT || VAU.ADD.f32 v2, v26, v16
	
___loop:
; (Myriad2 COMPATIBILITY ISSUE): CMU.CMZ.F32: SRF version no longer available at line number 115, in AccumulateWeighted.asm
	CMU.CMZ.f32 v25
    PEU.PVV32 GT || VAU.ADD.f32 v3, v10, v17
    LSU0.STO.64.L v31 i16 (8*10) || LSU1.STO.64.H v31 i16 (8*11)
    LSU0.STO.64.L v2 i16  (8*12) || LSU1.STO.64.H v2 i16  (8*13)
    LSU0.STO.64.L v3 i16  (8*14) || LSU1.STO.64.H v3 i16  (8*15)
	IAU.ADD i16 i16 (32*4)

_____compensation:
	CMU.CMZ.i32 i6
    PEU.PC1C EQ ||  BRU.BRA _____final
nop 6
	
; (Myriad2 COMPATIBILITY ISSUE): BRU.RPL: changed order of parameters at line number 127, in AccumulateWeighted.asm
	LSU0.LD.32.u8.u32   i1  i18 || IAU.ADD i18 i18 1 || bru.rpl i10, i6
	LSU0.LD.32.u8.u32   i2  i17 || IAU.ADD i17 i17 1
; (Myriad2 COMPATIBILITY ISSUE): LSU0.LD32: renamed to LSU0.LD.32; SRF version no longer available at line number 129, in AccumulateWeighted.asm
	LSU0.LD.32   i20  i16
nop  4
; (Myriad2 COMPATIBILITY ISSUE): CMU.CPII.I32.F32: SRF version no longer available at line number 131, in AccumulateWeighted.asm
	CMU.CPII.i32.f32  i11 i1 ;src img
	CMU.CMZ.i32 i2
; (Myriad2 COMPATIBILITY ISSUE): SAU.MUL.F32: SRF version no longer available at line number 133, in AccumulateWeighted.asm
	PEU.PC1C NEQ || SAU.mul.f32 i8 i11 i14
; (Myriad2 COMPATIBILITY ISSUE): SAU.MUL.F32: SRF version no longer available at line number 134, in AccumulateWeighted.asm
	PEU.PC1C NEQ || SAU.mul.f32 i4 i20 i0
nop
___compensation_loop:
nop
; (Myriad2 COMPATIBILITY ISSUE): SAU.ADD.F32: SRF version no longer available at line number 138, in AccumulateWeighted.asm
    PEU.PC1C NEQ || SAU.add.f32 i3 i8 i4
nop 2
; (Myriad2 COMPATIBILITY ISSUE): LSU0.ST32: SRF version no longer available at line number 140, in AccumulateWeighted.asm
	PEU.PC1C NEQ || LSU0.ST.32 i3 i16
	IAU.ADD i16 i16 4
	
	
_____final:
LSU0.LD.64.H  v31  i19 || IAU.ADD i19 i19 8    
LSU0.LD.64.L  v31  i19 || IAU.ADD i19 i19 8 
LSU0.LD.64.H  v30  i19 || IAU.ADD i19 i19 8    
LSU0.LD.64.L  v30  i19 || IAU.ADD i19 i19 8
LSU0.LD.64.H  v29  i19 || IAU.ADD i19 i19 8    
LSU0.LD.64.L  v29  i19 || IAU.ADD i19 i19 8
LSU0.LD.64.H  v28  i19 || IAU.ADD i19 i19 8    
LSU0.LD.64.L  v28  i19 || IAU.ADD i19 i19 8
LSU0.LD.64.H  v27  i19 || IAU.ADD i19 i19 8    
LSU0.LD.64.L  v27  i19 || IAU.ADD i19 i19 8
LSU0.LD.64.H  v26  i19 || IAU.ADD i19 i19 8    
LSU0.LD.64.L  v26  i19 || IAU.ADD i19 i19 8
LSU0.LD.64.H  v25  i19 || IAU.ADD i19 i19 8    
LSU0.LD.64.L  v25  i19 || IAU.ADD i19 i19 8
LSU0.LD.64.H  v24  i19 || IAU.ADD i19 i19 8    
LSU0.LD.64.L  v24  i19 || IAU.ADD i19 i19 8
LSU0.LD.32    i20  i19 || IAU.ADD i19 i19 4
nop 5

BRU.JMP i30
NOP 5
.end
