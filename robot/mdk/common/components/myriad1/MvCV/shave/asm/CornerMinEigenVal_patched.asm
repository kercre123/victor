; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief
; ///

;--------------------------------------------------------------------------------------------------------------
.version 00.50.05
.data .rodata.CornerMinEigenVal_patched 
.salign 16
	___val:
		.float32 0.1111111111F32 , 0.5f32, 255, 1.5


.code .text.CornerMinEigenVal_patched ;text
.salign 16

CornerMinEigenVal_patched_asm:
;void CornerMinEigenVal_patched(u8 **in_lines, u32 posy, u8 *out_pix, u32 width);
;                                  i18            i17        i16         i15
;CMU.CPTI    i14 P_CFG
;lsu0.ldil i13, 0x0006     ||  lsu1.ldih i13, 0x0000
;IAU.OR i14 i14 i13
;CMU.CPIT   P_CFG i14
lsu0.ldil i11, 0x59df     ||  lsu1.ldih i11, 0x5f37
lsu0.ldil i1, 0x1        ||  lsu1.ldih i1, 0x0
cmu.cpis.i32.f32 s19 i1
lsu0.ldil i12, ___skip     ||  lsu1.ldih i12, ___skip
	lsu0.ldil i7, ___val     ||  lsu1.ldih i7, ___val
	LSU0.LD32 s0 i7 || IAU.ADD i7 i7 4
	LSU0.LD32 s10 i7 || IAU.ADD i7 i7 4
	LSU0.LD32 s12 i7 || IAU.ADD i7 i7 4
	LSU0.LD32 s15 i7 
	
	;computing line pointers
	IAU.SUB i6 i17 2
	LSU0.LD32  i1  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i2  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i3  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i4  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i5  i18 || IAU.ADD i18 i18 4
	nop 
	IAU.ADD i1 i1 i6
	IAU.ADD i2 i2 i6
	IAU.ADD i3 i3 i6
	IAU.ADD i4 i4 i6
	IAU.ADD i5 i5 i6
	
	
	;loading values
	LSU0.LD128.u8.f16 v1 i1
	LSU0.LD128.u8.f16 v2 i2
	LSU0.LD128.u8.f16 v3 i3
	LSU0.LD128.u8.f16 v4 i4
	LSU0.LD128.u8.f16 v5 i5
	nop 
	cmu.vrot v6   v1  2
	cmu.vrot v7   v6  2
	cmu.vrot v8   v2  2 || 	VAU.SUB.f16 v16 v1 v7
	cmu.vrot v9   v8  2 || 	VAU.ADD.f16 v17 v2 v2
	cmu.vrot v10  v3  2 || 	VAU.ADD.f16 v18 v9 v9
	cmu.vrot v11  v10 2 || 	VAU.SUB.f16 v20 v1 v3
	cmu.vrot v12  v4  2 || 	VAU.ADD.f16 v21 v6 v6 
	cmu.vrot v13  v12 2 || 	VAU.SUB.f16 v18 v17 v18
	cmu.vrot v14  v5  2 || 	VAU.SUB.f16 v19 v3  v11
	cmu.vrot v15  v14 2 || 	VAU.ADD.f16 v22 v10 v10 


	;dx-first

	VAU.ADD.f16 v16 v16 v18
	VAU.SUB.f16 v23 v7  v11
	VAU.SUB.f16 v22 v21 v22
	VAU.ADD.f16 v19 v16 v19
	VAU.SUB.f16 v16 v2 v9
	VAU.ADD.f16 v20 v20 v22
	VAU.ADD.f16 v17 v3 v3
	VAU.ADD.f16 v18 v11 v11
	VAU.ADD.f16 v23 v23 v20


	;dx-second


	VAU.SUB.f16 v20 v2 v4
	VAU.ADD.f16 v21 v8 v8 
	VAU.SUB.f16 v18 v17 v18
	VAU.SUB.f16 v0  v4  v13
	VAU.ADD.f16 v22 v12 v12 
	VAU.ADD.f16 v16 v16 v18
	nop 
	VAU.SUB.f16 v22 v21 v22
	VAU.SUB.f16 v1  v9  v13	
	VAU.ADD.f16 v0 v16 v0
	VAU.ADD.f16 v20 v20 v22
	VAU.SUB.f16 v16 v3 v11
	VAU.ADD.f16 v17 v4 v4
	VAU.ADD.f16 v1 v1 v20
	
	
	;dx-third

	VAU.ADD.f16 v18 v13 v13
	VAU.SUB.f16 v20 v3 v5
	VAU.ADD.f16 v21 v10 v10
	VAU.SUB.f16 v18 v17 v18
	VAU.SUB.f16 v2  v5  v15
	VAU.ADD.f16 v22 v14 v14 
	VAU.ADD.f16 v16 v16 v18 || 	CMU.CPVV.f16l.f32  v0  v0 
	CMU.CPVV.f16l.f32  v19 v19    
	VAU.SUB.f16 v22 v21 v22
	VAU.SUB.f16 v4  v11  v15
	VAU.ADD.f16 v2 v16 v2  
	VAU.ADD.f16 v20 v20 v22
	CMU.CPVV.f16l.f32  v23 v23
	CMU.CPVV.f16l.f32  v2  v2 
	VAU.ADD.f16 v4 v4 v20
	VAU.mul.f32 v3 v19 v19		
	CMU.CPVV.f16l.f32  v1  v1 || VAU.mul.f32 v5 v0  v0
	CMU.CPVV.f16l.f32  v4  v4 || VAU.mul.f32 v7 v2  v2
	


	;dxdx


	;dydy
	VAU.mul.f32 v6  v23 v23
	VAU.mul.f32 v8  v1  v1
	VAU.mul.f32 v10 v4  v4
	

	;dxdy
	VAU.mul.f32 v11 v19 v23
	VAU.mul.f32 v12 v0  v1
	VAU.mul.f32 v13 v2  v4
	
	
	VAU.ADD.f32 v1 v3 v5
	VAU.ADD.f32 v2 v6 v8

	VAU.ADD.f32 v4 v11 v12
	VAU.ADD.f32 v1 v1 v7
	VAU.ADD.f32 v2 v2 v10
	VAU.ADD.f32 v4 v4 v13

	SAU.SUM.f32 s1 v1 7
	SAU.SUM.f32 s2 v2 7
	SAU.SUM.f32 s3 v4 7
	nop 2

	SAU.MUL.f32 s1 s1 s0
	SAU.MUL.f32 s2 s2 s0
	SAU.MUL.f32 s3 s3 s0;b

	SAU.MUL.f32 s1 s1 s10;a
	SAU.MUL.f32 s2 s2 s10;c
	nop 2

	SAU.ADD.f32 s4 s1 s2;a+c
	nop 2
	SAU.SUB.f32 s5 s1 s2;a-c
	nop 2
	SAU.MUL.f32 s6 s3 s3;b*b
	nop 2

	SAU.mul.f32 s7 s5 s5;(a-c)*(a-c)
	nop 2
	SAU.add.f32 s8 s7 s6 ;(a - c)*(a - c) + b*b)
	nop 2

	CMU.CMSS.f32   s8 s19
	peu.pc1c lt || cmu.cpss	s18 s8 || bru.jmp i12
	nop 5

	SAU.mul.f32 s11 s8 s10 ;number * 0.5
	nop 2
	cmu.cpsi i10 s8
	nop
	IAU.SHR.u32 i10 i10 1

	IAU.SUB i10 i11 i10
	cmu.cpis s13 i10 ;y
	nop

	sau.mul.f32 s16 s13 s13
	nop 2

	sau.mul.f32 s16 s16 s11
	nop 2

	sau.sub.f32 s16 s15 s16
	nop 2

	sau.mul.f32 s16 s16 s13
	nop 2
		
	sau.mul.f32 s17 s16 s16
	nop 2

	
	sau.mul.f32 s17 s17 s11
	nop 2
	sau.sub.f32 s17 s15 s17
	nop 2
	sau.mul.f32 s17 s17 s16
	nop 2
	
	sau.mul.f32 s18 s17 s8
	nop 2
___skip:
	sau.sub.f32 s18 s4 s18
	nop 2
	CMU.CLAMP0.f32 s18 s18 s12
	nop
	cmu.cpsi.f32.i32s i1 s18
	nop

	lsu0.st8 i1 i16
	nop
	
	bru.jmp i30
	nop 5
	
	
.end