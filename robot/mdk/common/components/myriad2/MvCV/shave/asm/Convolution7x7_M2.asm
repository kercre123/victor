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
	LSU0.ST.64.H v31  i19  || IAU.SUB i19 i19 8
	LSU0.ST.64.H v31  i19  


    LSU0.LD.32  i1  i18              || IAU.ADD i18 i18 4           
    lsu0.ldil i9, ___clampVal       ||  lsu1.ldih i9, ___clampVal
    LSU0.LDO.64.L v14 i16 0          || LSU1.LDO.64.H v14 i16 8      || IAU.ADD i16 i16 14
    LSU0.LD.32  i2  i18              || IAU.ADD i18 i18 4          
    LSU0.LDO.64.L v15 i16 0          || LSU1.LDO.64.H v15 i16 8      || IAU.ADD i16 i16 14
    LSU0.LD.32  i3  i18              || IAU.ADD i18 i18 4           || LSU1.LD.32  i17  i17
    LSU0.LDO.64.L v16 i16 0          || LSU1.LDO.64.H v16 i16 8      || IAU.ADD i16 i16 14
    LSU0.LD.32  i4  i18              || IAU.ADD i18 i18 4
    LSU0.LDO.64.L v17 i16 0          || LSU1.LDO.64.H v17 i16 8      || IAU.ADD i16 i16 14

	LSU0.LDIL i8 ___forLoop         || LSU1.LDIH i8 ___forLoop
    LSU0.LDO.64.L v18 i16 0          || LSU1.LDO.64.H v18 i16 8      || IAU.ADD i16 i16 14
    LSU0.LD.32  i5  i18              || IAU.ADD i18 i18 4
    LSU0.LDO.64.L v19 i16 0          || LSU1.LDO.64.H v19 i16 8      || IAU.ADD i16 i16 14
    LSU0.LD.16R v21, i9
    LSU0.LD.32  i6  i18              || IAU.ADD i18 i18 4
    LSU0.LDO.64.L v20 i16 0          || LSU1.LDO.64.H v20 i16 8
NOP
    LSU0.LD.32  i7  i18              || IAU.ADD i18 i18 4
