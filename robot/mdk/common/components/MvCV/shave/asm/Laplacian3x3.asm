; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief
; ///

.version 00.51.04

.data .data.laplacian3x3

.salign 16
    ___clampVal:
        .float16 255.0

.code .text.laplacian3x3

;void laplacian3x3(u8** in, u8** out, u32 inWidth);
;					i18         i17       i16
Laplacian3x3_asm: 
	LSU0.LDIL i3 ___clampVal || LSU1.LDIH i3 ___clampVal
	lsu0.ldil i5 ___loop || lsu1.ldih i5 ___loop
		
    LSU0.LD32  i0  i18 || IAU.ADD i18 i18 4 
    LSU0.LD32  i1  i18 || IAU.ADD i18 i18 4
    LSU0.LD32  i2  i18 || IAU.ADD i18 i18 4 || LSU1.LD32  i17  i17
	
	nop 3
	
    lsu0.ld16r v20, i3 || lsu1.ld64.l v3 i4 
	IAU.SHR.u32 i6 i16 3 
	
	IAU.ADD i0 i0 1 
	LSU0.LD128.u8.u16 v1 i1 || IAU.ADD i1 i1 1 
    LSU0.LD128.u8.u16 v9 i1 || IAU.ADD i1 i1 1 
    LSU0.LD128.u8.u16 v7 i0 || IAU.ADD i0 i0 7 || SAU.ADD.i32 i2 i2 1 
	LSU0.LD128.u8.u16 v10 i1 || IAU.ADD i1 i1 6 
	LSU0.LD128.u8.u16 v11 i2 || IAU.ADD i2 i2 7  
	nop 2
	
	vau.shl.u16 v13 v9 2 
	vau.accnz.u16 v7 || bru.rpl i5 i6;
	vau.accn.u16 v1 || IAU.ADD i0 i0 1 
	vau.accp.u16 v13 || LSU0.LD128.u8.u16 v1 i1  || IAU.ADD i1 i1 1 
	vau.accn.u16 v10 || LSU0.LD128.u8.u16 v9 i1  || IAU.ADD i1 i1 1 
	___loop:
	vau.accnw.u16 v18 v11 || LSU0.LD128.u8.u16 v7 i0 || IAU.ADD i0 i0 7 || SAU.ADD.i32 i2 i2 1 
	LSU0.LD128.u8.u16 v10 i1 || IAU.ADD i1 i1 6 
	LSU0.LD128.u8.u16 v11 i2 || IAU.ADD i2 i2 7  
	
	cmu.clamp0.i16 v19 v18 v20
	nop
	lsu0.st128.u16.u8 v19 i17 || iau.add i17 i17 8 || vau.shl.u16 v13 v9 2

    BRU.jmp i30
    nop 5

.end
 
 
