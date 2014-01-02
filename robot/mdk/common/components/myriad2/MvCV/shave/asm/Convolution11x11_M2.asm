; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief
; ///

.version 00.51.04

.data .data.convolution11x11

.salign 16
___clampVal:
        .float16 255.0


.code .text.convolution11x11

;void Convolution11x11_asm(u8** in(i18), u8** out(i17), half conv[49](i16), u32 inWidth(i15))
Convolution11x11_asm:
lsu0.ldil i12, ___clampVal       ||  lsu1.ldih i12, ___clampVal

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
	LSU0.ST.64.H v28  i19  

LSU0.LD.32  i0   i18 || IAU.ADD i18 i18 4
LSU0.LD.32  i1   i18 || IAU.ADD i18 i18 4
LSU0.LD.32  i2   i18 || IAU.ADD i18 i18 4
LSU0.LD.32  i3   i18 || IAU.ADD i18 i18 4
LSU0.LD.32  i4   i18 || IAU.ADD i18 i18 4
LSU0.LD.32  i5   i18 || IAU.ADD i18 i18 4
LSU0.LD.32  i6   i18 || IAU.ADD i18 i18 4
LSU0.LD.32  i7   i18 || IAU.ADD i18 i18 4
LSU0.LD.32  i8   i18 || IAU.ADD i18 i18 4
LSU0.LD.32  i9   i18 || IAU.ADD i18 i18 4
LSU0.LD.32  i10  i18 || IAU.ADD i18 i18 4
LSU1.LD.32 i17 i17
LSU0.LD.16R v28, i12
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
LSU0.LDO.64.L v21 i16 0          || LSU1.LDO.64.H v21 i16 8      || IAU.ADD i16 i16 16
LSU0.LDO.64.L v22 i16 0          || LSU1.LDO.64.H v22 i16 8      || IAU.ADD i16 i16 16
LSU0.LDO.64.L v23 i16 0          || LSU1.LDO.64.H v23 i16 8      || IAU.ADD i16 i16 16
LSU0.LDO.64.L v24 i16 0          || LSU1.LDO.64.H v24 i16 8      || IAU.ADD i16 i16 16
LSU0.LDO.64.L v25 i16 0          || LSU1.LDO.64.H v25 i16 8      || IAU.ADD i16 i16 16
LSU0.LDO.64.L v26 i16 0          || LSU1.LDO.64.H v26 i16 8      || IAU.ADD i16 i16 16

	IAU.SHR.u32 i15 i15 3 
	LSU0.LDIL i12 ___forLoop         || LSU1.LDIH i12 ___forLoop

LSU1.LD.128.U8.F16 v0    i0     || IAU.ADD i0  i0  1 
LSU1.LD.128.U8.F16 v1    i1     || IAU.ADD i1  i1  1	
LSU1.LD.128.U8.F16 v2    i2     || IAU.ADD i2  i2  1	
LSU1.LD.128.U8.F16 v3    i3     || IAU.ADD i3  i3  1	
LSU1.LD.128.U8.F16 v4    i4     || IAU.ADD i4  i4  1	
LSU1.LD.128.U8.F16 v5    i5     || IAU.ADD i5  i5  1	
LSU1.LD.128.U8.F16 v6    i6     || IAU.ADD i6  i6  1 
LSU1.LD.128.U8.F16 v7    i7     || IAU.ADD i7  i7  1 || VAU.MACPZ.f16   v11 v0  || lsu0.swzv8 [00000000] || bru.rpl i12 i15;one
LSU1.LD.128.U8.F16 v8    i8     || IAU.ADD i8  i8  1 || VAU.MACP.f16    v12 v1  || lsu0.swzv8 [33333333]
LSU1.LD.128.U8.F16 v9    i9     || IAU.ADD i9  i9  1 || VAU.MACP.f16    v13 v2  || lsu0.swzv8 [66666666]
LSU1.LD.128.U8.F16 v10   i10    || IAU.ADD i10 i10 1 || VAU.MACP.f16    v15 v3  || lsu0.swzv8 [11111111]
                                                                           
