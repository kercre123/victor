; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief    Color space transformation: YUV to RGB
; ///

.version 00.51.05
;-------------------------------------------------------------------------------
.data .rodata.cvtColorKernelYUVToRGB
.salign 16
___YUV2RGB_coefs:
	.float16 0.0F16, -0.344F16, 1.773F16, 0.0F16, 1.403F16, -0.714F16, 0.0F16, 0.0F16

;cvtColorKernelYUVToRGB_asm(unsigned char* yIn, unsigned char* uIn, unsigned char* vIn, unsigned char* out, u32 width); 

.code .text.cvtColorKernelYUVToRGB
cvtColorKernelYUVToRGB_asm:
	; function parameters:
	; i18 - y in buffer
	; i17 - u in buffer
	; i16 - v in buffer
	; i15 - out buffer
	; i14 - width
	; i13 - height
	
	;      Yi   Cv                 Cu 
	; ---------------------------------------------
	;  R = Y + 1.403 * (U - 128) +   0.0 * (V - 128)
	;  G = Y - 0.714 * (U - 128) - 0.344 * (V - 128) 
	;  B = Y +   0.0 * (U - 128) + 1.773 * (V - 128) 
	; ----------------------------------------------
	; - two input lines are processed at a time
	; - 4 adjiacent pixels will be processed each loop iteration from positions i, i + 1, i + width, i + width + 1
	;    (i is the current loop counter value)
	; - for each of these 4 pixels the u an v values are the same and are calculated only once  
	;
	; register mapping:
	; v5, v6: f16 u, v coeficients used for calculation
	; i6, i7: u and v values loaded from the input buffers 
	; i1, i2, i3, i4: y1-4 values loaded from the input buffers 
	; i9: loop counter
	; v6, v7: vectors containing u and v values (same value repeated in each position)
	; v1, v2, v3, v4: vectors containing y values (same value repeated in each position)
	; v9, v10: vectors containing intermediar results for y calculation u * Cu and v * Cv
	; v10, v11, v12, v13: vectors containing the final RGB values as f16 on the first 3 positions
	
	iau.sub i19 i19 4
	lsu0.st32  i20  i19 || iau.sub i19 i19 4
	lsu0.st32  i21  i19 || iau.sub i19 i19 4
	lsu0.st32  i22  i19 || iau.sub i19 i19 4
	lsu0.st32  i23  i19
	
	lsu0.ldil i5, ___YUV2RGB_coefs || lsu1.ldih i5, ___YUV2RGB_coefs
	;coeficients loading
	lsu0.ld64.l v5, i5 || iau.add i5, i5, 8 ; coeficients for u (Cu)
	lsu0.ld64.l v6, i5                      ; coeficients for v (Cv)
	lsu0.ldil i8, ___main_loop || lsu1.ldih i8, ___main_loop || iau.sub i9, i9, i9    ; loop counter
	iau.add i0, i18, i14  ; second line of y buffer	
	iau.mul i11, i14, 3   ; width * 3
	nop
	cmu.cpii i10, i15     ; out buffer reference for storing
	iau.add i11, i15, i11 ; out buffer + 3 * width
	
	;first y, u, v load
	sau.xor i6, i6, i6
	lsu0.ld8 i6, i17 || iau.add i17, i17, 1 || sau.xor i7, i7, i7 ; u 
	lsu1.ld8 i7, i16 || iau.add i16, i16, 1                       ; v
	iau.xor i1, i1, i1
	lsu0.ld8 i1, i18 || iau.add i18, i18, 1 || sau.xor i2, i2, i2 ; y1
	lsu0.ld8 i2, i18 || iau.add i18, i18, 1 || sau.xor i3, i3, i3 ; y2
	lsu0.ld8 i3, i0  || iau.add i0,  i0,  1 || sau.xor i4, i4, i4 ; y3
	lsu1.ld8 i4, i0  || iau.add i0,  i0,  1                       ; y4
	
	___main_loop:
		iau.add i9, i9, 2
		iau.sub i6, i6, 128
		iau.sub i7, i7, 128 || cmu.cpivr.i16.f16 v7, i6 ; u
		                       cmu.cpivr.i16.f16 v8, i7 ; v
		cmu.cpivr.i16.f16 v1, i1 || vau.mul.f16 v9,  v7, v5 ; u * Cu 
		cmu.cpivr.i16.f16 v2, i2 || vau.mul.f16 v10, v8, v6 ; v * Cv   
		cmu.cpivr.i16.f16 v3, i3
		cmu.cpivr.i16.f16 v4, i4 
		vau.add.f16 v9, v9, v10 || cmu.cmii.i32 i9, i14
		
		;RGB calculation and load next y, u and v values  
		iau.xor i6, i6, i6 || lsu0.ldil i23, 255 || lsu1.ldih i23, 0
		lsu0.ld8 i6, i17 || iau.add i17, i17, 1 || sau.xor i7, i7, i7 ; u
		vau.add.f16 v10, v1, v9 || lsu1.ld8 i7, i16 || iau.add i16, i16, 1 || sau.xor i1, i1, i1 ; v
		vau.add.f16 v11, v2, v9 || lsu0.ld8 i1, i18 || iau.add i18, i18, 1 || sau.xor i2, i2, i2 ; y1  
		vau.add.f16 v12, v3, v9 || lsu0.ld8 i2, i18 || iau.add i18, i18, 1 || sau.xor i3, i3, i3 ; y2
		vau.add.f16 v13, v4, v9 || lsu0.ld8 i3, i0  || iau.add i0,  i0,  1 || sau.xor i4, i4, i4 || cmu.cpvv.f16.i32s v10, v10 ; y3 
		lsu0.ld8 i4, i0  || iau.add i0,  i0,  1     || cmu.cpvv.f16.i32s v11, v11                ; y4 
	
		;store the results      
		cmu.cpvv.f16.i32s v12, v12
		cmu.cpvv.f16.i32s v13, v13
		
		cmu.cpvi i22, v10.0 
		cmu.cpvi i21, v10.1 || iau.clamp0.i32 i22, i22, i23
		cmu.cpvi i20, v10.2 || iau.clamp0.i32 i21, i21, i23 || lsu0.st8  i22, i10 
                               iau.clamp0.i32 i20, i20, i23 || lsu1.sto8 i21, i10, 1		
		cmu.cpvi i22, v11.0 || lsu0.sto8 i20, i10, 2 || iau.add i10, i10, 3
		cmu.cpvi i21, v11.1 || iau.clamp0.i32 i22, i22, i23
		cmu.cpvi i20, v11.2 || iau.clamp0.i32 i21, i21, i23 || lsu0.st8  i22, i10
		                       iau.clamp0.i32 i20, i20, i23 || lsu1.sto8 i21, i10, 1
		cmu.cpvi i22, v12.0 || lsu0.sto8 i20, i10, 2 || iau.add i10, i10, 3
		cmu.cpvi i21, v12.1 || iau.clamp0.i32 i22, i22, i23
		cmu.cpvi i20, v12.2 || iau.clamp0.i32 i21, i21, i23 || lsu0.st8  i22, i11
                               iau.clamp0.i32 i20, i20, i23 || lsu1.sto8 i21, i11, 1
        peu.pc1c LT || bru.jmp i8
		cmu.cpvi i22, v13.0 || lsu0.sto8 i20, i11, 2 || iau.add i11, i11, 3
		cmu.cpvi i21, v13.1 || iau.clamp0.i32 i22, i22, i23 
		cmu.cpvi i20, v13.2 || iau.clamp0.i32 i21, i21, i23 || lsu0.st8  i22, i11
                               iau.clamp0.i32 i20, i20, i23 || lsu1.sto8 i21, i11, 1
	    lsu0.sto8 i20, i11, 2 || iau.add i11, i11, 3
		                    
	lsu0.ld32  i23  i19 || iau.add i19 i19 4		                    
	lsu0.ld32  i22  i19 || iau.add i19 i19 4
	lsu0.ld32  i21  i19 || iau.add i19 i19 4
	lsu0.ld32  i20  i19 || iau.add i19 i19 4
	bru.jmp i30
	nop 5
.end
	