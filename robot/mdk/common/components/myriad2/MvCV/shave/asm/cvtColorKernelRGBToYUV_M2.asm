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
___mask:
	.short   0xffff, 0xffff, 0xffff, 0, 0, 0, 0, 0
	
	
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
	IAU.SUB i19 i19 4
LSU0.ST.32  i20  i19 || IAU.SUB i19 i19 4
LSU0.ST.32  i21  i19 || IAU.SUB i19 i19 4
LSU0.ST.32  i22  i19 || IAU.SUB i19 i19 4
LSU0.ST.32  i23  i19 
	lsu0.ldil i0, ___mask || lsu1.ldih i0, ___mask 
	LSU0.LD.64.L v23, i0 || IAU.ADD i0 i0 8
	LSU0.LD.64.H v23, i0
	
	lsu0.ldil i12, ___RGB2YUV_coefs || lsu1.ldih i12, ___RGB2YUV_coefs  || iau.sub i19, i19, 12
	LSU0.ST.32  i17, i19             || iau.sub i0, i0, i0               || sau.xor i3, i3, i3	
	LSU0.STO.32 i16, i19, 4          || iau.sub i1, i1, i1               || sau.xor i4, i4, i4
	LSU0.STO.32 i15, i19, 8          || iau.sub i2, i2, i2               || sau.xor i5, i5, i5
	
	;RGB read
	LSU0.LD.8  i0, i18     || LSU1.LDO.8 i1, i18, 1 || iau.mul i9, i14, 2  ; R1, G1
	LSU0.LDO.8 i2, i18, 2  || LSU1.LDO.8 i3, i18, 3 ; B1, R2 
	LSU0.LDO.8 i4, i18, 4  || LSU1.LDO.8 i5, i18, 5 || iau.add i18, i18, 6 ; G2, B2

	lsu0.ldil i8, ___main_loop     || lsu1.ldih i8, ___main_loop
	
	;y, u, v coeficients loading
	LSU0.LD.64.L v5, i12 || LSU1.LDO.16  i20, i12, 8 || iau.sub i13, i13, i13 ; loop counter reset
	LSU0.LDO.16  i21, i12, 10
	nop
	cmu.cpiv.x16 v1.0, i0.l
	cmu.cpiv.x16 v1.1, i1.l
	cmu.cpiv.x16 v1.2, i2.l
	cmu.cpvv.i16.f16 v1, v1
nop
___main_loop:
		;                             save i0
		iau.add i13, i13, 2       || cmu.cpii i10, i0
		vau.mul.f16 v3, v1, v5    || LSU0.LD.8  i0, i18     || cmu.cpii i11, i2
		cmu.cpiv.x16 v2.0, i3.l   || LSU1.LDO.8 i1, i18, 1      
		cmu.cpiv.x16 v2.1, i4.l
		cmu.cpiv.x16 v2.2, i5.l
		cmu.cpvv.i16.f16 v2, v2

		cmu.cpvv.f16.i16s v3, v3  || LSU0.LDO.8 i2, i18, 2  || LSU1.LDO.8 i3, i18, 3 ; B1, R2

		vau.mul.f16 v4, v2, v5 ; RGB2 * Cy
		VAU.AND v3 v3  v23
		sau.sumx.i16 i6, v3         || cmu.cmii.i32 i13, i14
		LSU0.LDO.8 i4, i18, 4      || LSU1.LDO.8 i5, i18, 5 || iau.add i18, i18, 6; G2, B2
		cmu.cpvv.f16.i16s v4, v4
		nop

		VAU.AND v4 v4  v23
		peu.pc1c LTE || bru.jmp i8
		iau.add i6, i6, 1                                 || cmu.cpiv.x16 v1.0, i0.l
		sau.sumx.i16 i7, v4                               || cmu.cpiv.x16 v1.1, i1.l || iau.sub i11, i11, i6 
		LSU0.ST.8  i6, i17      || iau.add i17, i17, 1     || cmu.cpiv.x16 v1.2, i2.l
		iau.add i7, i7, 1                                 || cmu.cpvv.i16.f16 v1, v1
		LSU0.ST.8  i7, i17      || iau.add i17, i17, 1
		nop
		;u and v calculation
		cmu.CPII.i16.f16 i22, i11 || iau.sub i10, i10, i6
		cmu.CPII.i16.f16 i23, i10
		sau.mul.f16 i22, i22, i20
		sau.mul.f16 i23, i23, i21   || cmu.cmii.i32 i13, i9
		peu.pc1c LT || bru.jmp i8
		cmu.CPII.f16.i16s i6, i22
		cmu.CPII.f16.i16s i7, i23
		iau.add i6, i6, 128
		iau.add i7, i7, 128
		LSU0.ST.8 i6, i16 || LSU1.ST.8 i7, i15 || iau.add i16, i16, 1 || sau.add.i32 i15, i15, 1
		nop
		
	LSU0.LD.32 i17, i19
	LSU0.LDO.32 i16, i19, 4
	LSU0.LDO.32 i15, i19, 8
	iau.add i19, i19, 12
	
LSU0.LD.32  i23  i19 || IAU.ADD i19 i19 4
LSU0.LD.32  i22  i19 || IAU.ADD i19 i19 4
LSU0.LD.32  i21  i19 || IAU.ADD i19 i19 4
LSU0.LD.32  i20  i19 || IAU.ADD i19 i19 4



	bru.jmp i30
nop 6
.end