LSU1.LD.128.U8.F16 v0    i0     || IAU.ADD i0  i0  1 || VAU.MACP.f16    v16 v4  || lsu0.swzv8 [44444444]
LSU1.LD.128.U8.F16 v1    i1     || IAU.ADD i1  i1  1 || VAU.MACP.f16    v17 v5  || lsu0.swzv8 [77777777] ;two
LSU1.LD.128.U8.F16 v2    i2     || IAU.ADD i2  i2  1 || VAU.MACP.f16    v19 v6  || lsu0.swzv8 [22222222]	
LSU1.LD.128.U8.F16 v3    i3     || IAU.ADD i3  i3  1 || VAU.MACP.f16    v20 v7  || lsu0.swzv8 [55555555]	
LSU1.LD.128.U8.F16 v4    i4     || IAU.ADD i4  i4  1 || VAU.MACP.f16    v22 v8  || lsu0.swzv8 [00000000]	
LSU1.LD.128.U8.F16 v5    i5     || IAU.ADD i5  i5  1 || VAU.MACP.f16    v23 v9  || lsu0.swzv8 [33333333]	
LSU1.LD.128.U8.F16 v6    i6     || IAU.ADD i6  i6  1 || VAU.MACP.f16    v24 v10 || lsu0.swzv8 [66666666]	
LSU1.LD.128.U8.F16 v7    i7     || IAU.ADD i7  i7  1 || VAU.MACP.f16    v11 v0  || lsu0.swzv8 [11111111]
LSU1.LD.128.U8.F16 v8    i8     || IAU.ADD i8  i8  1 || VAU.MACP.f16    v12 v1  || lsu0.swzv8 [44444444]
LSU1.LD.128.U8.F16 v9    i9     || IAU.ADD i9  i9  1 || VAU.MACP.f16    v13 v2  || lsu0.swzv8 [77777777]
LSU1.LD.128.U8.F16 v10   i10    || IAU.ADD i10 i10 1 || VAU.MACP.f16    v15 v3  || lsu0.swzv8 [22222222]
                                                                           
LSU1.LD.128.U8.F16 v0    i0     || IAU.ADD i0  i0  1 || VAU.MACP.f16    v16 v4  || lsu0.swzv8 [55555555]
LSU1.LD.128.U8.F16 v1    i1     || IAU.ADD i1  i1  1 || VAU.MACP.f16    v17 v5  || lsu0.swzv8 [00000000] ;three
LSU1.LD.128.U8.F16 v2    i2     || IAU.ADD i2  i2  1 || VAU.MACP.f16    v19 v6  || lsu0.swzv8 [33333333]	
LSU1.LD.128.U8.F16 v3    i3     || IAU.ADD i3  i3  1 || VAU.MACP.f16    v20 v7  || lsu0.swzv8 [66666666]	
LSU1.LD.128.U8.F16 v4    i4     || IAU.ADD i4  i4  1 || VAU.MACP.f16    v22 v8  || lsu0.swzv8 [11111111]	
LSU1.LD.128.U8.F16 v5    i5     || IAU.ADD i5  i5  1 || VAU.MACP.f16    v23 v9  || lsu0.swzv8 [44444444]	                                                        
LSU1.LD.128.U8.F16 v6    i6     || IAU.ADD i6  i6  1 || VAU.MACP.f16    v24 v10 || lsu0.swzv8 [77777777]	                                         
LSU1.LD.128.U8.F16 v7    i7     || IAU.ADD i7  i7  1 || VAU.MACP.f16    v11 v0  || lsu0.swzv8 [22222222]
LSU1.LD.128.U8.F16 v8    i8     || IAU.ADD i8  i8  1 || VAU.MACP.f16    v12 v1  || lsu0.swzv8 [55555555]
LSU1.LD.128.U8.F16 v9    i9     || IAU.ADD i9  i9  1 || VAU.MACP.f16    v14 v2  || lsu0.swzv8 [00000000]
LSU1.LD.128.U8.F16 v10   i10    || IAU.ADD i10 i10 1 || VAU.MACP.f16    v15 v3  || lsu0.swzv8 [33333333]
                                                                           
