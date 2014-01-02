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
	LSU0.LD32  i1  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i2  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i3  i18 || IAU.ADD i18 i18 4
	LSU1.LD32  i17  i17
	nop 5
    lsu0.ldil i4, ___clampVal     ||  lsu1.ldih i4, ___clampVal
	lsu0.ld16r v23, i4
	LSU0.ldil i5, ___loop || LSU1.ldih i5, ___loop
	

	;load elements
	LSU0.LD128.u8.f16 v0 i1  || IAU.ADD i1 i1 8   
    LSU0.LD128.u8.f16 v1 i2  || IAU.ADD i2 i2 8   || LSU1.LD128.u8.f16 v3 i1
    LSU0.LD128.u8.f16 v2 i3  || IAU.ADD i3 i3 8   || LSU1.LD128.u8.f16 v4 i2
    LSU0.LD128.u8.f16 v5 i3
    nop 3
	VAU.ALIGNVEC v6 v0 v3 2 
    VAU.ALIGNVEC v7 v0 v3 4 
    VAU.ALIGNVEC v8 v1 v4 2 
	VAU.ALIGNVEC v9 v1 v4 4 
    VAU.ALIGNVEC v10 v2 v5 2
    VAU.ALIGNVEC v11 v2 v5 4
	
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
	LSU0.LD128.u8.f16 v0 i1  || IAU.ADD i1 i1 8   || bru.rpl i5 i15   
	VAU.ADD.f16 v21 v12 v14    || LSU0.LD128.u8.f16 v1 i2  || IAU.ADD i2 i2 8   || LSU1.LD128.u8.f16 v3 i1
	VAU.MUL.f16 v19 v19 v19    || LSU0.LD128.u8.f16 v2 i3  || IAU.ADD i3 i3 8   || LSU1.LD128.u8.f16 v4 i2
	LSU0.LD128.u8.f16 v5 i3
	VAU.ADD.f16 v21 v21 v20
	nop 2
	VAU.MUL.f16 v21 v21 v21
	nop 2
	VAU.ADD.f16 v21 v19 v21
	nop 2
	CMU.cpvs.x16 s6.l v21.0 || 	VAU.ALIGNVEC v6 v0 v3 2 
	CMU.cpvs.x16 s6.h v21.1 || VAU.ALIGNVEC v7 v0 v3 4 
	CMU.cpvs.x16 s7.l v21.2 || 	SAU.sqt.f16.l s6, s6 || VAU.ALIGNVEC v8 v1 v4 2 
	CMU.cpvs.x16 s7.h v21.3 || 	SAU.sqt.f16.h s6, s6 || VAU.ALIGNVEC v9 v1 v4 4 
	CMU.cpvs.x16 s8.l v21.4 || SAU.sqt.f16.l s7, s7  || VAU.ALIGNVEC v10 v2 v5 2
	CMU.cpvs.x16 s8.h v21.5 || SAU.sqt.f16.h s7, s7  || VAU.ALIGNVEC v11 v2 v5 4
	CMU.cpvs.x16 s9.l v21.6 || SAU.sqt.f16.l s8, s8 || VAU.sub.f16 v15 v0 v7 
	CMU.cpvs.x16 s9.h v21.7 || SAU.sqt.f16.h s8, s8 || VAU.SUB.f16 v18 v2 v11
	SAU.sqt.f16.l s9, s9 || CMU.cpsv.x16  v21.0 s6.l || VAU.ADD.f16 v16 v1 v1
	SAU.sqt.f16.h s9, s9 || CMU.cpsv.x16  v21.1 s6.h || VAU.ADD.f16 v17 v9 v9
	CMU.cpsv.x16  v21.2 s7.l || VAU.sub.f16 v12 v0 v2
	CMU.cpsv.x16  v21.3 s7.h || VAU.sub.f16 v20 v7 v11   
	CMU.cpsv.x16  v21.4 s8.l || VAU.SUB.f16 v17 v16 v17  
	___loop:                     
	CMU.cpsv.x16  v21.5 s8.h ||  VAU.ADD.f16 v13 v6 v6   
	CMU.cpsv.x16  v21.6 s9.l ||  VAU.ADD.f16 v14 v10 v10 
	CMU.cpsv.x16  v21.7 s9.h ||  VAU.ADD.f16 v19 v15 v17
	CMU.CLAMP0.F16 v21 v21 v23
	VAU.sub.f16 v14 v13 v14      
	LSU0.ST128.f16.u8 v21 i17 || IAU.ADD i17 i17 8 || 	VAU.ADD.f16 v19 v19 v18 
	LSU0.ldil i5, ___final || LSU1.ldih i5, ___final
	
	;veryfing if there are some unprocessed pixel
	cmu.cmii.i32 i13 i12
	peu.pc1c eq || bru.jmp i5
	nop 5
