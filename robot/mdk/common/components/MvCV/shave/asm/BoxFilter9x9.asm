; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief
; ///

;--------------------------------------------------------------------------------------------------------------
.version 00.50.05
.data .rodata.boxfilter9x9_asm 
.salign 16
	___multiply:
		.float16    0.012345F16, 0


.code .text.boxfilter9x9_asm ;text
.salign 16

boxfilter9x9_asm:
;void boxfilter9x9(u8** in, u8** out, u8 normalize, u32 width);
;                     i18      i17        i16          i15
	LSU0.ldil i5, ___second || LSU1.ldih i5, ___second
	LSU0.ldil i13, 0x1 || LSU1.ldih i13, 0x0
	CMU.CMII.U8 i16 i13
	PEU.PC1C NEQ || BRU.JMP i5
	nop 5



lsu0.ldil i11, ___multiply     ||  lsu1.ldih i11, ___multiply
	LSU0.LD32  i0  i18 || IAU.ADD i18 i18 4 || VAU.XOR v18 v18 v18
	LSU0.LD32  i1  i18 || IAU.ADD i18 i18 4 || VAU.XOR v19 v19 v19
	LSU0.LD32  i2  i18 || IAU.ADD i18 i18 4 || VAU.XOR v20 v20 v20
	LSU0.LD32  i3  i18 || IAU.ADD i18 i18 4 || VAU.XOR v21 v21 v21
	LSU0.LD32  i4  i18 || IAU.ADD i18 i18 4 || VAU.XOR v22 v22 v22
	LSU0.LD32  i5  i18 || IAU.ADD i18 i18 4 || VAU.XOR v23 v23 v23
	LSU0.LD32  i6  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i7  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i8  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i17  i17
	lsu0.ld16r v0, i11 || IAU.ADD i11 i11 2
	nop 5
	lsu0.ldil i10, ___loop     ||  lsu1.ldih i10, ___loop
	IAU.SHR.u32 i15 i15 3
	
	
	
	
	
	
	;lsu0.ldil i15, 0x1     ||  lsu1.ldih i15, 0x0
	

	;first
	LSU0.LD128.u8.f16 v1   i0    || IAU.ADD i0 i0 1                ;first
	LSU0.LD128.u8.f16 v2   i1    || IAU.ADD i1 i1 1
	LSU0.LD128.u8.f16 v3   i2    || IAU.ADD i2 i2 1
	LSU0.LD128.u8.f16 v4   i3    || IAU.ADD i3 i3 1 || VAU.XOR v18 v18 v18 
	LSU0.LD128.u8.f16 v5   i4    || IAU.ADD i4 i4 1 || VAU.XOR v19 v19 v19
	LSU0.LD128.u8.f16 v6   i5    || IAU.ADD i5 i5 1 || VAU.XOR v20 v20 v20
	LSU0.LD128.u8.f16 v7   i6    || IAU.ADD i6 i6 1 || VAU.ADD.f16 v18 v18 v1  
	LSU0.LD128.u8.f16 v8   i7    || IAU.ADD i7 i7 1 || VAU.ADD.f16 v19 v19 v2 
	LSU0.LD128.u8.f16 v9   i8    || IAU.ADD i8 i8 1 || VAU.ADD.f16 v20 v20 v3 
	LSU0.LD128.u8.f16 v10  i0    || IAU.ADD i0 i0 1 || VAU.ADD.f16 v18 v18 v4 || bru.rpl i10 i15       ;second
	LSU0.LD128.u8.f16 v11  i1    || IAU.ADD i1 i1 1 || VAU.ADD.f16 v19 v19 v5 
	LSU0.LD128.u8.f16 v12  i2    || IAU.ADD i2 i2 1 || VAU.ADD.f16 v20 v20 v6 
	LSU0.LD128.u8.f16 v13  i3    || IAU.ADD i3 i3 1 || VAU.ADD.f16 v18 v18 v7 
	LSU0.LD128.u8.f16 v14  i4    || IAU.ADD i4 i4 1 || VAU.ADD.f16 v19 v19 v8 
	LSU0.LD128.u8.f16 v15  i5    || IAU.ADD i5 i5 1 || VAU.ADD.f16 v20 v20 v9 
	LSU0.LD128.u8.f16 v16  i6    || IAU.ADD i6 i6 1 || VAU.ADD.f16 v18 v18 v10
	LSU0.LD128.u8.f16 v17  i7    || IAU.ADD i7 i7 1 || VAU.ADD.f16 v19 v19 v11
	LSU0.LD128.u8.f16 v1   i8    || IAU.ADD i8 i8 1 || VAU.ADD.f16 v20 v20 v12
	LSU0.LD128.u8.f16 v2   i0    || IAU.ADD i0 i0 1 || VAU.ADD.f16 v18 v18 v13      ;third
	LSU0.LD128.u8.f16 v3   i1    || IAU.ADD i1 i1 1 || VAU.ADD.f16 v19 v19 v14
	LSU0.LD128.u8.f16 v4   i2    || IAU.ADD i2 i2 1 || VAU.ADD.f16 v20 v20 v15
	LSU0.LD128.u8.f16 v5   i3    || IAU.ADD i3 i3 1 || VAU.ADD.f16 v18 v18 v16
	LSU0.LD128.u8.f16 v6   i4    || IAU.ADD i4 i4 1 || VAU.ADD.f16 v19 v19 v17
	LSU0.LD128.u8.f16 v7   i5    || IAU.ADD i5 i5 1 || VAU.ADD.f16 v20 v20 v1 
	LSU0.LD128.u8.f16 v8   i6    || IAU.ADD i6 i6 1 || VAU.ADD.f16 v18 v18 v2 
	LSU0.LD128.u8.f16 v9   i7    || IAU.ADD i7 i7 1 || VAU.ADD.f16 v19 v19 v3 
	LSU0.LD128.u8.f16 v10  i8    || IAU.ADD i8 i8 1 || VAU.ADD.f16 v20 v20 v4 
	LSU0.LD128.u8.f16 v11  i0    || IAU.ADD i0 i0 1 || VAU.ADD.f16 v18 v18 v5       ;fourth
	LSU0.LD128.u8.f16 v12  i1    || IAU.ADD i1 i1 1 || VAU.ADD.f16 v19 v19 v6 
	LSU0.LD128.u8.f16 v13  i2    || IAU.ADD i2 i2 1 || VAU.ADD.f16 v20 v20 v7 
	LSU0.LD128.u8.f16 v14  i3    || IAU.ADD i3 i3 1 || VAU.ADD.f16 v18 v18 v8 
	LSU0.LD128.u8.f16 v15  i4    || IAU.ADD i4 i4 1 || VAU.ADD.f16 v19 v19 v9 
	LSU0.LD128.u8.f16 v16  i5    || IAU.ADD i5 i5 1 || VAU.ADD.f16 v20 v20 v10
	LSU0.LD128.u8.f16 v17  i6    || IAU.ADD i6 i6 1 || VAU.ADD.f16 v18 v18 v11
	LSU0.LD128.u8.f16 v1   i7    || IAU.ADD i7 i7 1 || VAU.ADD.f16 v19 v19 v12
	LSU0.LD128.u8.f16 v2   i8    || IAU.ADD i8 i8 1 || VAU.ADD.f16 v20 v20 v13
	LSU0.LD128.u8.f16 v3   i0    || IAU.ADD i0 i0 1 || VAU.ADD.f16 v18 v18 v14      ;fifth
	LSU0.LD128.u8.f16 v4   i1    || IAU.ADD i1 i1 1 || VAU.ADD.f16 v19 v19 v15
	LSU0.LD128.u8.f16 v5   i2    || IAU.ADD i2 i2 1 || VAU.ADD.f16 v20 v20 v16
	LSU0.LD128.u8.f16 v6   i3    || IAU.ADD i3 i3 1 || VAU.ADD.f16 v18 v18 v17
	LSU0.LD128.u8.f16 v7   i4    || IAU.ADD i4 i4 1 || VAU.ADD.f16 v19 v19 v1 
	LSU0.LD128.u8.f16 v8   i5    || IAU.ADD i5 i5 1 || VAU.ADD.f16 v20 v20 v2 
	LSU0.LD128.u8.f16 v9   i6    || IAU.ADD i6 i6 1 || VAU.ADD.f16 v18 v18 v3 
	LSU0.LD128.u8.f16 v10  i7    || IAU.ADD i7 i7 1 || VAU.ADD.f16 v19 v19 v4 
	LSU0.LD128.u8.f16 v11  i8    || IAU.ADD i8 i8 1 || VAU.ADD.f16 v20 v20 v5 
	LSU0.LD128.u8.f16 v12  i0    || IAU.ADD i0 i0 1 || VAU.ADD.f16 v18 v18 v6       ;sixth
	LSU0.LD128.u8.f16 v13  i1    || IAU.ADD i1 i1 1 || VAU.ADD.f16 v19 v19 v7 
	LSU0.LD128.u8.f16 v14  i2    || IAU.ADD i2 i2 1 || VAU.ADD.f16 v20 v20 v8 
	LSU0.LD128.u8.f16 v15  i3    || IAU.ADD i3 i3 1 || VAU.ADD.f16 v18 v18 v9 
	LSU0.LD128.u8.f16 v16  i4    || IAU.ADD i4 i4 1 || VAU.ADD.f16 v19 v19 v10
	LSU0.LD128.u8.f16 v17  i5    || IAU.ADD i5 i5 1 || VAU.ADD.f16 v20 v20 v11
	LSU0.LD128.u8.f16 v1   i6    || IAU.ADD i6 i6 1 || VAU.ADD.f16 v18 v18 v12
	LSU0.LD128.u8.f16 v2   i7    || IAU.ADD i7 i7 1 || VAU.ADD.f16 v19 v19 v13
	LSU0.LD128.u8.f16 v3   i8    || IAU.ADD i8 i8 1 || VAU.ADD.f16 v20 v20 v14
	LSU0.LD128.u8.f16 v4   i0    || IAU.ADD i0 i0 1 || VAU.ADD.f16 v21 v18 v15    ;seventh
	LSU0.LD128.u8.f16 v5   i1    || IAU.ADD i1 i1 1 || VAU.ADD.f16 v22 v19 v16
	LSU0.LD128.u8.f16 v6   i2    || IAU.ADD i2 i2 1 || VAU.ADD.f16 v23 v20 v17
	LSU0.LD128.u8.f16 v7   i3    || IAU.ADD i3 i3 1 || VAU.ADD.f16 v21 v21 v1 
	LSU0.LD128.u8.f16 v8   i4    || IAU.ADD i4 i4 1 || VAU.ADD.f16 v22 v22 v2 
	LSU0.LD128.u8.f16 v9   i5    || IAU.ADD i5 i5 1 || VAU.ADD.f16 v23 v23 v3 || lsu1.ld16r v18 i11 
	LSU0.LD128.u8.f16 v10  i6    || IAU.ADD i6 i6 1 || VAU.ADD.f16 v21 v21 v4 || lsu1.ld16r v19 i11
	LSU0.LD128.u8.f16 v11  i7    || IAU.ADD i7 i7 1 || VAU.ADD.f16 v22 v22 v5 || lsu1.ld16r v20 i11
	LSU0.LD128.u8.f16 v12  i8    || IAU.ADD i8 i8 1 || VAU.ADD.f16 v23 v23 v6 	
	LSU0.LD128.u8.f16 v13  i0    || IAU.ADD i0 i0 1 || VAU.ADD.f16 v21 v21 v7    ;eight
	LSU0.LD128.u8.f16 v14  i1    || IAU.ADD i1 i1 1 || VAU.ADD.f16 v22 v22 v8 
	LSU0.LD128.u8.f16 v15  i2    || IAU.ADD i2 i2 1 || VAU.ADD.f16 v23 v23 v9 
	LSU0.LD128.u8.f16 v16  i3    || IAU.ADD i3 i3 1 || VAU.ADD.f16 v21 v21 v10
	LSU0.LD128.u8.f16 v17  i4    || IAU.ADD i4 i4 1 || VAU.ADD.f16 v22 v22 v11
	LSU0.LD128.u8.f16 v1   i5    || IAU.ADD i5 i5 1 || VAU.ADD.f16 v23 v23 v12
	LSU0.LD128.u8.f16 v2   i6    || IAU.ADD i6 i6 1 || VAU.ADD.f16 v21 v21 v13
	LSU0.LD128.u8.f16 v3   i7    || IAU.ADD i7 i7 1 || VAU.ADD.f16 v22 v22 v14
	LSU0.LD128.u8.f16 v4   i8    || IAU.ADD i8 i8 1 || VAU.ADD.f16 v23 v23 v15                            
	LSU0.LD128.u8.f16 v5   i0    || IAU.ADD i0 i0 0 || VAU.ADD.f16 v21 v21 v16  ;nine
	LSU0.LD128.u8.f16 v6   i1    || IAU.ADD i1 i1 0 || VAU.ADD.f16 v22 v22 v17
	LSU0.LD128.u8.f16 v7   i2    || IAU.ADD i2 i2 0 || VAU.ADD.f16 v23 v23 v1 
	LSU0.LD128.u8.f16 v8   i3    || IAU.ADD i3 i3 0 || VAU.ADD.f16 v21 v21 v2 
	LSU0.LD128.u8.f16 v9   i4    || IAU.ADD i4 i4 0 || VAU.ADD.f16 v22 v22 v3 
	LSU0.LD128.u8.f16 v10  i5    || IAU.ADD i5 i5 0 || VAU.ADD.f16 v23 v23 v4 
	LSU0.LD128.u8.f16 v11  i6    || IAU.ADD i6 i6 0 || VAU.ADD.f16 v21 v21 v5 || CMU.cpvv v1 v5 || SAU.ADD.i32 i0 i0 1
	LSU0.LD128.u8.f16 v12  i7    || IAU.ADD i7 i7 0 || VAU.ADD.f16 v22 v22 v6 || CMU.cpvv v2 v6 || SAU.ADD.i32 i1 i1 1
	LSU0.LD128.u8.f16 v13  i8    || IAU.ADD i8 i8 0 || VAU.ADD.f16 v23 v23 v7 || CMU.cpvv v3 v7 || SAU.ADD.i32 i2 i2 1
	VAU.ADD.f16 v21 v21 v8       || CMU.cpvv v4 v8  || SAU.ADD.i32 i3 i3 1                                        
	VAU.ADD.f16 v22 v22 v9       || CMU.cpvv v5 v9  || SAU.ADD.i32 i4 i4 1                                        
	VAU.ADD.f16 v23 v23 v10      || CMU.cpvv v6 v10 || SAU.ADD.i32 i5 i5 1                                         
	VAU.ADD.f16 v21 v21 v11      || CMU.cpvv v7 v11 || SAU.ADD.i32 i6 i6 1                                          
	VAU.ADD.f16 v22 v22 v12      || CMU.cpvv v8 v12 || SAU.ADD.i32 i7 i7 1                                         
	VAU.ADD.f16 v23 v23 v13      || CMU.cpvv v9 v13 || SAU.ADD.i32 i8 i8 1                                         
	VAU.ADD.f16 v18 v18 v1  
	VAU.ADD.f16 v19 v19 v2	
	VAU.ADD.f16 v22 v21 v22                                                
	VAU.ADD.f16 v20 v20 v3 
	nop	
    VAU.ADD.f16 v23 v22 v23                                                
    
	___loop:
	nop 2
    VAU.mul.f16 v23 v23 v0
	nop 2
	LSU0.STi128.f16.u8 v23 i17 



	
	bru.jmp i30
	nop 5
	
	