LSU1.LD.128.U8.F16 v0    i0     || IAU.ADD i0  i0  1 || VAU.MACP.f16    v16 v4  || lsu0.swzv8 [66666666]
LSU1.LD.128.U8.F16 v1    i1     || IAU.ADD i1  i1  1 || VAU.MACP.f16    v18 v5  || lsu0.swzv8 [11111111] ;four
LSU1.LD.128.U8.F16 v2    i2     || IAU.ADD i2  i2  1 || VAU.MACP.f16    v19 v6  || lsu0.swzv8 [44444444]	
LSU1.LD.128.U8.F16 v3    i3     || IAU.ADD i3  i3  1 || VAU.MACP.f16    v20 v7  || lsu0.swzv8 [77777777]	
LSU1.LD.128.U8.F16 v4    i4     || IAU.ADD i4  i4  1 || VAU.MACP.f16    v22 v8  || lsu0.swzv8 [22222222]	
LSU1.LD.128.U8.F16 v5    i5     || IAU.ADD i5  i5  1 || VAU.MACP.f16    v23 v9  || lsu0.swzv8 [55555555]	                                                        
LSU1.LD.128.U8.F16 v6    i6     || IAU.ADD i6  i6  1 || VAU.MACP.f16    v25 v10 || lsu0.swzv8 [00000000]	                                         
LSU1.LD.128.U8.F16 v7    i7     || IAU.ADD i7  i7  1 || VAU.MACP.f16    v11 v0  || lsu0.swzv8 [33333333]
LSU1.LD.128.U8.F16 v8    i8     || IAU.ADD i8  i8  1 || VAU.MACP.f16    v12 v1  || lsu0.swzv8 [66666666]
LSU1.LD.128.U8.F16 v9    i9     || IAU.ADD i9  i9  1 || VAU.MACP.f16    v14 v2  || lsu0.swzv8 [11111111]
LSU1.LD.128.U8.F16 v10   i10    || IAU.ADD i10 i10 1 || VAU.MACP.f16    v15 v3  || lsu0.swzv8 [44444444]
                                                                           
LSU1.LD.128.U8.F16 v0    i0     || IAU.ADD i0  i0  1 || VAU.MACP.f16    v16 v4  || lsu0.swzv8 [77777777]
LSU1.LD.128.U8.F16 v1    i1     || IAU.ADD i1  i1  1 || VAU.MACP.f16    v18 v5  || lsu0.swzv8 [22222222] ;five
LSU1.LD.128.U8.F16 v2    i2     || IAU.ADD i2  i2  1 || VAU.MACP.f16    v19 v6  || lsu0.swzv8 [55555555]	
LSU1.LD.128.U8.F16 v3    i3     || IAU.ADD i3  i3  1 || VAU.MACP.f16    v21 v7  || lsu0.swzv8 [00000000]	
LSU1.LD.128.U8.F16 v4    i4     || IAU.ADD i4  i4  1 || VAU.MACP.f16    v22 v8  || lsu0.swzv8 [33333333]	
LSU1.LD.128.U8.F16 v5    i5     || IAU.ADD i5  i5  1 || VAU.MACP.f16    v23 v9  || lsu0.swzv8 [66666666]	                                                        
LSU1.LD.128.U8.F16 v6    i6     || IAU.ADD i6  i6  1 || VAU.MACP.f16    v25 v10 || lsu0.swzv8 [11111111]	                                         
LSU1.LD.128.U8.F16 v7    i7     || IAU.ADD i7  i7  1 || VAU.MACP.f16    v11 v0  || lsu0.swzv8 [44444444]
LSU1.LD.128.U8.F16 v8    i8     || IAU.ADD i8  i8  1 || VAU.MACP.f16    v12 v1  || lsu0.swzv8 [77777777]
LSU1.LD.128.U8.F16 v9    i9     || IAU.ADD i9  i9  1 || VAU.MACP.f16    v14 v2  || lsu0.swzv8 [22222222]
LSU1.LD.128.U8.F16 v10   i10    || IAU.ADD i10 i10 1 || VAU.MACP.f16    v15 v3  || lsu0.swzv8 [55555555]
                                                                           
