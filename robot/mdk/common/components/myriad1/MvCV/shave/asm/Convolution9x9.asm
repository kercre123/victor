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
lsu0.ld16r v10, i10
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

LSU0.LD32  i0  i18 || IAU.ADD i18 i18 4
LSU0.LD32  i1  i18 || IAU.ADD i18 i18 4
LSU0.LD32  i2  i18 || IAU.ADD i18 i18 4
LSU0.LD32  i3  i18 || IAU.ADD i18 i18 4
LSU0.LD32  i4  i18 || IAU.ADD i18 i18 4
LSU0.LD32  i5  i18 || IAU.ADD i18 i18 4
LSU0.LD32  i6  i18 || IAU.ADD i18 i18 4
LSU0.LD32  i7  i18 || IAU.ADD i18 i18 4
LSU0.LD32  i8  i18 || IAU.ADD i18 i18 4
LSU1.LD32 i17 i17
lsu0.ld16r v31, i10
LSU0.LDO64.l v10 i16 0          || LSU1.LDO64.h v10 i16 8      || IAU.ADD i16 i16 16
LSU0.LDO64.l v11 i16 0          || LSU1.LDO64.h v11 i16 8      || IAU.ADD i16 i16 16
LSU0.LDO64.l v12 i16 0          || LSU1.LDO64.h v12 i16 8      || IAU.ADD i16 i16 16
LSU0.LDO64.l v13 i16 0          || LSU1.LDO64.h v13 i16 8      || IAU.ADD i16 i16 16
LSU0.LDO64.l v14 i16 0          || LSU1.LDO64.h v14 i16 8      || IAU.ADD i16 i16 16
LSU0.LDO64.l v15 i16 0          || LSU1.LDO64.h v15 i16 8      || IAU.ADD i16 i16 16
LSU0.LDO64.l v16 i16 0          || LSU1.LDO64.h v16 i16 8      || IAU.ADD i16 i16 16
LSU0.LDO64.l v17 i16 0          || LSU1.LDO64.h v17 i16 8      || IAU.ADD i16 i16 16
LSU0.LDO64.l v18 i16 0          || LSU1.LDO64.h v18 i16 8      || IAU.ADD i16 i16 16
LSU0.LDO64.l v19 i16 0          || LSU1.LDO64.h v19 i16 8      || IAU.ADD i16 i16 16
LSU0.LDO64.l v20 i16 0          || LSU1.LDO64.h v20 i16 8      || IAU.ADD i16 i16 16

IAU.SHR.u32 i15 i15 3 
	LSU0.LDIL i10 ___forLoop         || LSU1.LDIH i10 ___forLoop
