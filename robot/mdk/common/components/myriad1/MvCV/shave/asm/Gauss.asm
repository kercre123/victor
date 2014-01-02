; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief
; ///

.version 00.50.05

.set StackPtr	 	i19

.ifndef SVU_COMMON_MACROS
.set SVU_COMMON_MACROS

;stack manipulation macros

.macro PUSH_V_REG VREG
lsu0.sto64.h \VREG,StackPtr,-8 || lsu1.sto64.l \VREG,StackPtr,-16 || iau.incs StackPtr,-16
.endm

.macro POP_V_REG VREG 
lsu0.ldo64.l \VREG,StackPtr,0 || lsu1.ldo64.h \VREG, StackPtr, 8 || iau.incs StackPtr,16
.endm

.macro PUSH_2_32BIT_REG REG1, REG2
lsu0.sto32 \REG1,StackPtr,-4 || lsu1.sto32 \REG2,StackPtr,-8 || iau.incs StackPtr,-8
.endm

.macro POP_2_32BIT_REG REG1, REG2
lsu0.ldo32 \REG2,StackPtr,0 || lsu1.ldo32 \REG1,StackPtr,4 || iau.incs StackPtr,8
.endm

.macro PUSH_1_32BIT_REG REG
lsu0.sto32 \REG,StackPtr,-4 || iau.incs StackPtr,-4
.endm

.macro POP_1_32BIT_REG REG
lsu0.ldo32 \REG,StackPtr,0 || iau.incs StackPtr,4
.endm

.endif

.data .rodata.gauss 
.salign 16
	___vect:
		.float16    1.4F16, 3.0F16, 3.8F16, 1.4F16, 3.0F16, 3.8F16, 0, 0
	___multiply:
		.float16    0.0062893F16
	___clampVal:
        .float16 255.0


.code .text.gauss ;text
.salign 16

gauss_asm:
;void gauss(u8** inLine, u8** out, u32 width);
;                 i18      i17        i16      
PUSH_V_REG v24 
PUSH_V_REG v25 
IAU.SUB i19 i19 i16
IAU.SUB i10 i19 0
;preparation
	LSU0.ldil i1, ___vect || LSU1.ldih i1, ___vect
	lsu0.ldo64.l v21 i1 0       || lsu1.ldo64.h v21 i1 8
	lsu0.ldil i2, ___clampVal   || lsu1.ldih i2, ___clampVal
	lsu0.ld16r v22, i2
	lsu0.ldil i3, ___multiply   || lsu1.ldih i3, ___multiply
	lsu0.ld16r v23, i3
	LSU0.LD32 i1 i18            || IAU.ADD i18 i18 4
	LSU0.LD32 i2 i18            || IAU.ADD i18 i18 4
	LSU0.LD32 i3 i18            || IAU.ADD i18 i18 4
	LSU0.LD32 i4 i18            || IAU.ADD i18 i18 4
	LSU0.LD32 i5 i18            || IAU.ADD i18 i18 4
	LSU0.LD32 i17 i17
	nop 5
	IAU.SHR.u32  i15 i16 3
	IAU.SHL      i14 i15 3
	IAU.SUB      i13 i16 i14

	;processing
	lsu0.ldil i8, ___loop   || lsu1.ldih i8, ___loop
	LSU0.LD128.u8.f16 v1 i1     || IAU.ADD i1 i1 8 
	LSU0.LD128.u8.f16 v2 i2     || IAU.ADD i2 i2 8  
	LSU0.LD128.u8.f16 v3 i3     || IAU.ADD i3 i3 8 
	LSU0.LD128.u8.f16 v4 i4     || IAU.ADD i4 i4 8  
	LSU0.LD128.u8.f16 v5 i5     || IAU.ADD i5 i5 8  
	LSU1.LD128.u8.f16 v6 i1   
	LSU1.LD128.u8.f16 v7 i2
	LSU1.LD128.u8.f16 v8 i3  
	LSU1.LD128.u8.f16 v9 i4   
	LSU0.LD128.u8.f16 v10 i5 
	VAU.ADD.f16 v11 v1 v5 
	VAU.ADD.f16 v12 v2 v4
	VAU.MUl.f16 v13 v3 v21 || lsu0.swzv8 [22222222] 
	VAU.MUl.f16 v11 v11 v21 || lsu0.swzv8 [00000000]
	VAU.MUl.f16 v12 v12 v21 || lsu0.swzv8 [11111111] 
	VAU.ADD.f16 v14 v6 v10
	VAU.ADD.f16 v15 v7 v9 
	VAU.ADD.f16 v12 v11 v12
	VAU.MUl.f16 v16 v8 v21  || lsu0.swzv8 [22222222]
	VAU.MUl.f16 v14 v14 v21 || lsu0.swzv8 [00000000] 
	VAU.ADD.f16 v13 v12 v13 || LSU0.LD128.u8.f16 v1 i1     || IAU.ADD i1 i1 8 
	VAU.MUl.f16 v15 v15 v21 || lsu0.swzv8 [11111111] || LSU1.LD128.u8.f16 v2 i2     || IAU.ADD i2 i2 8  || bru.rpl i8 i15
	LSU0.LD128.u8.f16 v3 i3     || IAU.ADD i3 i3 8 
	LSU0.LD128.u8.f16 v4 i4     || IAU.ADD i4 i4 8  
	VAU.ADD.f16 v15 v14 v15 || LSU0.LD128.u8.f16 v5 i5     || IAU.ADD i5 i5 8  
	LSU1.LD128.u8.f16 v6 i1   
	LSU1.LD128.u8.f16 v7 i2
	VAU.ADD.f16 v16 v15 v16 || 	LSU1.LD128.u8.f16 v8 i3  
	LSU1.LD128.u8.f16 v9 i4   
	LSU0.LD128.u8.f16 v10 i5 
    VAU.ALIGNVEC v17 v13 v16 2
    VAU.ALIGNVEC v18 v13 v16 4
    VAU.ALIGNVEC v19 v13 v16 6
	VAU.ALIGNVEC v20 v13 v16 8
	VAU.ADD.f16 v24 v13 v20
	VAU.ADD.f16 v25 v17 v19
	VAU.MUL.f16 v18 v18 v21 || lsu0.swzv8 [22222222]
	VAU.MUl.f16 v24 v24 v21 || lsu0.swzv8 [00000000]
	VAU.MUl.f16 v25 v25 v21 || lsu0.swzv8 [11111111]
	VAU.ADD.f16 v11 v1 v5 
	VAU.ADD.f16 v12 v2 v4
	VAU.ADD.f16 v25 v24 v25
	VAU.MUl.f16 v13 v3 v21 || lsu0.swzv8 [22222222] 
	VAU.MUl.f16 v11 v11 v21 || lsu0.swzv8 [00000000]
	VAU.ADD.f16 v25 v25 v18
	VAU.MUl.f16 v12 v12 v21 || lsu0.swzv8 [11111111] 
	VAU.ADD.f16 v14 v6 v10
