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
	___multiply:
		.float16    0.04F16


.code .text.boxfilter5x5 ;text
.salign 16

boxfilter5x5_asm:
;void boxfilter5x5(u8** in, u8** out, u8 normalize, u32 width);
;                     i18      i17        i16          i15
	LSU0.ldil i5, ___second || LSU1.ldih i5, ___second
	LSU0.ldil i13, 0x1 || LSU1.ldih i13, 0x0
	CMU.CMII.U8 i16 i13
	PEU.PC1C NEQ || BRU.JMP i5
	nop 5

lsu0.ldil i7, ___multiply     ||  lsu1.ldih i7, ___multiply
	LSU0.LD32  i0  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i1  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i2  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i3  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i4  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i17  i17
	lsu0.ld16r v0, i7
	nop 5
	lsu0.ldil i7, ___loop     ||  lsu1.ldih i7, ___loop
	IAU.SHR.u32 i15 i15 3
	VAU.XOR v18 v18 v18
	VAU.XOR v19 v19 v19
	VAU.XOR v20 v20 v20

	
	LSU0.LD128.u8.f16 v1 i0   || IAU.ADD i0 i0 1 || VAU.XOR v21 v21 v21
	LSU0.LD128.u8.f16 v2 i1   || IAU.ADD i1 i1 1 || VAU.XOR v22 v22 v22
	LSU0.LD128.u8.f16 v3 i2   || IAU.ADD i2 i2 1 || VAU.XOR v23 v23 v23
	LSU0.LD128.u8.f16 v4 i3   || IAU.ADD i3 i3 1 || VAU.XOR v18 v18 v18 
	LSU0.LD128.u8.f16 v5 i4   || IAU.ADD i4 i4 1 || VAU.XOR v19 v19 v19
	
	LSU0.LD128.u8.f16 v6 i0    || IAU.ADD i0 i0 1 || VAU.XOR v20 v20 v20
	LSU0.LD128.u8.f16 v7 i1    || IAU.ADD i1 i1 1 || VAU.ADD.f16 v18 v18 v1 
	LSU0.LD128.u8.f16 v8 i2    || IAU.ADD i2 i2 1 || VAU.ADD.f16 v19 v19 v2 || bru.rpl i7 i15
	LSU0.LD128.u8.f16 v9 i3    || IAU.ADD i3 i3 1 || VAU.ADD.f16 v20 v20 v3
	LSU0.LD128.u8.f16 v10 i4   || IAU.ADD i4 i4 1 || VAU.ADD.f16 v18 v18 v4	
	
	LSU0.LD128.u8.f16 v11 i0   || IAU.ADD i0 i0 1 || VAU.ADD.f16 v19 v19 v5
	LSU0.LD128.u8.f16 v12 i1   || IAU.ADD i1 i1 1 || VAU.ADD.f16 v20 v20 v6
	LSU0.LD128.u8.f16 v13 i2   || IAU.ADD i2 i2 1 || VAU.ADD.f16 v18 v18 v7
	LSU0.LD128.u8.f16 v14 i3   || IAU.ADD i3 i3 1 || VAU.ADD.f16 v19 v19 v8
	LSU0.LD128.u8.f16 v15 i4   || IAU.ADD i4 i4 1 || VAU.ADD.f16 v20 v20 v9
	
	LSU0.LD128.u8.f16 v16 i0   || IAU.ADD i0 i0 1 || VAU.ADD.f16 v21 v18 v10 
	LSU0.LD128.u8.f16 v17 i1   || IAU.ADD i1 i1 1 || VAU.ADD.f16 v22 v19 v11
	LSU0.LD128.u8.f16 v1  i2   || IAU.ADD i2 i2 1 || VAU.ADD.f16 v23 v20 v12
	LSU0.LD128.u8.f16 v2  i3   || IAU.ADD i3 i3 1 || VAU.ADD.f16 v21 v21 v13
	LSU0.LD128.u8.f16 v3  i4   || IAU.ADD i4 i4 1 || VAU.ADD.f16 v22 v22 v14
	
	LSU0.LD128.u8.f16 v4 i0   || IAU.ADD i0 i0 4 || VAU.ADD.f16 v23 v23 v15
	LSU0.LD128.u8.f16 v5 i1   || IAU.ADD i1 i1 4 || VAU.ADD.f16 v21 v21 v16
	LSU0.LD128.u8.f16 v6 i2   || IAU.ADD i2 i2 4 || VAU.ADD.f16 v22 v22 v17
	LSU0.LD128.u8.f16 v7 i3   || IAU.ADD i3 i3 4 || VAU.ADD.f16 v23 v23 v1
	LSU0.LD128.u8.f16 v8 i4   || IAU.ADD i4 i4 4 || VAU.ADD.f16 v21 v21 v2
	VAU.ADD.f16 v22 v22 v3
	VAU.ADD.f16 v23 v23 v4
	VAU.ADD.f16 v21 v21 v5
	LSU0.LD128.u8.f16 v1 i0   || IAU.ADD i0 i0 1 || VAU.ADD.f16 v22 v22 v6
	LSU0.LD128.u8.f16 v2 i1   || IAU.ADD i1 i1 1 || VAU.ADD.f16 v23 v23 v7
	LSU0.LD128.u8.f16 v3 i2   || IAU.ADD i2 i2 1 || VAU.ADD.f16 v21 v21 v8
	nop 2
	VAU.ADD.f16 v22 v21 v22
	LSU0.LD128.u8.f16 v4 i3   || IAU.ADD i3 i3 1 || VAU.XOR v18 v18 v18 
	nop
	VAU.ADD.f16 v23 v22 v23
	___loop:
	LSU0.LD128.u8.f16 v5 i4   || IAU.ADD i4 i4 1 || VAU.XOR v19 v19 v19
	nop
	VAU.mul.f16 v23 v23 v0
	LSU0.LD128.u8.f16 v6 i0    || IAU.ADD i0 i0 1 || VAU.XOR v20 v20 v20
	nop
	LSU0.STi128.f16.u8 v23 i17 || LSU1.LD128.u8.f16 v7 i1    || SAU.ADD.i32 i1 i1 1 || VAU.ADD.f16 v18 v18 v1 
	
	
	
	bru.jmp i30
	nop 5

	___second:
	
	LSU0.LD32  i0  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i1  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i2  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i3  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i4  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i17  i17
	nop 5
	lsu0.ldil i7, ___loop1     ||  lsu1.ldih i7, ___loop1
	IAU.SHR.u32 i15 i15 3
	VAU.XOR v18 v18 v18
	VAU.XOR v19 v19 v19
	VAU.XOR v20 v20 v20
	VAU.XOR v21 v21 v21
	VAU.XOR v22 v22 v22
	VAU.XOR v23 v23 v23
	
	
	LSU0.LD128.u8.u16 v1 i0   || IAU.ADD i0 i0 1 
	LSU0.LD128.u8.u16 v2 i1   || IAU.ADD i1 i1 1
	LSU0.LD128.u8.u16 v3 i2   || IAU.ADD i2 i2 1
	LSU0.LD128.u8.u16 v4 i3   || IAU.ADD i3 i3 1 || VAU.XOR v18 v18 v18 || bru.rpl i7 i15
	LSU0.LD128.u8.u16 v5 i4   || IAU.ADD i4 i4 1 || VAU.XOR v19 v19 v19
	
	LSU0.LD128.u8.u16 v6 i0    || IAU.ADD i0 i0 1 || VAU.XOR v20 v20 v20
	LSU0.LD128.u8.u16 v7 i1    || IAU.ADD i1 i1 1 || VAU.ADD.i16 v18 v18 v1
	LSU0.LD128.u8.u16 v8 i2    || IAU.ADD i2 i2 1 || VAU.ADD.i16 v19 v19 v2
	LSU0.LD128.u8.u16 v9 i3    || IAU.ADD i3 i3 1 || VAU.ADD.i16 v20 v20 v3
	LSU0.LD128.u8.u16 v10 i4   || IAU.ADD i4 i4 1 || VAU.ADD.i16 v18 v18 v4	
	
	LSU0.LD128.u8.u16 v11 i0   || IAU.ADD i0 i0 1 || VAU.ADD.i16 v19 v19 v5
	LSU0.LD128.u8.u16 v12 i1   || IAU.ADD i1 i1 1 || VAU.ADD.i16 v20 v20 v6
	LSU0.LD128.u8.u16 v13 i2   || IAU.ADD i2 i2 1 || VAU.ADD.i16 v18 v18 v7
	LSU0.LD128.u8.u16 v14 i3   || IAU.ADD i3 i3 1 || VAU.ADD.i16 v19 v19 v8
	LSU0.LD128.u8.u16 v15 i4   || IAU.ADD i4 i4 1 || VAU.ADD.i16 v20 v20 v9
	
	LSU0.LD128.u8.u16 v16 i0   || IAU.ADD i0 i0 1 || VAU.ADD.i16 v21 v18 v10 
	LSU0.LD128.u8.u16 v17 i1   || IAU.ADD i1 i1 1 || VAU.ADD.i16 v22 v19 v11
	LSU0.LD128.u8.u16 v1  i2   || IAU.ADD i2 i2 1 || VAU.ADD.i16 v23 v20 v12
	LSU0.LD128.u8.u16 v2  i3   || IAU.ADD i3 i3 1 || VAU.ADD.i16 v21 v21 v13
	LSU0.LD128.u8.u16 v3  i4   || IAU.ADD i4 i4 1 || VAU.ADD.i16 v22 v22 v14
	
	LSU0.LD128.u8.u16 v4 i0   || IAU.ADD i0 i0 4 || VAU.ADD.i16 v23 v23 v15
	LSU0.LD128.u8.u16 v5 i1   || IAU.ADD i1 i1 4 || VAU.ADD.i16 v21 v21 v16
	LSU0.LD128.u8.u16 v6 i2   || IAU.ADD i2 i2 4 || VAU.ADD.i16 v22 v22 v17
	LSU0.LD128.u8.u16 v7 i3   || IAU.ADD i3 i3 4 || VAU.ADD.i16 v23 v23 v1
	LSU0.LD128.u8.u16 v8 i4   || IAU.ADD i4 i4 4 || VAU.ADD.i16 v21 v21 v2
	VAU.ADD.i16 v22 v22 v3
	VAU.ADD.i16 v23 v23 v4
	VAU.ADD.i16 v21 v21 v5
	VAU.ADD.i16 v22 v22 v6
	VAU.ADD.i16 v23 v23 v7
	VAU.ADD.i16 v21 v21 v8
	LSU0.LD128.u8.u16 v1 i0   || IAU.ADD i0 i0 1 
		___loop1:
	VAU.ADD.i16 v22 v21 v22
	LSU0.LD128.u8.u16 v2 i1   || IAU.ADD i1 i1 1
	VAU.ADD.i16 v23 v22 v23

	LSU0.LD128.u8.u16 v3 i2   || IAU.ADD i2 i2 1
	LSU0.STi64.l v23 i17
	LSU0.STi64.h v23 i17
	
	bru.jmp i30
	nop 5
.end