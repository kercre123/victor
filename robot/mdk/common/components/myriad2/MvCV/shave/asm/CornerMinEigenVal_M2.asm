; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief
; ///

;--------------------------------------------------------------------------------------------------------------
.version 00.50.05
.data .rodata.CornerMinEigenVal 
.salign 16
___multiply:
		.float32    0.111111F32, 0.5F32, 1.5F32, 255.0F32


.code .text.CornerMinEigenVal ;text
.salign 16

CornerMinEigenVal_asm:
;CornerMinEigenVal(u8 **in_lines, u8 **out_line, u32 width);
;                     i18               i17        i16        


	IAU.SUB i19 i19 8
	LSU0.ST.64.L v24  i19  || IAU.SUB i19 i19 8
	LSU0.ST.64.H v24  i19  || IAU.SUB i19 i19 8
	LSU0.ST.64.L v25  i19  || IAU.SUB i19 i19 8
	LSU0.ST.64.H v25  i19  || IAU.SUB i19 i19 8
	LSU0.ST.64.L v26  i19  || IAU.SUB i19 i19 8
	LSU0.ST.64.H v26  i19  || IAU.SUB i19 i19 8
	LSU0.ST.64.L v27  i19  || IAU.SUB i19 i19 8
	LSU0.ST.64.H v27  i19  || IAU.SUB i19 i19 8
	LSU0.ST.64.L v28  i19  || IAU.SUB i19 i19 8
	LSU0.ST.64.H v28  i19  || IAU.SUB i19 i19 8
	LSU0.ST.64.L v29  i19  || IAU.SUB i19 i19 8
	LSU0.ST.64.H v29  i19  || IAU.SUB i19 i19 8
	LSU0.ST.64.L v30  i19  || IAU.SUB i19 i19 8
	LSU0.ST.64.H v30  i19  || IAU.SUB i19 i19 8
	LSU0.ST.64.L v31  i19  || IAU.SUB i19 i19 8
	LSU0.ST.64.H v31  i19  

	
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
LSU0.ST.32  i29  i19 

	
	lsu0.ldil i11, ___loop     ||  lsu1.ldih i11, ___loop
	lsu0.ldil i13, ___multiply     ||  lsu1.ldih i13, ___multiply
	LSU0.LD.32  i0  i18 || IAU.ADD i18 i18 4 ;line 0
	LSU0.LD.32  i1  i18 || IAU.ADD i18 i18 4 ;line 1
	LSU0.LD.32  i2  i18 || IAU.ADD i18 i18 4 ;line 2
	LSU0.LD.32  i3  i18 || IAU.ADD i18 i18 4 ;line 3
	LSU0.LD.32  i4  i18 || IAU.ADD i18 i18 4 ;line 4
	LSU0.LD.32  i17  i17
nop 5

;making room for interm buffers	
	IAU.SHL i15  i16  1
	IAU.SHR.u32  i14 i16 3
	IAU.ADD i14 i14 1
	IAU.ADD i15 i15 16
	IAU.SUB i19 i19 i15 
	IAU.SUB i5 i19 0 ;dx line 1
	IAU.SUB i19 i19 i15  
	IAU.SUB i6 i19 0 ;dx line 2
	IAU.SUB i19 i19 i15  
	IAU.SUB i7 i19 0 ;dx line 3
	IAU.SUB i19 i19 i15 
	IAU.SUB i8 i19 0 ;dy line 1
	IAU.SUB i19 i19 i15  
	IAU.SUB i9 i19 0 ;dy line 2
	IAU.SUB i19 i19 i15  
	IAU.SUB i10 i19 0;dy line 3
	

	
	LSU0.LDI.128.U8.F16 v0 i0  
	LSU0.LDI.128.U8.F16 v1 i1 
	LSU0.LDI.128.U8.F16 v2 i2 
	LSU0.LDI.128.U8.F16 v3 i3 
	LSU0.LDI.128.U8.F16 v4 i4 
	LSU1.LD.128.U8.F16 v5 i0 
	LSU1.LD.128.U8.F16 v6 i1
	LSU1.LD.128.U8.F16 v7 i2
	LSU1.LD.128.U8.F16 v8 i3
	LSU1.LD.128.U8.F16 v9 i4
