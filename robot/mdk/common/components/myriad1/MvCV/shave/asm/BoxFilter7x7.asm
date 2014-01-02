; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief
; ///

;--------------------------------------------------------------------------------------------------------------
.version 00.70.00

.data .rodata.boxfilter7x7_asm
.salign 16
    ___multiply:
        .float16  0.020408163F16
	___multiply1:
        .float16  1

; void boxfilter7x7(u8** in (i18), u8** out (i17), u8 normalize (i16), u32 width (i15))
.code .text.boxfilter7x7_asm
.salign 16

boxfilter7x7_asm:
	LSU0.ldil i5, ___second || LSU1.ldih i5, ___second
	LSU0.ldil i13, 0x1 || LSU1.ldih i13, 0x0
	CMU.CMII.U8 i16 i13
	PEU.PC1C NEQ || BRU.JMP i5
	nop 5


 	lsu0.ldil i7, ___multiply     ||  lsu1.ldih i7, ___multiply
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
	LSU0.ST64.h v31  i19  || IAU.SUB i19 i19 4 
	LSU0.ST32 i20 i19 || IAU.SUB i19 i19 4 
	LSU0.ST32 i21 i19 || IAU.SUB i19 i19 4 
	LSU0.ST32 i22 i19 
 
	LSU0.LD32  i0  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i1  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i2  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i3  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i4  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i5  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i6  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i17  i17
	lsu0.ld16r v0, i7
	nop 5
	lsu0.ldil i7, ___loop     ||  lsu1.ldih i7, ___loop

	IAU.SHR.u32 i15 i15 3
	VAU.XOR v31 v31 v31
	VAU.XOR v30 v30 v30
	VAU.XOR v29 v29 v29
	
	
	
	
;first
	LSU0.LD128.u8.f16 v1 i0   || IAU.ADD i0 i0 1 || VAU.XOR v28 v28 v28 
	LSU0.LD128.u8.f16 v2 i1   || IAU.ADD i1 i1 1 || VAU.XOR v27 v27 v27
	LSU0.LD128.u8.f16 v3 i2   || IAU.ADD i2 i2 1 || VAU.XOR v26 v26 v26
	LSU0.LD128.u8.f16 v4 i3   || IAU.ADD i3 i3 1 || VAU.XOR v31 v31 v31 
	LSU0.LD128.u8.f16 v5 i4   || IAU.ADD i4 i4 1 || VAU.XOR v30 v30 v30 
	LSU0.LD128.u8.f16 v6 i5   || IAU.ADD i5 i5 1 || VAU.XOR v29 v29 v29 
	LSU0.LD128.u8.f16 v7 i6   || IAU.ADD i6 i6 1 || VAU.ADD.f16 v29 v29 v1 
;second	
	LSU0.LD128.u8.f16 v8  i0  || IAU.ADD i0 i0 1 || VAU.ADD.f16 v30 v30 v2 
	LSU0.LD128.u8.f16 v9  i1  || IAU.ADD i1 i1 1 || VAU.ADD.f16 v31 v31 v3 || bru.rpl i7 i15
	LSU0.LD128.u8.f16 v10 i2  || IAU.ADD i2 i2 1 || VAU.ADD.f16 v29 v29 v4
	LSU0.LD128.u8.f16 v11 i3  || IAU.ADD i3 i3 1 || VAU.ADD.f16 v30 v30 v5
	LSU0.LD128.u8.f16 v12 i4  || IAU.ADD i4 i4 1 || VAU.ADD.f16 v31 v31 v6
	LSU0.LD128.u8.f16 v13 i5  || IAU.ADD i5 i5 1 || VAU.ADD.f16 v29 v29 v7
	LSU0.LD128.u8.f16 v14 i6  || IAU.ADD i6 i6 1 || VAU.ADD.f16 v30 v30 v8
