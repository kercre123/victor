; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief
; ///

.version 00.51.04

.data .data.convolution7x7

.salign 16
    ___clampVal:
        .float16 255.0


.code .text.convolution7x7

;void Convolution7x7_asm(u8** in(i18), u8** out(i17), half conv[49](i16), u32 inWidth(i15))
Convolution7x7_asm:
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
	LSU0.ST64.h v31  i19  || IAU.SUB i19 i19 8
	LSU0.ST64.h v31  i19  


    LSU0.LD32  i1  i18              || IAU.ADD i18 i18 4           
    lsu0.ldil i9, ___clampVal       ||  lsu1.ldih i9, ___clampVal
    LSU0.LDO64.l v14 i16 0          || LSU1.LDO64.h v14 i16 8      || IAU.ADD i16 i16 14
    LSU0.LD32  i2  i18              || IAU.ADD i18 i18 4          
    LSU0.LDO64.l v15 i16 0          || LSU1.LDO64.h v15 i16 8      || IAU.ADD i16 i16 14
    LSU0.LD32  i3  i18              || IAU.ADD i18 i18 4           || LSU1.LD32  i17  i17
    LSU0.LDO64.l v16 i16 0          || LSU1.LDO64.h v16 i16 8      || IAU.ADD i16 i16 14
    LSU0.LD32  i4  i18              || IAU.ADD i18 i18 4
    LSU0.LDO64.l v17 i16 0          || LSU1.LDO64.h v17 i16 8      || IAU.ADD i16 i16 14

	LSU0.LDIL i8 ___forLoop         || LSU1.LDIH i8 ___forLoop
    LSU0.LDO64.l v18 i16 0          || LSU1.LDO64.h v18 i16 8      || IAU.ADD i16 i16 14
    LSU0.LD32  i5  i18              || IAU.ADD i18 i18 4
    LSU0.LDO64.l v19 i16 0          || LSU1.LDO64.h v19 i16 8      || IAU.ADD i16 i16 14
    lsu0.ld16r v21, i9
    LSU0.LD32  i6  i18              || IAU.ADD i18 i18 4
    LSU0.LDO64.l v20 i16 0          || LSU1.LDO64.h v20 i16 8
    NOP
    LSU0.LD32  i7  i18              || IAU.ADD i18 i18 4