nop 2
	
	CMU.ALIGNVEC v10 v0 v5 2 || bru.rpl i11 i14
	CMU.ALIGNVEC v11 v0 v5 4
	CMU.ALIGNVEC v12 v1 v6 2
	CMU.ALIGNVEC v13 v1 v6 4
	CMU.ALIGNVEC v14 v2 v7 2
	CMU.ALIGNVEC v15 v2 v7 4
	CMU.ALIGNVEC v16 v3 v8 2
	CMU.ALIGNVEC v17 v3 v8 4
	CMU.ALIGNVEC v18 v4 v9 2
	CMU.ALIGNVEC v19 v4 v9 4

	
	VAU.sub.f16 v20 v0 v11
	VAU.sub.f16 v21 v2 v15
	VAU.ADD.f16 v22 v1 v1
	VAU.ADD.f16 v23 v13 v13
	VAU.ADD.f16 v21 v20 v21
	VAU.sub.f16 v20 v0 v2
	VAU.sub.f16 v23 v22 v23 || LSU0.LDI.128.U8.F16 v0 i0 
nop 2
	vau.add.f16 v5 v21 v23   ;dx1
	VAU.sub.f16 v21 v11 v15
	VAU.ADD.f16 v22 v10 v10
	VAU.ADD.f16 v23 v14 v14
	VAU.ADD.f16 v21 v20 v21
	VAU.sub.f16 v20 v1 v13
	VAU.sub.f16 v23 v22 v23
nop 2
	vau.add.f16 v6 v21 v23   ;dy1
	VAU.sub.f16 v21 v3 v17
	VAU.ADD.f16 v22 v2 v2
	VAU.ADD.f16 v23 v15 v15
	VAU.ADD.f16 v21 v20 v21
	VAU.sub.f16 v20 v1 v3
	VAU.sub.f16 v23 v22 v23 || LSU0.LDI.128.U8.F16 v1 i1 
nop 2
	vau.add.f16 v7 v21 v23   ;dx2
	VAU.sub.f16 v21 v13 v17
	VAU.ADD.f16 v22 v12 v12
	VAU.ADD.f16 v23 v16 v16
	VAU.ADD.f16 v21 v20 v21
	VAU.sub.f16 v20 v2 v15
	VAU.sub.f16 v23 v22 v23
nop 2
	vau.add.f16 v8 v21 v23   ;dy2
	VAU.sub.f16 v21 v4 v19
	VAU.ADD.f16 v22 v3 v3
	VAU.ADD.f16 v23 v17 v17 || LSU0.LDI.128.U8.F16 v3 i3 
	VAU.ADD.f16 v21 v20 v21
	VAU.sub.f16 v20 v2 v4
	VAU.sub.f16 v23 v22 v23 || LSU0.LDI.128.U8.F16 v2 i2 
	LSU0.LDI.128.U8.F16 v4 i4
nop
	vau.add.f16 v9 v21 v23   ;dx3
	VAU.sub.f16 v21 v15 v19
	VAU.ADD.f16 v22 v14 v14
	VAU.ADD.f16 v23 v18 v18
	VAU.ADD.f16 v21 v20 v21
nop
	VAU.sub.f16 v23 v22 v23
nop 2
	vau.add.f16 v10 v21 v23   ;dy3

	LSU0.STI.64.L   v5   i5 
	LSU0.STI.64.H   v5   i5 ;dx1

	LSU0.STI.64.L   v6   i8 || LSU1.LD.128.U8.F16 v5 i0
	LSU0.STI.64.H   v6   i8 ;dy1
	LSU0.STI.64.L   v7   i6 || LSU1.LD.128.U8.F16 v6 i1
	LSU0.STI.64.H   v7   i6 ;dx2
