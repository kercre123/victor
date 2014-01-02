; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief
; ///

;--------------------------------------------------------------------------------------------------------------
.version 00.70.00

.data .rodata.boxfilter7x7separable_asm
.salign 16
    ___multiply:
        .float16  0.020408F16


; void boxfilter7x1(u8** in (i18), u8** out (i17), u8 normalize (i16), u32 width (i15))
.code .text.boxfilter7x7separable_asm
.salign 16

boxfilter7x7separable_asm:
	LSU0.ldil i5, ___second || LSU1.ldih i5, ___second
	LSU0.ldil i13, 0x1 || LSU1.ldih i13, 0x0
	CMU.CMII.U8 i16 i13
	PEU.PC1C NEQ || BRU.JMP i5
	nop 5
	
	
	lsu0.ldil i10, 0x0f00     ||  lsu1.ldih i10, 0x0000
	IAU.SUB i19 i19 i10
	IAU.SUB i11 i19 0 ;first copy for interim address
	IAU.SUB i12 i19 0 ;second copy for interim address
	IAU.ADD i13 i15 6 ;width +6 for first step
	
	lsu0.ldil i8, ___loop     ||  lsu1.ldih i8, ___loop
	lsu0.ldil i7, ___multiply     ||  lsu1.ldih i7, ___multiply
	LSU0.LD32  i0  i18  || IAU.ADD i18 i18 4
	LSU0.LD32  i1  i18  || IAU.ADD i18 i18 4
	LSU0.LD32  i2  i18  || IAU.ADD i18 i18 4
	LSU0.LD32  i3  i18  || IAU.ADD i18 i18 4
	LSU0.LD32  i4  i18  || IAU.ADD i18 i18 4
	LSU0.LD32  i5  i18  || IAU.ADD i18 i18 4
	LSU0.LD32  i6  i18  || IAU.ADD i18 i18 4
	LSU0.LD32  i17  i17 || IAU.SHR.u32 i13 i13 3
	lsu0.ld16r v0, i7
	IAU.ADD i13 i13 1
	
	LSU1.LDi128.u8.f16 v1 i0 
	LSU1.LDi128.u8.f16 v2 i1
	LSU1.LDi128.u8.f16 v3 i2
	LSU1.LDi128.u8.f16 v4 i3
	LSU1.LDi128.u8.f16 v5 i4
	LSU1.LDi128.u8.f16 v6 i5
	LSU1.LDi128.u8.f16 v7 i6 || VAU.ACCPZ.f16      v1 || bru.rpl i8 i13
	VAU.ACCP.f16       v2
	VAU.ACCP.f16       v3
	VAU.ACCP.f16       v4
	VAU.ACCP.f16       v5
	VAU.ACCP.f16       v6
	VAU.ACCPW.f16 v8   v7
	LSU1.LDi128.u8.f16 v1 i0 
	LSU1.LDi128.u8.f16 v2 i1
	LSU1.LDi128.u8.f16 v3 i2
	LSU1.LDi128.u8.f16 v4 i3
	___loop:
	LSU1.LDi128.u8.f16 v5 i4
	LSU1.LDi128.u8.f16 v6 i5
	nop 2
	LSU0.STi64.l v8 i11 
	LSU0.STi64.h v8 i11
	
	IAU.SHR.u32 i15 i15 3
	
	lsu0.ldil i8, ___loop1     ||  lsu1.ldih i8, ___loop1
	
	LSU0.LDO64.l v1 i12 0 || LSU1.LDO64.h v1 i12 8  || IAU.ADD i12 i12 2 
	LSU0.LDO64.l v2 i12 0 || LSU1.LDO64.h v2 i12 8  || IAU.ADD i12 i12 2
	LSU0.LDO64.l v3 i12 0 || LSU1.LDO64.h v3 i12 8  || IAU.ADD i12 i12 2
	LSU0.LDO64.l v4 i12 0 || LSU1.LDO64.h v4 i12 8  || IAU.ADD i12 i12 2
	LSU0.LDO64.l v5 i12 0 || LSU1.LDO64.h v5 i12 8  || IAU.ADD i12 i12 2
	LSU0.LDO64.l v6 i12 0 || LSU1.LDO64.h v6 i12 8  || IAU.ADD i12 i12 2
	LSU0.LDO64.l v7 i12 0 || LSU1.LDO64.h v7 i12 8  || IAU.ADD i12 i12 4 || 	VAU.ACCPZ.f16      v1 || bru.rpl i8 i15
	VAU.ACCP.f16       v2
	VAU.ACCP.f16       v3
	VAU.ACCP.f16       v4
	VAU.ACCP.f16       v5
	VAU.ACCP.f16       v6
	VAU.ACCPW.f16 v8   v7
	LSU0.LDO64.l v1 i12 0 || LSU1.LDO64.h v1 i12 8  || IAU.ADD i12 i12 2 
	LSU0.LDO64.l v2 i12 0 || LSU1.LDO64.h v2 i12 8  || IAU.ADD i12 i12 2
	LSU0.LDO64.l v3 i12 0 || LSU1.LDO64.h v3 i12 8  || IAU.ADD i12 i12 2
	LSU0.LDO64.l v4 i12 0 || LSU1.LDO64.h v4 i12 8  || IAU.ADD i12 i12 2
	LSU0.LDO64.l v5 i12 0 || LSU1.LDO64.h v5 i12 8  || IAU.ADD i12 i12 2
	LSU0.LDO64.l v6 i12 0 || LSU1.LDO64.h v6 i12 8  || IAU.ADD i12 i12 2
	___loop1:
	nop 2
	VAU.mul.f16 v9 v8 v0
	nop 2
	LSU0.STi128.f16.u8 v9 i17
	
	IAU.ADD i19 i19 i10
	
    BRU.JMP i30
    nop 5
	
	
