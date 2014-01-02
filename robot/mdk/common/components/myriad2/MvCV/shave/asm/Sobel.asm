; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief
; ///

.version 00.50.05
.data .rodata.sobel
.salign 16
___clampVal:
        .float16 255.0
.code .text.sobel ;text
.salign 16

sobel_asm:
;void sobel(u8** in, u8** out, u32 width)
;             i18      i17        i16        

;see how much is width mod 8
IAU.SHR.u32 i15 i16 1
IAU.SHR.u32 i15 i15 1
IAU.SHR.u32 i15 i15 1
IAU.MUL i14 i15 8
nop
IAU.SUB i13 i16 i14
IAU.SUB i12 i12 i12

;load line pointers
	LSU0.LD.32  i1  i18 || IAU.ADD i18 i18 4
	LSU0.LD.32  i2  i18 || IAU.ADD i18 i18 4
	LSU0.LD.32  i3  i18 || IAU.ADD i18 i18 4
	LSU1.LD.32  i17  i17
nop 5
    lsu0.ldil i4, ___clampVal     ||  lsu1.ldih i4, ___clampVal
	LSU0.LD.16R v23, i4
	LSU0.ldil i5, ___loop || LSU1.ldih i5, ___loop
	

	;load elements
	LSU0.LD.128.U8.F16 v0 i1  || IAU.ADD i1 i1 8   
    LSU0.LD.128.U8.F16 v1 i2  || IAU.ADD i2 i2 8   || LSU1.LD.128.U8.F16 v3 i1
    LSU0.LD.128.U8.F16 v2 i3  || IAU.ADD i3 i3 8   || LSU1.LD.128.U8.F16 v4 i2
    LSU0.LD.128.U8.F16 v5 i3
nop 4
	CMU.ALIGNVEC v6 v0 v3 2 
    CMU.ALIGNVEC v7 v0 v3 4 
    CMU.ALIGNVEC v8 v1 v4 2 
	CMU.ALIGNVEC v9 v1 v4 4 
    CMU.ALIGNVEC v10 v2 v5 2
    CMU.ALIGNVEC v11 v2 v5 4
	
	;processing elements
	VAU.sub.f16 v15 v0 v7 
	VAU.SUB.f16 v18 v2 v11
	VAU.ADD.f16 v16 v1 v1 
	VAU.ADD.f16 v17 v9 v9
	VAU.sub.f16 v12 v0 v2 
	VAU.sub.f16 v20 v7 v11      
	VAU.SUB.f16 v17 v16 v17     
	VAU.ADD.f16 v13 v6 v6       
	VAU.ADD.f16 v14 v10 v10     
	VAU.ADD.f16 v19 v15 v17
nop
	VAU.sub.f16 v14 v13 v14      
	VAU.ADD.f16 v19 v19 v18      
	LSU0.LD.128.U8.F16 v0 i1  || IAU.ADD i1 i1 8   || bru.rpl i5 i15   
	VAU.ADD.f16 v21 v12 v14    || LSU0.LD.128.U8.F16 v1 i2  || IAU.ADD i2 i2 8   || LSU1.LD.128.U8.F16 v3 i1
	VAU.MUL.f16 v19 v19 v19    || LSU0.LD.128.U8.F16 v2 i3  || IAU.ADD i3 i3 8   || LSU1.LD.128.U8.F16 v4 i2
	LSU0.LD.128.U8.F16 v5 i3
	VAU.ADD.f16 v21 v21 v20
nop 2
	VAU.MUL.f16 v21 v21 v21
nop 2
	VAU.ADD.f16 v21 v19 v21
nop 2
	CMU.ALIGNVEC v6 v0 v3 2 
	CMU.ALIGNVEC v7 v0 v3 4
	CMU.ALIGNVEC v8 v1 v4 2
	CMU.ALIGNVEC v9 v1 v4 4
	CMU.ALIGNVEC v10 v2 v5 2
	CMU.ALIGNVEC v11 v2 v5 4
	
	CMU.cpvi.x16 i0.0 v21.0
	CMU.cpvi.x16 i0.1 v21.1 
	CMU.cpvi.x16 i7.0 v21.2 
	CMU.cpvi.x16 i7.1 v21.3 || SAU.SQT I0, I0.0
	CMU.cpvi.x16 i8.0 v21.4 || SAU.SQT I0, I0.1
	CMU.cpvi.x16 i8.1 v21.5 || SAU.SQT I7, I7.0
	CMU.cpvi.x16 i10.0 v21.6 || SAU.SQT I7, I7.1 || VAU.sub.f16 v15 v0 v7 
	CMU.cpvi.x16 i10.1 v21.7 || SAU.SQT I8, I8.0 || VAU.SUB.f16 v18 v2 v11
	CMU.CPIV.x16  v21.0 i0.0 || SAU.SQT I8, I8.1 || VAU.ADD.f16 v16 v1 v1
	CMU.CPIV.x16  v21.1 i0.0 || SAU.SQT I10, I10.0 || VAU.ADD.f16 v17 v9 v9
	CMU.CPIV.x16  v21.2 i7.0 || SAU.SQT I10, I10.1 || VAU.sub.f16 v12 v0 v2
	CMU.CPIV.x16  v21.3 i7.0 || VAU.sub.f16 v20 v7 v11
	CMU.CPIV.x16  v21.4 i8.0 || VAU.SUB.f16 v17 v16 v17
	
