; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief
; ///

.version 00.51.04

.data .data.convolution9x9

.salign 16
___clampVal:
        .float16 255.0


.code .text.convolution9x9

;void Convolution9x9_asm(u8** in(i18), u8** out(i17), half conv[49](i16), u32 inWidth(i15))
Convolution9x9_asm:
lsu0.ldil i10, ___clampVal       ||  lsu1.ldih i10, ___clampVal
LSU0.LD.16R v10, i10
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

LSU0.LD.32  i0  i18 || IAU.ADD i18 i18 4
LSU0.LD.32  i1  i18 || IAU.ADD i18 i18 4
LSU0.LD.32  i2  i18 || IAU.ADD i18 i18 4
LSU0.LD.32  i3  i18 || IAU.ADD i18 i18 4
LSU0.LD.32  i4  i18 || IAU.ADD i18 i18 4
LSU0.LD.32  i5  i18 || IAU.ADD i18 i18 4
LSU0.LD.32  i6  i18 || IAU.ADD i18 i18 4
LSU0.LD.32  i7  i18 || IAU.ADD i18 i18 4
LSU0.LD.32  i8  i18 || IAU.ADD i18 i18 4
LSU1.LD.32 i17 i17
LSU0.LD.16R v31, i10
LSU0.LDO.64.L v10 i16 0          || LSU1.LDO.64.H v10 i16 8      || IAU.ADD i16 i16 16
LSU0.LDO.64.L v11 i16 0          || LSU1.LDO.64.H v11 i16 8      || IAU.ADD i16 i16 16
LSU0.LDO.64.L v12 i16 0          || LSU1.LDO.64.H v12 i16 8      || IAU.ADD i16 i16 16
LSU0.LDO.64.L v13 i16 0          || LSU1.LDO.64.H v13 i16 8      || IAU.ADD i16 i16 16
LSU0.LDO.64.L v14 i16 0          || LSU1.LDO.64.H v14 i16 8      || IAU.ADD i16 i16 16
LSU0.LDO.64.L v15 i16 0          || LSU1.LDO.64.H v15 i16 8      || IAU.ADD i16 i16 16
LSU0.LDO.64.L v16 i16 0          || LSU1.LDO.64.H v16 i16 8      || IAU.ADD i16 i16 16
LSU0.LDO.64.L v17 i16 0          || LSU1.LDO.64.H v17 i16 8      || IAU.ADD i16 i16 16
LSU0.LDO.64.L v18 i16 0          || LSU1.LDO.64.H v18 i16 8      || IAU.ADD i16 i16 16
LSU0.LDO.64.L v19 i16 0          || LSU1.LDO.64.H v19 i16 8      || IAU.ADD i16 i16 16
LSU0.LDO.64.L v20 i16 0          || LSU1.LDO.64.H v20 i16 8      || IAU.ADD i16 i16 16

IAU.SHR.u32 i15 i15 3 
	LSU0.LDIL i10 ___forLoop         || LSU1.LDIH i10 ___forLoop
