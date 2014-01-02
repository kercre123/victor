; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013  all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief
; ///

.version 00.51.04

.data .data.convolution5x5

.salign 16
    ___clampVal:
        .float16 255.0

.code .text.convolution5x5

;void Convolution5x5_asm(u8** in(i18)  u8** out(i17)  half conv[25](i16)  u32 inWidth(i15))
Convolution5x5_asm: 
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
	LSU0.ST64.h v28  i19  

    LSU0.LD32 i1 i18            || IAU.ADD i18 i18 4
    lsu0.ldil i6 ___clampVal    || lsu1.ldih i6 ___clampVal
    LSU0.LDO64.l v10 i16 0      || LSU1.LDO64.h v10 i16 8      || IAU.ADD i16 i16 10
    LSU0.LD32 i2 i18            || IAU.ADD i18 i18 4
    LSU0.LDO64.l v11 i16 0      || LSU1.LDO64.h v11 i16 8      || IAU.ADD i16 i16 10
    LSU0.LD32 i3 i18            || IAU.ADD i18 i18 4           || LSU1.LD32 i17 i17
    LSU0.LDO64.l v12 i16 0      || LSU1.LDO64.h v12 i16 8      || IAU.ADD i16 i16 10
    LSU0.LD32 i4 i18            || IAU.ADD i18 i18 4
    LSU0.LDO64.l v13 i16 0      || LSU1.LDO64.h v13 i16 8      || IAU.ADD i16 i16 10
    lsu0.ld16r v22  i6
    LSU0.LDO64.l v14 i16 0      || LSU1.LDO64.h v14 i16 8      || IAU.ADD i16 i16 10
    LSU0.LD32 i5 i18            || IAU.ADD i18 i18 4

    IAU.SHR.u32 i15 i15 3   ; i15 = i15/8 because we process 8 pixels at once
	LSU0.LDIL i16 ___forLoop    || LSU1.LDIH i16 ___forLoop
	
    LSU0.LD128.u8.f16 v0 i1     || IAU.ADD i1 i1 8
    LSU0.LD128.u8.f16 v1 i2     || IAU.ADD i2 i2 8
    LSU0.LD128.u8.f16 v5 i1
    LSU0.LD128.u8.f16 v2 i3     || IAU.ADD i3 i3 8
    LSU0.LD128.u8.f16 v3 i4     || IAU.ADD i4 i4 8
    LSU0.LD128.u8.f16 v4 i5     || IAU.ADD i5 i5 8
    LSU0.LD128.u8.f16 v6 i2
    nop 2
    LSU0.LD128.u8.f16 v7 i3     || SAU.SUM.u32 i7  v5  0x01
    nop 1
    LSU0.LD128.u8.f16 v8 i4     || SAU.SUM.u32 i8  v5  0x02
    LSU0.LD128.u8.f16 v9 i5     || SAU.SUM.u32 i9  v6  0x01    || cmu.vrot v5 v0 2
    LSU0.LDIL i16 ___forLoop    || LSU1.LDIH i16 ___forLoop

    SAU.SUM.u32 i10  v6  0x02   || cmu.cpiv.x16 v5.7 i7.l
    SAU.SUM.u32 i11  v7  0x01   || cmu.vrot v6 v5 2
    SAU.SUM.u32 i12  v7  0x02   || cmu.cpiv.x16 v6.7 i7.h
    SAU.SUM.u32 i13  v8  0x01   || cmu.vrot v7 v6 2
    SAU.SUM.u32 i14  v8  0x02   || cmu.cpiv.x16 v7.7 i8.l
    SAU.SUM.u32  i0  v9  0x01   || cmu.vrot v8 v7 2
    SAU.SUM.u32  i6  v9  0x02   || cmu.cpiv.x16 v8.7 i8.h

    cmu.vrot v15 v1 2
    cmu.cpiv.x16 v15.7 i9.l
    cmu.vrot v16 v15 2
    cmu.cpiv.x16 v16.7 i9.h || bru.rpl i16 i15
    cmu.vrot v17 v16 2
    cmu.cpiv.x16 v17.7 i10.l
    cmu.vrot v18 v17 2
    cmu.cpiv.x16 v18.7 i10.h