LSU1.LD.128.U8.F16 v0    i0     || IAU.ADD i0  i0  1 || VAU.MACP.f16    v17 v4  || lsu0.swzv8 [00000000]
LSU1.LD.128.U8.F16 v1    i1     || IAU.ADD i1  i1  1 || VAU.MACP.f16    v18 v5  || lsu0.swzv8 [33333333] ;six
LSU1.LD.128.U8.F16 v2    i2     || IAU.ADD i2  i2  1 || VAU.MACP.f16    v19 v6  || lsu0.swzv8 [66666666]	
LSU1.LD.128.U8.F16 v3    i3     || IAU.ADD i3  i3  1 || VAU.MACP.f16    v21 v7  || lsu0.swzv8 [11111111]	
LSU1.LD.128.U8.F16 v4    i4     || IAU.ADD i4  i4  1 || VAU.MACP.f16    v22 v8  || lsu0.swzv8 [44444444]	
LSU1.LD.128.U8.F16 v5    i5     || IAU.ADD i5  i5  1 || VAU.MACP.f16    v23 v9  || lsu0.swzv8 [77777777]	                                                       
LSU1.LD.128.U8.F16 v6    i6     || IAU.ADD i6  i6  1 || VAU.MACP.f16    v25 v10 || lsu0.swzv8 [22222222]	                                         
LSU1.LD.128.U8.F16 v7    i7     || IAU.ADD i7  i7  1 || VAU.MACP.f16    v11 v0  || lsu0.swzv8 [55555555]
LSU1.LD.128.U8.F16 v8    i8     || IAU.ADD i8  i8  1 || VAU.MACP.f16    v13 v1  || lsu0.swzv8 [00000000]
LSU1.LD.128.U8.F16 v9    i9     || IAU.ADD i9  i9  1 || VAU.MACP.f16    v14 v2  || lsu0.swzv8 [33333333]
LSU1.LD.128.U8.F16 v10   i10    || IAU.ADD i10 i10 1 || VAU.MACP.f16    v15 v3  || lsu0.swzv8 [66666666]
                                                                           
LSU1.LD.128.U8.F16 v0    i0     || IAU.ADD i0  i0  1 || VAU.MACP.f16    v17 v4  || lsu0.swzv8 [11111111]
LSU1.LD.128.U8.F16 v1    i1     || IAU.ADD i1  i1  1 || VAU.MACP.f16    v18 v5  || lsu0.swzv8 [44444444] ;seven
LSU1.LD.128.U8.F16 v2    i2     || IAU.ADD i2  i2  1 || VAU.MACP.f16    v19 v6  || lsu0.swzv8 [77777777]	
LSU1.LD.128.U8.F16 v3    i3     || IAU.ADD i3  i3  1 || VAU.MACP.f16    v21 v7  || lsu0.swzv8 [22222222]	
LSU1.LD.128.U8.F16 v4    i4     || IAU.ADD i4  i4  1 || VAU.MACP.f16    v22 v8  || lsu0.swzv8 [55555555]	
LSU1.LD.128.U8.F16 v5    i5     || IAU.ADD i5  i5  1 || VAU.MACP.f16    v24 v9  || lsu0.swzv8 [00000000]	                                                        
LSU1.LD.128.U8.F16 v6    i6     || IAU.ADD i6  i6  1 || VAU.MACP.f16    v25 v10 || lsu0.swzv8 [33333333]	                                         
LSU1.LD.128.U8.F16 v7    i7     || IAU.ADD i7  i7  1 || VAU.MACP.f16    v11 v0  || lsu0.swzv8 [66666666]
LSU1.LD.128.U8.F16 v8    i8     || IAU.ADD i8  i8  1 || VAU.MACP.f16    v13 v1  || lsu0.swzv8 [11111111]
LSU1.LD.128.U8.F16 v9    i9     || IAU.ADD i9  i9  1 || VAU.MACP.f16    v14 v2  || lsu0.swzv8 [44444444]
LSU1.LD.128.U8.F16 v10   i10    || IAU.ADD i10 i10 1 || VAU.MACP.f16    v15 v3  || lsu0.swzv8 [77777777]
                                                                           
