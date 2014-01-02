; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief   
; ///

.version 00.51.05
;-------------------------------------------------------------------------------
.data .rodata.sumOfAbsDiff5x5
.salign 16


___clampVal:
    .float16 255.0
	
.code .text.sumOfAbsDiff5x5
;void sumOfAbsDiff5x5(u8** in1(i18), u8** in2(i17), u8** out(i16), u32 width(i15))
sumOfAbsDiff5x5_asm:

	LSU0.LDIL i10, ___clampVal 		|| LSU1.LDIH i10, ___clampVal
	LSU0.LDIL i11, ___loop || LSU1.LDIH i11, ___loop || IAU.SHR.u32 i12 i15 3
	IAU.SUB i19 i19 8   || LSU0.LD32 i16 i16
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
	
	LSU1.LD32 i0 i18  	|| IAU.ADD i18 i18 4	 
	LSU0.LD32 i1 i17 	|| IAU.ADD i17 i17 4
	LSU0.LD32 i2 i18 	|| IAU.ADD i18 i18 4
	LSU0.LD32 i3 i17 	|| IAU.ADD i17 i17 4	
	LSU0.LD32 i4 i18 	|| IAU.ADD i18 i18 4
	LSU0.LD32 i5 i17 	|| IAU.ADD i17 i17 4
	LSU0.LD32 i6 i18 	|| IAU.ADD i18 i18 4
	LSU0.LD32 i7 i17 	|| IAU.ADD i17 i17 4	
	LSU0.LD32 i8 i18 	|| LSU1.LD16r v31, i10 
	LSU1.LD32 i9 i17 	
	
	LSU0.LD128.u8.u16 v25 i0 	|| IAU.ADD i0 i0 1  	|| LSU1.LD128.u8.u16 v26 i1 	|| SAU.ADD.i32 i1 i1 1 	 	
	LSU0.LD128.u8.u16 v27 i2 	|| IAU.ADD i2 i2 1  	|| LSU1.LD128.u8.u16 v28 i3 	|| SAU.ADD.i32 i3 i3 1
	LSU0.LD128.u8.u16 v29 i4 	|| IAU.ADD i4 i4 1  	|| LSU1.LD128.u8.u16 v30 i5 	|| SAU.ADD.i32 i5 i5 1
	LSU0.LD128.u8.u16 v25 i6 	|| IAU.ADD i6 i6 1  	|| LSU1.LD128.u8.u16 v26 i7 	|| SAU.ADD.i32 i7 i7 1
	
	.lalign
	VAU.XOR v1 v1 v1 || BRU.RPL i11 i12
	LSU0.LD128.u8.u16 v27 i8 	|| IAU.ADD i8 i8 1  	|| LSU1.LD128.u8.u16 v28 i9 	|| SAU.ADD.i32 i9 i9 1	
	LSU0.LD128.u8.u16 v29 i0 	|| IAU.ADD i0 i0 1  	|| LSU1.LD128.u8.u16 v30 i1	    || SAU.ADD.i32 i1 i1 1
	LSU0.LD128.u8.u16 v25 i2 	|| IAU.ADD i2 i2 1  	|| LSU1.LD128.u8.u16 v26 i3	    || SAU.ADD.i32 i3 i3 1 	|| VAU.ADIFF.u16 v0 v25 v26
	LSU0.LD128.u8.u16 v27 i4 	|| IAU.ADD i4 i4 1  	|| LSU1.LD128.u8.u16 v28 i5	    || SAU.ADD.i32 i5 i5 1 	|| VAU.ADIFF.u16 v1 v27 v28
	LSU0.LD128.u8.u16 v29 i6 	|| IAU.ADD i6 i6 1  	|| LSU1.LD128.u8.u16 v30 i7	    || SAU.ADD.i32 i7 i7 1 	|| VAU.ADIFF.u16 v2 v29 v30
	LSU0.LD128.u8.u16 v25 i8 	|| IAU.ADD i8 i8 1  	|| LSU1.LD128.u8.u16 v26 i9 	|| SAU.ADD.i32 i9 i9 1 	|| VAU.ADIFF.u16 v3 v25 v26
	LSU0.LD128.u8.u16 v27 i0 	|| IAU.ADD i0 i0 1  	|| LSU1.LD128.u8.u16 v28 i1 	|| SAU.ADD.i32 i1 i1 1	|| VAU.ADIFF.u16 v4 v27 v28
	LSU0.LD128.u8.u16 v29 i2 	|| IAU.ADD i2 i2 1  	|| LSU1.LD128.u8.u16 v30 i3 	|| SAU.ADD.i32 i3 i3 1	|| VAU.ADIFF.u16 v5 v29 v30
	LSU0.LD128.u8.u16 v25 i4 	|| IAU.ADD i4 i4 1  	|| LSU1.LD128.u8.u16 v26 i5 	|| SAU.ADD.i32 i5 i5 1	|| VAU.ADIFF.u16 v6 v25 v26
	LSU0.LD128.u8.u16 v27 i6 	|| IAU.ADD i6 i6 1  	|| LSU1.LD128.u8.u16 v28 i7 	|| SAU.ADD.i32 i7 i7 1	|| VAU.ADIFF.u16 v7 v27 v28
	LSU0.LD128.u8.u16 v29 i8 	|| IAU.ADD i8 i8 1  	|| LSU1.LD128.u8.u16 v30 i9 	|| SAU.ADD.i32 i9 i9 1	|| VAU.ADIFF.u16 v8 v29 v30
	LSU0.LD128.u8.u16 v25 i0 	|| IAU.ADD i0 i0 1  	|| LSU1.LD128.u8.u16 v26 i1 	|| SAU.ADD.i32 i1 i1 1	|| VAU.ADIFF.u16 v9 v25 v26
	LSU0.LD128.u8.u16 v27 i2 	|| IAU.ADD i2 i2 1  	|| LSU1.LD128.u8.u16 v28 i3 	|| SAU.ADD.i32 i3 i3 1	|| VAU.ADIFF.u16 v10 v27 v28
	LSU0.LD128.u8.u16 v29 i4 	|| IAU.ADD i4 i4 1  	|| LSU1.LD128.u8.u16 v30 i5 	|| SAU.ADD.i32 i5 i5 1	|| VAU.ADIFF.u16 v11 v29 v30	
	LSU0.LD128.u8.u16 v25 i6 	|| IAU.ADD i6 i6 1  	|| LSU1.LD128.u8.u16 v26 i7 	|| SAU.ADD.i32 i7 i7 1	|| VAU.ADIFF.u16 v12 v25 v26
	LSU0.LD128.u8.u16 v27 i8 	|| IAU.ADD i8 i8 1  	|| LSU1.LD128.u8.u16 v28 i9 	|| SAU.ADD.i32 i9 i9 1	|| VAU.ADIFF.u16 v13 v27 v28
	LSU0.LD128.u8.u16 v29 i0 	|| IAU.ADD i0 i0 4	 	|| LSU1.LD128.u8.u16 v30 i1 	|| SAU.ADD.i32 i1 i1 4	|| VAU.ADIFF.u16 v14 v29 v30	
	LSU0.LD128.u8.u16 v25 i2 	|| IAU.ADD i2 i2 4	 	|| LSU1.LD128.u8.u16 v26 i3 	|| SAU.ADD.i32 i3 i3 4	|| VAU.ADIFF.u16 v15 v25 v26
	LSU0.LD128.u8.u16 v27 i4 	|| IAU.ADD i4 i4 4	 	|| LSU1.LD128.u8.u16 v28 i5 	|| SAU.ADD.i32 i5 i5 4	|| VAU.ADIFF.u16 v16 v27 v28
	LSU0.LD128.u8.u16 v29 i6 	|| IAU.ADD i6 i6 4	 	|| LSU1.LD128.u8.u16 v30 i7 	|| SAU.ADD.i32 i7 i7 4	|| VAU.ADIFF.u16 v17 v29 v30	
	LSU0.LD128.u8.u16 v25 i8 	|| IAU.ADD i8 i8 4	 	|| LSU1.LD128.u8.u16 v26 i9 	|| SAU.ADD.i32 i9 i9 4	|| VAU.ADIFF.u16 v18 v25 v26
	VAU.ADIFF.u16 v19 v27 v28
	VAU.ADIFF.u16 v20 v29 v30	
	VAU.ADIFF.u16 v21 v25 v26
	VAU.ADIFF.u16 v22 v27 v28
	VAU.ADIFF.u16 v23 v29 v30	
	VAU.ADIFF.u16 v24 v25 v26
	
	VAU.ACCPZ.u16 v0
	VAU.ACCP.u16 v1
	VAU.ACCP.u16 v2
	VAU.ACCP.u16 v3
	VAU.ACCP.u16 v4
	VAU.ACCP.u16 v5
	VAU.ACCP.u16 v6
	VAU.ACCP.u16 v7
	VAU.ACCP.u16 v8	
	VAU.ACCP.u16 v9		
	VAU.ACCP.u16 v10 	
	VAU.ACCP.u16 v11	
	VAU.ACCP.u16 v12			
	VAU.ACCP.u16 v13 	 
	VAU.ACCP.u16 v14		 
	VAU.ACCP.u16 v15	
	VAU.ACCP.u16 v16		
	VAU.ACCP.u16 v17   	
	VAU.ACCP.u16 v18   	
	VAU.ACCP.u16 v19   	
	VAU.ACCP.u16 v20 || LSU0.LD128.u8.u16 v25 i0 	|| IAU.ADD i0 i0 1  	|| LSU1.LD128.u8.u16 v26 i1 	|| SAU.ADD.i32 i1 i1 1	
	VAU.ACCP.u16 v21 || LSU0.LD128.u8.u16 v27 i2 	|| IAU.ADD i2 i2 1  	|| LSU1.LD128.u8.u16 v28 i3 	|| SAU.ADD.i32 i3 i3 1
	VAU.ACCP.u16 v22 ||	LSU0.LD128.u8.u16 v29 i4 	|| IAU.ADD i4 i4 1  	|| LSU1.LD128.u8.u16 v30 i5 	|| SAU.ADD.i32 i5 i5 1 
	
	___loop:
	VAU.ACCP.u16 v23  	 	
	VAU.ACCPW.u16 v0 v24
	NOP
	CMU.CLAMP0.f16 v0 v0 v31
	LSU0.LD128.u8.u16 v25 i6 || IAU.ADD i6 i6 1 || LSU1.LD128.u8.u16 v26 i7 || SAU.ADD.i32 i7 i7 1
	LSU0.STI128.u16.u8 v0 i16

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