;third
	LSU0.LD128.u8.f16 v15 i0  || IAU.ADD i0 i0 1 || VAU.ADD.f16 v31 v31 v9
	LSU0.LD128.u8.f16 v16 i1  || IAU.ADD i1 i1 1 || VAU.ADD.f16 v29 v29 v10
	LSU0.LD128.u8.f16 v17 i2  || IAU.ADD i2 i2 1 || VAU.ADD.f16 v30 v30 v11
	LSU0.LD128.u8.f16 v18 i3  || IAU.ADD i3 i3 1 || VAU.ADD.f16 v31 v31 v12
	LSU0.LD128.u8.f16 v19 i4  || IAU.ADD i4 i4 1 || VAU.ADD.f16 v29 v29 v13
	LSU0.LD128.u8.f16 v20 i5  || IAU.ADD i5 i5 1 || VAU.ADD.f16 v30 v30 v14
	LSU0.LD128.u8.f16 v21 i6  || IAU.ADD i6 i6 1 || VAU.ADD.f16 v31 v31 v15
;fourth
	LSU0.LD128.u8.f16 v22 i0  || IAU.ADD i0 i0 1 || VAU.ADD.f16 v29 v29 v16
	LSU0.LD128.u8.f16 v23 i1  || IAU.ADD i1 i1 1 || VAU.ADD.f16 v30 v30 v17
	LSU0.LD128.u8.f16 v24 i2  || IAU.ADD i2 i2 1 || VAU.ADD.f16 v31 v31 v18
	LSU0.LD128.u8.f16 v25 i3  || IAU.ADD i3 i3 1 || VAU.ADD.f16 v26 v29 v19
	LSU0.LD128.u8.f16 v1 i4   || IAU.ADD i4 i4 1 || VAU.ADD.f16 v27 v30 v20
	LSU0.LD128.u8.f16 v2 i5   || IAU.ADD i5 i5 1 || VAU.ADD.f16 v28 v31 v21
	LSU0.LD128.u8.f16 v3 i6   || IAU.ADD i6 i6 1 || VAU.ADD.f16 v26 v26 v22
;fifth
	LSU0.LD128.u8.f16 v4 i0    || IAU.ADD i0 i0 1 || VAU.ADD.f16 v27 v27 v23
	LSU0.LD128.u8.f16 v5 i1    || IAU.ADD i1 i1 1 || VAU.ADD.f16 v28 v28 v24
	LSU0.LD128.u8.f16 v6 i2    || IAU.ADD i2 i2 1 || VAU.ADD.f16 v26 v26 v25
	LSU0.LD128.u8.f16 v7 i3    || IAU.ADD i3 i3 1 || VAU.ADD.f16 v27 v27 v1 
	LSU0.LD128.u8.f16 v8 i4    || IAU.ADD i4 i4 1 || VAU.ADD.f16 v28 v28 v2 
	LSU0.LD128.u8.f16 v9 i5    || IAU.ADD i5 i5 1 || VAU.ADD.f16 v26 v26 v3
	LSU0.LD128.u8.f16 v10 i6   || IAU.ADD i6 i6 1 || VAU.ADD.f16 v27 v27 v4
;sixth 
	LSU0.LD128.u8.f16 v11  i0  || IAU.ADD i0 i0 1 || VAU.ADD.f16 v28 v28 v5
	LSU0.LD128.u8.f16 v12  i1  || IAU.ADD i1 i1 1 || VAU.ADD.f16 v26 v26 v6
	LSU0.LD128.u8.f16 v13  i2  || IAU.ADD i2 i2 1 || VAU.ADD.f16 v27 v27 v7
	LSU0.LD128.u8.f16 v14  i3  || IAU.ADD i3 i3 1 || VAU.ADD.f16 v28 v28 v8
	LSU0.LD128.u8.f16 v15  i4  || IAU.ADD i4 i4 1 || VAU.ADD.f16 v26 v26 v9
	LSU0.LD128.u8.f16 v16  i5  || IAU.ADD i5 i5 1 || VAU.ADD.f16 v27 v27 v10
	LSU0.LD128.u8.f16 v17  i6  || IAU.ADD i6 i6 1 || VAU.ADD.f16 v28 v28 v11
