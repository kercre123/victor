; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief   canny edge detection 
; ///

.version 00.51.05
;-------------------------------------------------------------------------------
.data .rodata.canny
.salign 16
	___blur:
		.short    14, 30, 38, 30, 14, 0, 0, 0
	
	___vect18:
        .float16 0.006289, 0.006289, 0.006289, 0.006289, 0.006289, 0.006289, 0.006289, 0.006289
		
	___multiply:
        .float16 0.14, 0.30, 0.38, 0, 0, 0, 0, 0
		
	___clampVal:
        .float16 255.0
		
	___sobel:
        .float16 -1, -2, -1, 1, 2, 1, 0, 0
	___tangent:
		.float16 0.414213, 2.414213, -2.414213, -0.414213


.code .text.canny
canny_asm:
.lalign
;void canny(u8** srcAddr, u8** dstAddr,  double threshold1, double threshold2, u32 width);
;                i18          i17             s23                s22              i16

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
	LSU0.ST64.h v31  i19  
	
	IAU.SUB i19 i19 4
	LSU0.ST32  s24  i19 || IAU.SUB i19 i19 4
	LSU0.ST32  s25  i19 || IAU.SUB i19 i19 4
	LSU0.ST32  s26  i19 || IAU.SUB i19 i19 4
	LSU0.ST32  s27  i19 || IAU.SUB i19 i19 4
	LSU0.ST32  s28  i19 || IAU.SUB i19 i19 4
	LSU0.ST32  s29  i19 || IAU.SUB i19 i19 4
	LSU0.ST32  s30  i19 || IAU.SUB i19 i19 4
	LSU0.ST32  s31  i19 

	IAU.SUB i19 i19 4
	LSU0.ST32  i20  i19 || IAU.SUB i19 i19 4
	LSU0.ST32  i21  i19 || IAU.SUB i19 i19 4
	LSU0.ST32  i22  i19 || IAU.SUB i19 i19 4
	LSU0.ST32  i23  i19 || IAU.SUB i19 i19 4
	LSU0.ST32  i24  i19 || IAU.SUB i19 i19 4
	LSU0.ST32  i25  i19 || IAU.SUB i19 i19 4
	LSU0.ST32  i26  i19 || IAU.SUB i19 i19 4
	LSU0.ST32  i27  i19 || IAU.SUB i19 i19 4
	LSU0.ST32  i28  i19 || IAU.SUB i19 i19 4
	LSU0.ST32  i29  i19 
	
	;lines for blur effect
	IAU.SUB i19 i19 i16
	IAU.SUB i19 i19 8
	IAU.SUB i10 i19 0
	IAU.SUB i19 i19 i16
	IAU.SUB i19 i19 8
	IAU.SUB i11 i19 0
	IAU.SUB i19 i19 i16
	IAU.SUB i19 i19 8
	IAU.SUB i12 i19 0
	IAU.SUB i19 i19 i16
	IAU.SUB i19 i19 8
	IAU.SUB i13 i19 0
	IAU.SUB i19 i19 i16
	IAU.SUB i19 i19 8
	IAU.SUB i14 i19 0
	

	
	lsu0.ldil i1, ___clampVal   || lsu1.ldih i1, ___clampVal
	lsu0.ldil i2, ___vect18     || lsu1.ldih i2, ___vect18
	lsu0.ldil i3, ___multiply     || lsu1.ldih i3, ___multiply
	lsu0.ldil i4, ___sobel     || lsu1.ldih i4, ___sobel
	lsu0.ld16r v22, i1
	LSU0.LD64.l   v25  i2 || IAU.ADD i2 i2 8
	LSU0.LD64.h   v25  i2
	LSU0.LD64.l   v26  i3 || IAU.ADD i3 i3 8
	LSU0.LD64.h   v26  i3
	LSU0.LD64.l   v30  i4 || IAU.ADD i4 i4 8
	LSU0.LD64.h   v30  i4
	

	;init cmx lines that will be processed
	LSU0.LD32  i1  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i2  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i3  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i4  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i5  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i6  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i7  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i8  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i9  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i17  i17 
	nop 5
	CMU.CPSS.f32.f16      s23 s23
	CMU.CPSS.f32.f16      s22 s22

	
	;vectors for blur effect
	LSU0.ldil i20, ___blur || LSU1.ldih i20, ___blur
	LSU0.LD64.l  v0  i20 || IAU.ADD i20 i20 8    
	LSU0.LD64.h  v0  i20 || IAU.ADD i20 i20 8
	nop 5    
	
	IAU.SUB i21 i21 i21 ;contor for loop
	IAU.SUB i16 i16 4
	LSU0.ldil i22, ___loop || LSU1.ldih i22, ___loop
	

	
	;skipp first 2 pixels addresses beause they are not computed corectly
	IAU.ADD i10 i10 2
	IAU.ADD i11 i11 2
	IAU.ADD i12 i12 2
	IAU.ADD i13 i13 2
	IAU.ADD i14 i14 2

	
	LSU0.LD128.u8.u16 v1 i1 || IAU.ADD i1 i1 8
	LSU0.LD128.u8.u16 v2 i2 || IAU.ADD i2 i2 8 || 	LSU1.LD128.u8.u16 v11 i1 
	LSU0.LD128.u8.u16 v3 i3 || IAU.ADD i3 i3 8 || 	LSU1.LD128.u8.u16 v12 i2 
	LSU0.LD128.u8.u16 v4 i4 || IAU.ADD i4 i4 8 || 	LSU1.LD128.u8.u16 v13 i3 
	LSU0.LD128.u8.u16 v5 i5 || IAU.ADD i5 i5 8 || 	LSU1.LD128.u8.u16 v14 i4 
	LSU0.LD128.u8.u16 v6 i6 || IAU.ADD i6 i6 8 || 	LSU1.LD128.u8.u16 v15 i5 
	LSU0.LD128.u8.u16 v7 i7 || IAU.ADD i7 i7 8 || 	LSU1.LD128.u8.u16 v16 i6 
	LSU0.LD128.u8.u16 v8 i8 || IAU.ADD i8 i8 8 || 	LSU1.LD128.u8.u16 v17 i7 
	LSU0.LD128.u8.u16 v9 i9 || IAU.ADD i9 i9 8 || 	LSU1.LD128.u8.u16 v18 i8 
	LSU0.LD128.u8.u16 v19 i9 
	nop 
	
