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
;                             i18         i17              i16          i15         s23

LSU1.LDIL i7 0
     ||  LSU0.LDIH i7 0x3f80
CMU.CPIS s15 i7
SAU.SUB.f32 s15 s15 s23
LSU0.ldil i1, ___vect_mask || LSU1.ldih i1, ___vect_mask
LSU0.LD64.l   v0  i1 || IAU.ADD i1 i1 8    
LSU0.LD64.h   v0  i1    ; vector full of ones
CMU.CPSVR.x32    v1 s23     ;vector with alpha value
LSU1.LD32  i18  i18 || LSU0.LD32  i17  i17
LSU0.LD32  i16  i16
LSU0.ldil i2, ___loop   || LSU1.ldih i2, ___loop
LSU0.ldil i10, ___compensation_loop   || LSU1.ldih i10, ___compensation_loop


IAU.SHR.u32 i5  i15  5 ;calculate how many pixels will remain 
IAU.SHL  i9  i5  5
IAU.SUB i6 i15 i9   ;width mod 32
IAU.SUB i7 i7 i7 ;contor for loop

VAU.SUB.f32  v0 v0 v1 ; 1-alpha
LSU0.ldil i13, 0x20 || LSU1.ldih i13, 0x0




IAU.SUB i19 i19 8
LSU0.ST64.l v24  i19  || IAU.SUB i19 i19 8
LSU0.ST64.h v24  i19  || IAU.SUB i19 i19 8
LSU0.ST64.l v25  i19  || IAU.SUB i19 i19 8
LSU0.ST64.h v25  i19  || IAU.SUB i19 i19 8
LSU0.ST64.l v26  i19  || IAU.SUB i19 i19 8
LSU0.ST64.h v26  i19  || IAU.SUB i19 i19 8
LSU0.ST64.l v27  i19  || IAU.SUB i19 i19 8
LSU0.ST64.h v27  i19  || IAU.SUB i19 i19 8
LSU0.ST64.l v28  i19  || IAU.SUB i19 i19 8
LSU0.ST64.h v28  i19  || IAU.SUB i19 i19 8
LSU0.ST64.l v29  i19  || IAU.SUB i19 i19 8
LSU0.ST64.h v29  i19  || IAU.SUB i19 i19 8
LSU0.ST64.l v30  i19  || IAU.SUB i19 i19 8
LSU0.ST64.h v30  i19  || IAU.SUB i19 i19 8
LSU0.ST64.l v31  i19  || IAU.SUB i19 i19 8
LSU0.ST64.h v31  i19

CMU.CMII.i32 i15 i13
PEU.PC1C LT ||  BRU.BRA _____compensation || IAU.SUB  i6 i15 0
nop 5

___loop_begin:

	LSU0.LD128.u8.f16 v2 i18  || IAU.ADD i18 i18 8 || bru.rpl i2, i5
	LSU0.LD128.u8.f16 v3 i18  || IAU.ADD i18 i18 8
	LSU0.LD128.u8.f16 v4 i18  || IAU.ADD i18 i18 8
	LSU0.LD128.u8.f16 v5 i18  || IAU.ADD i18 i18 8


	LSU0.LD128.u8.f16 v6 i17 || IAU.ADD i17 i17 8
    LSU0.LD128.u8.f16 v7 i17 || IAU.ADD i17 i17 8
    LSU0.LD128.u8.f16 v8 i17 || IAU.ADD i17 i17 8 || CMU.CPVV.f16l.f32 v10 v2
    LSU0.LD128.u8.f16 v9 i17 || IAU.ADD i17 i17 8 || CMU.CPVV.f16h.f32 v11 v2 || vau.mul.f32 v10 v10 v1
    
	CMU.CPVV.f16l.f32 v12 v3 || vau.mul.f32 v11 v11 v1 || LSU0.LD64.l  v26 i16         || LSU1.LDO64.h v26 i16, (8*1)
    CMU.CPVV.f16h.f32 v13 v3 || vau.mul.f32 v12 v12 v1 || LSU0.LDO64.l v27 i16, (8*2)  || LSU1.LDO64.h v27 i16, (8*3)

	CMU.CPVV.f16l.f32 v14 v4 || vau.mul.f32 v13 v13 v1 || LSU0.LDO64.l v28 i16  (8*4)  || LSU1.LDO64.h v28 i16, (8*5) 
    CMU.CPVV.f16h.f32 v15 v4 || vau.mul.f32 v14 v14 v1 || LSU0.LDO64.l v29 i16, (8*6)  || LSU1.LDO64.h v29 i16, (8*7)
    CMU.CPVV.f16l.f32 v16 v5 || vau.mul.f32 v15 v15 v1 || LSU0.LDO64.l v30 i16  (8*8)  || LSU1.LDO64.h v30 i16, (8*9) 
    CMU.CPVV.f16h.f32 v17 v5 || vau.mul.f32 v16 v16 v1 ||  LSU0.LDO64.l v31 i16, (8*10) || LSU1.LDO64.h v31 i16, (8*11) 

    CMU.CPVV.f16l.f32 v18 v6 || vau.mul.f32 v17 v17 v1 || LSU0.LDO64.l v2 i16   (8*12) || LSU1.LDO64.h v2 i16,  (8*13) 
    CMU.CPVV.f16h.f32 v19 v6 || LSU0.LDO64.l v3 i16,  (8*14) || LSU1.LDO64.h v3 i16,  (8*15) || vau.mul.f32 v4 v26 v0
    CMU.CPVV.f16l.f32 v20 v7 || vau.mul.f32 v5 v27 v0
    CMU.CPVV.f16h.f32 v21 v7 || vau.mul.f32 v6 v28 v0

    CMU.CPVV.f16l.f32 v22 v8 || vau.mul.f32 v7 v29 v0
    CMU.CPVV.f16h.f32 v23 v8
    CMU.CPVV.f16l.f32 v24 v9 || vau.mul.f32 v8 v30 v0
    CMU.CPVV.f16h.f32 v25 v9
	CMU.CMZ.f32 v18 || vau.mul.f32 v9 v31 v0
    PEU.PVV32 GT || VAU.ADD.f32 v26, v4, v10
	CMU.CMZ.f32 v19 || vau.mul.f32 v10 v3 v0
    PEU.PVV32 GT || VAU.ADD.f32 v27, v5, v11
	LSU0.ST64.l v26 i16  || LSU1.STO64.h v26 i16 (8*1)
	CMU.CMZ.f32 v20
    PEU.PVV32 GT || VAU.ADD.f32 v28, v6, v12
	CMU.CMZ.f32 v21 || vau.mul.f32 v26 v2 v0 || LSU0.STO64.l v27 i16 (8*2)  || LSU1.STO64.h v27 i16 (8*3)
    PEU.PVV32 GT || VAU.ADD.f32 v29, v7, v13
	CMU.CMZ.f32 v22 || LSU0.STO64.l v28 i16 (8*4)  || LSU1.STO64.h v28 i16 (8*5)
    PEU.PVV32 GT || VAU.ADD.f32 v30, v8, v14
	CMU.CMZ.f32 v23 || LSU0.STO64.l v29 i16 (8*6)  || LSU1.STO64.h v29 i16 (8*7)
    PEU.PVV32 GT || VAU.ADD.f32 v31, v9, v15
	CMU.CMZ.f32 v24 || LSU0.STO64.l v30 i16 (8*8)  || LSU1.STO64.h v30 i16 (8*9)
    PEU.PVV32 GT || VAU.ADD.f32 v2, v26, v16
	
