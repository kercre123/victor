; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief
; ///

;--------------------------------------------------------------------------------------------------------------
.version 00.50.05
.data .rodata.vect_mask 
.salign 16
	___vect_mask:
		.float16    0.1111
	___vect_mask2:
		.short    1


.code .text.boxfilter3x3 ;text
.salign 16

boxfilter3x3_asm:
;void boxfilter3x3(u8** in, u8** out, u8 normalize, u32 width)
;                    i18      i17        i16          i15
	LSU0.ldil i5, ___second || LSU1.ldih i5, ___second
	LSU0.ldil i13, 0x1 || LSU1.ldih i13, 0x0
	CMU.CMII.U8 i16 i13
	PEU.PC1C NEQ || BRU.JMP i5
	nop 5
	
	
 	lsu0.ldil i7, ___vect_mask     ||  lsu1.ldih i7, ___vect_mask
	LSU0.LD32  i0  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i1  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i2  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i17  i17
	lsu0.ld16r v0, i7
	nop 5
	lsu0.ldil i7, ___loop     ||  lsu1.ldih i7, ___loop
	IAU.SHR.u32 i15 i15 3
	VAU.XOR v10 v10 v10
	VAU.XOR v11 v11 v11
	VAU.XOR v12 v12 v12
	VAU.XOR v13 v13 v13
	VAU.XOR v14 v14 v14
	VAU.XOR v15 v15 v15
		
	LSU0.LD128.u8.f16 v1 i0   || IAU.ADD i0 i0 1 
	LSU0.LD128.u8.f16 v2 i1   || IAU.ADD i1 i1 1
	LSU0.LD128.u8.f16 v3 i2   || IAU.ADD i2 i2 1
	LSU0.LD128.u8.f16 v4 i0   || IAU.ADD i0 i0 1 || VAU.XOR v10 v10 v10 
	LSU0.LD128.u8.f16 v5 i1   || IAU.ADD i1 i1 1 || VAU.XOR v11 v11 v11
	LSU0.LD128.u8.f16 v6 i2   || IAU.ADD i2 i2 1 || VAU.XOR v12 v12 v12 
	LSU0.LD128.u8.f16 v7 i0   || IAU.ADD i0 i0 6 || VAU.ADD.f16 v10 v10 v1 
	LSU0.LD128.u8.f16 v8 i1   || IAU.ADD i1 i1 6 || VAU.ADD.f16 v11 v11 v2 || bru.rpl i7 i15
	LSU0.LD128.u8.f16 v9 i2   || IAU.ADD i2 i2 6 || VAU.ADD.f16 v12 v12 v3
	VAU.ADD.f16 v13 v10 v4
	VAU.ADD.f16 v14 v11 v5
	VAU.ADD.f16 v15 v12 v6
	LSU0.LD128.u8.f16 v1 i0   || IAU.ADD i0 i0 1 ||  VAU.ADD.f16 v13 v13 v7
	LSU0.LD128.u8.f16 v2 i1   || IAU.ADD i1 i1 1 || VAU.ADD.f16 v14 v14 v8
	LSU0.LD128.u8.f16 v3 i2   || IAU.ADD i2 i2 1 || VAU.ADD.f16 v15 v15 v9
	nop 2
	VAU.ADD.f16 v14 v13 v14
	LSU0.LD128.u8.f16 v4 i0   || IAU.ADD i0 i0 1 || VAU.XOR v10 v10 v10 
	nop
	VAU.ADD.f16 v15 v14 v15
	___loop:
	LSU0.LD128.u8.f16 v5 i1   || IAU.ADD i1 i1 1 || VAU.XOR v11 v11 v11
	nop
	VAU.MUL.f16 v15 v15 v0
	LSU0.LD128.u8.f16 v6 i2   || IAU.ADD i2 i2 1 || VAU.XOR v12 v12 v12 
	nop
	LSU0.STi128.f16.u8 v15 i17 || LSU1.LD128.u8.f16 v7 i0   || SAU.ADD.i32 i0 i0 6 || VAU.ADD.f16 v10 v10 v1 
	
	bru.jmp i30
	nop 5

	___second:
	LSU0.LD32  i0  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i1  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i2  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i17  i17

	nop 5
	lsu0.ldil i7, ___loop1     ||  lsu1.ldih i7, ___loop1
	IAU.SHR.u32 i15 i15 3
	VAU.XOR v10 v10 v10
	VAU.XOR v11 v11 v11
	VAU.XOR v12 v12 v12
	VAU.XOR v13 v13 v13
	VAU.XOR v14 v14 v14
	VAU.XOR v15 v15 v15
		
	LSU0.LD128.u8.u16 v1 i0   || IAU.ADD i0 i0 1 
	LSU0.LD128.u8.u16 v2 i1   || IAU.ADD i1 i1 1
	LSU0.LD128.u8.u16 v3 i2   || IAU.ADD i2 i2 1
	LSU0.LD128.u8.u16 v4 i0   || IAU.ADD i0 i0 1 || VAU.XOR v10 v10 v10 || bru.rpl i7 i15
	LSU0.LD128.u8.u16 v5 i1   || IAU.ADD i1 i1 1 || VAU.XOR v11 v11 v11
	LSU0.LD128.u8.u16 v6 i2   || IAU.ADD i2 i2 1 || VAU.XOR v12 v12 v12
	LSU0.LD128.u8.u16 v7 i0   || IAU.ADD i0 i0 6 || VAU.ADD.i16 v10 v10 v1
	LSU0.LD128.u8.u16 v8 i1   || IAU.ADD i1 i1 6 || VAU.ADD.i16 v11 v11 v2
	LSU0.LD128.u8.u16 v9 i2   || IAU.ADD i2 i2 6 || VAU.ADD.i16 v12 v12 v3
	VAU.ADD.i16 v13 v10 v4
	VAU.ADD.i16 v14 v11 v5
	VAU.ADD.i16 v15 v12 v6
	VAU.ADD.i16 v13 v13 v7
	VAU.ADD.i16 v14 v14 v8
	VAU.ADD.i16 v15 v15 v9
	LSU0.LD128.u8.u16 v1 i0   || IAU.ADD i0 i0 1 
		___loop1:
	VAU.ADD.i16 v14 v13 v14
	LSU0.LD128.u8.u16 v2 i1   || IAU.ADD i1 i1 1
	VAU.ADD.i16 v15 v14 v15
	LSU0.LD128.u8.u16 v3 i2   || IAU.ADD i2 i2 1
	LSU0.STi64.l v15 i17
	LSU0.STi64.h v15 i17
	
	bru.jmp i30
	nop 5
	
	
.end