___loop:
	LSU0.STI.64.L   v8   i9 || LSU1.LD.128.U8.F16 v7 i2
	LSU0.STI.64.H   v8   i9 ;dy2
	LSU0.STI.64.L   v9   i7 || LSU1.LD.128.U8.F16 v8 i3
	LSU0.STI.64.H   v9   i7 ;dx3
	LSU0.STI.64.L   v10  i10 || LSU1.LD.128.U8.F16 v9 i4 
	LSU0.STI.64.H   v10  i10 ;dy3
	nop


IAU.SHL i29 i14 4
	
IAU.sub i5  i5  i29
IAU.sub i6  i6  i29
IAU.sub i7  i7  i29
IAU.sub i8  i8  i29
IAU.sub i9  i9  i29	
IAU.sub i10 i10 i29



IAU.shl i12 i16 2
IAU.ADD i12 i12 16
IAU.SUB i19 i19 i12
IAU.sub i1 i19 0 ;dxx filtered
IAU.SUB i19 i19 i12
IAU.sub i2 i19 0 ;dxy filtered
IAU.SUB i19 i19 i12
IAU.sub i3 i19 0 ;dyy filtered


LSU0.LD.32R v0 i13 || IAU.ADD i13 i13 4
lsu0.ldil i11, ___loop2     ||  lsu1.ldih i11, ___loop2
IAU.SHR.u32  i14 i16 2
IAU.ADD i14 i14 1



; (Myriad2 COMPATIBILITY ISSUE): BRU.RPL: changed order of parameters at line number 206, in CornerMinEigenVal.asm
LSU0.LD.128.F16.F32  v1  i5  || IAU.ADD i5  i5  8 || bru.rpl i11 i14                             ;dx1
LSU0.LD.128.F16.F32  v2  i6  || IAU.ADD i6  i6  8 || LSU1.LD.128.F16.F32  v7  i5  ;dx2
LSU0.LD.128.F16.F32  v3  i7  || IAU.ADD i7  i7  8 || LSU1.LD.128.F16.F32  v8  i6  ;dx3
LSU0.LD.128.F16.F32  v4  i8  || IAU.ADD i8  i8  8 || LSU1.LD.128.F16.F32  v9  i7  ;dy1
LSU0.LD.128.F16.F32  v5  i9  || IAU.ADD i9  i9  8 || LSU1.LD.128.F16.F32  v10 i8  ;dy2
LSU0.LD.128.F16.F32  v6  i10 || IAU.ADD i10 i10 8 || LSU1.LD.128.F16.F32  v11 i9  ;dy3
LSU1.LD.128.F16.F32  v12 i10                                                     ;



VAU.mul.f32 v13  v1   v1 ;dxx1 
VAU.mul.f32 v14  v2   v2 ;dxx2
VAU.mul.f32 v15  v3   v3 ;dxx3
VAU.mul.f32 v16  v4   v4 ;dyy1
VAU.mul.f32 v17  v5   v5 ;dyy2
VAU.mul.f32 v18  v6   v6 ;dyy3
VAU.mul.f32 v19  v7   v7 ;dxx1
VAU.mul.f32 v20  v8   v8 ;dxx2
VAU.mul.f32 v21  v9   v9 ;dxx3
VAU.mul.f32 v22 v10  v10 ;dyy1
VAU.mul.f32 v23 v11  v11 ;dyy2
VAU.mul.f32 v24 v12  v12 ;dyy3
VAU.mul.f32 v25 v1 v4   ;dxy1
VAU.mul.f32 v26 v2 v5   ;dxy2
VAU.mul.f32 v27 v3 v6   ;dxy3
VAU.mul.f32 v28 v7 v10  ;dxy1
VAU.mul.f32 v29 v8 v11  ;dxy2
VAU.mul.f32 v30 v9 v12  ;dxy3
nop 2

	

CMU.ALIGNVEC v1  v13 v19 4 ;dxx
CMU.ALIGNVEC v2  v13 v19 8
CMU.ALIGNVEC v3  v14 v20 4
CMU.ALIGNVEC v4  v14 v20 8
CMU.ALIGNVEC v5  v15 v21 4
CMU.ALIGNVEC v6  v15 v21 8
                 
