; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief   Color space transformation RGB to YUV 
; ///

.version 00.51.05
;-------------------------------------------------------------------------------
.data .rodata.cvtColorKernelRGBToYUV
.salign 16
___RGB2YUV_coefs:
	.float16 0.299F16, 0.587F16, 0.114F16, 0.0f16, 0.564F16, 0.713F16

.code .text.cvtColorKernelRGBToYUV
cvtColorKernelRGBToYUV_asm:
	;i18 - in buffer
	;i17 - y out buffer
	;i16 - u out buffer
	;i15 - v out buffer
	;i14 - width
	;i13 - height
	
	; Y = 0.299 * R + 0.587 * G + 0.114 * B
	; U = (B - Y) * 0.713 + 128  
	; V = (R - Y) * 0.564 + 128
	;
	; - every iteration of the main loop two y values are calculated
	; - for every 4 values of y, one vaue of u and v is calculated 
	; register mapping:
	; - i12 addres for transformation coeficients
	; - i13 loop counter
	; - i0-2: value for first RGB bytes read from the input buffer
	; - i3-5: value for second RGB bytes read from the input buffer
	; - i10, i11 - saves the value of i0 (R) and i2 (B) for u and v calculation before
	;   they are overwritten by the next values loaded from input buffer
	; - s0, s1: u and v coeficients for tarnsformation
	; - s5, s6: 
	; - v3, v4 components for y1 and y2 values (will be summed to obtain the final value)
	
	lsu0.ldil i12, ___RGB2YUV_coefs || lsu1.ldih i12, ___RGB2YUV_coefs  || iau.sub i19, i19, 12
	lsu0.st32  i17, i19             || iau.sub i0, i0, i0               || sau.xor i3, i3, i3	
	lsu0.sto32 i16, i19, 4          || iau.sub i1, i1, i1               || sau.xor i4, i4, i4
	lsu0.sto32 i15, i19, 8          || iau.sub i2, i2, i2               || sau.xor i5, i5, i5
	
	;RGB read
	lsu0.ld8  i0, i18     || lsu1.ldo8 i1, i18, 1 || iau.mul i9, i14, 2  ; R1, G1
	lsu0.ldo8 i2, i18, 2  || lsu1.ldo8 i3, i18, 3 ; B1, R2 
	lsu0.ldo8 i4, i18, 4  || lsu1.ldo8 i5, i18, 5 || iau.add i18, i18, 6 ; G2, B2
	
	lsu0.ldil i8, ___main_loop     || lsu1.ldih i8, ___main_loop
	
	;y, u, v coeficients loading
	lsu0.ld64.l v5, i12 || lsu1.ldo16  s0, i12, 8 || iau.sub i13, i13, i13 ; loop counter reset
	lsu0.ldo16  s1, i12, 10
	cmu.cpiv.x16 v1.0, i0.l
	cmu.cpiv.x16 v1.1, i1.l
	cmu.cpiv.x16 v1.2, i2.l
	cmu.cpvv.i16.f16 v1, v1
	___main_loop:
		;                             save i0
		iau.add i13, i13, 2       || cmu.cpii i10, i0
		vau.mul.f16 v3, v1, v5    || lsu0.ld8  i0, i18     || cmu.cpii i11, i2
		cmu.cpiv.x16 v2.0, i3.l   || lsu1.ldo8 i1, i18, 1      
		cmu.cpiv.x16 v2.1, i4.l
		cmu.cpiv.x16 v2.2, i5.l
		cmu.cpvv.i16.f16 v2, v2
		cmu.cpvv.f16.i16s v3, v3  || lsu0.ldo8 i2, i18, 2  || lsu1.ldo8 i3, i18, 3 ; B1, R2
		vau.mul.f16 v4, v2, v5 ; RGB2 * Cy
		sau.sum.i16 i6, v3, 3     || cmu.cmii.i32 i13, i14
		lsu0.ldo8 i4, i18, 4      || lsu1.ldo8 i5, i18, 5 || iau.add i18, i18, 6; G2, B2
		cmu.cpvv.f16.i16s v4, v4
		peu.pc1c LTE || bru.jmp i8
		iau.add i6, i6, 1                                 || cmu.cpiv.x16 v1.0, i0.l
		sau.sum.i16 i7, v4, 3                             || cmu.cpiv.x16 v1.1, i1.l || iau.sub i11, i11, i6 
		lsu0.st8  i6, i17      || iau.add i17, i17, 1     || cmu.cpiv.x16 v1.2, i2.l
		iau.add i7, i7, 1                                 || cmu.cpvv.i16.f16 v1, v1
		lsu0.st8  i7, i17      || iau.add i17, i17, 1
		
		;u and v calculation
		cmu.cpis.i16.f16 s5, i11 || iau.sub i10, i10, i6
		cmu.cpis.i16.f16 s6, i10
		sau.mul.f16 s5, s5, s0
		sau.mul.f16 s6, s6, s1   || cmu.cmii.i32 i13, i9
		peu.pc1c LT || bru.jmp i8
		cmu.cpsi.f16.i16s i6, s5
		cmu.cpsi.f16.i16s i7, s6
		iau.add i6, i6, 128
		iau.add i7, i7, 128
		lsu0.st8 i6, i16 || lsu1.st8 i7, i15 || iau.add i16, i16, 1 || sau.add.i32 i15, i15, 1
		
	lsu0.ld32 i17, i19
	lsu0.ldo32 i16, i19, 4
	lsu0.ldo32 i15, i19, 8
	iau.add i19, i19, 12
	bru.jmp i30
	nop 5
.end
	