___loop:
	VAU.MUL.f16 v25 v25 v23
	VAU.ADD.f16 v15 v7 v9 
	VAU.ADD.f16 v12 v11 v12
	VAU.MUl.f16 v16 v8 v21  || lsu0.swzv8 [22222222] || CMU.CLAMP0.F16 v25 v25 v22
	VAU.MUl.f16 v14 v14 v21 || lsu0.swzv8 [00000000] 
	LSU0.ST128.f16.u8 v25 i17 || IAU.ADD i17 i17 8 || 	VAU.ADD.f16 v13 v12 v13 || LSU1.LD128.u8.f16 v1 i1     || SAU.ADD.i32 i1 i1 8 
	
	nop
	;Compensation for remaining pixels
	LSU0.LD128.u8.f16 v1 i1     || IAU.ADD i1 i1 8 
	LSU0.LD128.u8.f16 v2 i2     || IAU.ADD i2 i2 8  
	LSU0.LD128.u8.f16 v3 i3     || IAU.ADD i3 i3 8 
	LSU0.LD128.u8.f16 v4 i4     || IAU.ADD i4 i4 8  
	LSU0.LD128.u8.f16 v5 i5     || IAU.ADD i5 i5 8  
	LSU1.LD128.u8.f16 v6 i1   
	LSU1.LD128.u8.f16 v7 i2
	LSU1.LD128.u8.f16 v8 i3  
	LSU1.LD128.u8.f16 v9 i4   
	LSU0.LD128.u8.f16 v10 i5 
	VAU.ADD.f16 v11 v1 v5
	VAU.ADD.f16 v12 v2 v4
	VAU.MUl.f16 v13 v3 v21 || lsu0.swzv8 [22222222]
	VAU.MUl.f16 v11 v11 v21 || lsu0.swzv8 [00000000]
	VAU.MUl.f16 v12 v12 v21 || lsu0.swzv8 [11111111]
	VAU.ADD.f16 v14 v6 v10
	VAU.ADD.f16 v15 v7 v9
	VAU.ADD.f16 v12 v11 v12
	VAU.MUl.f16 v16 v8 v21  || lsu0.swzv8 [22222222]
	VAU.MUl.f16 v14 v14 v21 || lsu0.swzv8 [00000000]
	VAU.ADD.f16 v13 v12 v13
	VAU.MUl.f16 v15 v15 v21 || lsu0.swzv8 [11111111]
	nop 2
	VAU.ADD.f16 v15 v14 v15
	nop 2
	VAU.ADD.f16 v16 v15 v16
	nop 2
    VAU.ALIGNVEC v17 v13 v16 2
    VAU.ALIGNVEC v18 v13 v16 4
    VAU.ALIGNVEC v19 v13 v16 6
	VAU.ALIGNVEC v20 v13 v16 8
	
	VAU.ADD.f16 v24 v13 v20
	VAU.ADD.f16 v25 v17 v19
	VAU.MUL.f16 v18 v18 v21 || lsu0.swzv8 [22222222]
	VAU.MUl.f16 v24 v24 v21 || lsu0.swzv8 [00000000]
	VAU.MUl.f16 v25 v25 v21 || lsu0.swzv8 [11111111]
	nop 2
	VAU.ADD.f16 v25 v24 v25
	nop 2
	VAU.ADD.f16 v25 v25 v18
	nop 2
	VAU.MUL.f16 v25 v25 v23
	nop 2
	CMU.CLAMP0.F16 v25 v25 v22
	nop
	LSU0.ST128.f16.u8 v25 i10 || IAU.ADD i10 i10 8
	
	IAU.SUB i10 i10 8
	lsu0.ldil i8, ___loop2   || lsu1.ldih i8, ___loop2
	IAU.SUB i9 i9 i9
	___loop2:
	LSU0.LDi8  i1  i10
	nop 2
	IAU.ADD i9 i9 1
	cmu.cmii.i32 i9 i13
	peu.pc1c lt || bru.jmp i8
	LSU0.STi8  i1  i17
	nop 4
	
	
	
IAU.ADD i19 i19 i16
POP_V_REG v25
POP_V_REG v24	
	bru.jmp i30
	nop 5

	
.end