;*************pixels of first line proccessed
	VAU.ADD.i16 v21 v11 v15
	VAU.ADD.i16 v27 v12 v14
	VAU.MUL.i16 v21 v21 v0 || lsu0.swzv8 [00000000] 
	VAU.MUL.i16 v27 v27 v0 || lsu0.swzv8 [11111111] 
	VAU.MUL.i16 v28 v13 v0 || lsu0.swzv8 [22222222] 
	nop
	VAU.ADD.i16 v27 v21 v27
	VAU.ADD.i16 v21 v1 v5
	VAU.ADD.i16 v28 v27 v28
	VAU.ADD.i16 v27 v2 v4
	VAU.MUL.i16 v21 v21 v0 || lsu0.swzv8 [00000000] || 	CMU.CPVI.x16   i25.l v28.0   
	VAU.MUL.i16 v27 v27 v0 || lsu0.swzv8 [11111111] || 	CMU.CPVI.x16   i25.h v28.1 
	VAU.MUL.i16 v29 v3 v0 || lsu0.swzv8 [22222222] || 	CMU.CPVI.x16   i26.l v28.2	
	CMU.CPVI.x16   i26.h v28.3
	
	
	
	___loop:
	VAU.ADD.i16 v27 v21 v27
	LSU0.LD128.u8.u16 v1 i1 || IAU.ADD i1 i1 8
	VAU.ADD.i16 v29 v27 v29
	nop 
	cmu.vrot v10 v29 2 || 	VAU.ADD.i16 v21 v12 v16
	cmu.cpiv.x16 v10.7 i25.l || 	VAU.ADD.i16 v27 v13 v15;l1
	cmu.vrot v20 v10 2 || VAU.MUL.i16 v21 v21 v0 || lsu0.swzv8 [00000000] 
	cmu.cpiv.x16 v20.7  i25.h || 	VAU.MUL.i16 v27 v27 v0 || lsu0.swzv8 [11111111];l2
	cmu.vrot v23 v20 2 || 	VAU.MUL.i16 v28 v14 v0 || lsu0.swzv8 [22222222] 
	cmu.cpiv.x16 v23.7 i26.l ;l3 
	cmu.vrot v24 v23 2 || 	VAU.ADD.i16 v27 v21 v27
	cmu.cpiv.x16 v24.7 i26.h || 	VAU.ADD.i16 v21 v2 v6;l4
	CMU.CPVV.i16.f16  v29 v29 || 	VAU.ADD.i16 v28 v27 v28
	CMU.CPVV.i16.f16  v10 v10 || 	VAU.ADD.i16 v27 v3 v5
	CMU.CPVV.i16.f16  v20 v20 || 	LSU0.LD128.u8.u16 v2 i2 || IAU.ADD i2 i2 8 || 	LSU1.LD128.u8.u16 v11 i1 
	CMU.CPVV.i16.f16  v23 v23
	CMU.CPVV.i16.f16  v24 v24
	VAU.ADD.f16 v23 v10 v23
	VAU.ADD.f16 v24 v29 v24
	VAU.MUL.i16 v21 v21 v0 || lsu0.swzv8 [00000000] || 	CMU.CPVI.x16   i25.l v28.0   
	VAU.MUL.i16 v27 v27 v0 || lsu0.swzv8 [11111111] || 	CMU.CPVI.x16   i25.h v28.1 
	VAU.MUL.f16 v24 v24 v26 || lsu0.swzv8 [00000000]
	VAU.MUL.f16 v23 v23 v26 || lsu0.swzv8 [11111111]
	VAU.MUL.f16 v20 v20 v26 || lsu0.swzv8 [22222222]
	VAU.MUL.i16 v29 v4 v0 || lsu0.swzv8 [22222222] || 	CMU.CPVI.x16   i26.l v28.2	
	CMU.CPVI.x16   i26.h v28.3 || 	VAU.ADD.f16 v24 v23 v24

	nop 2
	VAU.ADD.f16 v20 v20 v24
	nop 2
	VAU.MUL.f16 v20 v20 v25
	nop 2
	CMU.CLAMP0.F16 v20 v20 v22
	LSU0.ST128.f16.u8 v20 i10 || IAU.ADD i10 i10 8
	
	
;*************pixels of second line proccessed

	VAU.ADD.i16 v27 v21 v27
	nop 
	VAU.ADD.i16 v29 v27 v29
	nop 
	cmu.vrot v10 v29 2       || VAU.ADD.i16 v21 v13 v17         
	cmu.cpiv.x16 v10.7 i25.l || VAU.ADD.i16 v27 v14 v16
	cmu.vrot v20 v10 2 || 	VAU.MUL.i16 v21 v21 v0 || lsu0.swzv8 [00000000] 
	cmu.cpiv.x16 v20.7 i25.h || 	VAU.MUL.i16 v27 v27 v0 || lsu0.swzv8 [11111111] ;l2
	cmu.vrot v23 v20 2 || 	VAU.MUL.i16 v28 v15 v0 || lsu0.swzv8 [22222222] 
	cmu.cpiv.x16 v23.7 i26.l ;l3 
	cmu.vrot v24 v23 2 || 	VAU.ADD.i16 v27 v21 v27
	cmu.cpiv.x16 v24.7 i26.h || 	VAU.ADD.i16 v21 v3 v7;l4
	CMU.CPVV.i16.f16  v29 v29 || 	VAU.ADD.i16 v28 v27 v28
	CMU.CPVV.i16.f16  v10 v10 || 	VAU.ADD.i16 v27 v4 v6
	CMU.CPVV.i16.f16  v20 v20 || LSU0.LD128.u8.u16 v3 i3 || IAU.ADD i3 i3 8 || 	LSU1.LD128.u8.u16 v12 i2 
	CMU.CPVV.i16.f16  v23 v23
	CMU.CPVV.i16.f16  v24 v24
	VAU.ADD.f16 v23 v10 v23
	VAU.ADD.f16 v24 v29 v24
	VAU.MUL.i16 v21 v21 v0 || lsu0.swzv8 [00000000] || 	CMU.CPVI.x16   i25.l v28.0   
	VAU.MUL.i16 v27 v27 v0 || lsu0.swzv8 [11111111] || 	CMU.CPVI.x16   i25.h v28.1 
	VAU.MUL.f16 v24 v24 v26 || lsu0.swzv8 [00000000]
	VAU.MUL.f16 v23 v23 v26 || lsu0.swzv8 [11111111]
	VAU.MUL.f16 v20 v20 v26 || lsu0.swzv8 [22222222]
	VAU.MUL.i16 v29 v5 v0 || lsu0.swzv8 [22222222] || 	CMU.CPVI.x16   i26.l v28.2	
	CMU.CPVI.x16   i26.h v28.3 || 	VAU.ADD.f16 v24 v23 v24

	nop 2
	VAU.ADD.f16 v20 v20 v24
	nop 2
	VAU.MUL.f16 v20 v20 v25
	nop 2
	CMU.CLAMP0.F16 v20 v20 v22
	LSU0.ST128.f16.u8 v20 i11 || IAU.ADD i11 i11 8
	
	
