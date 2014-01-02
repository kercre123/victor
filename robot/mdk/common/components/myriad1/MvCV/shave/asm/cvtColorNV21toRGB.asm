; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief
; ///

;--------------------------------------------------------------------------------------------------------------
.version 00.50.37.8
.data .rodata.cvtColorNV21toRGB_asm 
.salign 16
		___NV21toRGB_coefs:
		.float16 0.344F16, 0.714F16	
		___clamp: 
		.float16 255.0
		___const:
		.float16 128.0
		___coef1:
		.float16 1.402F16
		___coef2:
		.float16 1.772F16
.code .text.cvtColorNV21toRGB_asm ;text
cvtColorNV21toRGB_asm:
;void cvtColorNV21toRGB(u8** yin, u8** uvin, u8** outR, u8** outG, u8** outB, u32 width)
;                          i18      i17        i16          i15      i14          i13      
	
	LSU0.LD32 i18 i18 || LSU1.LD32 i17 i17  ; yIn and uvIn
	LSU0.LD32 i16 i16 || LSU1.LD32 i15 i15  ;outR and outG
	LSU0.LD32 i14 i14	;outB
	nop 5
	lsu0.ldil i12 ___NV21toRGB_coefs || lsu1.ldih i12 ___NV21toRGB_coefs
	LSU0.LDIL i0 ___clamp || LSU1.LDIH i0 ___clamp
	LSU0.LDIL i1 ___const || LSU1.LDIH i1 ___const
	LSU0.LDIL i2 ___coef1 || LSU1.LDIH i2 ___coef1
	LSU0.LDIL i4 ___coef2 || LSU1.LDIH i4 ___coef2	
	LSU0.LDIL i3 ___loop || LSU1.LDIH i3 ___loop ; i3 loop
	
	nop 5
	lsu0.ld16r v6, i2   ; vect with 1.402F16
	LSU0.LD32r v10, i12 ; vect with 0.343F16, 0.714F16....
	lsu0.ld16r v11, i4  ; vect with 0.772F16
	lsu0.ld16r v0, i0 	; vect with 255
	lsu0.ld16r v1, i1 	; vect with 128
	
	iau.sub i5 i5 i5 ;i5 contor for loop 
	___loop:
	 ;loading y and uv
	lsu0.ld128.u8.f16 v3 i18 || iau.add i18 i18 8 ; y
	lsu0.ld128.u8.f16 v4 i17 || iau.add i17 i17 8 ; uv
	nop 5
	vau.sub.f16 v5 v4 v1 ; uv-128
	nop 5
	
	; calc R
	vau.mul.f16 v7 v6 v5 || lsu0.swzv8 [77553311]
	nop 2
	vau.add.f16 v8 v3 v7 ; outR 
	nop 2
	CMU.CLAMP0.F16 v9 v8 v0 ; outR clamp
	nop
	; end of calc R
	
	; calc G
	vau.mul.f16 v12 v5 v10
	nop 2
	vau.add.f16 v13 v12 v12 || lsu0.swzv8 [77553311]
	nop 2
	vau.sub.f16 v14 v3 v13 || lsu0.swzv8 [66442200] ; outG
	nop 2
	CMU.CLAMP0.F16 v15 v14 v0 ; outG clamp
	nop
	; end of calc G
	
	;calc B
	vau.mul.f16 v16 v11 v5 || lsu0.swzv8 [66442200]
	nop 2
	vau.add.f16 v17 v3 v16 ; outB
	nop 2
	CMU.CLAMP0.F16 v18 v17 v0 ; outB clamp
	nop
	;end of calc b
	
	;store outR, outG, outB
	LSU0.ST128.f16.u8 v9 i16 || IAU.ADD i16 i16 8
	LSU0.ST128.f16.u8 v15 i15 || IAU.ADD i15 i15 8
	LSU0.ST128.f16.u8 v18 i14 || IAU.ADD i14 i14 8
	nop
	; end loop
	iau.add i5 i5 8
	cmu.cmii.i32 i5 i13
	peu.pc1c lt || bru.jmp i3
	nop 5
	
	;nop 5
	;bru.swih 0x1F
	
	bru.jmp i30
	nop 5
	
	
.end

;==============================================
	;LSU0.LDIL i2 ___const2 || LSU1.LDIH i2 ___const2	
	;LSU0.LDIL i3 ___const3 || LSU1.LDIH i3 ___const3	
	;lsu0.ld64.l v10 i2 || iau.add i2 i2 8
	;lsu0.ld64.h v10 i2 
	;lsu0.ld64.l v11 i3 || iau.add i3 i3 8
	;lsu0.ld64.h v11 i3 
	;nop 5
	;cmu.vdilv.x16 v12 v13 v10 v11
	;================================================