LSU1.LD128.u8.f16 v0   i0     || IAU.ADD i0 i0 1 
LSU1.LD128.u8.f16 v1   i1     || IAU.ADD i1 i1 1
LSU1.LD128.u8.f16 v2   i2     || IAU.ADD i2 i2 1
LSU1.LD128.u8.f16 v3   i3     || IAU.ADD i3 i3 1
LSU1.LD128.u8.f16 v4   i4     || IAU.ADD i4 i4 1
LSU1.LD128.u8.f16 v5   i5     || IAU.ADD i5 i5 1 
LSU1.LD128.u8.f16 v6   i6     || IAU.ADD i6 i6 1 || VAU.MACPZ.f16        v0   v10 || lsu0.swzv8 [00000000] || bru.rpl i10 i15;one
LSU1.LD128.u8.f16 v7   i7     || IAU.ADD i7 i7 1 || VAU.MACP.f16         v1   v11 || lsu0.swzv8 [11111111]
LSU1.LD128.u8.f16 v8   i8     || IAU.ADD i8 i8 1 || VAU.MACP.f16         v2   v12 || lsu0.swzv8 [22222222]
LSU1.LD128.u8.f16 v9   i0     || IAU.ADD i0 i0 1 || VAU.MACP.f16         v3   v13 || lsu0.swzv8 [33333333];two
LSU1.LD128.u8.f16 v21  i1     || IAU.ADD i1 i1 1 || VAU.MACP.f16         v4   v14 || lsu0.swzv8 [44444444]
LSU1.LD128.u8.f16 v22  i2     || IAU.ADD i2 i2 1 || VAU.MACP.f16         v5   v15 || lsu0.swzv8 [55555555]
LSU1.LD128.u8.f16 v23  i3     || IAU.ADD i3 i3 1 || VAU.MACP.f16         v6   v16 || lsu0.swzv8 [66666666]
LSU1.LD128.u8.f16 v24  i4     || IAU.ADD i4 i4 1 || VAU.MACP.f16         v7   v17 || lsu0.swzv8 [77777777]
LSU1.LD128.u8.f16 v25  i5     || IAU.ADD i5 i5 1 || VAU.MACP.f16         v8   v19 || lsu0.swzv8 [00000000] ;*******
LSU1.LD128.u8.f16 v26  i6     || IAU.ADD i6 i6 1 || VAU.MACP.f16         v9   v10 || lsu0.swzv8 [11111111]
LSU1.LD128.u8.f16 v27  i7     || IAU.ADD i7 i7 1 || VAU.MACP.f16         v21  v11 || lsu0.swzv8 [22222222]
LSU1.LD128.u8.f16 v28  i8     || IAU.ADD i8 i8 1 || VAU.MACP.f16         v22  v12 || lsu0.swzv8 [33333333]
LSU1.LD128.u8.f16 v29  i0     || IAU.ADD i0 i0 1 || VAU.MACP.f16         v23  v13 || lsu0.swzv8 [44444444];three
LSU1.LD128.u8.f16 v30  i1     || IAU.ADD i1 i1 1 || VAU.MACP.f16         v24  v14 || lsu0.swzv8 [55555555]
LSU1.LD128.u8.f16 v0   i2     || IAU.ADD i2 i2 1 || VAU.MACP.f16         v25  v15 || lsu0.swzv8 [66666666]
LSU1.LD128.u8.f16 v1   i3     || IAU.ADD i3 i3 1 || VAU.MACP.f16         v26  v16 || lsu0.swzv8 [77777777]
LSU1.LD128.u8.f16 v2   i4     || IAU.ADD i4 i4 1 || VAU.MACP.f16         v27  v18 || lsu0.swzv8 [00000000]
LSU1.LD128.u8.f16 v3   i5     || IAU.ADD i5 i5 1 || VAU.MACP.f16         v28  v19 || lsu0.swzv8 [11111111] ;*******
LSU1.LD128.u8.f16 v4   i6     || IAU.ADD i6 i6 1 || VAU.MACP.f16         v29  v10 || lsu0.swzv8 [22222222]
LSU1.LD128.u8.f16 v5   i7     || IAU.ADD i7 i7 1 || VAU.MACP.f16         v30  v11 || lsu0.swzv8 [33333333]
LSU1.LD128.u8.f16 v6   i8     || IAU.ADD i8 i8 1 || VAU.MACP.f16         v0   v12 || lsu0.swzv8 [44444444]
LSU1.LD128.u8.f16 v7   i0     || IAU.ADD i0 i0 1 || VAU.MACP.f16         v1   v13 || lsu0.swzv8 [55555555];four
LSU1.LD128.u8.f16 v8   i1     || IAU.ADD i1 i1 1 || VAU.MACP.f16         v2   v14 || lsu0.swzv8 [66666666]
LSU1.LD128.u8.f16 v9   i2     || IAU.ADD i2 i2 1 || VAU.MACP.f16         v3   v15 || lsu0.swzv8 [77777777]
LSU1.LD128.u8.f16 v21  i3     || IAU.ADD i3 i3 1 || VAU.MACP.f16         v4   v17 || lsu0.swzv8 [00000000]
LSU1.LD128.u8.f16 v22  i4     || IAU.ADD i4 i4 1 || VAU.MACP.f16         v5   v18 || lsu0.swzv8 [11111111]
LSU1.LD128.u8.f16 v23  i5     || IAU.ADD i5 i5 1 || VAU.MACP.f16         v6   v19 || lsu0.swzv8 [22222222] ;*******
LSU1.LD128.u8.f16 v24  i6     || IAU.ADD i6 i6 1 || VAU.MACP.f16         v7   v10 || lsu0.swzv8 [33333333]
LSU1.LD128.u8.f16 v25  i7     || IAU.ADD i7 i7 1 || VAU.MACP.f16         v8   v11 || lsu0.swzv8 [44444444]
LSU1.LD128.u8.f16 v26  i8     || IAU.ADD i8 i8 1 || VAU.MACP.f16         v9   v12 || lsu0.swzv8 [55555555]
LSU1.LD128.u8.f16 v27  i0     || IAU.ADD i0 i0 1 || VAU.MACP.f16         v21  v13 || lsu0.swzv8 [66666666];five
LSU1.LD128.u8.f16 v28  i1     || IAU.ADD i1 i1 1 || VAU.MACP.f16         v22  v14 || lsu0.swzv8 [77777777]
LSU1.LD128.u8.f16 v29  i2     || IAU.ADD i2 i2 1 || VAU.MACP.f16         v23  v16 || lsu0.swzv8 [00000000]
LSU1.LD128.u8.f16 v30  i3     || IAU.ADD i3 i3 1 || VAU.MACP.f16         v24  v17 || lsu0.swzv8 [11111111]
LSU1.LD128.u8.f16 v0   i4     || IAU.ADD i4 i4 1 || VAU.MACP.f16         v25  v18 || lsu0.swzv8 [22222222]
LSU1.LD128.u8.f16 v1   i5     || IAU.ADD i5 i5 1 || VAU.MACP.f16         v26  v19 || lsu0.swzv8 [33333333] ;*******
LSU1.LD128.u8.f16 v2   i6     || IAU.ADD i6 i6 1 || VAU.MACP.f16         v27  v10 || lsu0.swzv8 [44444444]
LSU1.LD128.u8.f16 v3   i7     || IAU.ADD i7 i7 1 || VAU.MACP.f16         v28  v11 || lsu0.swzv8 [55555555]
LSU1.LD128.u8.f16 v4   i8     || IAU.ADD i8 i8 1 || VAU.MACP.f16         v29  v12 || lsu0.swzv8 [66666666]
LSU1.LD128.u8.f16 v5   i0     || IAU.ADD i0 i0 1 || VAU.MACP.f16         v30  v13 || lsu0.swzv8 [77777777];six
LSU1.LD128.u8.f16 v6   i1     || IAU.ADD i1 i1 1 || VAU.MACP.f16         v0   v15 || lsu0.swzv8 [00000000]
LSU1.LD128.u8.f16 v7   i2     || IAU.ADD i2 i2 1 || VAU.MACP.f16         v1   v16 || lsu0.swzv8 [11111111]
LSU1.LD128.u8.f16 v8   i3     || IAU.ADD i3 i3 1 || VAU.MACP.f16         v2   v17 || lsu0.swzv8 [22222222]
LSU1.LD128.u8.f16 v9   i4     || IAU.ADD i4 i4 1 || VAU.MACP.f16         v3   v18 || lsu0.swzv8 [33333333]
LSU1.LD128.u8.f16 v21  i5     || IAU.ADD i5 i5 1 || VAU.MACP.f16         v4   v19 || lsu0.swzv8 [44444444] ;*******
LSU1.LD128.u8.f16 v22  i6     || IAU.ADD i6 i6 1 || VAU.MACP.f16         v5   v10 || lsu0.swzv8 [55555555]
LSU1.LD128.u8.f16 v23  i7     || IAU.ADD i7 i7 1 || VAU.MACP.f16         v6   v11 || lsu0.swzv8 [66666666]
LSU1.LD128.u8.f16 v24  i8     || IAU.ADD i8 i8 1 || VAU.MACP.f16         v7   v12 || lsu0.swzv8 [77777777]
LSU1.LD128.u8.f16 v25  i0     || IAU.ADD i0 i0 1 || VAU.MACP.f16         v8   v14 || lsu0.swzv8 [00000000];seven
LSU1.LD128.u8.f16 v26  i1     || IAU.ADD i1 i1 1 || VAU.MACP.f16         v9   v15 || lsu0.swzv8 [11111111]
LSU1.LD128.u8.f16 v27  i2     || IAU.ADD i2 i2 1 || VAU.MACP.f16         v21  v16 || lsu0.swzv8 [22222222]
LSU1.LD128.u8.f16 v28  i3     || IAU.ADD i3 i3 1 || VAU.MACP.f16         v22  v17 || lsu0.swzv8 [33333333]
LSU1.LD128.u8.f16 v29  i4     || IAU.ADD i4 i4 1 || VAU.MACP.f16         v23  v18 || lsu0.swzv8 [44444444]
LSU1.LD128.u8.f16 v30  i5     || IAU.ADD i5 i5 1 || VAU.MACP.f16         v24  v19 || lsu0.swzv8 [55555555] ;*******
LSU1.LD128.u8.f16 v0   i6     || IAU.ADD i6 i6 1 || VAU.MACP.f16         v25  v10 || lsu0.swzv8 [66666666]
LSU1.LD128.u8.f16 v1   i7     || IAU.ADD i7 i7 1 || VAU.MACP.f16         v26  v11 || lsu0.swzv8 [77777777]
LSU1.LD128.u8.f16 v2   i8     || IAU.ADD i8 i8 1 || VAU.MACP.f16         v27  v13 || lsu0.swzv8 [00000000]
LSU1.LD128.u8.f16 v3   i0     || IAU.ADD i0 i0 1 || VAU.MACP.f16         v28  v14 || lsu0.swzv8 [11111111];eight
LSU1.LD128.u8.f16 v4   i1     || IAU.ADD i1 i1 1 || VAU.MACP.f16         v29  v15 || lsu0.swzv8 [22222222]
LSU1.LD128.u8.f16 v5   i2     || IAU.ADD i2 i2 1 || VAU.MACP.f16         v30  v16 || lsu0.swzv8 [33333333]
LSU1.LD128.u8.f16 v6   i3     || IAU.ADD i3 i3 1 || VAU.MACP.f16         v0   v17 || lsu0.swzv8 [44444444]
LSU1.LD128.u8.f16 v7   i4     || IAU.ADD i4 i4 1 || VAU.MACP.f16         v1   v18 || lsu0.swzv8 [55555555]
LSU1.LD128.u8.f16 v8   i5     || IAU.ADD i5 i5 1 || VAU.MACP.f16         v2   v19 || lsu0.swzv8 [66666666] ;*******
LSU1.LD128.u8.f16 v9   i6     || IAU.ADD i6 i6 1 || VAU.MACP.f16         v3   v10 || lsu0.swzv8 [77777777]
LSU1.LD128.u8.f16 v21  i7     || IAU.ADD i7 i7 1 || VAU.MACP.f16         v4   v12 || lsu0.swzv8 [00000000]
LSU1.LD128.u8.f16 v22  i8     || IAU.ADD i8 i8 1 || VAU.MACP.f16         v5   v13 || lsu0.swzv8 [11111111]
LSU1.LD128.u8.f16 v23  i0     || IAU.ADD i0 i0 0 || VAU.MACP.f16         v6   v14 || lsu0.swzv8 [22222222];nine
LSU1.LD128.u8.f16 v24  i1     || IAU.ADD i1 i1 0 || VAU.MACP.f16         v7   v15 || lsu0.swzv8 [33333333]
LSU1.LD128.u8.f16 v25  i2     || IAU.ADD i2 i2 0 || VAU.MACP.f16         v8   v16 || lsu0.swzv8 [44444444]
LSU1.LD128.u8.f16 v26  i3     || IAU.ADD i3 i3 0 || VAU.MACP.f16         v9   v17 || lsu0.swzv8 [55555555]
LSU1.LD128.u8.f16 v27  i4     || IAU.ADD i4 i4 0 || VAU.MACP.f16         v21  v18 || lsu0.swzv8 [66666666]
LSU1.LD128.u8.f16 v28  i5     || IAU.ADD i5 i5 0 || VAU.MACP.f16         v22  v19 || lsu0.swzv8 [77777777] ;*******
LSU1.LD128.u8.f16 v29  i6     || IAU.ADD i6 i6 0 || VAU.MACP.f16         v23  v11 || lsu0.swzv8 [00000000]
LSU1.LD128.u8.f16 v30  i7     || IAU.ADD i7 i7 0 || VAU.MACP.f16         v24  v12 || lsu0.swzv8 [11111111]
LSU1.LD128.u8.f16 v0   i8     || IAU.ADD i8 i8 0 || VAU.MACP.f16         v25  v13 || lsu0.swzv8 [22222222]
VAU.MACP.f16         v26  v14 || lsu0.swzv8 [33333333]                   
VAU.MACP.f16         v27  v15 || lsu0.swzv8 [44444444]                   
VAU.MACP.f16         v28  v16 || lsu0.swzv8 [55555555]                   
VAU.MACP.f16         v29  v17 || lsu0.swzv8 [66666666]                   
VAU.MACP.f16         v30  v18 || lsu0.swzv8 [77777777]                   
VAU.MACPW.f16 v26    v0   v20 || lsu0.swzv8 [00000000] ;*******          
LSU1.LD128.u8.f16 v0   i0     || IAU.ADD i0 i0 1 
LSU1.LD128.u8.f16 v1   i1     || IAU.ADD i1 i1 1
LSU1.LD128.u8.f16 v2   i2     || IAU.ADD i2 i2 1
LSU1.LD128.u8.f16 v3   i3     || IAU.ADD i3 i3 1
LSU1.LD128.u8.f16 v4   i4     || IAU.ADD i4 i4 1
LSU1.LD128.u8.f16 v5   i5     || IAU.ADD i5 i5 1 
___forLoop:
nop 3
CMU.CLAMP0.F16 v26 v26 v31
nop
LSU0.STi128.f16.u8 v26 i17
													  
                                                                                                 
                                                                                                 
                                                                                                 
                                                                                               


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
    BRU.jmp i30
    nop 5

.end
 