LSU1.LD.128.U8.F16 v0   i0     || IAU.ADD i0 i0 1 
LSU1.LD.128.U8.F16 v1   i1     || IAU.ADD i1 i1 1
LSU1.LD.128.U8.F16 v2   i2     || IAU.ADD i2 i2 1
LSU1.LD.128.U8.F16 v3   i3     || IAU.ADD i3 i3 1
LSU1.LD.128.U8.F16 v4   i4     || IAU.ADD i4 i4 1
LSU1.LD.128.U8.F16 v5   i5     || IAU.ADD i5 i5 1 
LSU1.LD.128.U8.F16 v6   i6     || IAU.ADD i6 i6 1 
LSU1.LD.128.U8.F16 v7   i7     || IAU.ADD i7 i7 1 || VAU.MACPZ.f16        v10 v0   || lsu0.swzv8 [00000000] || bru.rpl i10 i15;one
LSU1.LD.128.U8.F16 v8   i8     || IAU.ADD i8 i8 1 || VAU.MACP.f16         v11 v1   || lsu0.swzv8 [11111111]
LSU1.LD.128.U8.F16 v9   i0     || IAU.ADD i0 i0 1 || VAU.MACP.f16         v12 v2   || lsu0.swzv8 [22222222]
LSU1.LD.128.U8.F16 v21  i1     || IAU.ADD i1 i1 1 || VAU.MACP.f16         v13 v3   || lsu0.swzv8 [33333333];two
LSU1.LD.128.U8.F16 v22  i2     || IAU.ADD i2 i2 1 || VAU.MACP.f16         v14 v4   || lsu0.swzv8 [44444444]
LSU1.LD.128.U8.F16 v23  i3     || IAU.ADD i3 i3 1 || VAU.MACP.f16         v15 v5   || lsu0.swzv8 [55555555]
LSU1.LD.128.U8.F16 v24  i4     || IAU.ADD i4 i4 1 || VAU.MACP.f16         v16 v6   || lsu0.swzv8 [66666666]
LSU1.LD.128.U8.F16 v25  i5     || IAU.ADD i5 i5 1 || VAU.MACP.f16         v17 v7   || lsu0.swzv8 [77777777]
LSU1.LD.128.U8.F16 v26  i6     || IAU.ADD i6 i6 1 || VAU.MACP.f16         v19 v8   || lsu0.swzv8 [00000000] ;*******
LSU1.LD.128.U8.F16 v27  i7     || IAU.ADD i7 i7 1 || VAU.MACP.f16         v10 v9   || lsu0.swzv8 [11111111]
LSU1.LD.128.U8.F16 v28  i8     || IAU.ADD i8 i8 1 || VAU.MACP.f16         v11 v21  || lsu0.swzv8 [22222222]
LSU1.LD.128.U8.F16 v29  i0     || IAU.ADD i0 i0 1 || VAU.MACP.f16         v12 v22  || lsu0.swzv8 [33333333]
LSU1.LD.128.U8.F16 v30  i1     || IAU.ADD i1 i1 1 || VAU.MACP.f16         v13 v23  || lsu0.swzv8 [44444444];three
LSU1.LD.128.U8.F16 v0   i2     || IAU.ADD i2 i2 1 || VAU.MACP.f16         v14 v24  || lsu0.swzv8 [55555555]
LSU1.LD.128.U8.F16 v1   i3     || IAU.ADD i3 i3 1 || VAU.MACP.f16         v15 v25  || lsu0.swzv8 [66666666]
LSU1.LD.128.U8.F16 v2   i4     || IAU.ADD i4 i4 1 || VAU.MACP.f16         v16 v26  || lsu0.swzv8 [77777777]
LSU1.LD.128.U8.F16 v3   i5     || IAU.ADD i5 i5 1 || VAU.MACP.f16         v18 v27  || lsu0.swzv8 [00000000]
LSU1.LD.128.U8.F16 v4   i6     || IAU.ADD i6 i6 1 || VAU.MACP.f16         v19 v28  || lsu0.swzv8 [11111111] ;*******
LSU1.LD.128.U8.F16 v5   i7     || IAU.ADD i7 i7 1 || VAU.MACP.f16         v10 v29  || lsu0.swzv8 [22222222]
LSU1.LD.128.U8.F16 v6   i8     || IAU.ADD i8 i8 1 || VAU.MACP.f16         v11 v30  || lsu0.swzv8 [33333333]
LSU1.LD.128.U8.F16 v7   i0     || IAU.ADD i0 i0 1 || VAU.MACP.f16         v12 v0   || lsu0.swzv8 [44444444]
LSU1.LD.128.U8.F16 v8   i1     || IAU.ADD i1 i1 1 || VAU.MACP.f16         v13 v1   || lsu0.swzv8 [55555555];four
LSU1.LD.128.U8.F16 v9   i2     || IAU.ADD i2 i2 1 || VAU.MACP.f16         v14 v2   || lsu0.swzv8 [66666666]
LSU1.LD.128.U8.F16 v21  i3     || IAU.ADD i3 i3 1 || VAU.MACP.f16         v15 v3   || lsu0.swzv8 [77777777]
LSU1.LD.128.U8.F16 v22  i4     || IAU.ADD i4 i4 1 || VAU.MACP.f16         v17 v4   || lsu0.swzv8 [00000000]
LSU1.LD.128.U8.F16 v23  i5     || IAU.ADD i5 i5 1 || VAU.MACP.f16         v18 v5   || lsu0.swzv8 [11111111]
LSU1.LD.128.U8.F16 v24  i6     || IAU.ADD i6 i6 1 || VAU.MACP.f16         v19 v6   || lsu0.swzv8 [22222222] ;*******
LSU1.LD.128.U8.F16 v25  i7     || IAU.ADD i7 i7 1 || VAU.MACP.f16         v10 v7   || lsu0.swzv8 [33333333]
LSU1.LD.128.U8.F16 v26  i8     || IAU.ADD i8 i8 1 || VAU.MACP.f16         v11 v8   || lsu0.swzv8 [44444444]
LSU1.LD.128.U8.F16 v27  i0     || IAU.ADD i0 i0 1 || VAU.MACP.f16         v12 v9   || lsu0.swzv8 [55555555]
LSU1.LD.128.U8.F16 v28  i1     || IAU.ADD i1 i1 1 || VAU.MACP.f16         v13 v21  || lsu0.swzv8 [66666666];five
LSU1.LD.128.U8.F16 v29  i2     || IAU.ADD i2 i2 1 || VAU.MACP.f16         v14 v22  || lsu0.swzv8 [77777777]
LSU1.LD.128.U8.F16 v30  i3     || IAU.ADD i3 i3 1 || VAU.MACP.f16         v16 v23  || lsu0.swzv8 [00000000]
LSU1.LD.128.U8.F16 v0   i4     || IAU.ADD i4 i4 1 || VAU.MACP.f16         v17 v24  || lsu0.swzv8 [11111111]
LSU1.LD.128.U8.F16 v1   i5     || IAU.ADD i5 i5 1 || VAU.MACP.f16         v18 v25  || lsu0.swzv8 [22222222]
LSU1.LD.128.U8.F16 v2   i6     || IAU.ADD i6 i6 1 || VAU.MACP.f16         v19 v26  || lsu0.swzv8 [33333333] ;*******
LSU1.LD.128.U8.F16 v3   i7     || IAU.ADD i7 i7 1 || VAU.MACP.f16         v10 v27  || lsu0.swzv8 [44444444]
LSU1.LD.128.U8.F16 v4   i8     || IAU.ADD i8 i8 1 || VAU.MACP.f16         v11 v28  || lsu0.swzv8 [55555555]
LSU1.LD.128.U8.F16 v5   i0     || IAU.ADD i0 i0 1 || VAU.MACP.f16         v12 v29  || lsu0.swzv8 [66666666]
LSU1.LD.128.U8.F16 v6   i1     || IAU.ADD i1 i1 1 || VAU.MACP.f16         v13 v30  || lsu0.swzv8 [77777777];six
LSU1.LD.128.U8.F16 v7   i2     || IAU.ADD i2 i2 1 || VAU.MACP.f16         v15 v0   || lsu0.swzv8 [00000000]
LSU1.LD.128.U8.F16 v8   i3     || IAU.ADD i3 i3 1 || VAU.MACP.f16         v16 v1   || lsu0.swzv8 [11111111]
LSU1.LD.128.U8.F16 v9   i4     || IAU.ADD i4 i4 1 || VAU.MACP.f16         v17 v2   || lsu0.swzv8 [22222222]
LSU1.LD.128.U8.F16 v21  i5     || IAU.ADD i5 i5 1 || VAU.MACP.f16         v18 v3   || lsu0.swzv8 [33333333]
LSU1.LD.128.U8.F16 v22  i6     || IAU.ADD i6 i6 1 || VAU.MACP.f16         v19 v4   || lsu0.swzv8 [44444444] ;*******
LSU1.LD.128.U8.F16 v23  i7     || IAU.ADD i7 i7 1 || VAU.MACP.f16         v10 v5   || lsu0.swzv8 [55555555]
LSU1.LD.128.U8.F16 v24  i8     || IAU.ADD i8 i8 1 || VAU.MACP.f16         v11 v6   || lsu0.swzv8 [66666666]
LSU1.LD.128.U8.F16 v25  i0     || IAU.ADD i0 i0 1 || VAU.MACP.f16         v12 v7   || lsu0.swzv8 [77777777]
LSU1.LD.128.U8.F16 v26  i1     || IAU.ADD i1 i1 1 || VAU.MACP.f16         v14 v8   || lsu0.swzv8 [00000000];seven
LSU1.LD.128.U8.F16 v27  i2     || IAU.ADD i2 i2 1 || VAU.MACP.f16         v15 v9   || lsu0.swzv8 [11111111]
LSU1.LD.128.U8.F16 v28  i3     || IAU.ADD i3 i3 1 || VAU.MACP.f16         v16 v21  || lsu0.swzv8 [22222222]
LSU1.LD.128.U8.F16 v29  i4     || IAU.ADD i4 i4 1 || VAU.MACP.f16         v17 v22  || lsu0.swzv8 [33333333]
LSU1.LD.128.U8.F16 v30  i5     || IAU.ADD i5 i5 1 || VAU.MACP.f16         v18 v23  || lsu0.swzv8 [44444444]
LSU1.LD.128.U8.F16 v0   i6     || IAU.ADD i6 i6 1 || VAU.MACP.f16         v19 v24  || lsu0.swzv8 [55555555] ;*******
LSU1.LD.128.U8.F16 v1   i7     || IAU.ADD i7 i7 1 || VAU.MACP.f16         v10 v25  || lsu0.swzv8 [66666666]
LSU1.LD.128.U8.F16 v2   i8     || IAU.ADD i8 i8 1 || VAU.MACP.f16         v11 v26  || lsu0.swzv8 [77777777]
LSU1.LD.128.U8.F16 v3   i0     || IAU.ADD i0 i0 1 || VAU.MACP.f16         v13 v27  || lsu0.swzv8 [00000000]
LSU1.LD.128.U8.F16 v4   i1     || IAU.ADD i1 i1 1 || VAU.MACP.f16         v14 v28  || lsu0.swzv8 [11111111];eight
LSU1.LD.128.U8.F16 v5   i2     || IAU.ADD i2 i2 1 || VAU.MACP.f16         v15 v29  || lsu0.swzv8 [22222222]
LSU1.LD.128.U8.F16 v6   i3     || IAU.ADD i3 i3 1 || VAU.MACP.f16         v16 v30  || lsu0.swzv8 [33333333]
LSU1.LD.128.U8.F16 v7   i4     || IAU.ADD i4 i4 1 || VAU.MACP.f16         v17 v0   || lsu0.swzv8 [44444444]
LSU1.LD.128.U8.F16 v8   i5     || IAU.ADD i5 i5 1 || VAU.MACP.f16         v18 v1   || lsu0.swzv8 [55555555]
LSU1.LD.128.U8.F16 v9   i6     || IAU.ADD i6 i6 1 || VAU.MACP.f16         v19 v2   || lsu0.swzv8 [66666666] ;*******
LSU1.LD.128.U8.F16 v21  i7     || IAU.ADD i7 i7 1 || VAU.MACP.f16         v10 v3   || lsu0.swzv8 [77777777]
LSU1.LD.128.U8.F16 v22  i8     || IAU.ADD i8 i8 1 || VAU.MACP.f16         v12 v4   || lsu0.swzv8 [00000000]
LSU1.LD.128.U8.F16 v23  i0     || IAU.ADD i0 i0 0 || VAU.MACP.f16         v13 v5   || lsu0.swzv8 [11111111]
LSU1.LD.128.U8.F16 v24  i1     || IAU.ADD i1 i1 0 || VAU.MACP.f16         v14 v6   || lsu0.swzv8 [22222222];nine
LSU1.LD.128.U8.F16 v25  i2     || IAU.ADD i2 i2 0 || VAU.MACP.f16         v15 v7   || lsu0.swzv8 [33333333]
LSU1.LD.128.U8.F16 v26  i3     || IAU.ADD i3 i3 0 || VAU.MACP.f16         v16 v8   || lsu0.swzv8 [44444444]
LSU1.LD.128.U8.F16 v27  i4     || IAU.ADD i4 i4 0 || VAU.MACP.f16         v17 v9   || lsu0.swzv8 [55555555]
LSU1.LD.128.U8.F16 v28  i5     || IAU.ADD i5 i5 0 || VAU.MACP.f16         v18 v21  || lsu0.swzv8 [66666666]
LSU1.LD.128.U8.F16 v29  i6     || IAU.ADD i6 i6 0 || VAU.MACP.f16         v19 v22  || lsu0.swzv8 [77777777] ;*******
LSU1.LD.128.U8.F16 v30  i7     || IAU.ADD i7 i7 0 || VAU.MACP.f16         v11 v23  || lsu0.swzv8 [00000000]
LSU1.LD.128.U8.F16 v0   i8     || IAU.ADD i8 i8 0 || VAU.MACP.f16         v12 v24  || lsu0.swzv8 [11111111]
VAU.MACP.f16         v13 v25  || lsu0.swzv8 [22222222]
VAU.MACP.f16         v14 v26  || lsu0.swzv8 [33333333]                   
VAU.MACP.f16         v15 v27  || lsu0.swzv8 [44444444]                   
VAU.MACP.f16         v16 v28  || lsu0.swzv8 [55555555]                   
VAU.MACP.f16         v17 v29  || lsu0.swzv8 [66666666]                   
VAU.MACP.f16         v18 v30  || lsu0.swzv8 [77777777]                   
VAU.MACPW.f16 v26    v20 v0   || lsu0.swzv8 [00000000] ;*******          
LSU1.LD.128.U8.F16 v0   i0     || IAU.ADD i0 i0 1 
LSU1.LD.128.U8.F16 v1   i1     || IAU.ADD i1 i1 1
LSU1.LD.128.U8.F16 v2   i2     || IAU.ADD i2 i2 1
LSU1.LD.128.U8.F16 v3   i3     || IAU.ADD i3 i3 1
LSU1.LD.128.U8.F16 v4   i4     || IAU.ADD i4 i4 1
LSU1.LD.128.U8.F16 v5   i5     || IAU.ADD i5 i5 1 
___forLoop:
LSU1.LD.128.U8.F16 v6   i6     || IAU.ADD i6 i6 1 
nop 3
CMU.CLAMP0.F16 v26 v26 v31
nop
LSU0.STI.128.F16.U8 v26 i17
													  
                                                                                                 
                                                                                                 
                                                                                                 
                                                                                               


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
    BRU.jmp i30
nop 6

.end