;seventh 
	LSU0.LD128.u8.f16 v18 i0  || IAU.ADD i0 i0 2 || VAU.ADD.f16 v26 v26 v12
	LSU0.LD128.u8.f16 v19 i1  || IAU.ADD i1 i1 2 || VAU.ADD.f16 v27 v27 v13
	LSU0.LD128.u8.f16 v20 i2  || IAU.ADD i2 i2 2 || VAU.ADD.f16 v28 v28 v14
	LSU0.LD128.u8.f16 v21 i3  || IAU.ADD i3 i3 2 || VAU.ADD.f16 v26 v26 v15
	LSU0.LD128.u8.f16 v22 i4  || IAU.ADD i4 i4 2 || VAU.ADD.f16 v27 v27 v16
	LSU0.LD128.u8.f16 v23 i5  || IAU.ADD i5 i5 2 || VAU.ADD.f16 v28 v28 v17
	LSU0.LD128.u8.f16 v24 i6  || IAU.ADD i6 i6 2 || VAU.ADD.f16 v26 v26 v18
	VAU.ADD.f16 v27 v27 v19
	VAU.ADD.f16 v28 v28 v20
	VAU.ADD.f16 v26 v26 v21
	LSU0.LD128.u8.f16 v1 i0   || IAU.ADD i0 i0 1 || VAU.ADD.f16 v27 v27 v22	
	LSU0.LD128.u8.f16 v2 i1   || IAU.ADD i1 i1 1 || VAU.ADD.f16 v28 v28 v23
	LSU0.LD128.u8.f16 v3 i2   || IAU.ADD i2 i2 1 || VAU.ADD.f16 v26 v26 v24
	nop 2
	VAU.ADD.f16 v26 v26 v27
	LSU0.LD128.u8.f16 v4 i3   || IAU.ADD i3 i3 1 || VAU.XOR v31 v31 v31 
	nop 
	VAU.ADD.f16 v28 v26 v28
	___loop:
	LSU0.LD128.u8.f16 v5 i4   || IAU.ADD i4 i4 1 || VAU.XOR v30 v30 v30 
	nop
	VAU.MUL.f16 v28 v28 v0
	LSU0.LD128.u8.f16 v6 i5   || IAU.ADD i5 i5 1 || VAU.XOR v29 v29 v29 
	LSU0.LD128.u8.f16 v7 i6   || IAU.ADD i6 i6 1 || VAU.ADD.f16 v29 v29 v1 
	LSU0.STi128.f16.u8 v28 i17 || 	LSU1.LD128.u8.f16 v8  i0  || SAU.ADD.i32 i0 i0 1 || VAU.ADD.f16 v30 v30 v2 
	

	
	LSU0.LD32  i22 i19 || IAU.ADD i19 i19 4
	LSU0.LD32  i21 i19 || IAU.ADD i19 i19 4
	LSU0.LD32  i20 i19 || IAU.ADD i19 i19 4
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
    nop 5
	
	___second:
	

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
	LSU0.ST64.h v31  i19  || IAU.SUB i19 i19 4 
	LSU0.ST32 i20 i19 || IAU.SUB i19 i19 4 
	LSU0.ST32 i21 i19 || IAU.SUB i19 i19 4 
	LSU0.ST32 i22 i19 
 
	LSU0.LD32  i0  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i1  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i2  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i3  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i4  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i5  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i6  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i17  i17
	nop 5
	lsu0.ldil i7, ___loop1     ||  lsu1.ldih i7, ___loop1

	IAU.SHR.u32 i15 i15 3
	nop
	VAU.XOR v31 v31 v31
	VAU.XOR v30 v30 v30
	VAU.XOR v29 v29 v29
	VAU.XOR v28 v28 v28
	VAU.XOR v27 v27 v27
	VAU.XOR v26 v26 v26
	
