; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief   Color space transformation RGB to YUV422
; ///

.version 00.51.05
;-------------------------------------------------------------------------------
.data .rodata.cvtColorKernelRGBToYUB422
.salign 16
___RGB2YUV_coefs:
	.float16 0.299F16, 0.587F16, 0.114F16, 1.0F16, 0.564F16, 0.713F16, 0.0F16, 0.0F16

___plusUV:
    .float16 128.0

___clampVal:
    .float16 255.0
	
.code .text.cvtColorKernelRGBToYUV422
cvtColorKernelRGBToYUV422_asm:

	IAU.SUB i19 i19 8
	LSU0.ST64.l v24 i19 || IAU.SUB i19 i19 8
	LSU0.ST64.h v24 i19 || IAU.SUB i19 i19 8
	LSU0.ST64.l v25 i19 || IAU.SUB i19 i19 8
	LSU0.ST64.h v25 i19 || IAU.SUB i19 i19 8
	LSU0.ST64.l v26 i19 || IAU.SUB i19 i19 8
	LSU0.ST64.h v26 i19 || IAU.SUB i19 i19 8
	LSU0.ST64.l v27 i19 || IAU.SUB i19 i19 8
	LSU0.ST64.h v27 i19 || IAU.SUB i19 i19 8
	LSU0.ST64.l v28 i19 || IAU.SUB i19 i19 8
	LSU0.ST64.h v28 i19 || IAU.SUB i19 i19 8
	LSU0.ST64.l v29 i19 || IAU.SUB i19 i19 8
	LSU0.ST64.h v29 i19 || IAU.SUB i19 i19 8
	LSU0.ST64.l v30 i19 || IAU.SUB i19 i19 8
	LSU0.ST64.h v30 i19 || IAU.SUB i19 i19 8
	LSU0.ST64.l v31 i19 || IAU.SUB i19 i19 8
	LSU0.ST64.h v31 i19
	;void cvtColorKernelRGBToYUV422(u8** rIn(i18), u8** gIn(i17), u8** bIn(i16), u8** output(i15), u32 width(i14))
	;i18 - in R line
	;i17 - in G line
	;i16 - in B line
	;i15 - out buffer
	;i14 - width
	
	;y = ceil(0.299f * inR[j] + 0.587f * inG[j] + 0.114f * inB[j]);
	;u1 = (int)(((float)inB[j] - y) * 0.564f) + 128;
    ;v1 = (int)(((float)inR[j] - y) * 0.713f) + 128;
	
	; register mapping:
	;i11 - address of clamp val 255
	;i12 - address of coefs
	;i13 - address of plus_uv 128
	;v0  - coefs
	;v1 	- 8 values of R
	;v2 	- 8 values of G
	;v3 	- 8 values of B
	;v4, v5, v6, v9 - intermediar computations
	;v7  - 0 vector
	;v11 - 255 vector
	;v13 - 128 vector
	;v8, v10 vectors that contains the output
	
	
	;load  transformation coeficients, clamp values 255 and plus value 128 nedded to compute U and V
	;R line, G line, B line address
	LSU0.LDIL i12, ___RGB2YUV_coefs || LSU1.LDIH i12, ___RGB2YUV_coefs 	
	LSU0.LD32 i18 i18 				|| LSU1.LD32 i17 i17				
	LSU0.LD32 i16 i16		    	|| LSU1.LD32 i15 i15      
	LSU0.LDIL i13, ___plusUV 		|| LSU1.LDIH i13, ___plusUV
	LSU0.LDIL i11, ___clampVal 		|| LSU1.LDIH i11, ___clampVal
	LSU0.LD16r v22, i13 			|| LSU1.LD16r v31, i11
	
	
	LSU0.LD64.l v0 i12 || IAU.ADD i12 i12 8
	LSU1.LD64.h v0 i12 ;incarcare coeficienti
	
	
	
	;load 8 values of R, 8 values of G, 8 values of B
	LSU0.LDI128.u8.f16 v1 i18 || LSU1.LDI128.u8.f16 v2 i17
	LSU0.LDI128.u8.f16 v3 i16
	
	LSU0.LDIL i6, ___loop || LSU1.LDIH i6, ___loop || IAU.SHR.u32 i7 i14 3 || VAU.XOR v23, v23, v23 
	NOP 3	
	
		
	VAU.MUL.f16 v4 v1 v0 	|| LSU0.SWZV8 [00000000] 
	VAU.MUL.f16 v5 v2 v0 	|| LSU0.SWZV8 [11111111] ;0.587 * G
	VAU.MUL.f16 v6 v3 v0 	|| LSU0.SWZV8 [22222222] ;0.114  * B
	NOP
	VAU.ADD.f16 v7 v4 v5 ; 0.299 * R + 0.587 * G
	NOP 2	
	VAU.ADD.f16 v8 v7 v6 ; 0.299 * R + 0.587 * G + 0.114  * B => Y
	NOP 2
	VAU.SUB.f16 v9  v1  v8 ;R - Y
	VAU.SUB.f16 v10 v3  v8 ;B - Y
	VAU.ADD.f16 v11 v23 v8 	|| LSU0.SWZV8 [03020100] ;Y0 # Y1 # Y3 # y4 #
	VAU.ADD.f16 v12 v23 v8 	|| LSU0.SWZV8 [07060504] ;Y4 # y5 # y6 # y7 #
	VAU.MUL.f16 v13 v9  v0 	|| LSU0.SWZV8 [55555555] ;(R - Y) * 0.713
	VAU.MUL.f16 v14 v10 v0 	|| LSU0.SWZV8 [44444444] ;(B - Y) * 0.564
	VAU.MUL.f16 v15 v11 v0 	|| LSU0.SWZV8 [63636363] 
		
		.lalign
		VAU.MUL.f16 v16 v12 v0 	|| LSU0.SWZV8 [63636363]  || LSU1.LDI128.u8.f16 v1 i18 || BRU.RPL i6 i7;0.299 * R ;Y0 0 Y1 0 Y2 0 Y3 0
		VAU.ADD.f16 v17 v13 v22 || LSU1.LDI128.u8.f16 v2 i17 || LSU0.LDI128.u8.f16 v3 i16 ;(R - Y) * 0.713 + 128 =>V
		VAU.ADD.f16 v18 v14 v22 ;(B - Y) * 0.564 + 128 =>U
		NOP
		VAU.ADD.f16 v19 v17 v17 || LSU0.SWZV8 [67452301]
		VAU.ADD.f16 v20 v18 v18 || LSU0.SWZV8 [67452301]
		VAU.MUL.f16 v4 v1 v0 	|| LSU0.SWZV8 [00000000] 
		VAU.MUL.f16 v19 v19 0.5
		VAU.MUL.f16 v20 v20 0.5
		VAU.MUL.f16 v5 v2 v0 	|| LSU0.SWZV8 [11111111]
		VAU.MUL.f16 v21 v19 v0 	|| LSU0.SWZV8 [63636363] ;v12 0 v34 0 v56 0 v78
		VAU.MUL.f16 v24 v20 v0 	|| LSU0.SWZV8 [36363636] ;u12 0 u34 0 u56 0 u78
		VAU.MUL.f16 v6 v3 v0 	|| LSU0.SWZV8 [22222222]
		VAU.ADD.f16 v7 v4 v5
		VAU.ADD.f16 v25 v21 v24 ; u12 v12 u34 v34 u56 v56 u78 v78
		NOP 
		VAU.ADD.f16 v8 v7 v6
		VAU.ADD.f16 v26 v23 v25 || LSU0.SWZV8 [20300010] ;# u12 # v12 # u34 # v34
		VAU.ADD.f16 v27 v23 v25 || LSU0.SWZV8 [60704050] ;# u56 # v56 # u78 #v78
		VAU.SUB.f16 v9  v1  v8
	
		VAU.MUL.f16 v28 v26 v0 	|| LSU0.SWZV8 [36363636] ;0 u12 0 v12 0 u34 0 v34
		VAU.MUL.f16 v29 v27 v0 	|| LSU0.SWZV8 [36363636] ;0 u56 0 v56 0 u78 0 v78	
	
		;load 8 values of R, 8 values of G, 8 values of B
		VAU.SUB.f16 v10 v3  v8 ;B - Y
		VAU.ADD.f16 v11 v23 v8 	|| LSU0.SWZV8 [03020100] ;Y0 # Y1 # Y3 # y4 #
	
		VAU.ADD.f16 v28 v15 v28
		___loop:
		VAU.ADD.f16 v29 v16 v29
		VAU.ADD.f16 v12 v23 v8 		|| LSU0.SWZV8 [07060504] ;Y4 # y5 # y6 # y7 #
		CMU.CLAMP0.f16 v28 v28 v31 	|| VAU.MUL.f16 v13 v9  v0 || LSU0.SWZV8 [55555555] ;(R - Y) * 0.713
		CMU.CLAMP0.f16 v29 v29 v31 	|| VAU.MUL.f16 v14 v10 v0 || LSU0.SWZV8 [44444444] ;(B - Y) * 0.564
		LSU0.STI128.f16.u8 v28 i15 	|| VAU.MUL.f16 v15 v11 v0 || LSU1.SWZV8 [63636363]  
		LSU0.STI128.f16.u8 v29 i15
		
	LSU0.LD64.h v31 i19 || IAU.ADD i19 i19 8 
	LSU0.LD64.l v31 i19 || IAU.ADD i19 i19 8
	LSU0.LD64.h v30 i19 || IAU.ADD i19 i19 8 
	LSU0.LD64.l v30 i19 || IAU.ADD i19 i19 8
	LSU0.LD64.h v29 i19 || IAU.ADD i19 i19 8 
	LSU0.LD64.l v29 i19 || IAU.ADD i19 i19 8
	LSU0.LD64.h v28 i19 || IAU.ADD i19 i19 8 
	LSU0.LD64.l v28 i19 || IAU.ADD i19 i19 8
	LSU0.LD64.h v27 i19 || IAU.ADD i19 i19 8 
	LSU0.LD64.l v27 i19 || IAU.ADD i19 i19 8		
	LSU0.LD64.h v26 i19 || IAU.ADD i19 i19 8 
	LSU0.LD64.l v26 i19 || IAU.ADD i19 i19 8		
	LSU0.LD64.h v25 i19 || IAU.ADD i19 i19 8 
	LSU0.LD64.l v25 i19 || IAU.ADD i19 i19 8
	LSU0.LD64.h v24 i19 || IAU.ADD i19 i19 8
	LSU0.LD64.l v24 i19 || IAU.ADD i19 i19 8
	BRU.jmp i30
	nop 5
.end