IAU.SHR.u32 i15 i15 3 

	
	
	
	LSU1.LD.128.U8.F16 v0  i1     || IAU.ADD i1 i1 1 
	LSU1.LD.128.U8.F16 v1  i2     || IAU.ADD i2 i2 1
	LSU1.LD.128.U8.F16 v2  i3     || IAU.ADD i3 i3 1
	LSU1.LD.128.U8.F16 v3  i4     || IAU.ADD i4 i4 1 
	LSU1.LD.128.U8.F16 v4  i5     || IAU.ADD i5 i5 1 
	LSU1.LD.128.U8.F16 v5  i6     || IAU.ADD i6 i6 1 
	LSU1.LD.128.U8.F16 v6  i7     || IAU.ADD i7 i7 1 
	LSU1.LD.128.U8.F16 v7  i1     || IAU.ADD i1 i1 1 || VAU.MACPZ.f16        v14 v0  || lsu0.swzv8 [00000000] || bru.rpl i8 i15 ;first
	LSU1.LD.128.U8.F16 v8  i2     || IAU.ADD i2 i2 1 || VAU.MACP.f16         v15 v1  || lsu0.swzv8 [00000000] ;second
	LSU1.LD.128.U8.F16 v9  i3     || IAU.ADD i3 i3 1 || VAU.MACP.f16         v16 v2  || lsu0.swzv8 [00000000] 
	LSU1.LD.128.U8.F16 v10 i4     || IAU.ADD i4 i4 1 || VAU.MACP.f16         v17 v3  || lsu0.swzv8 [00000000]
	LSU1.LD.128.U8.F16 v11 i5     || IAU.ADD i5 i5 1 || VAU.MACP.f16         v18 v4  || lsu0.swzv8 [00000000]
	LSU1.LD.128.U8.F16 v12 i6     || IAU.ADD i6 i6 1 || VAU.MACP.f16         v19 v5  || lsu0.swzv8 [00000000]
	LSU1.LD.128.U8.F16 v13 i7     || IAU.ADD i7 i7 1 || VAU.MACP.f16         v20 v6  || lsu0.swzv8 [00000000]  
	LSU1.LD.128.U8.F16 v22 i1     || IAU.ADD i1 i1 1 || VAU.MACP.f16         v14 v7  || lsu0.swzv8 [11111111] 
	LSU1.LD.128.U8.F16 v23 i2     || IAU.ADD i2 i2 1 || VAU.MACP.f16         v15 v8  || lsu0.swzv8 [11111111]       ;third
	LSU1.LD.128.U8.F16 v24 i3     || IAU.ADD i3 i3 1 || VAU.MACP.f16         v16 v9  || lsu0.swzv8 [11111111] 
	LSU1.LD.128.U8.F16 v31 i4     || IAU.ADD i4 i4 1 || VAU.MACP.f16         v17 v10 || lsu0.swzv8 [11111111] 
	LSU1.LD.128.U8.F16 v0  i5     || IAU.ADD i5 i5 1 || VAU.MACP.f16         v18 v11 || lsu0.swzv8 [11111111] 
	LSU1.LD.128.U8.F16 v1  i6     || IAU.ADD i6 i6 1 || VAU.MACP.f16         v19 v12 || lsu0.swzv8 [11111111]    
	LSU1.LD.128.U8.F16 v2  i7     || IAU.ADD i7 i7 1 || VAU.MACP.f16         v20 v13 || lsu0.swzv8 [11111111]    
	LSU1.LD.128.U8.F16 v3  i1     || IAU.ADD i1 i1 1 || VAU.MACP.f16         v14 v22 || lsu0.swzv8 [22222222]
	LSU1.LD.128.U8.F16 v4  i2     || IAU.ADD i2 i2 1 || VAU.MACP.f16         v15 v23 || lsu0.swzv8 [22222222] ;fourth
	LSU1.LD.128.U8.F16 v5  i3     || IAU.ADD i3 i3 1 || VAU.MACP.f16         v16 v24 || lsu0.swzv8 [22222222] 
	LSU1.LD.128.U8.F16 v6  i4     || IAU.ADD i4 i4 1 || VAU.MACP.f16         v17 v31 || lsu0.swzv8 [22222222] 
	LSU1.LD.128.U8.F16 v7  i5     || IAU.ADD i5 i5 1 || VAU.MACP.f16         v18 v0  || lsu0.swzv8 [22222222] 
	LSU1.LD.128.U8.F16 v8  i6     || IAU.ADD i6 i6 1 || VAU.MACP.f16         v19 v1  || lsu0.swzv8 [22222222]
	LSU1.LD.128.U8.F16 v9  i7     || IAU.ADD i7 i7 1 || VAU.MACP.f16         v20 v2  || lsu0.swzv8 [22222222]
	LSU1.LD.128.U8.F16 v10 i1     || IAU.ADD i1 i1 1 || VAU.MACP.f16         v14 v3  || lsu0.swzv8 [33333333]	               
	LSU1.LD.128.U8.F16 v11 i2     || IAU.ADD i2 i2 1 || VAU.MACP.f16         v15 v4  || lsu0.swzv8 [33333333] ;fifth         
	LSU1.LD.128.U8.F16 v12 i3     || IAU.ADD i3 i3 1 || VAU.MACP.f16         v16 v5  || lsu0.swzv8 [33333333]       
	LSU1.LD.128.U8.F16 v13 i4     || IAU.ADD i4 i4 1 || VAU.MACP.f16         v17 v6  || lsu0.swzv8 [33333333]           
	LSU1.LD.128.U8.F16 v22 i5     || IAU.ADD i5 i5 1 || VAU.MACP.f16         v18 v7  || lsu0.swzv8 [33333333]           
	LSU1.LD.128.U8.F16 v23 i6     || IAU.ADD i6 i6 1 || VAU.MACP.f16         v19 v8  || lsu0.swzv8 [33333333]
	LSU1.LD.128.U8.F16 v24 i7     || IAU.ADD i7 i7 1 || VAU.MACP.f16         v20 v9  || lsu0.swzv8 [33333333] 
	LSU1.LD.128.U8.F16 v31 i1     || IAU.ADD i1 i1 1 || VAU.MACP.f16         v14 v10 || lsu0.swzv8 [44444444]                      
	LSU1.LD.128.U8.F16 v0  i2     || IAU.ADD i2 i2 1 || VAU.MACP.f16         v15 v11 || lsu0.swzv8 [44444444] ;sixth
	LSU1.LD.128.U8.F16 v1  i3     || IAU.ADD i3 i3 1 || VAU.MACP.f16         v16 v12 || lsu0.swzv8 [44444444]
	LSU1.LD.128.U8.F16 v2  i4     || IAU.ADD i4 i4 1 || VAU.MACP.f16         v17 v13 || lsu0.swzv8 [44444444]
	LSU1.LD.128.U8.F16 v3  i5     || IAU.ADD i5 i5 1 || VAU.MACP.f16         v18 v22 || lsu0.swzv8 [44444444]
	LSU1.LD.128.U8.F16 v4  i6     || IAU.ADD i6 i6 1 || VAU.MACP.f16         v19 v23 || lsu0.swzv8 [44444444] 
	LSU1.LD.128.U8.F16 v5  i7     || IAU.ADD i7 i7 1 || VAU.MACP.f16         v20 v24 || lsu0.swzv8 [44444444]         
	LSU1.LD.128.U8.F16 v6  i1     || IAU.ADD i1 i1 2 || VAU.MACP.f16         v14 v31 || lsu0.swzv8 [55555555] ;sixth      
	LSU1.LD.128.U8.F16 v7  i2     || IAU.ADD i2 i2 2 || VAU.MACP.f16         v15 v0  || lsu0.swzv8 [55555555];seventh
	LSU1.LD.128.U8.F16 v8  i3     || IAU.ADD i3 i3 2 || VAU.MACP.f16         v16 v1  || lsu0.swzv8 [55555555]
	LSU1.LD.128.U8.F16 v9  i4     || IAU.ADD i4 i4 2 || VAU.MACP.f16         v17 v2  || lsu0.swzv8 [55555555]
	LSU1.LD.128.U8.F16 v10 i5     || IAU.ADD i5 i5 2 || VAU.MACP.f16         v18 v3  || lsu0.swzv8 [55555555] 
	LSU1.LD.128.U8.F16 v11 i6     || IAU.ADD i6 i6 2 || VAU.MACP.f16         v19 v4  || lsu0.swzv8 [55555555] 
	LSU1.LD.128.U8.F16 v12 i7     || IAU.ADD i7 i7 2 || VAU.MACP.f16         v20 v5  || lsu0.swzv8 [55555555] 
	VAU.MACP.f16         v14 v6  || lsu0.swzv8 [66666666]
	VAU.MACP.f16         v15 v7  || lsu0.swzv8 [66666666]
	VAU.MACP.f16         v16 v8  || lsu0.swzv8 [66666666]
	VAU.MACP.f16         v17 v9  || lsu0.swzv8 [66666666]
	VAU.MACP.f16         v18 v10 || lsu0.swzv8 [66666666]
	VAU.MACP.f16         v19 v11 || lsu0.swzv8 [66666666]
	VAU.MACPW.f16   v30  v20 v12 || lsu0.swzv8 [66666666]
	LSU1.LD.128.U8.F16 v0  i1     || IAU.ADD i1 i1 1 
	LSU1.LD.128.U8.F16 v1  i2     || IAU.ADD i2 i2 1
	LSU1.LD.128.U8.F16 v2  i3     || IAU.ADD i3 i3 1
	LSU1.LD.128.U8.F16 v3  i4     || IAU.ADD i4 i4 1 
	LSU1.LD.128.U8.F16 v4  i5     || IAU.ADD i5 i5 1 
	LSU1.LD.128.U8.F16 v5  i6     || IAU.ADD i6 i6 1 
	___forLoop:
	LSU1.LD.128.U8.F16 v6  i7     || IAU.ADD i7 i7 1 
nop 3
	CMU.CLAMP0.F16 v30 v30 v21
nop
	LSU0.STI.128.F16.U8 v30 i17

  

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
