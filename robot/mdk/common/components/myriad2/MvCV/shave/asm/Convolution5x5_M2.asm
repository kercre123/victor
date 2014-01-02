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

    LSU0.LD.32 i1 i18            || IAU.ADD i18 i18 4
    lsu0.ldil i6 ___clampVal    || lsu1.ldih i6 ___clampVal
    LSU0.LDO.64.L v10 i16 0      || LSU1.LDO.64.H v10 i16 8      || IAU.ADD i16 i16 10
    LSU0.LD.32 i2 i18            || IAU.ADD i18 i18 4
    LSU0.LDO.64.L v11 i16 0      || LSU1.LDO.64.H v11 i16 8      || IAU.ADD i16 i16 10
    LSU0.LD.32 i3 i18            || IAU.ADD i18 i18 4           || LSU1.LD.32 i17 i17
    LSU0.LDO.64.L v12 i16 0      || LSU1.LDO.64.H v12 i16 8      || IAU.ADD i16 i16 10
    LSU0.LD.32 i4 i18            || IAU.ADD i18 i18 4
    LSU0.LDO.64.L v13 i16 0      || LSU1.LDO.64.H v13 i16 8      || IAU.ADD i16 i16 10
    LSU0.LD.16R v22  i6
    LSU0.LDO.64.L v14 i16 0      || LSU1.LDO.64.H v14 i16 8      || IAU.ADD i16 i16 10
    LSU0.LD.32 i5 i18            || IAU.ADD i18 i18 4

    IAU.SHR.u32 i15 i15 3   ; i15 = i15/8 because we process 8 pixels at once
	LSU0.LDIL i16 ___forLoop    || LSU1.LDIH i16 ___forLoop
	
    LSU0.LD.128.U8.F16 v0 i1     || IAU.ADD i1 i1 8 || bru.rpl i16 i15
    LSU0.LD.128.U8.F16 v1 i2     || IAU.ADD i2 i2 8
    LSU0.LD.128.U8.F16 v5 i1
    LSU0.LD.128.U8.F16 v2 i3     || IAU.ADD i3 i3 8
    LSU0.LD.128.U8.F16 v3 i4     || IAU.ADD i4 i4 8
    LSU0.LD.128.U8.F16 v4 i5     || IAU.ADD i5 i5 8
    LSU0.LD.128.U8.F16 v6 i2
nop 2
    LSU0.LD.128.U8.F16 v7 i3     || SAU.SUMX.U32 i7  v5 || LSU1.SWZMS4.word [3210] [zzzU]
nop 2
    LSU0.LD.128.U8.F16 v8 i4     || SAU.SUMX.U32 i8  v5  || LSU1.SWZMS4.word [3210] [zzUZ]
    LSU0.LD.128.U8.F16 v9 i5     || SAU.SUMX.U32 i9  v6  || LSU1.SWZMS4.word [3210] [zzzU]  || CMU.ALIGNVEC V5, v0 v0 2
    LSU0.LDIL i16 ___forLoop    || LSU1.LDIH i16 ___forLoop
nop
    SAU.SUMX.U32 i10  v6  || LSU1.SWZMS4.word [3210] [zzUz]  || cmu.cpiv.x16 v5.7 i7.l
    SAU.SUMX.U32 i11  v7  || LSU1.SWZMS4.word [3210] [zzzU]  || CMU.ALIGNVEC V6, v5 v5 2
    SAU.SUMX.U32 i12  v7  || LSU1.SWZMS4.word [3210] [zzUz]  || cmu.cpiv.x16 v6.7 i7.h
    SAU.SUMX.U32 i13  v8  || LSU1.SWZMS4.word [3210] [zzzU]  || CMU.ALIGNVEC V7, v6 v6 2
    SAU.SUMX.U32 i14  v8  || LSU1.SWZMS4.word [3210] [zzUz]  || cmu.cpiv.x16 v7.7 i8.l
    SAU.SUMX.U32  i0  v9  || LSU1.SWZMS4.word [3210] [zzzU]  || CMU.ALIGNVEC V8, v7 v7 2
    SAU.SUMX.U32  i6  v9  || LSU1.SWZMS4.word [3210] [zzUz]  || cmu.cpiv.x16 v8.7 i8.h

    CMU.ALIGNVEC V15, v1 v1 2
    cmu.cpiv.x16 v15.7 i9.l
    CMU.ALIGNVEC V16, v15 v15 2
    cmu.cpiv.x16 v16.7 i9.h 
    CMU.ALIGNVEC V17, v16 v16 2
    cmu.cpiv.x16 v17.7 i10.l
    CMU.ALIGNVEC V18, v17 v17 2
    cmu.cpiv.x16 v18.7 i10.h