___loop:
	CMU.CPIV.x16  v21.5 i8.0 ||  VAU.ADD.f16 v13 v6 v6  
	CMU.CPIV.x16  v21.6 i10.0 ||  VAU.ADD.f16 v14 v10 v10
	CMU.CPIV.x16  v21.7 i10.0 ||  VAU.ADD.f16 v19 v15 v17
	CMU.CLAMP0.F16 v21 v21 v23
	VAU.sub.f16 v14 v13 v14      
	LSU0.ST.128.F16.U8 v21 i17 || IAU.ADD i17 i17 8 || 	VAU.ADD.f16 v19 v19 v18 
	nop
	LSU0.ldil i5, ___final || LSU1.ldih i5, ___final
	
	;veryfing if there are some unprocessed pixel
	cmu.cmii.i32 i13 i12
	peu.pc1c eq || bru.jmp i5
nop 6
;if they exist making place on the stack for them
	IAU.SUB i19 i19 8
	IAU.SUB i9 i19 0
	
	LSU0.LD.128.U8.F16 v0 i1  || IAU.ADD i1 i1 8   
    LSU0.LD.128.U8.F16 v1 i2  || IAU.ADD i2 i2 8   || LSU1.LD.128.U8.F16 v3 i1
    LSU0.LD.128.U8.F16 v2 i3  || IAU.ADD i3 i3 8   || LSU1.LD.128.U8.F16 v4 i2
    LSU0.LD.128.U8.F16 v5 i3
nop 4
	CMU.ALIGNVEC v6 v0 v3 2
    CMU.ALIGNVEC v7 v0 v3 4
    CMU.ALIGNVEC v8 v1 v4 2 
	CMU.ALIGNVEC v9 v1 v4 4
    CMU.ALIGNVEC v10 v2 v5 2
    CMU.ALIGNVEC v11 v2 v5 4
	
	VAU.sub.f16 v15 v0 v7
	VAU.SUB.f16 v18 v2 v11
	VAU.ADD.f16 v16 v1 v1
	VAU.ADD.f16 v17 v9 v9
	VAU.sub.f16 v12 v0 v2
	VAU.sub.f16 v20 v7 v11
	VAU.SUB.f16 v17 v16 v17
	VAU.ADD.f16 v13 v6 v6
	VAU.ADD.f16 v14 v10 v10
	VAU.ADD.f16 v19 v15 v17
nop
	VAU.sub.f16 v14 v13 v14
	VAU.ADD.f16 v19 v19 v18
nop
	VAU.ADD.f16 v21 v12 v14
	VAU.MUL.f16 v19 v19 v19
nop
	VAU.ADD.f16 v21 v21 v20
nop 2
	VAU.MUL.f16 v21 v21 v21
nop 2
	VAU.ADD.f16 v21 v19 v21
nop 2

	CMU.cpvi.x16 i0.0 v21.0
	CMU.cpvi.x16 i0.1 v21.1
	CMU.cpvi.x16 i7.0 v21.2  
	CMU.cpvi.x16 i7.1 v21.3 || SAU.SQT I0, I0.0  
	CMU.cpvi.x16 i8.0 v21.4 || SAU.SQT I0, I0.1 
	CMU.cpvi.x16 i8.1 v21.5 || SAU.SQT I7, I7.0
	CMU.cpvi.x16 i10.0 v21.6 || SAU.SQT I7, I7.1
	CMU.cpvi.x16 i10.1 v21.7 || SAU.SQT I8, I8.0 
	CMU.CPIV.x16  v21.0 i0.0 || SAU.SQT I8, I8.1 
	CMU.CPIV.x16  v21.1 i0.0 || SAU.SQT I10, I10.0
	CMU.CPIV.x16  v21.2 i7.0 || SAU.SQT I10, I10.1
	CMU.CPIV.x16  v21.3 i7.0
	CMU.CPIV.x16  v21.4 i8.0
	CMU.CPIV.x16  v21.5 i8.0
	CMU.CPIV.x16  v21.6 i10.0
	CMU.CPIV.x16  v21.7 i10.0
	CMU.CLAMP0.F16 v21 v21 v23
nop
	LSU0.ST.128.F16.U8 v21 i9 || IAU.ADD i9 i9 8
	LSU0.ldil i5, ___compensation || LSU1.ldih i5, ___compensation || IAU.SUB i6 i6 i6
	
___compensation:
	LSU0.LDI.8 i11 i9
nop 3
	IAU.ADD i6 i6 1
	cmu.cmii.i32 i6 i13
	peu.pc1c lt || bru.jmp i5
	LSU0.STI.8 i11 i17
nop 5
	

	IAU.ADD i19 i19 8
___final:
	
	bru.jmp i30
nop 6

	
	
.end