;*************pixels of third line proccessed

	VAU.ADD.i16 v27 v21 v27
	nop 
	VAU.ADD.i16 v29 v27 v29
	nop 
	cmu.vrot v10 v29 2       ||   VAU.ADD.i16 v21 v14 v18
	cmu.cpiv.x16 v10.7 i25.l ||   VAU.ADD.i16 v27 v15 v17
	cmu.vrot v20 v10 2       ||   VAU.MUL.i16 v21 v21 v0 || lsu0.swzv8 [00000000] 
	cmu.cpiv.x16 v20.7 i25.h ||   VAU.MUL.i16 v27 v27 v0 || lsu0.swzv8 [11111111] 
	cmu.vrot v23 v20 2       ||   VAU.MUL.i16 v28 v16 v0 || lsu0.swzv8 [22222222] 
	cmu.cpiv.x16 v23.7 i26.l  
	cmu.vrot v24 v23 2         || VAU.ADD.i16 v27 v21 v27
	cmu.cpiv.x16 v24.7 i26.h   || VAU.ADD.i16 v21 v4 v8
	CMU.CPVV.i16.f16  v29 v29  || VAU.ADD.i16 v28 v27 v28
	CMU.CPVV.i16.f16  v10 v10  || VAU.ADD.i16 v27 v5 v7
	CMU.CPVV.i16.f16  v20 v20 || LSU0.LD128.u8.u16 v4 i4 || IAU.ADD i4 i4 8 || 	LSU1.LD128.u8.u16 v13 i3 
	CMU.CPVV.i16.f16  v23 v23
	CMU.CPVV.i16.f16  v24 v24
	VAU.ADD.f16 v23 v10 v23
	VAU.ADD.f16 v24 v29 v24
	VAU.MUL.i16 v21 v21 v0 || lsu0.swzv8 [00000000] || 	CMU.CPVI.x16   i25.l v28.0   
	VAU.MUL.i16 v27 v27 v0 || lsu0.swzv8 [11111111] || 	CMU.CPVI.x16   i25.h v28.1 
	VAU.MUL.f16 v24 v24 v26 || lsu0.swzv8 [00000000]
	VAU.MUL.f16 v23 v23 v26 || lsu0.swzv8 [11111111]
	VAU.MUL.f16 v20 v20 v26 || lsu0.swzv8 [22222222]
	VAU.MUL.i16 v29 v6 v0 || lsu0.swzv8 [22222222] || 	CMU.CPVI.x16   i26.l v28.2	
	CMU.CPVI.x16   i26.h v28.3 	|| VAU.ADD.f16 v24 v23 v24
	nop 2
	VAU.ADD.f16 v20 v20 v24
	nop 2
	VAU.MUL.f16 v20 v20 v25
	nop 2
	CMU.CLAMP0.F16 v20 v20 v22
	LSU0.ST128.f16.u8 v20 i12 || IAU.ADD i12 i12 8
	
;*************pixels of fourth line proccessed

	VAU.ADD.i16 v27 v21 v27
	nop 
	VAU.ADD.i16 v29 v27 v29
	nop 
	cmu.vrot v10 v29 2        ||   VAU.ADD.i16 v21 v15 v19
	cmu.cpiv.x16 v10.7 i25.l  ||   VAU.ADD.i16 v27 v16 v18
	cmu.vrot v20 v10 2        ||   VAU.MUL.i16 v21 v21 v0 || lsu0.swzv8 [00000000] 
	cmu.cpiv.x16 v20.7 i25.h  ||   VAU.MUL.i16 v27 v27 v0 || lsu0.swzv8 [11111111] 
	cmu.vrot v23 v20 2        ||   VAU.MUL.i16 v28 v17 v0 || lsu0.swzv8 [22222222] 
	cmu.cpiv.x16 v23.7 i26.l  
	cmu.vrot v24 v23 2        ||  VAU.ADD.i16 v27 v21 v27
	cmu.cpiv.x16 v24.7 i26.h  ||  VAU.ADD.i16 v21 v5 v9
	CMU.CPVV.i16.f16  v29 v29 ||  VAU.ADD.i16 v28 v27 v28
	CMU.CPVV.i16.f16  v10 v10 ||  VAU.ADD.i16 v27 v6 v8
	CMU.CPVV.i16.f16  v20 v20 || LSU0.LD128.u8.u16 v5 i5 || IAU.ADD i5 i5 8 || 	LSU1.LD128.u8.u16 v14 i4 
	CMU.CPVV.i16.f16  v23 v23 || LSU0.LD128.u8.u16 v6 i6 || IAU.ADD i6 i6 8 || 	LSU1.LD128.u8.u16 v15 i5 
	CMU.CPVV.i16.f16  v24 v24
	VAU.ADD.f16 v23 v10 v23
	VAU.ADD.f16 v24 v29 v24
	VAU.MUL.i16 v21 v21 v0 || lsu0.swzv8 [00000000] || 	CMU.CPVI.x16   i25.l v28.0   
	VAU.MUL.i16 v27 v27 v0 || lsu0.swzv8 [11111111] || 	CMU.CPVI.x16   i25.h v28.1 
	VAU.MUL.f16 v24 v24 v26 || lsu0.swzv8 [00000000]
	VAU.MUL.f16 v23 v23 v26 || lsu0.swzv8 [11111111]
	VAU.MUL.f16 v20 v20 v26 || lsu0.swzv8 [22222222]
	VAU.MUL.i16 v29 v7 v0 || lsu0.swzv8 [22222222] || 	CMU.CPVI.x16   i26.l v28.2	
	CMU.CPVI.x16   i26.h v28.3 || 	VAU.ADD.f16 v24 v23 v24
	LSU0.LD128.u8.u16 v7 i7 || IAU.ADD i7 i7 8 || 	LSU1.LD128.u8.u16 v16 i6 
	LSU0.LD128.u8.u16 v8 i8 || IAU.ADD i8 i8 8 || 	LSU1.LD128.u8.u16 v17 i7 
	VAU.ADD.f16 v20 v20 v24
	LSU0.LD128.u8.u16 v9 i9 || IAU.ADD i9 i9 8 || 	LSU1.LD128.u8.u16 v18 i8 
	LSU0.LD128.u8.u16 v19 i9 
	VAU.MUL.f16 v20 v20 v25
	nop 2
	CMU.CLAMP0.F16 v20 v20 v22
	LSU0.ST128.f16.u8 v20 i13 || IAU.ADD i13 i13 8
	