___second:

	IAU.SUB i11 i11 i11
	LSU0.LD32  i0  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i1  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i2  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i3  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i4  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i5  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i6  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i7  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i8  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i17  i17
	;CMU.CPIVR.x16    v0 i11
	nop 5
	lsu0.ldil i10, ___loop1     ||  lsu1.ldih i10, ___loop1
	IAU.SHR.u32 i15 i15 3
	VAU.XOR v18 v18 v18
	VAU.XOR v19 v19 v19
	VAU.XOR v20 v20 v20
	VAU.XOR v21 v21 v21
	VAU.XOR v22 v22 v22
	VAU.XOR v23 v23 v23
	

	;first
	LSU0.LD128.u8.u16 v1   i0    || IAU.ADD i0 i0 1                ;first
	LSU0.LD128.u8.u16 v2   i1    || IAU.ADD i1 i1 1
	LSU0.LD128.u8.u16 v3   i2    || IAU.ADD i2 i2 1
	LSU0.LD128.u8.u16 v4   i3    || IAU.ADD i3 i3 1 || VAU.XOR v18 v18 v18 
	LSU0.LD128.u8.u16 v5   i4    || IAU.ADD i4 i4 1 || VAU.XOR v19 v19 v19
	LSU0.LD128.u8.u16 v6   i5    || IAU.ADD i5 i5 1 || VAU.XOR v20 v20 v20
	LSU0.LD128.u8.u16 v7   i6    || IAU.ADD i6 i6 1 || VAU.ADD.i16 v18 v18 v1 
	LSU0.LD128.u8.u16 v8   i7    || IAU.ADD i7 i7 1 || VAU.ADD.i16 v19 v19 v2 
	LSU0.LD128.u8.u16 v9   i8    || IAU.ADD i8 i8 1 || VAU.ADD.i16 v20 v20 v3 
	LSU0.LD128.u8.u16 v10  i0    || IAU.ADD i0 i0 1 || VAU.ADD.i16 v18 v18 v4 || bru.rpl i10 i15       ;second
	LSU0.LD128.u8.u16 v11  i1    || IAU.ADD i1 i1 1 || VAU.ADD.i16 v19 v19 v5 
	LSU0.LD128.u8.u16 v12  i2    || IAU.ADD i2 i2 1 || VAU.ADD.i16 v20 v20 v6 
	LSU0.LD128.u8.u16 v13  i3    || IAU.ADD i3 i3 1 || VAU.ADD.i16 v18 v18 v7 
	LSU0.LD128.u8.u16 v14  i4    || IAU.ADD i4 i4 1 || VAU.ADD.i16 v19 v19 v8 
	LSU0.LD128.u8.u16 v15  i5    || IAU.ADD i5 i5 1 || VAU.ADD.i16 v20 v20 v9 
	LSU0.LD128.u8.u16 v16  i6    || IAU.ADD i6 i6 1 || VAU.ADD.i16 v18 v18 v10
	LSU0.LD128.u8.u16 v17  i7    || IAU.ADD i7 i7 1 || VAU.ADD.i16 v19 v19 v11
	LSU0.LD128.u8.u16 v1   i8    || IAU.ADD i8 i8 1 || VAU.ADD.i16 v20 v20 v12
	LSU0.LD128.u8.u16 v2   i0    || IAU.ADD i0 i0 1 || VAU.ADD.i16 v18 v18 v13      ;third
	LSU0.LD128.u8.u16 v3   i1    || IAU.ADD i1 i1 1 || VAU.ADD.i16 v19 v19 v14
	LSU0.LD128.u8.u16 v4   i2    || IAU.ADD i2 i2 1 || VAU.ADD.i16 v20 v20 v15
	LSU0.LD128.u8.u16 v5   i3    || IAU.ADD i3 i3 1 || VAU.ADD.i16 v18 v18 v16
	LSU0.LD128.u8.u16 v6   i4    || IAU.ADD i4 i4 1 || VAU.ADD.i16 v19 v19 v17
	LSU0.LD128.u8.u16 v7   i5    || IAU.ADD i5 i5 1 || VAU.ADD.i16 v20 v20 v1 
	LSU0.LD128.u8.u16 v8   i6    || IAU.ADD i6 i6 1 || VAU.ADD.i16 v18 v18 v2 
	LSU0.LD128.u8.u16 v9   i7    || IAU.ADD i7 i7 1 || VAU.ADD.i16 v19 v19 v3 
	LSU0.LD128.u8.u16 v10  i8    || IAU.ADD i8 i8 1 || VAU.ADD.i16 v20 v20 v4 
	LSU0.LD128.u8.u16 v11  i0    || IAU.ADD i0 i0 1 || VAU.ADD.i16 v18 v18 v5       ;fourth
	LSU0.LD128.u8.u16 v12  i1    || IAU.ADD i1 i1 1 || VAU.ADD.i16 v19 v19 v6 
	LSU0.LD128.u8.u16 v13  i2    || IAU.ADD i2 i2 1 || VAU.ADD.i16 v20 v20 v7 
	LSU0.LD128.u8.u16 v14  i3    || IAU.ADD i3 i3 1 || VAU.ADD.i16 v18 v18 v8 
	LSU0.LD128.u8.u16 v15  i4    || IAU.ADD i4 i4 1 || VAU.ADD.i16 v19 v19 v9 
	LSU0.LD128.u8.u16 v16  i5    || IAU.ADD i5 i5 1 || VAU.ADD.i16 v20 v20 v10
	LSU0.LD128.u8.u16 v17  i6    || IAU.ADD i6 i6 1 || VAU.ADD.i16 v18 v18 v11
	LSU0.LD128.u8.u16 v1   i7    || IAU.ADD i7 i7 1 || VAU.ADD.i16 v19 v19 v12
	LSU0.LD128.u8.u16 v2   i8    || IAU.ADD i8 i8 1 || VAU.ADD.i16 v20 v20 v13
	LSU0.LD128.u8.u16 v3   i0    || IAU.ADD i0 i0 1 || VAU.ADD.i16 v18 v18 v14      ;fifth
	LSU0.LD128.u8.u16 v4   i1    || IAU.ADD i1 i1 1 || VAU.ADD.i16 v19 v19 v15
	LSU0.LD128.u8.u16 v5   i2    || IAU.ADD i2 i2 1 || VAU.ADD.i16 v20 v20 v16
	LSU0.LD128.u8.u16 v6   i3    || IAU.ADD i3 i3 1 || VAU.ADD.i16 v18 v18 v17
	LSU0.LD128.u8.u16 v7   i4    || IAU.ADD i4 i4 1 || VAU.ADD.i16 v19 v19 v1 
	LSU0.LD128.u8.u16 v8   i5    || IAU.ADD i5 i5 1 || VAU.ADD.i16 v20 v20 v2 
	LSU0.LD128.u8.u16 v9   i6    || IAU.ADD i6 i6 1 || VAU.ADD.i16 v18 v18 v3 
	LSU0.LD128.u8.u16 v10  i7    || IAU.ADD i7 i7 1 || VAU.ADD.i16 v19 v19 v4 
	LSU0.LD128.u8.u16 v11  i8    || IAU.ADD i8 i8 1 || VAU.ADD.i16 v20 v20 v5 
	LSU0.LD128.u8.u16 v12  i0    || IAU.ADD i0 i0 1 || VAU.ADD.i16 v18 v18 v6       ;sixth
	LSU0.LD128.u8.u16 v13  i1    || IAU.ADD i1 i1 1 || VAU.ADD.i16 v19 v19 v7 
	LSU0.LD128.u8.u16 v14  i2    || IAU.ADD i2 i2 1 || VAU.ADD.i16 v20 v20 v8 
	LSU0.LD128.u8.u16 v15  i3    || IAU.ADD i3 i3 1 || VAU.ADD.i16 v18 v18 v9 
	LSU0.LD128.u8.u16 v16  i4    || IAU.ADD i4 i4 1 || VAU.ADD.i16 v19 v19 v10
	LSU0.LD128.u8.u16 v17  i5    || IAU.ADD i5 i5 1 || VAU.ADD.i16 v20 v20 v11
	LSU0.LD128.u8.u16 v1   i6    || IAU.ADD i6 i6 1 || VAU.ADD.i16 v18 v18 v12
	LSU0.LD128.u8.u16 v2   i7    || IAU.ADD i7 i7 1 || VAU.ADD.i16 v19 v19 v13
	LSU0.LD128.u8.u16 v3   i8    || IAU.ADD i8 i8 1 || VAU.ADD.i16 v20 v20 v14
	LSU0.LD128.u8.u16 v4   i0    || IAU.ADD i0 i0 1 || VAU.ADD.i16 v21 v18 v15    ;seventh
	LSU0.LD128.u8.u16 v5   i1    || IAU.ADD i1 i1 1 || VAU.ADD.i16 v22 v19 v16
	LSU0.LD128.u8.u16 v6   i2    || IAU.ADD i2 i2 1 || VAU.ADD.i16 v23 v20 v17
	LSU0.LD128.u8.u16 v7   i3    || IAU.ADD i3 i3 1 || VAU.ADD.i16 v21 v21 v1 
	LSU0.LD128.u8.u16 v8   i4    || IAU.ADD i4 i4 1 || VAU.ADD.i16 v22 v22 v2 
	LSU0.LD128.u8.u16 v9   i5    || IAU.ADD i5 i5 1 || VAU.ADD.i16 v23 v23 v3 || CMU.CPIVR.x16    v18 i11
	LSU0.LD128.u8.u16 v10  i6    || IAU.ADD i6 i6 1 || VAU.ADD.i16 v21 v21 v4 || CMU.CPIVR.x16    v19 i11
	LSU0.LD128.u8.u16 v11  i7    || IAU.ADD i7 i7 1 || VAU.ADD.i16 v22 v22 v5 || CMU.CPIVR.x16    v20 i11
	LSU0.LD128.u8.u16 v12  i8    || IAU.ADD i8 i8 1 || VAU.ADD.i16 v23 v23 v6 	
	LSU0.LD128.u8.u16 v13  i0    || IAU.ADD i0 i0 1 || VAU.ADD.i16 v21 v21 v7    ;eight
	LSU0.LD128.u8.u16 v14  i1    || IAU.ADD i1 i1 1 || VAU.ADD.i16 v22 v22 v8 
	LSU0.LD128.u8.u16 v15  i2    || IAU.ADD i2 i2 1 || VAU.ADD.i16 v23 v23 v9 
	LSU0.LD128.u8.u16 v16  i3    || IAU.ADD i3 i3 1 || VAU.ADD.i16 v21 v21 v10
	LSU0.LD128.u8.u16 v17  i4    || IAU.ADD i4 i4 1 || VAU.ADD.i16 v22 v22 v11
	LSU0.LD128.u8.u16 v1   i5    || IAU.ADD i5 i5 1 || VAU.ADD.i16 v23 v23 v12
	LSU0.LD128.u8.u16 v2   i6    || IAU.ADD i6 i6 1 || VAU.ADD.i16 v21 v21 v13
	LSU0.LD128.u8.u16 v3   i7    || IAU.ADD i7 i7 1 || VAU.ADD.i16 v22 v22 v14
	LSU0.LD128.u8.u16 v4   i8    || IAU.ADD i8 i8 1 || VAU.ADD.i16 v23 v23 v15                            
	LSU0.LD128.u8.u16 v5   i0    || IAU.ADD i0 i0 0 || VAU.ADD.i16 v21 v21 v16  ;nine
	LSU0.LD128.u8.u16 v6   i1    || IAU.ADD i1 i1 0 || VAU.ADD.i16 v22 v22 v17
	LSU0.LD128.u8.u16 v7   i2    || IAU.ADD i2 i2 0 || VAU.ADD.i16 v23 v23 v1 
	LSU0.LD128.u8.u16 v8   i3    || IAU.ADD i3 i3 0 || VAU.ADD.i16 v21 v21 v2 
	LSU0.LD128.u8.u16 v9   i4    || IAU.ADD i4 i4 0 || VAU.ADD.i16 v22 v22 v3 
	LSU0.LD128.u8.u16 v10  i5    || IAU.ADD i5 i5 0 || VAU.ADD.i16 v23 v23 v4 
	LSU0.LD128.u8.u16 v11  i6    || IAU.ADD i6 i6 0 || VAU.ADD.i16 v21 v21 v5 || CMU.cpvv v1 v5 || SAU.ADD.i32 i0 i0 1
	LSU0.LD128.u8.u16 v12  i7    || IAU.ADD i7 i7 0 || VAU.ADD.i16 v22 v22 v6 || CMU.cpvv v2 v6 || SAU.ADD.i32 i1 i1 1
	LSU0.LD128.u8.u16 v13  i8    || IAU.ADD i8 i8 0 || VAU.ADD.i16 v23 v23 v7 || CMU.cpvv v3 v7 || SAU.ADD.i32 i2 i2 1
	VAU.ADD.i16 v21 v21 v8       || CMU.cpvv v4 v8  || SAU.ADD.i32 i3 i3 1                                              
	VAU.ADD.i16 v22 v22 v9       || CMU.cpvv v5 v9  || SAU.ADD.i32 i4 i4 1                                              
	VAU.ADD.i16 v23 v23 v10      || CMU.cpvv v6 v10 || SAU.ADD.i32 i5 i5 1                                              
	VAU.ADD.i16 v21 v21 v11      || CMU.cpvv v7 v11 || SAU.ADD.i32 i6 i6 1                                              
	VAU.ADD.i16 v22 v22 v12      || CMU.cpvv v8 v12 || SAU.ADD.i32 i7 i7 1                                              
	VAU.ADD.i16 v23 v23 v13      || CMU.cpvv v9 v13 || SAU.ADD.i32 i8 i8 1                                              
	___loop1:	
	VAU.ADD.i16 v22 v21 v22                                                
	VAU.ADD.i16 v18 v18 v1                                                      
	VAU.ADD.i16 v23 v22 v23                                                
	VAU.ADD.i16 v19 v19 v2 
	LSU0.STi64.l v23 i17 || VAU.ADD.i16 v20 v20 v3 
	LSU0.STi64.h v23 i17
	
	
	bru.jmp i30
	nop 5
.end