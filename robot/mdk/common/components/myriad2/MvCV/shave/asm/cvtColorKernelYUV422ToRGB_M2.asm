; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief   Color space transformation YUV422 to RGB
; ///

.version 00.51.05
;-------------------------------------------------------------------------------
.data .rodata.cvtColorKernelYUV422ToRGB
.salign 16
___YUV2RGB_coefs:
	.float16 1.402f16, 0.344f16, 0.714f16, 1.772f16

___minusUV:
    .float16 128.0

___clampVal:
    .float16 255.0
	
.code .text.cvtColorKernelYUV422ToRGB
cvtColorKernelYUV422ToRGB_asm:
	;void cvtColorKernelYUV422ToRGB_asm(u8** input(i18), u8** rOut(i17), u8** gOut(i16), u8** bOut(i15), u32 width(i14));
	;i18 - in buffer
	;i17 - r out buffer
	;i16 - g out buffer
	;i15 - b out buffer
	;i14 - width
	
	;R = Y+ (int)(1.402f * V);
    ;G = Y - (int)(0.344f * U + 0.714f * V);
    ;B = Y + (int)(1.772f * U);
	;
	; - every iteration of the main loop computes 8 pixels (8xR. 8xG and 8xB)

	; register mapping:
	;i6	addres loop
	;i7	width / 8	
	;i12	addres for transformation coeficients
	;i13	adresa of 128 value
	;i14	width
	;i15	line B
	;i16	line G
	;i17	line R
	;i18	pointer inline
	
	;v0	transformation coeficients
	;v1	8 values of yuv  (4*y, 2*u, 2*v)
	;v2	128 value	
	;v3	v1 - v2	
	;v4	1.402f * (y1-128) | 1.772f  * (u1-128) | 1.402f * (y2-128) | 1.402f * (v1-128) | 1.402f * (y3-128) |  1.772f * (u2-128) |  1.402f * (y4-128) | 1.402f * (v2-128)	
	;v5	1.402f * (y1-128) | 0.344f * (u1-128) | 1.402f * (y2-128) | 0.714f *  (v1-128) | 1.402f * (y3-128) |  0.344f * (u2-128) |  1.402f * (y4-128) | 0.714f * (v2-128)
	;v6	first 4 values of B (will be clamped)	
	;v7	first 4 values of R (will be clamped)		
	;v8	* | V1 + U1 | * | V1 + U1 | * | V2 + U2 | * | V2 + U2	where    V2 = 0.714f *( v2-128)  etc
	;v9	first 4 values of G (will be clamped)		
	;v10	255 value	
	;v11	0 value	
	;v12	contain B values in the lower half, composed with v23 => all B values
	;v13	contain R values in the lower half, composed with v24 => all R values
	;v14	contain G values in the lower half, composed with v25 => all G values
	;v15	omolog of v1 contain the next 8 values	
	;v16	omolog of v3	 (for the next 8 values)
	;v17	omolog of v4	(for the next 8 values)
	;v18	omolog of v5 	--||--	
	;v19	omolog of v6 	--||--		
	;v20	omolog of v7 	--||--		
	;v21	omolog of v8 	--||--		
	;v22	omolog of v9 	--||--		
	;v23	omolog of v12 	--||--		
	;v24	omolog of v13 	--||--	
	;v25	omolog of v14 	--||--		
	
	;save v24 and v25 on the stack
	IAU.SUB i19 i19 8
	LSU0.ST.64.L v24 i19 || IAU.SUB i19 i19 8
	LSU0.ST.64.H v24 i19 || IAU.SUB i19 i19 8
	LSU0.ST.64.L v25 i19 || IAU.SUB i19 i19 8
	LSU0.ST.64.H v25 i19 || IAU.SUB i19 i19 8
	LSU0.ST.64.L v26 i19 || IAU.SUB i19 i19 8
	LSU0.ST.64.H v26 i19
	
	
	LSU0.LDIL i12, ___YUV2RGB_coefs || LSU1.LDIH i12, ___YUV2RGB_coefs 	
	LSU0.LD.32 i18 i18 				|| LSU1.LD.32 i17 i17				
	LSU0.LD.32 i16 i16		        || LSU1.LD.32 i15 i15      
	LSU0.LDIL i13, ___minusUV  		|| LSU1.LDIH i13, ___minusUV
	LSU0.LDIL i11, ___clampVal 		|| LSU1.LDIH i11, ___clampVal
	LSU0.LD.16R v2, i13 				|| LSU1.LD.16R v10, i11
NOP
	LSU0.LD.64.L v0 i12 
	LSU0.LDIL i6, ___loop 			|| LSU1.LDIH i6, ___loop || IAU.SHR.u32 i7 i14 3 || VAU.XOR v11, v11, v11 
	
	LSU0.LDI.128.U8.F16 v1 i18 ;load first 8 values