;*************pixels of fifth line proccessed

	VAU.ADD.i16 v27 v21 v27
	IAU.ADD i21 i21 8
	VAU.ADD.i16 v29 v27 v29
	CMU.CMII.i32 i21 i16 
	cmu.vrot v10 v29 2        ||  VAU.ADD.i16 v21 v11 v15   
	cmu.cpiv.x16 v10.7 i25.l  ||  VAU.ADD.i16 v27 v12 v14
	cmu.vrot v20 v10 2        ||  VAU.MUL.i16 v21 v21 v0 || lsu0.swzv8 [00000000] 
	cmu.cpiv.x16 v20.7 i25.h  ||  VAU.MUL.i16 v27 v27 v0 || lsu0.swzv8 [11111111] 
	cmu.vrot v23 v20 2        ||  VAU.MUL.i16 v28 v13 v0 || lsu0.swzv8 [22222222] 
	cmu.cpiv.x16 v23.7 i26.l 
	cmu.vrot v24 v23 2         || VAU.ADD.i16 v27 v21 v27
	cmu.cpiv.x16 v24.7 i26.h   || VAU.ADD.i16 v21 v1 v5
	CMU.CPVV.i16.f16  v29 v29  || VAU.ADD.i16 v28 v27 v28
	CMU.CPVV.i16.f16  v10 v10  || VAU.ADD.i16 v27 v2 v4
	CMU.CPVV.i16.f16  v20 v20
	CMU.CPVV.i16.f16  v23 v23
	CMU.CPVV.i16.f16  v24 v24
	VAU.ADD.f16 v23 v10 v23
	VAU.ADD.f16 v24 v29 v24
	VAU.MUL.i16 v21 v21 v0 || lsu0.swzv8 [00000000] || 	CMU.CPVI.x16   i25.l v28.0   
	VAU.MUL.i16 v27 v27 v0 || lsu0.swzv8 [11111111] || 	CMU.CPVI.x16   i25.h v28.1 
	VAU.MUL.f16 v24 v24 v26 || lsu0.swzv8 [00000000]
	VAU.MUL.f16 v23 v23 v26 || lsu0.swzv8 [11111111]
	VAU.MUL.f16 v20 v20 v26 || lsu0.swzv8 [22222222]
	VAU.MUL.i16 v29 v3 v0 || lsu0.swzv8 [22222222] || 	CMU.CPVI.x16   i26.l v28.2	
	CMU.CPVI.x16   i26.h v28.3
	VAU.ADD.f16 v24 v23 v24
	nop 2
	VAU.ADD.f16 v20 v20 v24
	nop 
	PEU.PC1C LT  || BRU.JMP i22
	VAU.MUL.f16 v20 v20 v25
	nop 2
	CMU.CLAMP0.F16 v20 v20 v22
	LSU0.ST128.f16.u8 v20 i14 || IAU.ADD i14 i14 8

	;reseting line pointers
	IAU.SUB i10 i10 i21
	IAU.SUB i11 i11 i21
	IAU.SUB i12 i12 i21
	IAU.SUB i13 i13 i21
	IAU.SUB i14 i14 i21

	IAU.ADD i16 i16 4

	LSU0.ldil i1, ___sobel_loop || LSU1.ldih i1, ___sobel_loop || IAU.SUB i21 i21 i21
	
	IAU.SUB i19 i19 i16
	IAU.SUB i19 i19 i16
	IAU.SUB i19 i19 0x10
	IAU.SUB i2 i19 0 ;address for gradient
	IAU.SUB i19 i19 i16
	IAU.SUB i19 i19 i16
	IAU.SUB i19 i19 0x10
	IAU.SUB i3 i19 0 ;address for arctan
	IAU.SUB i19 i19 i16
	IAU.SUB i19 i19 i16
	IAU.SUB i19 i19 0x10
	IAU.SUB i4 i19 0 ;address for arctan
	IAU.SUB i19 i19 i16
	IAU.SUB i19 i19 i16
	IAU.SUB i19 i19 0x10
	IAU.SUB i5 i19 0 ;address for arctan
	IAU.SUB i19 i19 i16
	IAU.SUB i19 i19 i16
	IAU.SUB i19 i19 0x10
	IAU.SUB i6 i19 0 ;address for arctan
	IAU.SUB i19 i19 i16
	IAU.SUB i19 i19 i16
	IAU.SUB i19 i19 0x10
	IAU.SUB i7 i19 0 ;address for arctan
	
	IAU.SUB i16 i16 6
	
	LSU0.LD128.u8.f16  v1 i10 || IAU.ADD i10 i10 8
	LSU0.LD128.u8.f16  v2 i11 || IAU.ADD i11 i11 8 || 	LSU1.LD128.u8.f16 v11 i10 
	LSU0.LD128.u8.f16  v3 i12 || IAU.ADD i12 i12 8 || 	LSU1.LD128.u8.f16 v12 i11 
	LSU0.LD128.u8.f16  v4 i13 || IAU.ADD i13 i13 8 || 	LSU1.LD128.u8.f16 v13 i12 
	LSU0.LD128.u8.f16  v5 i14 || IAU.ADD i14 i14 8 || 	LSU1.LD128.u8.f16 v14 i13 
	LSU1.LD128.u8.f16 v15 i14 
	nop 
	
	;computation for first line
	VAU.ALIGNVEC v6 v1 v11 2
    VAU.ALIGNVEC v7 v1 v11 4
    VAU.ALIGNVEC v8 v2 v12 2 
    VAU.ALIGNVEC v9 v2 v12 4
    VAU.ALIGNVEC v10 v3 v13 2
    VAU.ALIGNVEC v20 v3 v13 4
	VAU.ALIGNVEC v21 v4 v14 2
    VAU.ALIGNVEC v22 v4 v14 4
	VAU.ALIGNVEC v23 v5 v15 2
    VAU.ALIGNVEC v24 v5 v15 4
	
		;***********************first line
	;gx first line
	VAU.sub.f16 v0 v1 v7 
	VAU.SUB.f16 v16 v3 v20
	VAU.add.f16 v17 v2 v2
	VAU.add.f16 v18 v9 v9
	VAU.sub.f16 v19 v1 v3
	VAU.SUB.f16 v25 v7 v20
	VAU.ADD.f16 v16 v0 v16

	VAU.SUB.f16 v17 v17 v18
	VAU.ADD.f16 v26 v6 v6
	VAU.ADD.f16 v27 v10 v10
	VAU.ADD.f16 v17 v16 v17
	VAU.sub.f16 v0 v2 v9 
	VAU.ADD.f16 v25 v19 v25
	VAU.SUB.f16 v26 v26 v27 || CMU.cpvs.x16 s12.l v17.0
	CMU.cpvs.x16 s12.h v17.1
	CMU.cpvs.x16 s13.l v17.2				
	VAU.ADD.f16 v25 v25 v26 || CMU.cpvs.x16 s13.h v17.3
	CMU.cpvs.x16 s14.l v17.4
	CMU.cpvs.x16 s14.h v17.5
	VAU.MUL.f16 v1 v17 v17  || CMU.cpvs.x16 s15.l v17.6 
	VAU.MUL.f16 v28 v25 v25 || CMU.cpvs.x16 s15.h v17.7
	___sobel_loop:
	
	CMU.cpvs.x16 s16.l v25.0 || VAU.SUB.f16 v16 v4 v22
	CMU.cpvs.x16 s16.h v25.1 || VAU.add.f16 v17 v3 v3
	CMU.cpvs.x16 s17.l v25.2 || SAU.DIV.f16  s16  s12 s16 || 	VAU.add.f16 v18 v20 v20
	CMU.cpvs.x16 s17.h v25.3 || 	VAU.ADD.f16 v28 v1 v28;*****************
	LSU0.LD128.u8.f16  v1 i10 || IAU.ADD i10 i10 8 || CMU.cpvs.x16 s18.l v25.4 || SAU.DIV.f16  s17  s13 s17
	CMU.cpvs.x16 s18.h v25.5 || 	VAU.ADD.f16 v16 v0 v16
	CMU.cpvs.x16 s19.l v25.6 || SAU.DIV.f16  s18  s14 s18 || 	VAU.SUB.f16 v17 v17 v18
	CMU.cpvs.x16 s19.h v25.7
	
	SAU.DIV.f16  s19  s15 s19 || CMU.cpvs.x16 s0.l v28.0
	VAU.ADD.f16 v17 v16 v17 || CMU.cpvs.x16 s0.h v28.1
	
	CMU.CPSV.x16  v27.0  s16.l || SAU.sqt.f16.l s0, s0
	CMU.CPSV.x16  v27.1  s16.h 
	CMU.CPSV.x16  v27.2  s17.l || SAU.sqt.f16.h s0, s0
	CMU.CPSV.x16  v27.3  s17.h
	CMU.CPSV.x16  v27.4  s18.l
	CMU.CPSV.x16  v27.5  s18.h || VAU.sub.f16 v19 v2 v4
	CMU.CPSV.x16  v27.6  s19.l || VAU.SUB.f16 v25 v9 v22
	CMU.CPSV.x16  v27.7  s19.h || VAU.ADD.f16 v26 v8 v8
	CMU.cpvs.x16 s20.l v17.0
	LSU0.ST64.l  v27 i5 || IAU.ADD i5 i5 8   || CMU.cpvs.x16 s20.h v17.1   
	LSU0.ST64.h  v27 i5 || IAU.ADD i5 i5 8   || CMU.cpvs.x16 s21.l v17.2
