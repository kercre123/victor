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
IAU.SUB i19 i19 4
LSU0.ST.32  i20  i19 || IAU.SUB i19 i19 4
LSU0.ST.32  i21  i19 || IAU.SUB i19 i19 4
LSU0.ST.32  i22  i19 || IAU.SUB i19 i19 4
LSU0.ST.32  i23  i19 || IAU.SUB i19 i19 4
LSU0.ST.32  i24  i19 || IAU.SUB i19 i19 4
LSU0.ST.32  i25  i19 || IAU.SUB i19 i19 4
LSU0.ST.32  i26  i19 || IAU.SUB i19 i19 4
LSU0.ST.32  i27  i19 || IAU.SUB i19 i19 4
LSU0.ST.32  i28  i19 || IAU.SUB i19 i19 4
LSU0.ST.32  i29  i19 || IAU.SUB i19 i19 4
LSU0.ST.32  i30  i19 || IAU.SUB i19 i19 4
LSU0.ST.32  i31  i19 
;void CornerMinEigenVal_patched(u8 **in_lines, u32 posy, u8 *out_pix, u32 width);
;                                  i18            i17        i16         i15
;CMU.CPTI    i14 P_CFG
;lsu0.ldil i13, 0x0006     ||  lsu1.ldih i13, 0x0000
;IAU.OR i14 i14 i13
;CMU.CPIT   P_CFG i14
lsu0.ldil i11, 0x59df     ||  lsu1.ldih i11, 0x5f37
lsu0.ldil i1, 0x1        ||  lsu1.ldih i1, 0x0
cmu.cpii.i32.f32 i9 i1
lsu0.ldil i12, ___skip     ||  lsu1.ldih i12, ___skip
	lsu0.ldil i7, ___val     ||  lsu1.ldih i7, ___val
	LSU0.LD.32 i0 i7 || IAU.ADD i7 i7 4
	LSU0.LD.32 i9 i7 || IAU.ADD i7 i7 4
	LSU0.LD.32 i13 i7 || IAU.ADD i7 i7 4
	LSU0.LD.32 i14 i7 
	
	;computing line pointers
	IAU.SUB i6 i17 2
	LSU0.LD.32  i1  i18 || IAU.ADD i18 i18 4
	LSU0.LD.32  i2  i18 || IAU.ADD i18 i18 4
	LSU0.LD.32  i3  i18 || IAU.ADD i18 i18 4
	LSU0.LD.32  i4  i18 || IAU.ADD i18 i18 4
	LSU0.LD.32  i5  i18 || IAU.ADD i18 i18 4
nop 2
	IAU.ADD i1 i1 i6
	IAU.ADD i2 i2 i6
	IAU.ADD i3 i3 i6
	IAU.ADD i4 i4 i6
	IAU.ADD i5 i5 i6
	
	
	;loading values
	LSU0.LD.128.U8.F16 v1 i1
	LSU0.LD.128.U8.F16 v2 i2
	LSU0.LD.128.U8.F16 v3 i3
	LSU0.LD.128.U8.F16 v4 i4
	LSU0.LD.128.U8.F16 v5 i5
nop 2
	CMU.ALIGNVEC v6   V1, v1  2
	CMU.ALIGNVEC v7   V6, v6  2
	CMU.ALIGNVEC v8   v2  v2  2 || 	VAU.SUB.f16 v16 v1 V7
	CMU.ALIGNVEC v9   v8  v8  2 || 	VAU.ADD.f16 v17 v2 V2
	CMU.ALIGNVEC v10  v3  v3  2 || 	VAU.ADD.f16 v18 v9 V9
	CMU.ALIGNVEC v11  v10 v10 2 || 	VAU.SUB.f16 v20 v1 V3
	CMU.ALIGNVEC v12  v4  v4  2 || 	VAU.ADD.f16 v21 v6 V6
	CMU.ALIGNVEC v13  v12 v12 2 || 	VAU.SUB.f16 v18 v17 V18
	CMU.ALIGNVEC v14  v5  v5  2 || 	VAU.SUB.f16 v19 v3  V11
	CMU.ALIGNVEC v15  v14 v14 2 || 	VAU.ADD.f16 v22 v10 V10


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

	SAU.SUMx.f32 i15 v1 || LSU0.SWZMS4.word [3210] [ZUUU] 
	SAU.SUMx.f32 i20 v2 || LSU0.SWZMS4.word [3210] [ZUUU]
	SAU.SUMx.f32 i21 v4 || LSU0.SWZMS4.word [3210] [ZUUU]
nop 2

	SAU.MUL.f32 i15 i15 i0
	SAU.MUL.f32 i20 i20 i0
	SAU.MUL.f32 i21 i21 i0;b

	SAU.MUL.f32 i15 i15 i9;a
	SAU.MUL.f32 i20 i20 i9;c
nop 2

	SAU.ADD.f32 i22 i15 i20;a+c
nop 2
	SAU.SUB.f32 i23 i15 i20;a-c
nop 2
	SAU.MUL.f32 i24 i21 i21;b*b
nop 2

	SAU.mul.f32 i25 i23 i23;(a-c)*(a-c)
nop 2
	SAU.add.f32 i26 i25 i24 ;(a - c)*(a - c) + b*b)
nop 2

	CMU.cmii.f32   i26 i9
	peu.pc1c lt || cmu.cpII	i27 i26 || bru.jmp i12
nop 5

	SAU.mul.f32 i28 i26 i9 ;number * 0.5
nop 2
	cmu.cpii i10 i26
nop
	IAU.SHR.u32 i10 i10 1

	IAU.SUB i10 i11 i10
	cmu.cpii i29 i10 ;y
nop

	sau.mul.f32 i30 i29 i29
nop 2

	sau.mul.f32 i30 i30 i28
nop 2

	sau.sub.f32 i30 i14 i30
nop 2

	sau.mul.f32 i30 i30 i29
nop 2
		
	sau.mul.f32 i31 i30 i30
nop 2

	sau.mul.f32 i31 i31 i28
nop 2
	sau.sub.f32 i31 i14 i31
nop 2
	sau.mul.f32 i31 i31 i30
nop 2
	
	sau.mul.f32 i27 i31 i26
nop 2
___skip:
	sau.sub.f32 i27 i22 i27
nop 2
	CMU.CLAMP0.f32 i27 i27 i13
nop
	cmu.cpii.f32.i32s i1 i27
nop

	LSU0.ST.8 i1 i16
nop
LSU0.LD.32  i31  i19 || IAU.ADD i19 i19 4
LSU0.LD.32  i30  i19 || IAU.ADD i19 i19 4
LSU0.LD.32  i29  i19 || IAU.ADD i19 i19 4
LSU0.LD.32  i28  i19 || IAU.ADD i19 i19 4	
LSU0.LD.32  i27  i19 || IAU.ADD i19 i19 4
LSU0.LD.32  i26  i19 || IAU.ADD i19 i19 4
LSU0.LD.32  i25  i19 || IAU.ADD i19 i19 4
LSU0.LD.32  i24  i19 || IAU.ADD i19 i19 4
LSU0.LD.32  i23  i19 || IAU.ADD i19 i19 4
LSU0.LD.32  i22  i19 || IAU.ADD i19 i19 4
LSU0.LD.32  i21  i19 || IAU.ADD i19 i19 4
LSU0.LD.32  i20  i19 || IAU.ADD i19 i19 4
nop 6	
	bru.jmp i30
nop 6
	
	
.end