;first
	LSU0.LD128.u8.u16 v1 i0   || IAU.ADD i0 i0 1 
	LSU0.LD128.u8.u16 v2 i1   || IAU.ADD i1 i1 1 
	LSU0.LD128.u8.u16 v3 i2   || IAU.ADD i2 i2 1 
	LSU0.LD128.u8.u16 v4 i3   || IAU.ADD i3 i3 1 || VAU.XOR v31 v31 v31 || bru.rpl i7 i15
	LSU0.LD128.u8.u16 v5 i4   || IAU.ADD i4 i4 1 || VAU.XOR v30 v30 v30 
	LSU0.LD128.u8.u16 v6 i5   || IAU.ADD i5 i5 1 || VAU.XOR v29 v29 v29 
	LSU0.LD128.u8.u16 v7 i6   || IAU.ADD i6 i6 1 || VAU.ADD.i16 v29 v29 v1 
;second	
	LSU0.LD128.u8.u16 v8  i0  || IAU.ADD i0 i0 1 || VAU.ADD.i16 v30 v30 v2 
	LSU0.LD128.u8.u16 v9  i1  || IAU.ADD i1 i1 1 || VAU.ADD.i16 v31 v31 v3 
	LSU0.LD128.u8.u16 v10 i2  || IAU.ADD i2 i2 1 || VAU.ADD.i16 v29 v29 v4
	LSU0.LD128.u8.u16 v11 i3  || IAU.ADD i3 i3 1 || VAU.ADD.i16 v30 v30 v5
	LSU0.LD128.u8.u16 v12 i4  || IAU.ADD i4 i4 1 || VAU.ADD.i16 v31 v31 v6
	LSU0.LD128.u8.u16 v13 i5  || IAU.ADD i5 i5 1 || VAU.ADD.i16 v29 v29 v7
	LSU0.LD128.u8.u16 v14 i6  || IAU.ADD i6 i6 1 || VAU.ADD.i16 v30 v30 v8
;third
	LSU0.LD128.u8.u16 v15 i0  || IAU.ADD i0 i0 1 || VAU.ADD.i16 v31 v31 v9
	LSU0.LD128.u8.u16 v16 i1  || IAU.ADD i1 i1 1 || VAU.ADD.i16 v29 v29 v10
	LSU0.LD128.u8.u16 v17 i2  || IAU.ADD i2 i2 1 || VAU.ADD.i16 v30 v30 v11
	LSU0.LD128.u8.u16 v18 i3  || IAU.ADD i3 i3 1 || VAU.ADD.i16 v31 v31 v12
	LSU0.LD128.u8.u16 v19 i4  || IAU.ADD i4 i4 1 || VAU.ADD.i16 v29 v29 v13
	LSU0.LD128.u8.u16 v20 i5  || IAU.ADD i5 i5 1 || VAU.ADD.i16 v30 v30 v14
	LSU0.LD128.u8.u16 v21 i6  || IAU.ADD i6 i6 1 || VAU.ADD.i16 v31 v31 v15
;fourth
	LSU0.LD128.u8.u16 v22 i0  || IAU.ADD i0 i0 1 || VAU.ADD.i16 v29 v29 v16
	LSU0.LD128.u8.u16 v23 i1  || IAU.ADD i1 i1 1 || VAU.ADD.i16 v30 v30 v17
	LSU0.LD128.u8.u16 v24 i2  || IAU.ADD i2 i2 1 || VAU.ADD.i16 v31 v31 v18
	LSU0.LD128.u8.u16 v25 i3  || IAU.ADD i3 i3 1 || VAU.ADD.i16 v26 v29 v19
	LSU0.LD128.u8.u16 v1 i4   || IAU.ADD i4 i4 1 || VAU.ADD.i16 v27 v30 v20
	LSU0.LD128.u8.u16 v2 i5   || IAU.ADD i5 i5 1 || VAU.ADD.i16 v28 v31 v21
	LSU0.LD128.u8.u16 v3 i6   || IAU.ADD i6 i6 1 || VAU.ADD.i16 v26 v26 v22