___forLoopHead:
    CMU.ALIGNVEC V19, v2 v2 2            || VAU.MACPZ.f16       v10 v0   || lsu0.swzv8 [00000000] 
    cmu.cpiv.x16 v19.7 i11.l             || VAU.MACP.f16        v10 v5   || lsu0.swzv8 [11111111] 
    CMU.ALIGNVEC V27, v19 v19 2          || VAU.MACP.f16        v10 v6   || lsu0.swzv8 [22222222]
    cmu.cpiv.x16 v27.7 i11.h             || VAU.MACP.f16        v10 v7   || lsu0.swzv8 [33333333]
    CMU.ALIGNVEC V21, v27 v27 2          || VAU.MACP.f16        v10 v8   || lsu0.swzv8 [44444444]
    cmu.cpiv.x16 v21.7 i12.l             || VAU.MACP.f16        v11 v1   || lsu0.swzv8 [00000000]
    CMU.ALIGNVEC V28, v21 v21 2          || VAU.MACP.f16        v11 v15  || lsu0.swzv8 [11111111]
    cmu.cpiv.x16 v28.7 i12.h             || VAU.MACP.f16        v11 v16  || lsu0.swzv8 [22222222]
    CMU.ALIGNVEC V15, v3 v3 2            || VAU.MACP.f16        v11 v17  || lsu0.swzv8 [33333333]
    cmu.cpiv.x16 v15.7 i13.l             || VAU.MACP.f16        v11 v18  || lsu0.swzv8 [44444444] 
    CMU.ALIGNVEC V16, v15 v15 2          || VAU.MACP.f16        v12 v2   || lsu0.swzv8 [00000000]
    cmu.cpiv.x16 v16.7 i13.h             || VAU.MACP.f16        v12 v19  || lsu0.swzv8 [11111111] 
    CMU.ALIGNVEC V17, v16 v16 2          || VAU.MACP.f16        v12 v27  || lsu0.swzv8 [22222222] 
    cmu.cpiv.x16 v17.7 i14.l             || VAU.MACP.f16        v12 v21  || lsu0.swzv8 [33333333] 
    CMU.ALIGNVEC V18, v17 v17 2          || VAU.MACP.f16        v12 v28  || lsu0.swzv8 [44444444] 
    cmu.cpiv.x16 v18.7 i14.h             || VAU.MACP.f16        v13 v3   || lsu0.swzv8 [00000000]
    CMU.ALIGNVEC V23, v4 v4 2            || VAU.MACP.f16        v13 v15  || lsu0.swzv8 [11111111] 
    cmu.cpiv.x16 v23.7 i0.l              || VAU.MACP.f16        v13 v16  || lsu0.swzv8 [22222222] 
    CMU.ALIGNVEC V24, v23 v23 2          || VAU.MACP.f16        v13 v17  || lsu0.swzv8 [33333333] 
    cmu.cpiv.x16 v24.7 i0.h              || VAU.MACP.f16        v13 v18  || lsu0.swzv8 [44444444] 
    CMU.ALIGNVEC V25, v24 v24 2          || VAU.MACP.f16        v14 v4   || lsu0.swzv8 [00000000] 
    cmu.cpiv.x16 v25.7 i6.l              || VAU.MACP.f16        v14 v23  || lsu0.swzv8 [11111111] 
    CMU.ALIGNVEC V26, v25 v25 2          || VAU.MACP.f16        v14 v24  || lsu0.swzv8 [22222222] 
    cmu.cpiv.x16 v26.7 i6.h              || VAU.MACP.f16        v14 v25  || lsu0.swzv8 [33333333] 
											VAU.MACPW.f16   v20 v14 v26  || lsu0.swzv8 [44444444] 
nop 6
___forLoop:
nop 3
    CMU.CLAMP0.F16 v20 v20 v22
nop
    LSU0.ST.128.F16.U8 v20 i17 || IAU.ADD i17 i17 8 ||     CMU.ALIGNVEC V16, v15 v15 2 
nop


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