;***********************second line	

	VAU.ADD.f16 v27 v21 v21 || CMU.cpvs.x16 s21.h v17.3
	CMU.cpvs.x16 s24.l v17.4
	CMU.cpvs.x16 s24.h v17.5
	VAU.ADD.f16 v25 v19 v25 || CMU.cpvs.x16 s25.l v17.6
	VAU.SUB.f16 v26 v26 v27 || CMU.cpvs.x16 s25.h v17.7
	VAU.sub.f16 v0 v3 v20 || CMU.cpvs.x16 s1.l v28.2
	VAU.SUB.f16 v16 v5 v24 || CMU.cpvs.x16 s1.h v28.3
	VAU.ADD.f16 v25 v25 v26 || CMU.cpvs.x16 s2.l v28.4 || SAU.sqt.f16.l s1, s1
	CMU.cpvs.x16 s2.h v28.5 || SAU.sqt.f16.h s1, s1
	CMU.cpvs.x16 s3.l v28.6	|| SAU.sqt.f16.l s2, s2
	VAU.MUL.f16 v2 v17 v17  || CMU.cpvs.x16 s26.l v25.0 || SAU.sqt.f16.h s2, s2
	VAU.MUL.f16 v29 v25 v25 || CMU.cpvs.x16 s26.h v25.1 || SAU.sqt.f16.l s3, s3
	
	CMU.cpvs.x16 s27.l v25.2 || 	SAU.DIV.f16  s26  s20 s26 || 	VAU.add.f16 v17 v4 v4
	CMU.cpvs.x16 s27.h v25.3 || 	VAU.add.f16 v18 v22 v22
	CMU.cpvs.x16 s28.l v25.4 || 	SAU.DIV.f16  s27  s21 s27
	CMU.cpvs.x16 s28.h v25.5 || VAU.ADD.f16 v29 v2 v29
	LSU0.LD128.u8.f16  v2 i11 || IAU.ADD i11 i11 8 || 	LSU1.LD128.u8.f16 v11 i10 || CMU.cpvs.x16 s29.l v25.6 || 	SAU.DIV.f16  s28  s24 s28 || 	VAU.ADD.f16 v16 v0 v16
	CMU.cpvs.x16 s29.h v25.7 || 	VAU.SUB.f16 v17 v17 v18

	SAU.DIV.f16  s29  s25 s29 || CMU.cpvs.x16 s3.h v28.7
	CMU.cpvs.x16 s4.l v29.0
	
	CMU.CPSV.x16  v27.0  s26.l || 	VAU.ADD.f16 v17 v16 v17 || SAU.sqt.f16.h s3, s3
	CMU.CPSV.x16  v27.1  s26.h || 	VAU.sub.f16 v19 v3 v5
	CMU.CPSV.x16  v27.2  s27.l || VAU.SUB.f16 v25 v20 v24
	CMU.CPSV.x16  v27.3  s27.h || VAU.ADD.f16 v26 v10 v10
	CMU.CPSV.x16  v27.4  s28.l
	CMU.CPSV.x16  v27.5  s28.h
	CMU.CPSV.x16  v27.6  s29.l
	CMU.CPSV.x16  v27.7  s29.h
	CMU.cpvs.x16 s12.l v17.0
	LSU0.ST64.l  v27 i6 || IAU.ADD i6 i6 8   || CMU.cpvs.x16 s12.h v17.1    
	LSU0.ST64.h  v27 i6 || IAU.ADD i6 i6 8   || CMU.cpvs.x16 s13.l v17.2

	VAU.ADD.f16 v27 v23 v23 || 	CMU.cpvs.x16 s13.h v17.3
	CMU.cpvs.x16 s14.l v17.4
	CMU.cpvs.x16 s14.h v17.5
	VAU.ADD.f16 v25 v19 v25  || CMU.cpvs.x16 s15.l v17.6
	VAU.SUB.f16 v26 v26 v27  || CMU.cpvs.x16 s15.h v17.7
	CMU.CPSV.x16  v28.0  s0.l
	CMU.CPSV.x16  v28.1  s0.h
	
	VAU.ADD.f16 v25 v25 v26 || CMU.CPSV.x16  v28.2  s1.l
	CMU.CPSV.x16  v28.3  s1.h
	CMU.CPSV.x16  v28.4  s2.l
	
	VAU.MUL.f16 v3 v17 v17  || CMU.cpvs.x16 s16.l v25.0
	VAU.MUL.f16 v30 v25 v25 || CMU.cpvs.x16 s16.h v25.1
	

	CMU.cpvs.x16 s17.l v25.2 || 	SAU.DIV.f16  s16  s12 s16
	CMU.cpvs.x16 s17.h v25.3
	CMU.cpvs.x16 s18.l v25.4 || 	SAU.DIV.f16  s17  s13 s17
	CMU.cpvs.x16 s18.h v25.5 || VAU.ADD.f16 v30 v3 v30
	LSU0.LD128.u8.f16  v3 i12 || IAU.ADD i12 i12 8 || 	LSU1.LD128.u8.f16 v12 i11  || CMU.cpvs.x16 s19.l v25.6 || 	SAU.DIV.f16  s18  s14 s18
	LSU0.LD128.u8.f16  v4 i13 || IAU.ADD i13 i13 8 || 	LSU1.LD128.u8.f16 v13 i12 ||CMU.cpvs.x16 s19.h v25.7
		
	LSU0.LD128.u8.f16  v5 i14 || IAU.ADD i14 i14 8 || 	LSU1.LD128.u8.f16 v14 i13 || SAU.DIV.f16  s19  s15 s19 || CMU.CPSV.x16  v28.5  s2.h
	LSU1.LD128.u8.f16 v15 i14 || CMU.CPSV.x16  v28.6  s3.l
	
	CMU.CPSV.x16  v27.0  s16.l  
	CMU.CPSV.x16  v27.1  s16.h || VAU.ALIGNVEC v6 v1 v11 2
	CMU.CPSV.x16  v27.2  s17.l || VAU.ALIGNVEC v7 v1 v11 4
	CMU.CPSV.x16  v27.3  s17.h || VAU.ALIGNVEC v8 v2 v12 2 
	CMU.CPSV.x16  v27.4  s18.l || VAU.ALIGNVEC v9 v2 v12 4
	CMU.CPSV.x16  v27.5  s18.h || VAU.ALIGNVEC v10 v3 v13 2
	CMU.CPSV.x16  v27.6  s19.l || VAU.ALIGNVEC v20 v3 v13 4
	CMU.CPSV.x16  v27.7  s19.h || VAU.ALIGNVEC v21 v4 v14 2
	CMU.CPSV.x16  v28.7  s3.h  || VAU.ALIGNVEC v22 v4 v14 4
	LSU0.ST64.l  v27 i7 || IAU.ADD i7 i7 8   || CMU.cpvs.x16 s4.h v29.1   || 	VAU.ALIGNVEC v23 v5 v15 2
	LSU0.ST64.h  v27 i7 || IAU.ADD i7 i7 8   || CMU.cpvs.x16 s5.l v29.2 || 	SAU.sqt.f16.l s4, s4 ||     VAU.ALIGNVEC v24 v5 v15 4


	VAU.sub.f16 v0 v1 v7   || LSU0.ST64.l  v28 i2 || IAU.ADD i2 i2 8 || CMU.cpvs.x16 s5.h v29.3 || 	SAU.sqt.f16.h s4, s4
	VAU.SUB.f16 v16 v3 v20 || LSU0.ST64.h  v28 i2 || IAU.ADD i2 i2 8 || CMU.cpvs.x16 s6.l v29.4 || SAU.sqt.f16.l s5, s5
	VAU.add.f16 v17 v2 v2  || CMU.cpvs.x16 s6.h v29.5 || SAU.sqt.f16.h s5, s5
	VAU.add.f16 v18 v9 v9  || CMU.cpvs.x16 s7.l v29.6 || SAU.sqt.f16.l s6, s6
	VAU.sub.f16 v19 v1 v3  || CMU.cpvs.x16 s7.h v29.7 || SAU.sqt.f16.h s6, s6
	VAU.SUB.f16 v25 v7 v20 || SAU.sqt.f16.l s7, s7 || CMU.CPSV.x16  v29.0  s4.l
	VAU.ADD.f16 v16 v0 v16 || SAU.sqt.f16.h s7, s7 || CMU.CPSV.x16  v29.1  s4.h

	VAU.SUB.f16 v17 v17 v18 || CMU.CPSV.x16  v29.2  s5.l
	VAU.ADD.f16 v26 v6 v6   || CMU.CPSV.x16  v29.3  s5.h
	VAU.ADD.f16 v27 v10 v10 || CMU.CPSV.x16  v29.4  s6.l
	VAU.ADD.f16 v17 v16 v17 || CMU.CPSV.x16  v29.5  s6.h
	VAU.sub.f16 v0 v2 v9    || CMU.CPSV.x16  v29.6  s7.l
	VAU.ADD.f16 v25 v19 v25 || CMU.CPSV.x16  v29.7  s7.h

	
	;third line
	LSU0.ST64.l  v29 i3 || IAU.ADD i3 i3 8 || CMU.cpvs.x16 s8.l  v30.0
	LSU0.ST64.h  v29 i3 || IAU.ADD i3 i3 8 || CMU.cpvs.x16 s8.h  v30.1 || SAU.sqt.f16.l s8, s8
	CMU.cpvs.x16 s9.l  v30.2 || SAU.sqt.f16.h s8, s8
	CMU.cpvs.x16 s9.h  v30.3 || SAU.sqt.f16.l s9, s9
	CMU.cpvs.x16 s10.l v30.4 || SAU.sqt.f16.h s9, s9
	CMU.cpvs.x16 s10.h v30.5 || SAU.sqt.f16.l s10, s10
	CMU.cpvs.x16 s11.l v30.6 || SAU.sqt.f16.h s10, s10
	CMU.cpvs.x16 s11.h v30.7 || SAU.sqt.f16.l s11, s11
	
	SAU.sqt.f16.h s11, s11 || 	CMU.CPSV.x16  v30.0  s8.l




	CMU.CPSV.x16  v30.1  s8.h
	CMU.CPSV.x16  v30.2  s9.l
	CMU.CPSV.x16  v30.3  s9.h
	CMU.CPSV.x16  v30.4  s10.l
	CMU.CPSV.x16  v30.5  s10.h
	CMU.CPSV.x16  v30.6  s11.l
	CMU.CPSV.x16  v30.7  s11.h

	   
	LSU0.ST64.l  v30 i4 || IAU.ADD i4 i4 8    || VAU.SUB.f16 v26 v26 v27 || CMU.cpvs.x16 s12.l v17.0  
	LSU0.ST64.h  v30 i4 || IAU.ADD i4 i4 8    || CMU.cpvs.x16 s12.h v17.1
	
	IAU.ADD i21 i21 8 || 	CMU.cpvs.x16 s13.l v17.2
	CMU.CMII.i32 i21 i16 
	PEU.PC1C LT  || BRU.JMP i1
	VAU.ADD.f16 v25 v25 v26 || CMU.cpvs.x16 s13.h v17.3
	CMU.cpvs.x16 s14.l v17.4
	CMU.cpvs.x16 s14.h v17.5
	VAU.MUL.f16 v1 v17 v17  || CMU.cpvs.x16 s15.l v17.6 
	VAU.MUL.f16 v28 v25 v25 || CMU.cpvs.x16 s15.h v17.7
	;*************end sobel loop   
	
	IAU.SUB i2 i2 i21
	IAU.SUB i3 i3 i21
	IAU.SUB i4 i4 i21
	IAU.SUB i5 i5 i21
	IAU.SUB i6 i6 i21
	IAU.SUB i7 i7 i21
	IAU.SUB i2 i2 i21
	IAU.SUB i3 i3 i21
	IAU.SUB i4 i4 i21
	IAU.SUB i5 i5 i21
	IAU.SUB i6 i6 i21
	IAU.SUB i7 i7 i21
	
	IAU.ADD i16 i16 6
	
	;non maxim supression step
	
	LSU0.ldil i1, ___tangent || LSU1.ldih i1, ___tangent || IAU.SUB i21 i21 i21
	LSU0.LD16  s1 i1 || IAU.ADD i1 i1 2 ;1
	LSU0.LD16  s2 i1 || IAU.ADD i1 i1 2 ;2
	LSU0.LD16  s3 i1 || IAU.ADD i1 i1 2 ;3
	LSU0.LD16  s4 i1 || IAU.ADD i1 i1 2 ;4
	LSU0.ldil i1, ___supression || LSU1.ldih i1, ___supression 
	IAU.SUB i16 i16 8
	IAU.ADD i6 i6 2
	
	

	LSU0.LDo16          s5  i2 0  
	LSU0.LDo16          s6  i2 2 
	LSU0.LDo16          s7  i2 4 || LSU1.LDo16          s10  i3 4 || bru.rpl i1 i16;first line elem
	LSU0.LDo16          s8  i3 0 || LSU1.LDo16          s11  i4 0
	LSU0.LDo16          s9  i3 2 || LSU1.LDo16          s12  i4 2
	;second line elem
	LSU0.LDo16          s13  i4 4|| LSU1.LDi16          s15  i6;third line elem
	 ;tangent elem
	nop 2
	IAU.add i2 i2 2
	IAU.add i3 i3 2
	IAU.add i4 i4 2
	
	;first comparison
	CMU.CMSS.f16  s15 s4
	PEU.ANDACC  || CMU.CMSS.f16  s1 s15
	PEU.ANDACC  || CMU.CMSS.f16  s6 s9
	PEU.PC1C GTE || SAU.SUB.f16  s9  s9 s9

	CMU.CMSS.f16  s15 s4 || LSU0.LDo16          s6  i2 2 
	PEU.ANDACC  || CMU.CMSS.f16  s1 s15
	PEU.ANDACC  || CMU.CMSS.f16  s12 s9
	PEU.PC1C GTE || SAU.SUB.f16  s9  s9 s9

	;second comparison
	CMU.CMSS.f16  s15 s1
	PEU.ANDACC  || CMU.CMSS.f16  s2 s15
	PEU.ANDACC  || CMU.CMSS.f16  s7 s9
	PEU.PC1C GTE || SAU.SUB.f16  s9  s9 s9

	CMU.CMSS.f16  s15 s1
	PEU.ANDACC  || CMU.CMSS.f16  s2 s15
	PEU.ANDACC  || CMU.CMSS.f16  s11 s9
	PEU.PC1C GTE || SAU.SUB.f16  s9  s9 s9

	;third comparison
	CMU.CMSS.f16  s15 s2
	PEU.ANDACC  || CMU.CMSS.f16  s10 s9
	PEU.PC1C GTE || SAU.SUB.f16  s9  s9 s9

	CMU.CMSS.f16  s15 s2
	PEU.ANDACC  || CMU.CMSS.f16  s8 s9
	PEU.PC1C GTE || SAU.SUB.f16  s9  s9 s9

	CMU.CMSS.f16  s3 s15 
	PEU.ANDACC  || CMU.CMSS.f16  s10 s9
	PEU.PC1C GTE || SAU.SUB.f16  s9  s9 s9

	CMU.CMSS.f16  s3 s15 
	PEU.ANDACC  || CMU.CMSS.f16  s8 s9
	PEU.PC1C GTE || SAU.SUB.f16  s9  s9 s9

	;fourth comparison
	CMU.CMSS.f16  s15 s3
	PEU.ANDACC  || CMU.CMSS.f16  s4 s15
	PEU.ANDACC  || CMU.CMSS.f16  s5 s9
	PEU.PC1C GTE || SAU.SUB.f16  s9  s9 s9

	CMU.CMSS.f16  s15 s3 || LSU0.LDo16          s5  i2 0  
	___supression:
	PEU.ANDACC  || CMU.CMSS.f16  s4 s15
	PEU.ANDACC  || CMU.CMSS.f16  s13 s9
	PEU.PC1C GTE || SAU.SUB.f16  s9  s9 s9
	IAU.ADD i21 i21 1
	CMU.CMII.i32 i21 i16 
	LSU0.ST16          s9 i3


	IAU.SUB i2 i2 i21
	IAU.SUB i3 i3 i21
	IAU.SUB i4 i4 i21
	IAU.SUB i2 i2 i21
	IAU.SUB i3 i3 i21
	IAU.SUB i4 i4 i21
	
	
	LSU0.ldil i1, ___final || LSU1.ldih i1, ___final || IAU.SUB i21 i21 i21
	
	LSU0.ldil i28, 0xff || LSU1.ldih i28, 0x0
	LSU0.ldil i29, 0x0  || LSU1.ldih i29, 0x0
	

	IAU.SUB i10 i10 i10
	lsu0.sti8 i10 i17
	lsu0.sti8 i10 i17
	lsu0.sti8 i10 i17
	lsu0.sti8 i10 i17
	;Edge tracking by hysteresis

	LSU0.LDo16          s5  i2  0 
	LSU0.LDo16          s6  i2  2
	LSU0.LDo16          s7  i2  4;first line elem
	LSU0.LDo16          s8  i3  0 
	LSU0.LDo16          s9  i3  2
	LSU0.LDo16          s10  i3 4;second line elem
	LSU0.LDo16          s11  i4 0 || bru.rpl i1 i16 
	LSU0.LDo16          s12  i4 2
	LSU0.LDo16          s13  i4 4;third line elem

	IAU.add i2 i2 2
	IAU.add i3 i3 2
	IAU.add i4 i4 2
	
	CMU.CMSS.f16  s9 s23
	peu.pc1c lt || lsu0.st8 i29 i17
	
	CMU.CMSS.f16  s9 s22
	peu.pc1c gt || lsu0.st8 i28 i17
	
	CMU.CMSS.f16  s22 s9
	PEU.ANDACC  || CMU.CMSS.f16  s9 s23
	PEU.ANDACC  || CMU.CMSS.f16  s5 s22
	peu.pc1c gte || lsu0.st8 i28 i17
	peu.pc1c lt || lsu0.st8 i29 i17
	
	CMU.CMSS.f16  s22 s9 || LSU0.LDo16          s5  i2  0 
	PEU.ANDACC  || CMU.CMSS.f16  s9 s23
	PEU.ANDACC  || CMU.CMSS.f16  s6 s22
	peu.pc1c gte || lsu0.st8 i28 i17
	peu.pc1c lt || lsu0.st8 i29 i17
	
	CMU.CMSS.f16  s22 s9 || LSU0.LDo16          s6  i2  2
	PEU.ANDACC  || CMU.CMSS.f16  s9 s23
	PEU.ANDACC  || CMU.CMSS.f16  s7 s22
	peu.pc1c gte || lsu0.st8 i28 i17
	peu.pc1c lt || lsu0.st8 i29 i17
	
	CMU.CMSS.f16  s22 s9 || LSU0.LDo16          s7  i2  4
	PEU.ANDACC  || CMU.CMSS.f16  s9 s23
	PEU.ANDACC  || CMU.CMSS.f16  s8 s22
	peu.pc1c gte || lsu0.st8 i28 i17
	peu.pc1c lt || lsu0.st8 i29 i17
	
	CMU.CMSS.f16  s22 s9 || LSU0.LDo16          s8  i3  0 
	PEU.ANDACC  || CMU.CMSS.f16  s9 s23
	PEU.ANDACC  || CMU.CMSS.f16  s10 s22
	peu.pc1c gte || lsu0.st8 i28 i17
	peu.pc1c lt || lsu0.st8 i29 i17
	
	CMU.CMSS.f16  s22 s9 || LSU0.LDo16          s10  i3 4
	PEU.ANDACC  || CMU.CMSS.f16  s9 s23
	PEU.ANDACC  || CMU.CMSS.f16  s11 s22
	peu.pc1c gte || lsu0.st8 i28 i17
	peu.pc1c lt || lsu0.st8 i29 i17
	
	CMU.CMSS.f16  s22 s9
	PEU.ANDACC  || CMU.CMSS.f16  s9 s23
	PEU.ANDACC  || CMU.CMSS.f16  s12 s22
	peu.pc1c gte || lsu0.st8 i28 i17
	peu.pc1c lt || lsu0.st8 i29 i17
