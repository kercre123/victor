; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief
; ///

;--------------------------------------------------------------------------------------------------------------
.version 00.70.00

.data .rodata.boxfilter1x7_asm
.salign 16
    ___multiply:
        .float16  0.142857F16


; void boxfilter1x7(u8** in (i18), u8** out (i17), u8 normalize (i16), u32 width (i15))
.code .text.boxfilter1x7_asm
.salign 16

boxfilter1x7_asm:
	LSU0.ldil i5, ___second || LSU1.ldih i5, ___second
	LSU0.ldil i13, 0x1 || LSU1.ldih i13, 0x0
	CMU.CMII.U8 i16 i13
	PEU.PC1C NEQ || BRU.JMP i5
	nop 5

	lsu0.ldil i8, ___loop     ||  lsu1.ldih i8, ___loop
	lsu0.ldil i7, ___multiply     ||  lsu1.ldih i7, ___multiply
	LSU0.LD32  i0  i18  || IAU.SHR.u32 i15 i15 3
	LSU0.LD32  i17  i17
	lsu0.ld16r v0, i7
	nop 5

	
	LSU1.LD128.u8.f16 v1  i0 || IAU.ADD i0 i0 1
	LSU1.LD128.u8.f16 v2  i0 || IAU.ADD i0 i0 1
	LSU1.LD128.u8.f16 v3  i0 || IAU.ADD i0 i0 1
	LSU1.LD128.u8.f16 v4  i0 || IAU.ADD i0 i0 1
	LSU1.LD128.u8.f16 v5  i0 || IAU.ADD i0 i0 1
	LSU1.LD128.u8.f16 v6  i0 || IAU.ADD i0 i0 1
	LSU1.LD128.u8.f16 v7  i0 || IAU.ADD i0 i0 2 || VAU.ACCPZ.f16   v1 || bru.rpl i8 i15
	VAU.ACCP.f16       v2
	VAU.ACCP.f16       v3
	VAU.ACCP.f16       v4
	VAU.ACCP.f16       v5
	VAU.ACCP.f16       v6
	VAU.ACCPW.f16 v8   v7
	LSU1.LD128.u8.f16 v1 i0 || IAU.ADD i0 i0 1
	LSU1.LD128.u8.f16 v2 i0 || IAU.ADD i0 i0 1
	LSU1.LD128.u8.f16 v3 i0 || IAU.ADD i0 i0 1
	LSU1.LD128.u8.f16 v4 i0 || IAU.ADD i0 i0 1
	LSU1.LD128.u8.f16 v5 i0 || IAU.ADD i0 i0 1
	LSU1.LD128.u8.f16 v6 i0 || IAU.ADD i0 i0 1
	___loop:
	nop 2
	VAU.mul.f16 v9 v8 v0
	nop 2
	LSU0.STi128.f16.u8 v9 i17

    BRU.JMP i30
    nop 5
	
	___second:	

	LSU0.LD32  i0  i18 
	LSU0.LD32  i17  i17

	nop 5
	
	IAU.SHR.u32 i15 i15 3
	lsu0.ldil i8, ___loop1     ||  lsu1.ldih i8, ___loop1
	
	
	LSU0.LD128.u8.u16 v1  i0 || IAU.ADD i0 i0 1 
	LSU0.LD128.u8.u16 v2  i0 || IAU.ADD i0 i0 1
	LSU0.LD128.u8.u16 v3  i0 || IAU.ADD i0 i0 1
	LSU0.LD128.u8.u16 v4  i0 || IAU.ADD i0 i0 1
	LSU0.LD128.u8.u16 v5  i0 || IAU.ADD i0 i0 1
	LSU0.LD128.u8.u16 v6  i0 || IAU.ADD i0 i0 1
	LSU0.LD128.u8.u16 v7  i0 || IAU.ADD i0 i0 2 || VAU.ACCPZ.u16   v1 || bru.rpl i8 i15
	VAU.ACCP.u16       v2
	VAU.ACCP.u16       v3
	VAU.ACCP.u16       v4
	VAU.ACCP.u16       v5 || LSU0.LD128.u8.u16 v1 i0 || IAU.ADD i0 i0 1
	VAU.ACCP.u16       v6 || LSU0.LD128.u8.u16 v2 i0 || IAU.ADD i0 i0 1
	
	___loop1:
	VAU.ACCPW.u16 v8   v7 || LSU0.LD128.u8.u16 v3 i0 || IAU.ADD i0 i0 1
	LSU0.LD128.u8.u16 v4 i0 || IAU.ADD i0 i0 1
	LSU0.LD128.u8.u16 v5 i0 || IAU.ADD i0 i0 1
	LSU0.LD128.u8.u16 v6 i0 || IAU.ADD i0 i0 1
	LSU0.STi64.l v8 i17 
	LSU0.STi64.h v8 i17
	

	
	
    BRU.JMP i30
    nop 5
	
	
.end