;if they exist making place on the stack for them
	IAU.SUB i19 i19 8
	IAU.SUB i9 i19 0
	
	LSU0.LD128.u8.f16 v0 i1  || IAU.ADD i1 i1 8   
    LSU0.LD128.u8.f16 v1 i2  || IAU.ADD i2 i2 8   || LSU1.LD128.u8.f16 v3 i1
    LSU0.LD128.u8.f16 v2 i3  || IAU.ADD i3 i3 8   || LSU1.LD128.u8.f16 v4 i2
    LSU0.LD128.u8.f16 v5 i3
    nop 3
	VAU.ALIGNVEC v6 v0 v3 2
    VAU.ALIGNVEC v7 v0 v3 4
    VAU.ALIGNVEC v8 v1 v4 2 
	VAU.ALIGNVEC v9 v1 v4 4
    VAU.ALIGNVEC v10 v2 v5 2
    VAU.ALIGNVEC v11 v2 v5 4
	
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
	CMU.cpvs.x16 s6.l v21.0
	CMU.cpvs.x16 s6.h v21.1
	CMU.cpvs.x16 s7.l v21.2 || SAU.sqt.f16.l s6, s6
	CMU.cpvs.x16 s7.h v21.3 || SAU.sqt.f16.h s6, s6
	CMU.cpvs.x16 s8.l v21.4 || SAU.sqt.f16.l s7, s7
	CMU.cpvs.x16 s8.h v21.5 || SAU.sqt.f16.h s7, s7
	CMU.cpvs.x16 s9.l v21.6 || SAU.sqt.f16.l s8, s8
	CMU.cpvs.x16 s9.h v21.7 || SAU.sqt.f16.h s8, s8
	SAU.sqt.f16.l s9, s9 || CMU.cpsv.x16  v21.0 s6.l
	SAU.sqt.f16.h s9, s9 || CMU.cpsv.x16  v21.1 s6.h
	CMU.cpsv.x16  v21.2 s7.l
	CMU.cpsv.x16  v21.3 s7.h
	CMU.cpsv.x16  v21.4 s8.l
	CMU.cpsv.x16  v21.5 s8.h
	CMU.cpsv.x16  v21.6 s9.l
	CMU.cpsv.x16  v21.7 s9.h
	CMU.CLAMP0.F16 v21 v21 v23
	nop
	LSU0.ST128.f16.u8 v21 i9 || IAU.ADD i9 i9 8
	LSU0.ldil i5, ___compensation || LSU1.ldih i5, ___compensation || IAU.SUB i6 i6 i6
	
___compensation:
	LSU0.LDI8 i11 i9
	nop 2
	IAU.ADD i6 i6 1
	cmu.cmii.i32 i6 i13
	peu.pc1c lt || bru.jmp i5
	LSU0.STI8 i11 i17
	nop 4
	

	IAU.ADD i19 i19 8
___final:	
	
	bru.jmp i30
	nop 5

	
	
.end
