; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief
; ///

;--------------------------------------------------------------------------------------------------------------
.version 00.70.00

.data .rodata.boxfilter7x1_asm
.salign 16
    ___multiply:
        .float16  0.142857F16


; void boxfilter7x1(u8** in (i18), u8** out (i17), u8 normalize (i16), u32 width (i15))
.code .text.boxfilter7x1_asm
.salign 16

boxfilter7x1_asm:
	LSU0.ldil i5, ___second || LSU1.ldih i5, ___second
	LSU0.ldil i13, 0x1 || LSU1.ldih i13, 0x0
	CMU.CMII.U8 i16 i13
	PEU.PC1C NEQ || BRU.JMP i5
	nop 5

	lsu0.ldil i8, ___loop     ||  lsu1.ldih i8, ___loop
	lsu0.ldil i7, ___multiply     ||  lsu1.ldih i7, ___multiply
	LSU0.LD32  i0  i18  || IAU.ADD i18 i18 4
	LSU0.LD32  i1  i18  || IAU.ADD i18 i18 4
	LSU0.LD32  i2  i18  || IAU.ADD i18 i18 4
	LSU0.LD32  i3  i18  || IAU.ADD i18 i18 4
	LSU0.LD32  i4  i18  || IAU.ADD i18 i18 4
	LSU0.LD32  i5  i18  || IAU.ADD i18 i18 4
	LSU0.LD32  i6  i18  || IAU.ADD i18 i18 4
	LSU0.LD32  i17  i17 || IAU.SHR.u32 i15 i15 3
	lsu0.ld16r v0, i7


	
	LSU1.LDi128.u8.f16 v1 i0 
	LSU1.LDi128.u8.f16 v2 i1
	LSU1.LDi128.u8.f16 v3 i2
	LSU1.LDi128.u8.f16 v4 i3
	LSU1.LDi128.u8.f16 v5 i4
	LSU1.LDi128.u8.f16 v6 i5
	LSU1.LDi128.u8.f16 v7 i6 || VAU.ACCPZ.f16   v1 || bru.rpl i8 i15
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
	LSU1.LDi128.u8.f16 v5 i4
	LSU1.LDi128.u8.f16 v6 i5
	___loop:
	nop 2
	VAU.mul.f16 v9 v8 v0
	nop 2
	LSU0.STi128.f16.u8 v9 i17
	
	BRU.JMP i30
    nop 5

	___second:	

	LSU0.LD32  i0  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i1  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i2  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i3  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i4  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i5  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i6  i18 || IAU.ADD i18 i18 4
	LSU0.LD32  i17  i17

	nop 5
	
	IAU.SHR.u32 i15 i15 3
	lsu0.ldil i8, ___loop1     ||  lsu1.ldih i8, ___loop1
	
	
	LSU0.LDi128.u8.u16 v1 i0  
	LSU0.LDi128.u8.u16 v2 i1
	LSU0.LDi128.u8.u16 v3 i2
	LSU0.LDi128.u8.u16 v4 i3
	LSU0.LDi128.u8.u16 v5 i4
	LSU0.LDi128.u8.u16 v6 i5
	LSU0.LDi128.u8.u16 v7 i6 || VAU.ACCPZ.u16   v1 || bru.rpl i8 i15
	VAU.ACCP.u16       v2
	VAU.ACCP.u16       v3
	VAU.ACCP.u16       v4
	VAU.ACCP.u16       v5 || LSU0.LDi128.u8.u16 v1 i0
	VAU.ACCP.u16       v6 || LSU0.LDi128.u8.u16 v2 i1
	
	___loop1:
	VAU.ACCPW.u16 v8   v7 || LSU0.LDi128.u8.u16 v3 i2
	LSU0.LDi128.u8.u16 v4 i3
	LSU0.LDi128.u8.u16 v5 i4
	LSU0.LDi128.u8.u16 v6 i5
	LSU0.STi64.l v8 i17 
	LSU0.STi64.h v8 i17
	

	
	
    BRU.JMP i30
    nop 5
	
	
.end