___second:

	lsu0.ldil i10, 0x0f00     ||  lsu1.ldih i10, 0x0000
	IAU.SUB i19 i19 i10
	IAU.SUB i11 i19 0 ;first copy for interim address
	IAU.SUB i12 i19 0 ;second copy for interim address
	IAU.ADD i13 i15 6 ;width +6 for first step
	
	lsu0.ldil i8, ___loop2     ||  lsu1.ldih i8, ___loop2

	LSU0.LD32  i0  i18  || IAU.ADD i18 i18 4
	LSU0.LD32  i1  i18  || IAU.ADD i18 i18 4
	LSU0.LD32  i2  i18  || IAU.ADD i18 i18 4
	LSU0.LD32  i3  i18  || IAU.ADD i18 i18 4
	LSU0.LD32  i4  i18  || IAU.ADD i18 i18 4
	LSU0.LD32  i5  i18  || IAU.ADD i18 i18 4
	LSU0.LD32  i6  i18  || IAU.ADD i18 i18 4
	LSU0.LD32  i17  i17 || IAU.SHR.u32 i13 i13 3
	IAU.ADD i13 i13 1
	
	LSU1.LDi128.u8.u16 v1 i0 
	LSU1.LDi128.u8.u16 v2 i1
	LSU1.LDi128.u8.u16 v3 i2
	LSU1.LDi128.u8.u16 v4 i3
	LSU1.LDi128.u8.u16 v5 i4
	LSU1.LDi128.u8.u16 v6 i5
	LSU1.LDi128.u8.u16 v7 i6 || VAU.ACCPZ.u16      v1 || bru.rpl i8 i13
	VAU.ACCP.u16       v2
	VAU.ACCP.u16       v3
	VAU.ACCP.u16       v4
	VAU.ACCP.u16       v5 || LSU1.LDi128.u8.u16 v1 i0
	VAU.ACCP.u16       v6 || LSU1.LDi128.u8.u16 v2 i1
		___loop2:
	VAU.ACCPW.u16 v8   v7 || LSU1.LDi128.u8.u16 v3 i2
	LSU1.LDi128.u8.u16 v4 i3
	LSU1.LDi128.u8.u16 v5 i4
	LSU1.LDi128.u8.u16 v6 i5
	LSU0.STi64.l v8 i11 
	LSU0.STi64.h v8 i11
	
	IAU.SHR.u32 i15 i15 3
	
	lsu0.ldil i8, ___loop3     ||  lsu1.ldih i8, ___loop3
	
	LSU0.LDO64.l v1 i12 0 || LSU1.LDO64.h v1 i12 8  || IAU.ADD i12 i12 2 
	LSU0.LDO64.l v2 i12 0 || LSU1.LDO64.h v2 i12 8  || IAU.ADD i12 i12 2
	LSU0.LDO64.l v3 i12 0 || LSU1.LDO64.h v3 i12 8  || IAU.ADD i12 i12 2
	LSU0.LDO64.l v4 i12 0 || LSU1.LDO64.h v4 i12 8  || IAU.ADD i12 i12 2
	LSU0.LDO64.l v5 i12 0 || LSU1.LDO64.h v5 i12 8  || IAU.ADD i12 i12 2
	LSU0.LDO64.l v6 i12 0 || LSU1.LDO64.h v6 i12 8  || IAU.ADD i12 i12 2
	LSU0.LDO64.l v7 i12 0 || LSU1.LDO64.h v7 i12 8  || IAU.ADD i12 i12 4 || VAU.ACCPZ.u16      v1 || bru.rpl i8 i15
	VAU.ACCP.u16       v2
	VAU.ACCP.u16       v3
	VAU.ACCP.u16       v4
	VAU.ACCP.u16       v5 || LSU0.LDO64.l v1 i12 0 || LSU1.LDO64.h v1 i12 8  || IAU.ADD i12 i12 2 
	VAU.ACCP.u16       v6 || LSU0.LDO64.l v2 i12 0 || LSU1.LDO64.h v2 i12 8  || IAU.ADD i12 i12 2
		___loop3:
	VAU.ACCPW.u16 v8   v7 || LSU0.LDO64.l v3 i12 0 || LSU1.LDO64.h v3 i12 8  || IAU.ADD i12 i12 2
	LSU0.LDO64.l v4 i12 0 || LSU1.LDO64.h v4 i12 8  || IAU.ADD i12 i12 2
	LSU0.LDO64.l v5 i12 0 || LSU1.LDO64.h v5 i12 8  || IAU.ADD i12 i12 2
	LSU0.LDO64.l v6 i12 0 || LSU1.LDO64.h v6 i12 8  || IAU.ADD i12 i12 2
	LSU0.STi64.l v8 i17 
	LSU0.STi64.h v8 i17
	
	IAU.ADD i19 i19 i10
	
    BRU.JMP i30
    nop 5
	
.end