NOP 5
	LSU0.LDI.128.U8.F16 v15 i18 ;;load the next 8 values		
	VAU.SUB.f16 v3 v1 v2   ;substract 128 from the first 8 values
NOP 5
	
	VAU.SUB.f16 v16 v15 v2  ;substract 128 from the second 8 values
	VAU.MUL.f16 v4  v0 v3  	|| LSU0.SWZV8 [00300030] 
	VAU.MUL.f16 v5  v0 v3  	|| LSU0.SWZV8 [20102010]  	
	VAU.MUL.f16 v17 v0 v16 	|| LSU0.SWZV8 [00300030] 
	VAU.MUL.f16 v18 v0 v16 	|| LSU0.SWZV8 [20102010]
	VAU.ADD.f16 v6  v4 v1  	|| LSU0.SWZV8 [05050101]
	
	;;######################################################################## BEBINE LOOP ###############################
	.lalign
	VAU.ADD.f16 v19 v17 v15 	|| LSU0.SWZV8 [05050101] || BRU.RPL i6 i7	
	
	VAU.ADD.f16 v7  v4  v1 		|| LSU0.SWZV8 [07070303]
	VAU.ADD.f16 v8  v5  v5 		|| LSU0.SWZV8 [50701030]
	VAU.ADD.f16 v20 v17 v15 	|| LSU0.SWZV8 [07070303]	
	VAU.ADD.f16 v21 v18 v18	 	|| LSU0.SWZV8 [50701030] 	|| CMU.CLAMP0.f16 v6 v6 v10		|| LSU1.LDI.128.U8.F16 v1 i18
	CMU.CPVV v26 v8 || LSU0.SWZC8 [05050101] 
	VAU.SUB.f16 v9 v1 v26 		|| CMU.CLAMP0.f16 v7 v7 v10 	|| LSU1.LDI.128.U8.F16 v15 i18
	VAU.ADD.f16 v12 v6 v11  	|| LSU0.SWZV8 [00006420] 	|| CMU.CLAMP0.f16 v20 v20 v10
	CMU.CPVV v26 v21 || LSU0.SWZC8 [05050101] 
	VAU.SUB.f16 v22 v15 v26 	 	
	VAU.ADD.f16 v13 v7  v11		|| LSU0.SWZV8 [00006420]	|| CMU.CLAMP0.f16 v9 v9 v10	
	VAU.ADD.f16 v14 v9	v11  	|| LSU0.SWZV8 [00006420] 	|| CMU.CLAMP0.f16 v19 v19 v10
	VAU.ADD.f16 v23 v19 v11		|| LSU0.SWZV8 [00006420]
	VAU.ADD.f16 v24 v20 v11 	|| LSU0.SWZV8 [00006420]
	CMU.CLAMP0.f16 v22 v22 v10 	|| VAU.SUB.f16 v3 v1 v2	
	VAU.ADD.f16 v25 v22  v11	|| LSU0.SWZV8 [00006420]
	
___loop:
	CMU.VSZM.WORD v13 v24 [10DD] || VAU.SUB.f16 v16 v15 v2
	CMU.VSZM.WORD v12 v23 [10DD] || VAU.MUL.f16 v4 v0 v3 	|| LSU1.SWZV8 [00300030]	
	;store the results
	LSU0.ST.128.F16.U8 v12 i15 	|| IAU.ADD i15, i15, 8 		|| VAU.MUL.f16 v5  v0 v3  		|| LSU1.SWZV8 [20102010] 	|| CMU.VSZM.WORD v14 v25 [10DD]	
	LSU0.ST.128.F16.U8 v13 i17 	|| IAU.ADD i17, i17, 8 		|| VAU.MUL.f16 v17 v0 v16 		|| LSU1.SWZV8 [00300030] 
	LSU0.ST.128.F16.U8 v14 i16 	|| IAU.ADD i16, i16, 8 		|| VAU.MUL.f16 v18 v0 v16 		|| LSU1.SWZV8 [20102010]
	VAU.ADD.f16 v6 v4 v1 		|| LSU0.SWZV8 [05050101]
	nop
	
	;;######################################################################## END LOOP ###############################
	LSU0.LD.64.H v26 i19 || IAU.ADD i19 i19 8 
	LSU0.LD.64.L v26 i19 || IAU.ADD i19 i19 8	
	LSU0.LD.64.H v25 i19 || IAU.ADD i19 i19 8 
	LSU0.LD.64.L v25 i19 || IAU.ADD i19 i19 8
	LSU0.LD.64.H v24 i19 || IAU.ADD i19 i19 8
	LSU0.LD.64.L v24 i19 || IAU.ADD i19 i19 8
	
	BRU.jmp i30
NOP 6
.end
