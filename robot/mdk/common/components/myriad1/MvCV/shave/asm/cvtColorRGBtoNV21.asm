; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief
; ///

;--------------------------------------------------------------------------------------------------------------
.version 00.50.37.8
.data .rodata.cvtColorRGBtoNV21_asm 
.salign 16
		___RGBtoNV21_coefs:
		.float16 0.299F16, 0.587F16, 0.114F16, 0.0F16, 0.564F16, 0.713F16	
		___clamp: 
		.float16 255.0
		___const:
		.float16 128.0
		;___const1:
		;.float16 1.0
.code .text.cvtColorRGBtoNV21_asm ;text
cvtColorRGBtoNV21_asm:
;void cvtColorRGBtoNV21(u8** inR, u8** inG, u8** inB, u8** yOut, u8** uvOut, u32 width, u32 k)
;                       i18      i17        i16          i15      i14          i13       i12
	
	IAU.SUB i19 i19 8
	LSU0.ST64.l v24  i19  || IAU.SUB i19 i19 8
	LSU0.ST64.h v24  i19  || IAU.SUB i19 i19 8
	LSU0.ST64.l v25  i19  || IAU.SUB i19 i19 8
	LSU0.ST64.h v25  i19  || IAU.SUB i19 i19 8
	LSU0.ST64.l v26  i19  || IAU.SUB i19 i19 8
	LSU0.ST64.h v26  i19  || IAU.SUB i19 i19 8
	LSU0.ST64.l v27  i19  || IAU.SUB i19 i19 8
	LSU0.ST64.h v27  i19   
	
	; load coefs and const
	LSU0.LDIL i11 ___RGBtoNV21_coefs || LSU1.LDIH i11 ___RGBtoNV21_coefs
	LSU0.LDIL i1 ___clamp || LSU1.LDIH i1 ___clamp
	LSU0.LDIL i4 ___const || LSU1.LDIH i4 ___const
	
	LSU0.LDIL i3 ___loop || LSU1.LDIH i3 ___loop ;i3 loop
	;======
	lsu0.ldil i8 ___endif || lsu1.ldih i8 ___endif
	;======
	
	lsu0.ldil i5 0x1 || lsu1.ldih i5 0x0
	LSU0.LD32 i18 i18 || LSU1.LD32 i17 i17
	LSU0.LD32 i16 i16 || LSU1.LD32 i15 i15
	LSU0.LD32 i14 i14 || lsu1.ld16r v9, i1
	
	lsu0.ld16r v11, i4 || LSU1.ld64.l v0, i11 || IAu.add i11 i11 8 ;put in v0 coefs = like a vector
	LSU0.ld64.h v0, i11	
	
	iau.shr.u32 i2 i13 3	
	iau.and i0 i12 i5
	cmu.cmz.i32 i0
	peu.pc1c neq || bru.jmp i8 
	nop 5
	;loading RGB
	LSU0.LD128.u8.f16 v1 i18 || IAU.ADD i18 i18 8 ;Red
	LSU0.LD128.u8.f16 v2 i17 || IAU.ADD i17 i17 8 ;Green
	LSU0.LD128.u8.f16 v3 i16 || IAU.ADD i16 i16 8 ;Blue
	nop 3
	
	; calc. y
	VAU.MUL.f16 v4 v1 v0 || lsu0.swzv8 [00000000] ; reg * 0.299
	
	VAU.MUL.f16 v5 v2 v0 || lsu0.swzv8 [11111111] ;green * 0.587
	
	VAU.MUL.f16 v6 v3 v0 || lsu0.swzv8 [22222222] ;blue * 0.144
	nop
	
	vau.add.f16 v7 v4 v5 
	nop 2
	vau.add.f16 v8 v7 v6 ; yOut
	;end of calc y
	
	;=======
	 
	nop 2
	;=======
	
	;---------------------------------------------
	; calc. u
	vau.sub.f16 v12 v3 v8 
	vau.sub.f16 v15 v1 v8
	nop 
	vau.mul.f16 v13 v12 v0 || lsu0.swzv8 [44444444] 
	vau.mul.f16 v16 v15 v0 || lsu0.swzv8 [55555555] 
    LSU0.LD128.u8.f16 v1 i18 || IAU.ADD i18 i18 8 || bru.rpl i3 i2
	
	vau.add.f16 v14 v13 v11 || LSU0.LD128.u8.f16 v2 i17 || IAU.ADD i17 i17 8; in v14 is u
	vau.add.f16 v17 v16 v11 || LSU0.LD128.u8.f16 v3 i16 || IAU.ADD i16 i16 8 ; in v17 is v
	nop
	vau.add.f16 v18 v14 v14 || lsu0.swzv8 [77553311] ;sum u1,u2..
	vau.add.f16 v20 v17 v17 || lsu0.swzv8 [77553311] ;sum v1,v2..
	VAU.MUL.f16 v4 v1 v0 || lsu0.swzv8 [00000000] ; reg * 0.299

	vau.mul.f16 v19 v18 0.5 ; 0,2,4,6=avg u1,u2
	; end of calc. u
	
	; calc. v
	
	vau.mul.f16 v21 v20 0.5 ;0,2,4,6=avg v1,v2
	; end of calc. v	
	VAU.MUL.f16 v5 v2 v0 || lsu0.swzv8 [11111111] ;green * 0.587
	VAU.MUL.f16 v6 v3 v0 || lsu0.swzv8 [22222222] ;blue * 0.144
	;------------
	
	cmu.vilv.x16 v24 v25 v21 v19
	cmu.vszmword v26 v24 [DD20] || vau.add.f16 v7 v4 v5 
	cmu.vszmword v26 v25 [20DD] ;in v26 is uvOut
	CMU.CLAMP0.F16 v27 v26 v9 ; in v27 is clamp uvOut
	
	vau.add.f16 v8 v7 v6 

	;-----------------------------------------
	
	LSU0.ST128.f16.u8 v27 i14 || IAU.ADD i14 i14 8 ;store uvOut
	;========
	;========
	___loop:

	CMU.CLAMP0.F16 v10 v8 v9 
	vau.sub.f16 v12 v3 v8 
	
		
	LSU0.ST128.f16.u8 v10 i15 || IAU.ADD i15 i15 8 || vau.sub.f16 v15 v1 v8;store yOut
    nop 
	vau.mul.f16 v13 v12 v0 || lsu0.swzv8 [44444444] 
	vau.mul.f16 v16 v15 v0 || lsu0.swzv8 [55555555] 
	
	
	LSU0.LD64.h  v27  i19 || IAU.ADD i19 i19 8    
	LSU0.LD64.l  v27  i19 || IAU.ADD i19 i19 8
	LSU0.LD64.h  v26  i19 || IAU.ADD i19 i19 8    
	LSU0.LD64.l  v26  i19 || IAU.ADD i19 i19 8
	LSU0.LD64.h  v25  i19 || IAU.ADD i19 i19 8    
	LSU0.LD64.l  v25  i19 || IAU.ADD i19 i19 8
	LSU0.LD64.h  v24  i19 || IAU.ADD i19 i19 8    
	LSU0.LD64.l  v24  i19 || IAU.ADD i19 i19 8
		
	bru.jmp i30
	nop 5
	
	___endif:
	
	LSU0.LDIL i3 ___loop2 || LSU1.LDIH i3 ___loop2 ;i3 loop
	LSU0.LD128.u8.f16 v1 i18 || IAU.ADD i18 i18 8 ;Red
	LSU0.LD128.u8.f16 v2 i17 || IAU.ADD i17 i17 8 ;Green
	LSU0.LD128.u8.f16 v3 i16 || IAU.ADD i16 i16 8 ;Blue
	nop 3
	
	; calc. y
	VAU.MUL.f16 v4 v1 v0 || lsu0.swzv8 [00000000] ; reg * 0.299
	
	VAU.MUL.f16 v5 v2 v0 || lsu0.swzv8 [11111111]  ;green * 0.587
	
	VAU.MUL.f16 v6 v3 v0 || lsu0.swzv8 [22222222]  ;blue * 0.144
	nop 
	vau.add.f16 v7 v4 v5 
	LSU0.LD128.u8.f16 v1 i18 || IAU.ADD i18 i18 8 || bru.rpl i3 i2
	LSU0.LD128.u8.f16 v2 i17 || IAU.ADD i17 i17 8 
	vau.add.f16 v8 v7 v6 || LSU0.LD128.u8.f16 v3 i16 || IAU.ADD i16 i16 8 ; yOut
	;end of calc y
	nop 2
	;=======
	;========
	___loop2:

	CMU.CLAMP0.F16 v10 v8 v9 
	VAU.MUL.f16 v4 v1 v0 || lsu0.swzv8 [00000000] ; reg * 0.299
	
	LSU0.ST128.f16.u8 v10 i15 || IAU.ADD i15 i15 8 || VAU.MUL.f16 v5 v2 v0 || lsu1.swzv8 [11111111] ;store yOut
	VAU.MUL.f16 v6 v3 v0 || lsu0.swzv8 [22222222]  ;blue * 0.144
    nop 
	vau.add.f16 v7 v4 v5 
	
	LSU0.LD64.h  v27  i19 || IAU.ADD i19 i19 8    
	LSU0.LD64.l  v27  i19 || IAU.ADD i19 i19 8
	LSU0.LD64.h  v26  i19 || IAU.ADD i19 i19 8    
	LSU0.LD64.l  v26  i19 || IAU.ADD i19 i19 8
	LSU0.LD64.h  v25  i19 || IAU.ADD i19 i19 8    
	LSU0.LD64.l  v25  i19 || IAU.ADD i19 i19 8
	LSU0.LD64.h  v24  i19 || IAU.ADD i19 i19 8    
	LSU0.LD64.l  v24  i19 || IAU.ADD i19 i19 8
		
	bru.jmp i30
	nop 5
	
.end