;fifth
	LSU0.LD128.u8.u16 v4 i0    || IAU.ADD i0 i0 1 || VAU.ADD.i16 v27 v27 v23
	LSU0.LD128.u8.u16 v5 i1    || IAU.ADD i1 i1 1 || VAU.ADD.i16 v28 v28 v24
	LSU0.LD128.u8.u16 v6 i2    || IAU.ADD i2 i2 1 || VAU.ADD.i16 v26 v26 v25
	LSU0.LD128.u8.u16 v7 i3    || IAU.ADD i3 i3 1 || VAU.ADD.i16 v27 v27 v1 
	LSU0.LD128.u8.u16 v8 i4    || IAU.ADD i4 i4 1 || VAU.ADD.i16 v28 v28 v2 
	LSU0.LD128.u8.u16 v9 i5    || IAU.ADD i5 i5 1 || VAU.ADD.i16 v26 v26 v3
	LSU0.LD128.u8.u16 v10 i6   || IAU.ADD i6 i6 1 || VAU.ADD.i16 v27 v27 v4
;sixth 
	LSU0.LD128.u8.u16 v11  i0  || IAU.ADD i0 i0 1 || VAU.ADD.i16 v28 v28 v5
	LSU0.LD128.u8.u16 v12  i1  || IAU.ADD i1 i1 1 || VAU.ADD.i16 v26 v26 v6
	LSU0.LD128.u8.u16 v13  i2  || IAU.ADD i2 i2 1 || VAU.ADD.i16 v27 v27 v7
	LSU0.LD128.u8.u16 v14  i3  || IAU.ADD i3 i3 1 || VAU.ADD.i16 v28 v28 v8
	LSU0.LD128.u8.u16 v15  i4  || IAU.ADD i4 i4 1 || VAU.ADD.i16 v26 v26 v9
	LSU0.LD128.u8.u16 v16  i5  || IAU.ADD i5 i5 1 || VAU.ADD.i16 v27 v27 v10
	LSU0.LD128.u8.u16 v17  i6  || IAU.ADD i6 i6 1 || VAU.ADD.i16 v28 v28 v11
;seventh 
	LSU0.LD128.u8.u16 v18 i0  || IAU.ADD i0 i0 2 || VAU.ADD.i16 v26 v26 v12
	LSU0.LD128.u8.u16 v19 i1  || IAU.ADD i1 i1 2 || VAU.ADD.i16 v27 v27 v13
	LSU0.LD128.u8.u16 v20 i2  || IAU.ADD i2 i2 2 || VAU.ADD.i16 v28 v28 v14
	LSU0.LD128.u8.u16 v21 i3  || IAU.ADD i3 i3 2 || VAU.ADD.i16 v26 v26 v15
	LSU0.LD128.u8.u16 v22 i4  || IAU.ADD i4 i4 2 || VAU.ADD.i16 v27 v27 v16
	LSU0.LD128.u8.u16 v23 i5  || IAU.ADD i5 i5 2 || VAU.ADD.i16 v28 v28 v17
	LSU0.LD128.u8.u16 v24 i6  || IAU.ADD i6 i6 2 || VAU.ADD.i16 v26 v26 v18
	LSU0.LD128.u8.u16 v1 i0   || IAU.ADD i0 i0 1 || VAU.ADD.i16 v27 v27 v19
	LSU0.LD128.u8.u16 v2 i1   || IAU.ADD i1 i1 1 || VAU.ADD.i16 v28 v28 v20
	LSU0.LD128.u8.u16 v3 i2   || IAU.ADD i2 i2 1 || VAU.ADD.i16 v26 v26 v21
	VAU.ADD.i16 v27 v27 v22	
	VAU.ADD.i16 v28 v28 v23
	VAU.ADD.i16 v26 v26 v24
	nop
		___loop1:
	VAU.ADD.i16 v26 v26 v27
	nop 
	VAU.ADD.i16 v28 v26 v28
	nop
	LSU0.STi64.l v28 i17 
	LSU0.STi64.h v28 i17

	
	LSU0.LD32  i22 i19 || IAU.ADD i19 i19 4
	LSU0.LD32  i21 i19 || IAU.ADD i19 i19 4
	LSU0.LD32  i20 i19 || IAU.ADD i19 i19 4
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
    nop 5
	
	
.end
