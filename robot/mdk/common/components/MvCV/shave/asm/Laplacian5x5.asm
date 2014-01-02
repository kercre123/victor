; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief
; ///

.version 00.51.04

.data .data.laplacian5x5

.salign 16
    ___clampVal:
        .float16 255.0
	___laplacian:
        .short 17

.code .text.laplacian5x5

;void laplacian3x3(u8** in, u8** out, u32 inWidth);
;					i18         i17       i16
Laplacian5x5_asm: 
	LSU0.LDIL i5 ___clampVal || LSU1.LDIH i5 ___clampVal
	LSU0.LDIL i6 ___laplacian || LSU1.LDIH i6 ___laplacian
	lsu0.ldil i7 ___loop || lsu1.ldih i7 ___loop
		
    LSU0.LD32  i0  i18 || IAU.ADD i18 i18 4 
    LSU0.LD32  i1  i18 || IAU.ADD i18 i18 4
    LSU0.LD32  i2  i18 || IAU.ADD i18 i18 4 
    LSU0.LD32  i3  i18 || IAU.ADD i18 i18 4 
    LSU0.LD32  i4  i18 || IAU.ADD i18 i18 4 
	LSU0.LD32  i17  i17
	nop 
	lsu0.ld16r v13 i6 || lsu1.ld16r v21 i5 
	IAU.SHR.u32 i8 i16 3 
	
	
	IAU.ADD i0 i0 2 || SAU.ADD.i32 i1 i1 1
	LSU0.LD128.u8.u16 v0 i0 || iau.add i0 i0 6 
	
	LSU1.LD128.u8.u16 v1 i1 || IAU.ADD i1 i1 1 
	LSU0.LD128.u8.u16 v2 i1 || IAU.ADD i1 i1 1 
		
	LSU0.LD128.u8.u16 v4 i2 || IAU.ADD i2 i2 1 || LSU1.LD128.u8.u16 v3 i1 || SAU.ADD.i32 i1 i1 5
	LSU0.LD128.u8.u16 v5 i2 || IAU.ADD i2 i2 1
	LSU0.LD128.u8.u16 v6 i2 || IAU.ADD i2 i2 1
	LSU0.LD128.u8.u16 v7 i2 || IAU.ADD i2 i2 1
	LSU0.LD128.u8.u16 v8 i2 || IAU.ADD i2 i2 4
	
	IAU.ADD i3 i3 1 || SAU.ADD.i32 i4 i4 2 
	LSU0.LD128.u8.u16 v9 i3 || IAU.ADD i3 i3 1 
	LSU0.LD128.u8.u16 v10 i3 || IAU.ADD i3 i3 1 
	LSU0.LD128.u8.u16 v11 i3 || IAU.ADD i3 i3 5 || LSU1.LD128.u8.u16 v12 i4 || sau.add.i32 i4 i4 6
	;nop

	vau.shl.u16 v15 v2 1 ;|| bru.rpl i7 i8
	vau.shl.u16 v16 v5 1
	vau.shl.u16 v17 v7 1
	vau.mul.i16 v14 v6 v13
	nop
	vau.shl.u16 v18 v10 1 || bru.rpl i7 i8
	
	vau.accnz.u16 v0 || IAU.ADD i0 i0 2 || SAU.ADD.i32 i1 i1 1
	vau.accn.u16 v1  || LSU0.LD128.u8.u16 v0 i0 || iau.add i0 i0 6
	vau.accn.u16 v15 || LSU1.LD128.u8.u16 v1 i1 || IAU.ADD i1 i1 1 
	vau.accn.u16 v3 || LSU0.LD128.u8.u16 v2 i1 || IAU.ADD i1 i1 1 
	vau.accn.u16 v4 || LSU0.LD128.u8.u16 v4 i2 || IAU.ADD i2 i2 1 || LSU1.LD128.u8.u16 v3 i1 || SAU.ADD.i32 i1 i1 5 
	vau.accn.u16 v16 || LSU0.LD128.u8.u16 v5 i2 || IAU.ADD i2 i2 1
	vau.accp.u16 v14 || LSU0.LD128.u8.u16 v6 i2 || IAU.ADD i2 i2 1 
	vau.accn.u16 v17 || LSU0.LD128.u8.u16 v7 i2 || IAU.ADD i2 i2 1
	vau.accn.u16 v8 || IAU.ADD i3 i3 1 || SAU.ADD.i32 i4 i4 2
	vau.accn.u16 v9  || LSU0.LD128.u8.u16 v8 i2 || IAU.ADD i2 i2 4
	vau.accn.u16 v18  || LSU0.LD128.u8.u16 v9 i3 || IAU.ADD i3 i3 1
	vau.accn.u16 v11   || LSU0.LD128.u8.u16 v10 i3 || IAU.ADD i3 i3 1
	___loop:
	vau.accnw.u16 v19 v12 
	LSU0.LD128.u8.u16 v11 i3 || IAU.ADD i3 i3 5 || LSU1.LD128.u8.u16 v12 i4 || sau.add.i32 i4 i4 6 || vau.shl.u16 v15 v2 1
			
	cmu.clamp0.i16 v20 v19 v21 || vau.shl.u16 v16 v5 1
	vau.shl.u16 v17 v7 1
	lsu0.st128.u16.u8 v20 i17 || iau.add i17 i17 8 || vau.mul.i16 v14 v6 v13
	nop 
	
    BRU.jmp i30
    nop 5

.end
 
 