CMU.ALIGNVEC v7  v16 v22 4 ;dyy
CMU.ALIGNVEC v8  v16 v22 8
CMU.ALIGNVEC v9  v17 v23 4
CMU.ALIGNVEC v10 v17 v23 8
CMU.ALIGNVEC v11 v18 v24 4
CMU.ALIGNVEC v12 v18 v24 8


CMU.ALIGNVEC v19  v25 v28 4 ;dxy
CMU.ALIGNVEC v20  v25 v28 8
CMU.ALIGNVEC v21  v26 v29 4
CMU.ALIGNVEC v22  v26 v29 8
CMU.ALIGNVEC v23  v27 v30 4
CMU.ALIGNVEC v24  v27 v30 8



vau.add.f32 v28 v13 v1 ;dxx filtered
vau.add.f32 v29 v2  v14
vau.add.f32 v30 v3  v4
vau.add.f32 v31 v15 v5
vau.add.f32 v1  v28 v6
vau.add.f32 v2  v29 v30
vau.add.f32 v28 v16 v7
vau.add.f32 v1 v31 v1
vau.add.f32 v29 v8  v17
vau.add.f32 v30 v9  v10
vau.add.f32 v1 v1 v2
vau.add.f32 v31 v18 v11
vau.add.f32 v7  v28 v12
vau.mul.f32 v1 v1 v0
vau.add.f32 v8  v29 v30
vau.add.f32 v28 v25 v19  ;dxy filtered
vau.add.f32 v7 v31 v7 || LSU0.STI.64.L v1  i1
vau.add.f32 v29 v20 v26 || LSU0.STI.64.H v1  i1 
vau.add.f32 v30 v21  v22
vau.add.f32 v7 v7 v8
vau.add.f32 v31 v27 v23
vau.add.f32 v19  v28 v24
vau.mul.f32 v7 v7 v0
vau.add.f32 v20  v29 v30
nop
vau.add.f32 v19 v31 v19 || LSU0.STI.64.L v7  i3 
LSU0.STI.64.H v7  i3
nop
vau.add.f32 v19 v19 v20
nop
___loop2:
nop
vau.mul.f32 v19 v19 v0
nop 3
LSU0.STI.64.L v19 i2 
LSU0.STI.64.H v19 i2  

IAU.shl i0 i14 4

IAU.sub i1 i1 i0
IAU.sub i2 i2 i0
IAU.sub i3 i3 i0



LSU0.LD.32R v4 i13  || iau.add i13 i13 4 ;0.5
LSU0.LD.32R v14 i13 || iau.add i13 i13 4 ;1.5
LSU0.LD.32R v30 i13
IAU.SUB i11 i16 4
IAU.SHR.u32 i10 i11 2  ;(width - 4)/4
lsu0.ldil i4, 0x3     ||  lsu1.ldih i4, 0x0
IAU.AND i5 i11 i4
lsu0.ldil i9, 0x59df      ||  lsu1.ldih i9, 0x5f37
CMU.CPIVR.x32  v12   i9
lsu0.ldil i9, 0x0     ||  lsu1.ldih i9, 0x7f80
CMU.CPIVR.x32  v31   i9
lsu0.ldil i13, ___loop3     ||  lsu1.ldih i13, ___loop3
IAU.sub i0 i0 i0
CMU.CPIVR.x32  v29   i0
LSU0.STI.8 i0 i17 
LSU0.STI.8 i0 i17



CMU.CMZ.i32  i10 || LSU0.ldil i7, ___compensate || LSU1.ldih i7, ___compensate
peu.pc1c eq      || bru.jmp i7
nop 6


;// punct 1
LSU0.LD.64.L v1 i1  || IAU.ADD i1 i1 8  
LSU0.LD.64.H v1 i1  || IAU.ADD i1 i1 8      
LSU0.LD.64.L v2 i2  || IAU.ADD i2 i2 8      
LSU0.LD.64.H v2 i2  || IAU.ADD i2 i2 8      
LSU0.LD.64.L v3 i3  || IAU.ADD i3 i3 8      
LSU0.LD.64.H v3 i3  || IAU.ADD i3 i3 8    
nop 6