___forLoopHead:
    cmu.vrot v19 v2 2           || VAU.MACPZ.f16       v0  v10  || lsu0.swzv8 [00000000] 
    cmu.cpiv.x16 v19.7 i11.l    || VAU.MACP.f16        v5  v10  || lsu0.swzv8 [11111111] || LSU1.LD128.u8.f16 v0 i1     || IAU.ADD i1 i1 8 
    cmu.vrot v27 v19 2          || VAU.MACP.f16        v6  v10  || lsu0.swzv8 [22222222] || LSU1.LD128.u8.f16 v5 i1
    cmu.cpiv.x16 v27.7 i11.h    || VAU.MACP.f16        v7  v10  || lsu0.swzv8 [33333333]
    cmu.vrot v21 v27 2          || VAU.MACP.f16        v8  v10  || lsu0.swzv8 [44444444]
    cmu.cpiv.x16 v21.7 i12.l    || VAU.MACP.f16        v1  v11  || lsu0.swzv8 [00000000]
    cmu.vrot v28 v21 2          || VAU.MACP.f16        v15 v11  || lsu0.swzv8 [11111111] || LSU1.LD128.u8.f16 v1 i2     || IAU.ADD i2 i2 8
    cmu.cpiv.x16 v28.7 i12.h    || VAU.MACP.f16        v16 v11  || lsu0.swzv8 [22222222] || LSU1.LD128.u8.f16 v6 i2
    cmu.vrot v15 v3 2           || VAU.MACP.f16        v17 v11  || lsu0.swzv8 [33333333] || SAU.SUM.u32 i7  v5  0x01
    cmu.cpiv.x16 v15.7 i13.l    || VAU.MACP.f16        v18 v11  || lsu0.swzv8 [44444444] || SAU.SUM.u32 i8  v5  0x02
    cmu.vrot v16 v15 2          || VAU.MACP.f16        v2  v12  || lsu0.swzv8 [00000000]
    cmu.cpiv.x16 v16.7 i13.h    || VAU.MACP.f16        v19 v12  || lsu0.swzv8 [11111111] || LSU1.LD128.u8.f16 v2 i3     || IAU.ADD i3 i3 8
    cmu.vrot v17 v16 2          || VAU.MACP.f16        v27 v12  || lsu0.swzv8 [22222222] || LSU1.LD128.u8.f16 v7 i3
    cmu.cpiv.x16 v17.7 i14.l    || VAU.MACP.f16        v21 v12  || lsu0.swzv8 [33333333] || SAU.SUM.u32 i9  v6  0x01 
    cmu.vrot v18 v17 2          || VAU.MACP.f16        v28 v12  || lsu0.swzv8 [44444444] || SAU.SUM.u32 i10  v6  0x02
    cmu.cpiv.x16 v18.7 i14.h    || VAU.MACP.f16        v3  v13  || lsu0.swzv8 [00000000]
    cmu.vrot v23 v4 2           || VAU.MACP.f16        v15 v13  || lsu0.swzv8 [11111111] || LSU1.LD128.u8.f16 v3 i4     || IAU.ADD i4 i4 8
    cmu.cpiv.x16 v23.7 i0.l     || VAU.MACP.f16        v16 v13  || lsu0.swzv8 [22222222] || LSU1.LD128.u8.f16 v8 i4   
    cmu.vrot v24 v23 2          || VAU.MACP.f16        v17 v13  || lsu0.swzv8 [33333333] || SAU.SUM.u32 i11  v7  0x01 
    cmu.cpiv.x16 v24.7 i0.h     || VAU.MACP.f16        v18 v13  || lsu0.swzv8 [44444444] || SAU.SUM.u32 i12  v7  0x02 
    cmu.vrot v25 v24 2          || VAU.MACP.f16        v4  v14  || lsu0.swzv8 [00000000] 
    cmu.cpiv.x16 v25.7 i6.l     || VAU.MACP.f16        v23 v14  || lsu0.swzv8 [11111111] || LSU1.LD128.u8.f16 v4 i5     || IAU.ADD i5 i5 8 
    cmu.vrot v26 v25 2          || VAU.MACP.f16        v24 v14  || lsu0.swzv8 [22222222] || LSU1.LD128.u8.f16 v9 i5  
    cmu.cpiv.x16 v26.7 i6.h     || VAU.MACP.f16        v25 v14  || lsu0.swzv8 [33333333] || SAU.SUM.u32 i13  v8  0x01 
                                   VAU.MACPW.f16   v20 v26 v14  || lsu0.swzv8 [44444444] || SAU.SUM.u32 i14  v8  0x02 || cmu.vrot v5 v0 2
	cmu.cpiv.x16 v5.7 i7.l
	cmu.vrot v6 v5 2									   
	cmu.cpiv.x16 v6.7 i7.h								   
	SAU.SUM.u32  i0  v9  0x01 || cmu.vrot v7 v6 2	 
	SAU.SUM.u32  i6  v9  0x02 || cmu.cpiv.x16 v7.7 i8.l	 								   			   
	cmu.vrot v8 v7 2
    ___forLoop:
	cmu.cpiv.x16 v8.7 i8.h	
	cmu.vrot v15 v1 2
	cmu.cpiv.x16 v15.7 i9.l
    CMU.CLAMP0.F16 v20 v20 v22
    nop
    LSU0.ST128.f16.u8 v20 i17 || IAU.ADD i17 i17 8 ||     cmu.vrot v16 v15 2 

		

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
 