___final:
	CMU.CMSS.f16  s22 s9
	PEU.ANDACC  || CMU.CMSS.f16  s9 s23
	PEU.ANDACC  || CMU.CMSS.f16  s13 s22
	peu.pc1c gte || lsu0.st8 i28 i17
	peu.pc1c lt || lsu0.st8 i29 i17
	IAU.ADD i17 i17 1 || LSU0.LDo16          s9  i3  2

	lsu0.sti8 i10 i17
	lsu0.sti8 i10 i17
	lsu0.sti8 i10 i17
	lsu0.sti8 i10 i17
	

	
	IAU.ADD i16 i16 8
	IAU.ADD i19 i19 i16
	IAU.ADD i19 i19 i16
	IAU.ADD i19 i19 i16
	IAU.ADD i19 i19 i16
	IAU.ADD i19 i19 0x20
	IAU.ADD i19 i19 i16
	IAU.ADD i19 i19 i16
	IAU.ADD i19 i19 i16
	IAU.ADD i19 i19 i16
	IAU.ADD i19 i19 0x20
	IAU.ADD i19 i19 i16
	IAU.ADD i19 i19 i16
	IAU.ADD i19 i19 i16
	IAU.ADD i19 i19 i16
	IAU.ADD i19 i19 0x20
	IAU.ADD i19 i19 i16
	IAU.ADD i19 i19 i16
	IAU.ADD i19 i19 i16
	IAU.ADD i19 i19 i16
	IAU.ADD i19 i19 i16
	IAU.ADD i19 i19 0x28
	nop
	
	LSU0.LD32  i29  i19 || IAU.ADD i19 i19 4
	LSU0.LD32  i28  i19 || IAU.ADD i19 i19 4	
	LSU0.LD32  i27  i19 || IAU.ADD i19 i19 4
	LSU0.LD32  i26  i19 || IAU.ADD i19 i19 4
	LSU0.LD32  i25  i19 || IAU.ADD i19 i19 4
	LSU0.LD32  i24  i19 || IAU.ADD i19 i19 4
	LSU0.LD32  i23  i19 || IAU.ADD i19 i19 4
	LSU0.LD32  i22  i19 || IAU.ADD i19 i19 4
	LSU0.LD32  i21  i19 || IAU.ADD i19 i19 4
	LSU0.LD32  i20  i19 || IAU.ADD i19 i19 4
	
	LSU0.LD32  s31  i19 || IAU.ADD i19 i19 4
	LSU0.LD32  s30  i19 || IAU.ADD i19 i19 4	
	LSU0.LD32  s29  i19 || IAU.ADD i19 i19 4
	LSU0.LD32  s28  i19 || IAU.ADD i19 i19 4
	LSU0.LD32  s27  i19 || IAU.ADD i19 i19 4
	LSU0.LD32  s26  i19 || IAU.ADD i19 i19 4
	LSU0.LD32  s25  i19 || IAU.ADD i19 i19 4
	LSU0.LD32  s24  i19 || IAU.ADD i19 i19 4

	
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


	bru.jmp i30
	nop 5
.end