LSU1.LD.128.U8.F16 v0    i0     || IAU.ADD i0  i0  1 || VAU.MACP.f16    v17 v4  || lsu0.swzv8 [22222222]
LSU1.LD.128.U8.F16 v1    i1     || IAU.ADD i1  i1  1 || VAU.MACP.f16    v18 v5  || lsu0.swzv8 [55555555] ;eight
LSU1.LD.128.U8.F16 v2    i2     || IAU.ADD i2  i2  1 || VAU.MACP.f16    v20 v6  || lsu0.swzv8 [00000000]	
LSU1.LD.128.U8.F16 v3    i3     || IAU.ADD i3  i3  1 || VAU.MACP.f16    v21 v7  || lsu0.swzv8 [33333333]	
LSU1.LD.128.U8.F16 v4    i4     || IAU.ADD i4  i4  1 || VAU.MACP.f16    v22 v8  || lsu0.swzv8 [66666666]	
LSU1.LD.128.U8.F16 v5    i5     || IAU.ADD i5  i5  1 || VAU.MACP.f16    v24 v9  || lsu0.swzv8 [11111111]	                                                        
LSU1.LD.128.U8.F16 v6    i6     || IAU.ADD i6  i6  1 || VAU.MACP.f16    v25 v10 || lsu0.swzv8 [44444444]	                                         
LSU1.LD.128.U8.F16 v7    i7     || IAU.ADD i7  i7  1 || VAU.MACP.f16    v11 v0  || lsu0.swzv8 [77777777]
LSU1.LD.128.U8.F16 v8    i8     || IAU.ADD i8  i8  1 || VAU.MACP.f16    v13 v1  || lsu0.swzv8 [22222222]
LSU1.LD.128.U8.F16 v9    i9     || IAU.ADD i9  i9  1 || VAU.MACP.f16    v14 v2  || lsu0.swzv8 [55555555]
LSU1.LD.128.U8.F16 v10   i10    || IAU.ADD i10 i10 1 || VAU.MACP.f16    v16 v3  || lsu0.swzv8 [00000000]
                                                                          
LSU1.LD.128.U8.F16 v0    i0     || IAU.ADD i0  i0  1 || VAU.MACP.f16    v17 v4  || lsu0.swzv8 [33333333]
LSU1.LD.128.U8.F16 v1    i1     || IAU.ADD i1  i1  1 || VAU.MACP.f16    v18 v5  || lsu0.swzv8 [66666666] ;nine
LSU1.LD.128.U8.F16 v2    i2     || IAU.ADD i2  i2  1 || VAU.MACP.f16    v20 v6  || lsu0.swzv8 [11111111]	
LSU1.LD.128.U8.F16 v3    i3     || IAU.ADD i3  i3  1 || VAU.MACP.f16    v21 v7  || lsu0.swzv8 [44444444]	
LSU1.LD.128.U8.F16 v4    i4     || IAU.ADD i4  i4  1 || VAU.MACP.f16    v22 v8  || lsu0.swzv8 [77777777]	
LSU1.LD.128.U8.F16 v5    i5     || IAU.ADD i5  i5  1 || VAU.MACP.f16    v24 v9  || lsu0.swzv8 [22222222]	                                                        
LSU1.LD.128.U8.F16 v6    i6     || IAU.ADD i6  i6  1 || VAU.MACP.f16    v25 v10 || lsu0.swzv8 [55555555]	                                         
LSU1.LD.128.U8.F16 v7    i7     || IAU.ADD i7  i7  1 || VAU.MACP.f16    v12 v0  || lsu0.swzv8 [00000000]
LSU1.LD.128.U8.F16 v8    i8     || IAU.ADD i8  i8  1 || VAU.MACP.f16    v13 v1  || lsu0.swzv8 [33333333]
LSU1.LD.128.U8.F16 v9    i9     || IAU.ADD i9  i9  1 || VAU.MACP.f16    v14 v2  || lsu0.swzv8 [66666666]
LSU1.LD.128.U8.F16 v10   i10    || IAU.ADD i10 i10 1 || VAU.MACP.f16    v16 v3  || lsu0.swzv8 [11111111]
                                                                           
LSU1.LD.128.U8.F16 v0    i0     || IAU.ADD i0  i0  1 || VAU.MACP.f16    v17 v4  || lsu0.swzv8 [44444444]
LSU1.LD.128.U8.F16 v1    i1     || IAU.ADD i1  i1  1 || VAU.MACP.f16    v18 v5  || lsu0.swzv8 [77777777] ;ten
LSU1.LD.128.U8.F16 v2    i2     || IAU.ADD i2  i2  1 || VAU.MACP.f16    v20 v6  || lsu0.swzv8 [22222222]	
LSU1.LD.128.U8.F16 v3    i3     || IAU.ADD i3  i3  1 || VAU.MACP.f16    v21 v7  || lsu0.swzv8 [55555555]	
LSU1.LD.128.U8.F16 v4    i4     || IAU.ADD i4  i4  1 || VAU.MACP.f16    v23 v8  || lsu0.swzv8 [00000000]	
LSU1.LD.128.U8.F16 v5    i5     || IAU.ADD i5  i5  1 || VAU.MACP.f16    v24 v9  || lsu0.swzv8 [33333333]	                                                      
LSU1.LD.128.U8.F16 v6    i6     || IAU.ADD i6  i6  1 || VAU.MACP.f16    v25 v10 || lsu0.swzv8 [66666666]	                                         
LSU1.LD.128.U8.F16 v7    i7     || IAU.ADD i7  i7  1 || VAU.MACP.f16    v12 v0  || lsu0.swzv8 [11111111]
LSU1.LD.128.U8.F16 v8    i8     || IAU.ADD i8  i8  1 || VAU.MACP.f16    v13 v1  || lsu0.swzv8 [44444444]
LSU1.LD.128.U8.F16 v9    i9     || IAU.ADD i9  i9  1 || VAU.MACP.f16    v14 v2  || lsu0.swzv8 [77777777]
LSU1.LD.128.U8.F16 v10   i10    || IAU.ADD i10 i10 1 || VAU.MACP.f16    v16 v3  || lsu0.swzv8 [22222222]
                                                                           