vau.mul.f32 v5 v1 v4 ;a
vau.mul.f32 v6 v3 v4 ;c
vau.mul.f32 v7 v2 v2 ;b*b
nop
vau.sub.f32 v9 v5 v6 ;a-c
; (Myriad2 COMPATIBILITY ISSUE): BRU.RPL: changed order of parameters at line number 348, in CornerMinEigenVal.asm
vau.add.f32 v8 v5 v6 || bru.rpl i13 i10     ;a+c
LSU0.LD.64.L v1 i1  || IAU.ADD i1 i1 8  
vau.mul.f32 v10 v9 v9;(a-c)*(a-c)
LSU0.LD.64.H v1 i1  || IAU.ADD i1 i1 8      
LSU0.LD.64.L v2 i2  || IAU.ADD i2 i2 8   
vau.add.f32 v11 v7 v10 ;(a-c)*(a-c) + b * b
LSU0.LD.64.H v2 i2  || IAU.ADD i2 i2 8      
LSU0.LD.64.L v3 i3  || IAU.ADD i3 i3 8      
vau.mul.f32  v15 v11 v4 ; x = number *0.5
LSU0.LD.64.H v3 i3  || IAU.ADD i3 i3 8    
VAU.SHR.U32  v16 v11 1 ; ( i >> 1 )
nop
vau.sub.i32 v17 v12 v16 ;0x5f3759df - ( i >> 1 );
nop 2

vau.mul.f32 v18 v17 v17
vau.mul.f32 v5 v1 v4 ;a
vau.mul.f32 v6 v3 v4 ;c
vau.mul.f32 v19 v18 v15
vau.mul.f32 v7 v2 v2 ;b*b
nop
vau.sub.f32 v20 v14 v19
vau.sub.f32 v9 v5 v6 ;a-c
nop
vau.mul.f32 v21 v20 v17
nop 2
vau.mul.f32 v22 v21 v21
nop 2
vau.mul.f32 v23 v22 v15
nop 2
vau.sub.f32 v24 v14 v23
nop 2
vau.mul.f32 v25 v24 v21
nop 2
vau.mul.f32 v26 v11 v25
nop 2
vau.sub.f32 v27 v8 v26
nop 2
CMU.CMVV.u32 v27 v31
PEU.PVV32 gt || vau.xor v27 v27 v27
CMU.CLAMP0.f32 v27 v27 v30
nop
CMU.CPVV.f32.i32s v28 v27
___loop3:
nop 2
CMU.CPVi.x32 i20  v28.0
CMU.CPVi.x32 i21  v28.1 || LSU0.STI.8 i20 i17
CMU.CPVi.x32 i22  v28.2 || LSU0.STI.8 i21 i17
CMU.CPVi.x32 i23  v28.3 || LSU0.STI.8 i22 i17
LSU0.STI.8 i23 i17


___compensate:
CMU.CMZ.i32  i5 || LSU0.ldil i7, ___final || LSU1.ldih i7, ___final
peu.pc1c eq || bru.jmp i7
nop 6



LSU0.LD.64.L v1 i1  || IAU.ADD i1 i1 8  
LSU0.LD.64.H v1 i1  || IAU.ADD i1 i1 8      
LSU0.LD.64.L v2 i2  || IAU.ADD i2 i2 8      
LSU0.LD.64.H v2 i2  || IAU.ADD i2 i2 8      
LSU0.LD.64.L v3 i3  || IAU.ADD i3 i3 8      
LSU0.LD.64.H v3 i3  || IAU.ADD i3 i3 8    
nop 5

IAU.sub i19 i19 4
iau.sub i26 i19 0
iau.sub i6  i19 0


vau.mul.f32 v5 v1 v4 ;a
vau.mul.f32 v6 v3 v4 ;c
vau.mul.f32 v7 v2 v2 ;b*b
nop
vau.sub.f32 v9 v5 v6 ;a-c
vau.add.f32 v8 v5 v6 ;a+c
nop
vau.mul.f32 v10 v9 v9;(a-c)*(a-c)
nop 2
vau.add.f32 v11 v7 v10 ;(a-c)*(a-c) + b * b
nop 2