___loop:
	CMU.CMZ.f32 v25
    PEU.PVV32 GT || VAU.ADD.f32 v3, v10, v17
    LSU0.STO64.l v31 i16 (8*10) || LSU1.STO64.h v31 i16 (8*11)
    LSU0.STO64.l v2 i16  (8*12) || LSU1.STO64.h v2 i16  (8*13)
    LSU0.STO64.l v3 i16  (8*14) || LSU1.STO64.h v3 i16  (8*15)
	IAU.ADD i16 i16 (32*4)

_____compensation:
	CMU.CMZ.i32 i6
    PEU.PC1C EQ ||  BRU.BRA _____final
	nop 5
	
	LSU0.LD32.u8.u32   i1  i18 || IAU.ADD i18 i18 1 || bru.rpl i10, i6
	LSU0.LD32.u8.u32   i2  i17 || IAU.ADD i17 i17 1
	LSU0.LD32   s1  i16
	nop  3
	CMU.CPIS.i32.f32  s2 i1 ;src img
	CMU.CMZ.i32 i2
	PEU.PC1C NEQ || SAU.mul.f32 s3 s2 s23
	PEU.PC1C NEQ || SAU.mul.f32 s4 s1 s15
	nop 
	___compensation_loop:
	nop
    PEU.PC1C NEQ || SAU.add.f32 s5 s3 s4
	nop 2
	PEU.PC1C NEQ || LSU0.ST32 s5 i16
	IAU.ADD i16 i16 4
	
	
_____final:
LSU0.LD64.h  v31  i19 || IAU.ADD i19 i19 8    
LSU0.LD64.l  v31  i19 || IAU.ADD i19 i19 8 
LSU0.LD64.h  v30  i19 || IAU.ADD i19 i19 8    
LSU0.LD64.l  v30  i19 || IAU.ADD i19 i19 8
LSU0.LD64.h  v29  i19 || IAU.ADD i19 i19 8    
LSU0.LD64.l  v29  i19 || IAU.ADD i19 i19 8
LSU0.LD64.h  v28  i19 || IAU.ADD i19 i19 8    
LSU0.LD64.l  v28  i19 || IAU.ADD i19 i19 8
LSU0.LD64.h  v27  i19 || IAU.ADD i19 i19 8    
LSU0.LD64.l  v27  i19 || IAU.ADD i19 i19 8
LSU0.LD64.h  v26  i19 || IAU.ADD i19 i19 8    
LSU0.LD64.l  v26  i19 || IAU.ADD i19 i19 8
LSU0.LD64.h  v25  i19 || IAU.ADD i19 i19 8    
LSU0.LD64.l  v25  i19 || IAU.ADD i19 i19 8
LSU0.LD64.h  v24  i19 || IAU.ADD i19 i19 8    
LSU0.LD64.l  v24  i19 || IAU.ADD i19 i19 8
nop 5

BRU.JMP i30
NOP 5
.end
 