LSU1.LD.128.U8.F16 v0    i0     || IAU.sub i0  i0  2 || VAU.MACP.f16    v17 v4  || lsu0.swzv8 [55555555]
LSU1.LD.128.U8.F16 v1    i1     || IAU.sub i1  i1  2 || VAU.MACP.f16    v19 v5  || lsu0.swzv8 [00000000] ;eleven
LSU1.LD.128.U8.F16 v2    i2     || IAU.sub i2  i2  2 || VAU.MACP.f16    v20 v6  || lsu0.swzv8 [33333333]	
LSU1.LD.128.U8.F16 v3    i3     || IAU.sub i3  i3  2 || VAU.MACP.f16    v21 v7  || lsu0.swzv8 [66666666]	
LSU1.LD.128.U8.F16 v4    i4     || IAU.sub i4  i4  2 || VAU.MACP.f16    v23 v8  || lsu0.swzv8 [11111111]	
LSU1.LD.128.U8.F16 v5    i5     || IAU.sub i5  i5  2 || VAU.MACP.f16    v24 v9  || lsu0.swzv8 [44444444]	                                               
LSU1.LD.128.U8.F16 v6    i6     || IAU.sub i6  i6  2 || VAU.MACP.f16    v25 v10 || lsu0.swzv8 [77777777]	                                         
LSU1.LD.128.U8.F16 v7    i7     || IAU.sub i7  i7  2 || VAU.MACP.f16    v12 v0  || lsu0.swzv8 [22222222]
LSU1.LD.128.U8.F16 v8    i8     || IAU.sub i8  i8  2 || VAU.MACP.f16    v13 v1  || lsu0.swzv8 [55555555]
LSU1.LD.128.U8.F16 v9    i9     || IAU.sub i9  i9  2 || VAU.MACP.f16    v15 v2  || lsu0.swzv8 [00000000]
LSU1.LD.128.U8.F16 v10   i10    || IAU.sub i10 i10 2 || VAU.MACP.f16    v16 v3  || lsu0.swzv8 [33333333]
VAU.MACP.f16          v17  v4  || lsu0.swzv8 [66666666]
VAU.MACP.f16          v19  v5  || lsu0.swzv8 [11111111]
VAU.MACP.f16          v20  v6  || lsu0.swzv8 [44444444]
VAU.MACP.f16          v21  v7  || lsu0.swzv8 [77777777]
VAU.MACP.f16          v23  v8  || lsu0.swzv8 [22222222]
VAU.MACP.f16          v24  v9  || lsu0.swzv8 [55555555]
VAU.MACPw.f16  v27    v26  v10 || lsu0.swzv8 [00000000]
LSU1.LD.128.U8.F16 v0    i0     || IAU.ADD i0  i0  1 
LSU1.LD.128.U8.F16 v1    i1     || IAU.ADD i1  i1  1	
LSU1.LD.128.U8.F16 v2    i2     || IAU.ADD i2  i2  1	
LSU1.LD.128.U8.F16 v3    i3     || IAU.ADD i3  i3  1	
LSU1.LD.128.U8.F16 v4    i4     || IAU.ADD i4  i4  1	
LSU1.LD.128.U8.F16 v5    i5     || IAU.ADD i5  i5  1
___forLoop:
LSU1.LD.128.U8.F16 v6    i6     || IAU.ADD i6  i6  1 	
nop 3
CMU.CLAMP0.F16 v27 v27 v28
nop
LSU0.STI.128.F16.U8 v27 i17
	

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