vau.mul.f32  v15 v11 v4 ; x = number *0.5
nop
VAU.SHR.U32  v16 v11 1 ; ( i >> 1 )
nop
vau.sub.i32 v17 v12 v16 ;0x5f3759df - ( i >> 1 );
nop 2

vau.mul.f32 v18 v17 v17
nop 2
vau.mul.f32 v19 v18 v15
nop 2
vau.sub.f32 v20 v14 v19
nop 2
vau.mul.f32 v21 v20 v17
nop 2

vau.mul.f32 v22 v21 v21
nop 2
vau.mul.f32 v23 v22 v15
nop 2
vau.sub.f32 v24 v14 v23
nop 2
vau.mul.f32 v25 v24 v21
nop 2

vau.mul.f32 v26 v11 v25
nop 2

vau.sub.f32 v27 v8 v26
nop 2


CMU.CMVV.u32 v27 v31
PEU.PVV32 gt || vau.xor v27 v27 v27

CMU.CLAMP0.f32 v27 v27 v30
nop
CMU.CPVV.f32.i32s v28 v27
nop
CMU.CPVi.x32 i20  v28.0
CMU.CPVi.x32 i21  v28.1 || LSU0.STI.8 i20 i26
CMU.CPVi.x32 i22  v28.2 || LSU0.STI.8 i21 i26
CMU.CPVi.x32 i23  v28.3 || LSU0.STI.8 i22 i26
LSU0.STI.8 i23 i26

LSU0.ldil i8, ___compensate_loop || LSU1.ldih i8, ___compensate_loop

; (Myriad2 COMPATIBILITY ISSUE): BRU.RPL: changed order of parameters at line number 480, in CornerMinEigenVal.asm
LSU0.LDI.8 i7, i6 || bru.rpl i8 i5
___compensate_loop:
nop 6
LSU0.ST.8 i7, i17 || IAU.ADD i17 i17 1


IAU.ADD i19 i19 4

___final:
LSU0.STI.8 i0 i17 
LSU0.STI.8 i0 i17
IAU.ADD i19 i19 i12
IAU.ADD i19 i19 i12
IAU.ADD i19 i19 i12

IAU.ADD i19 i19 i15
IAU.ADD i19 i19 i15
IAU.ADD i19 i19 i15
IAU.ADD i19 i19 i15
IAU.ADD i19 i19 i15
IAU.ADD i19 i19 i15
	
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
	
	
	
	LSU0.LD.64.H  v31  i19 || IAU.ADD i19 i19 8    
	LSU0.LD.64.L  v31  i19 || IAU.ADD i19 i19 8
	LSU0.LD.64.H  v30  i19 || IAU.ADD i19 i19 8    
	LSU0.LD.64.L  v30  i19 || IAU.ADD i19 i19 8
	LSU0.LD.64.H  v29  i19 || IAU.ADD i19 i19 8    
	LSU0.LD.64.L  v29  i19 || IAU.ADD i19 i19 8
	LSU0.LD.64.H  v28  i19 || IAU.ADD i19 i19 8    
	LSU0.LD.64.L  v28  i19 || IAU.ADD i19 i19 8
	LSU0.LD.64.H  v27  i19 || IAU.ADD i19 i19 8    
	LSU0.LD.64.L  v27  i19 || IAU.ADD i19 i19 8
	LSU0.LD.64.H  v26  i19 || IAU.ADD i19 i19 8    
	LSU0.LD.64.L  v26  i19 || IAU.ADD i19 i19 8
	LSU0.LD.64.H  v25  i19 || IAU.ADD i19 i19 8    
	LSU0.LD.64.L  v25  i19 || IAU.ADD i19 i19 8
	LSU0.LD.64.H  v24  i19 || IAU.ADD i19 i19 8    
	LSU0.LD.64.L  v24  i19 || IAU.ADD i19 i19 8
nop 6





	bru.jmp i30
nop 6
.end