IAU.SHR.u32 i15 i15 3 

	
	
	
	LSU1.LD128.u8.f16 v0  i1     || IAU.ADD i1 i1 1 
	LSU1.LD128.u8.f16 v1  i2     || IAU.ADD i2 i2 1
	LSU1.LD128.u8.f16 v2  i3     || IAU.ADD i3 i3 1
	LSU1.LD128.u8.f16 v3  i4     || IAU.ADD i4 i4 1 
	LSU1.LD128.u8.f16 v4  i5     || IAU.ADD i5 i5 1 
	LSU1.LD128.u8.f16 v5  i6     || IAU.ADD i6 i6 1 
	LSU1.LD128.u8.f16 v6  i7     || IAU.ADD i7 i7 1 || VAU.MACPZ.f16        v0  v14 || lsu0.swzv8 [00000000] || bru.rpl i8 i15 ;first
	LSU1.LD128.u8.f16 v7  i1     || IAU.ADD i1 i1 1 || VAU.MACP.f16         v1  v15 || lsu0.swzv8 [00000000] ;second
	LSU1.LD128.u8.f16 v8  i2     || IAU.ADD i2 i2 1 || VAU.MACP.f16         v2  v16 || lsu0.swzv8 [00000000] 
	LSU1.LD128.u8.f16 v9  i3     || IAU.ADD i3 i3 1 || VAU.MACP.f16         v3  v17 || lsu0.swzv8 [00000000]
	LSU1.LD128.u8.f16 v10 i4     || IAU.ADD i4 i4 1 || VAU.MACP.f16         v4  v18 || lsu0.swzv8 [00000000]
	LSU1.LD128.u8.f16 v11 i5     || IAU.ADD i5 i5 1 || VAU.MACP.f16         v5  v19 || lsu0.swzv8 [00000000]
	LSU1.LD128.u8.f16 v12 i6     || IAU.ADD i6 i6 1 || VAU.MACP.f16         v6  v20 || lsu0.swzv8 [00000000]  
	LSU1.LD128.u8.f16 v13 i7     || IAU.ADD i7 i7 1 || VAU.MACP.f16         v7  v14 || lsu0.swzv8 [11111111] 
	LSU1.LD128.u8.f16 v22 i1     || IAU.ADD i1 i1 1 || VAU.MACP.f16         v8  v15 || lsu0.swzv8 [11111111]       ;third
	LSU1.LD128.u8.f16 v23 i2     || IAU.ADD i2 i2 1 || VAU.MACP.f16         v9  v16 || lsu0.swzv8 [11111111] 
	LSU1.LD128.u8.f16 v24 i3     || IAU.ADD i3 i3 1 || VAU.MACP.f16         v10 v17 || lsu0.swzv8 [11111111] 
	LSU1.LD128.u8.f16 v31 i4     || IAU.ADD i4 i4 1 || VAU.MACP.f16         v11 v18 || lsu0.swzv8 [11111111] 
	LSU1.LD128.u8.f16 v0  i5     || IAU.ADD i5 i5 1 || VAU.MACP.f16         v12 v19 || lsu0.swzv8 [11111111]    
	LSU1.LD128.u8.f16 v1  i6     || IAU.ADD i6 i6 1 || VAU.MACP.f16         v13 v20 || lsu0.swzv8 [11111111]    
	LSU1.LD128.u8.f16 v2  i7     || IAU.ADD i7 i7 1 || VAU.MACP.f16         v22 v14 || lsu0.swzv8 [22222222]
	LSU1.LD128.u8.f16 v3  i1     || IAU.ADD i1 i1 1 || VAU.MACP.f16         v23 v15 || lsu0.swzv8 [22222222] ;fourth
	LSU1.LD128.u8.f16 v4  i2     || IAU.ADD i2 i2 1 || VAU.MACP.f16         v24 v16 || lsu0.swzv8 [22222222] 
	LSU1.LD128.u8.f16 v5  i3     || IAU.ADD i3 i3 1 || VAU.MACP.f16         v31 v17 || lsu0.swzv8 [22222222] 
	LSU1.LD128.u8.f16 v6  i4     || IAU.ADD i4 i4 1 || VAU.MACP.f16         v0  v18 || lsu0.swzv8 [22222222] 
	LSU1.LD128.u8.f16 v7  i5     || IAU.ADD i5 i5 1 || VAU.MACP.f16         v1  v19 || lsu0.swzv8 [22222222]
	LSU1.LD128.u8.f16 v8  i6     || IAU.ADD i6 i6 1 || VAU.MACP.f16         v2  v20 || lsu0.swzv8 [22222222]
	LSU1.LD128.u8.f16 v9  i7     || IAU.ADD i7 i7 1 || VAU.MACP.f16         v3  v14 || lsu0.swzv8 [33333333]	               
	LSU1.LD128.u8.f16 v10 i1     || IAU.ADD i1 i1 1 || VAU.MACP.f16         v4  v15 || lsu0.swzv8 [33333333] ;fifth         
	LSU1.LD128.u8.f16 v11 i2     || IAU.ADD i2 i2 1 || VAU.MACP.f16         v5  v16 || lsu0.swzv8 [33333333]       
	LSU1.LD128.u8.f16 v12 i3     || IAU.ADD i3 i3 1 || VAU.MACP.f16         v6  v17 || lsu0.swzv8 [33333333]           
	LSU1.LD128.u8.f16 v13 i4     || IAU.ADD i4 i4 1 || VAU.MACP.f16         v7  v18 || lsu0.swzv8 [33333333]           
	LSU1.LD128.u8.f16 v22 i5     || IAU.ADD i5 i5 1 || VAU.MACP.f16         v8  v19 || lsu0.swzv8 [33333333]
	LSU1.LD128.u8.f16 v23 i6     || IAU.ADD i6 i6 1 || VAU.MACP.f16         v9  v20 || lsu0.swzv8 [33333333] 
	LSU1.LD128.u8.f16 v24 i7     || IAU.ADD i7 i7 1 || VAU.MACP.f16         v10 v14 || lsu0.swzv8 [44444444]                      
	LSU1.LD128.u8.f16 v31 i1     || IAU.ADD i1 i1 1 || VAU.MACP.f16         v11 v15 || lsu0.swzv8 [44444444] ;sixth
	LSU1.LD128.u8.f16 v0  i2     || IAU.ADD i2 i2 1 || VAU.MACP.f16         v12 v16 || lsu0.swzv8 [44444444]
	LSU1.LD128.u8.f16 v1  i3     || IAU.ADD i3 i3 1 || VAU.MACP.f16         v13 v17 || lsu0.swzv8 [44444444]
	LSU1.LD128.u8.f16 v2  i4     || IAU.ADD i4 i4 1 || VAU.MACP.f16         v22 v18 || lsu0.swzv8 [44444444]
	LSU1.LD128.u8.f16 v3  i5     || IAU.ADD i5 i5 1 || VAU.MACP.f16         v23 v19 || lsu0.swzv8 [44444444] 
	LSU1.LD128.u8.f16 v4  i6     || IAU.ADD i6 i6 1 || VAU.MACP.f16         v24 v20 || lsu0.swzv8 [44444444]         
	LSU1.LD128.u8.f16 v5  i7     || IAU.ADD i7 i7 1 || VAU.MACP.f16         v31 v14 || lsu0.swzv8 [55555555] ;sixth      
	LSU1.LD128.u8.f16 v6  i1     || IAU.ADD i1 i1 2 || VAU.MACP.f16         v0  v15 || lsu0.swzv8 [55555555];seventh
	LSU1.LD128.u8.f16 v7  i2     || IAU.ADD i2 i2 2 || VAU.MACP.f16         v1  v16 || lsu0.swzv8 [55555555]
	LSU1.LD128.u8.f16 v8  i3     || IAU.ADD i3 i3 2 || VAU.MACP.f16         v2  v17 || lsu0.swzv8 [55555555]
	LSU1.LD128.u8.f16 v9  i4     || IAU.ADD i4 i4 2 || VAU.MACP.f16         v3  v18 || lsu0.swzv8 [55555555] 
	LSU1.LD128.u8.f16 v10 i5     || IAU.ADD i5 i5 2 || VAU.MACP.f16         v4  v19 || lsu0.swzv8 [55555555] 
	LSU1.LD128.u8.f16 v11 i6     || IAU.ADD i6 i6 2 || VAU.MACP.f16         v5  v20 || lsu0.swzv8 [55555555] 
	LSU1.LD128.u8.f16 v12 i7     || IAU.ADD i7 i7 2 || VAU.MACP.f16         v6  v14 || lsu0.swzv8 [66666666]
	VAU.MACP.f16         v7  v15 || lsu0.swzv8 [66666666]
	VAU.MACP.f16         v8  v16 || lsu0.swzv8 [66666666]
	VAU.MACP.f16         v9  v17 || lsu0.swzv8 [66666666]
	VAU.MACP.f16         v10 v18 || lsu0.swzv8 [66666666]
	VAU.MACP.f16         v11 v19 || lsu0.swzv8 [66666666]
	VAU.MACPW.f16   v30  v12 v20 || lsu0.swzv8 [66666666]
	LSU1.LD128.u8.f16 v0  i1     || IAU.ADD i1 i1 1 
	LSU1.LD128.u8.f16 v1  i2     || IAU.ADD i2 i2 1
	LSU1.LD128.u8.f16 v2  i3     || IAU.ADD i3 i3 1
	LSU1.LD128.u8.f16 v3  i4     || IAU.ADD i4 i4 1 
	LSU1.LD128.u8.f16 v4  i5     || IAU.ADD i5 i5 1 
	LSU1.LD128.u8.f16 v5  i6     || IAU.ADD i6 i6 1 
	___forLoop:
	nop 3
	CMU.CLAMP0.F16 v30 v30 v21
	nop
	LSU0.STi128.f16.u8 v30 i17

  

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
 
