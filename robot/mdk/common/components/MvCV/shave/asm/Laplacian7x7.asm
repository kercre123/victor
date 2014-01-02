; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief
; ///

.version 00.51.04

.data .data.laplacian7x7

.salign 16
    ___clampVal:
        .float16 255.0
	___laplacian:
        .short 3, 5, 6, 7, 10, 
	___laplacian1:
        .short 1, 2, 4, 8 

		
.code .text.laplacian7x7

;void laplacian3x3(u8** in, u8** out, u32 inWidth);
;					i18         i17       i16
Laplacian7x7_asm: 
	LSU0.LDIL i7 ___clampVal || LSU1.LDIH i7 ___clampVal
	LSU0.LDIL i8 ___laplacian || LSU1.LDIH i8 ___laplacian
	LSU0.LDIL i10 ___laplacian1 || LSU1.LDIH i10 ___laplacian1
	lsu0.ldil i9 ___loop || lsu1.ldih i9 ___loop
	
    LSU0.LD32  i0  i18 || IAU.ADD i18 i18 4 
    LSU0.LD32  i1  i18 || IAU.ADD i18 i18 4
    LSU0.LD32  i2  i18 || IAU.ADD i18 i18 4 
    LSU0.LD32  i3  i18 || IAU.ADD i18 i18 4 
    LSU0.LD32  i4  i18 || IAU.ADD i18 i18 4 
    LSU0.LD32  i5  i18 || IAU.ADD i18 i18 4 
    LSU0.LD32  i6  i18 || IAU.ADD i18 i18 4 
	LSU0.LD32  i17  i17
	nop 
	lsu0.ld16r v21 i7 || lsu1.ld64.l v22 i8 || iau.add i8 i8 8
	lsu0.ld64.h v22 i8 || lsu1.ld64.l v23 i10 || iau.add i10 i10 8
	lsu0.ld64.h v23 i10
	IAU.SHR.u32 i11 i16 3 ;for loop 
	
	nop 5
	
	lsu0.ld128.u8.u16 v0 i0 || iau.add i0 i0 1 
	lsu0.ld128.u8.u16 v1 i1 || iau.add i1 i1 2 
	lsu0.ld128.u8.u16 v2 i2 || iau.add i2 i2 1 
	lsu0.ld128.u8.u16 v3 i3 || iau.add i3 i3 1 
	lsu0.ld128.u8.u16 v4 i4 || iau.add i4 i4 1 
	lsu0.ld128.u8.u16 v5 i5 || iau.add i5 i5 2 
	lsu1.ld128.u8.u16 v6 i6 || iau.add i6 i6 1 || vau.macnz.i16 v0 v22 || lsu0.swzv8 [44444444] || bru.rpl i9 i11
	
	lsu1.ld128.u8.u16 v7 i0 || iau.add i0 i0 1 || vau.macn.u16 v1 v22 || lsu0.swzv8 [11111111] 
	lsu1.ld128.u8.u16 v8 i1 || iau.add i1 i1 1 || vau.macn.u16 v2 v23 || lsu0.swzv8 [11111111]
	lsu1.ld128.u8.u16 v10 i2 || iau.add i2 i2 1 || vau.macn.u16 v3 v23 || lsu0.swzv8 [00000000]
	lsu1.ld128.u8.u16 v11 i3 || iau.add i3 i3 1 || vau.macn.u16 v4 v23 || lsu0.swzv8 [11111111]
	lsu1.ld128.u8.u16 v13 i4 || iau.add i4 i4 1 || vau.macn.u16 v5 v22 || lsu0.swzv8 [11111111]
	lsu1.ld128.u8.u16 v14 i5 || iau.add i5 i5 1 || vau.macn.u16 v6 v22 || lsu0.swzv8 [44444444] ;end of prima procesare
	lsu1.ld128.u8.u16 v15 i6 || iau.add i6 i6 1 || vau.macn.u16 v7 v22 || lsu0.swzv8 [11111111]
	
	lsu1.ld128.u8.u16 v16 i0 || iau.add i0 i0 1 || vau.macp.u16 v8 v22 || lsu0.swzv8 [00000000]
	lsu1.ld128.u8.u16 v17 i1 || iau.add i1 i1 1 || vau.macp.u16 v10 v22 || lsu0.swzv8 [00000000]
	lsu1.ld128.u8.u16 v18 i2 || iau.add i2 i2 1 || vau.macp.u16  v11 v23 || lsu0.swzv8 [22222222]
	lsu1.ld128.u8.u16 v0 i3 || iau.add i3 i3 1 || vau.macp.u16  v13 v22 || lsu0.swzv8 [00000000]
	lsu1.ld128.u8.u16 v1 i4 || iau.add i4 i4 1 || vau.macp.u16  v14 v22 || lsu0.swzv8 [00000000]
	lsu1.ld128.u8.u16 v2 i5 || iau.add i5 i5 1 ||  vau.macn.u16 v15 v22 || lsu0.swzv8 [11111111]; end 2
	lsu1.ld128.u8.u16 v3 i6 || iau.add i6 i6 1 || vau.macn.u16 v16 v23 || lsu0.swzv8 [11111111]
	
	lsu1.ld128.u8.u16 v4 i0 || iau.add i0 i0 1 || vau.macp.u16 v17 v23 || lsu0.swzv8 [22222222]
	lsu1.ld128.u8.u16 v5 i1 || iau.add i1 i1 2 || vau.macp.u16 v18 v22 || lsu0.swzv8 [22222222]
	lsu1.ld128.u8.u16 v6 i2 || iau.add i2 i2 1 || vau.macp.u16 v0 v22 || lsu0.swzv8 [33333333]
	lsu1.ld128.u8.u16 v7 i3 || iau.add i3 i3 1 || vau.macp.u16 v1 v22 || lsu0.swzv8 [22222222]
	lsu1.ld128.u8.u16 v8 i4 || iau.add i4 i4 1 || vau.macp.u16 v2 v23 || lsu0.swzv8 [22222222] 
	lsu1.ld128.u8.u16 v9 i5 || iau.add i5 i5 2 || vau.macn.u16 v3 v23 || lsu0.swzv8 [11111111] ;;end 3
	lsu1.ld128.u8.u16 v10 i6 || iau.add i6 i6 1 || vau.macn.u16 v4 v23 || lsu0.swzv8 [00000000]
	
	lsu1.ld128.u8.u16 v11 i0 || iau.add i0 i0 1 || vau.macp.u16 v5 v22 || lsu0.swzv8 [00000000]
	lsu1.ld128.u8.u16 v13 i1 || iau.add i1 i1 2 || vau.macp.u16 v6 v22 || lsu0.swzv8 [33333333]
	lsu1.ld128.u8.u16 v14 i2 || iau.add i2 i2 1 || vau.macp.u16 v7 v23 || lsu0.swzv8 [33333333]
	lsu1.ld128.u8.u16 v15 i3 || iau.add i3 i3 1 || vau.macp.u16 v8 v22 || lsu0.swzv8 [33333333]
	lsu1.ld128.u8.u16 v16 i4 || iau.add i4 i4 1 || vau.macp.u16 v9 v22 || lsu0.swzv8 [00000000]
	lsu1.ld128.u8.u16 v17 i5 || iau.add i5 i5 2 || vau.macn.u16 v10 v23 || lsu0.swzv8 [00000000] ;end 4
	lsu1.ld128.u8.u16 v18 i6 || iau.add i6 i6 1 || vau.macn.u16 v11 v23 || lsu0.swzv8 [11111111] 
	
	lsu1.ld128.u8.u16 v0 i0 || iau.add i0 i0 1 || vau.macn.u16 v13 v22 || lsu0.swzv8 [11111111]
	lsu1.ld128.u8.u16 v1 i2 || iau.add i2 i2 1 || vau.macp.u16 v14 v22 || lsu0.swzv8 [22222222]
	lsu1.ld128.u8.u16 v2 i3 || iau.add i3 i3 1 || vau.macp.u16 v15 v22 || lsu0.swzv8 [33333333]
	lsu1.ld128.u8.u16 v3 i4 || iau.add i4 i4 1 || vau.macp.u16 v16 v22 || lsu0.swzv8 [22222222]
	lsu1.ld128.u8.u16 v4 i6 || iau.add i6 i6 1 || vau.macn.u16 v17 v22 || lsu0.swzv8 [11111111]
	
	lsu1.ld128.u8.u16 v5 i0 || iau.add i0 i0 2 || vau.macn.u16 v18 v23 || lsu0.swzv8 [11111111] ;end 5
	lsu1.ld128.u8.u16 v6 i2 || iau.add i2 i2 2 || vau.macn.u16 v0 v22 || lsu0.swzv8 [11111111]
	lsu1.ld128.u8.u16 v7 i3 || iau.add i3 i3 2 || vau.macp.u16 v1 v22 || lsu0.swzv8 [00000000]
	lsu1.ld128.u8.u16 v8 i4 || iau.add i4 i4 2 || vau.macp.u16 v2 v23 || lsu0.swzv8 [22222222]
	lsu1.ld128.u8.u16 v9 i6 || iau.add i6 i6 2 || vau.macp.u16 v3 v22 || lsu0.swzv8 [00000000]
	
	vau.macn.u16 v4 v22 || lsu0.swzv8 [11111111] 
	vau.macn.u16 v5 v22 || lsu0.swzv8 [44444444] 
    vau.macn.u16 v6 v23 || lsu0.swzv8 [11111111] 
	vau.macn.u16 v7 v23 || lsu0.swzv8 [00000000]
	vau.macn.u16 v8 v23 || lsu0.swzv8 [11111111]  
	vau.macnw.u16 v12 v9 v22 || lsu0.swzv8 [44444444]  
	___loop:
	lsu1.ld128.u8.u16 v0 i0 || iau.add i0 i0 1 
	lsu1.ld128.u8.u16 v1 i1 || iau.add i1 i1 2	
	lsu1.ld128.u8.u16 v2 i2 || iau.add i2 i2 1 
	cmu.clamp0.i16 v19 v12 v21 || lsu1.ld128.u8.u16 v3 i3 || iau.add i3 i3 1 
	lsu1.ld128.u8.u16 v4 i4 || iau.add i4 i4 1
	lsu0.st128.u16.u8 v19 i17 || iau.add i17 i17 8 ||lsu1.ld128.u8.u16 v5 i5 || sau.add.i32 i5 i5 2 
	

    BRU.jmp i30
    nop 5

.end
 

	;bru.swih 0x1F
	;nop 5
	
