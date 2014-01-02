; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief
; ///

;--------------------------------------------------------------------------------------------------------------
.version 00.50.05
.data .rodata.boxfilter15x15_asm 
.salign 16


.code .text.boxfilter15x15_asm ;text
.salign 16

boxfilter15x15_asm:
;void boxfilter13x13(u8** in, u8** out, u8 normalize, u32 width);
;                     i18      i17        i16          i15

	LSU0.ldil i5, ___second || LSU1.ldih i5, ___second
	LSU0.ldil i13, 0x1 || LSU1.ldih i13, 0x0
	CMU.CMII.U8 i16 i13
	PEU.PC1C NEQ || BRU.JMP i5
	nop 5


lsu0.ldil i1, 0x1      ||  lsu1.ldih i1, 0x0
lsu0.ldil i2, 0xE1     ||  lsu1.ldih i2, 0x0
CMU.CPIS.i16.f16  s1 i1
CMU.CPIS.i16.f16  s2 i2
LSU0.LD32  i0  i18 || IAU.ADD i18 i18 4
SAU.div.f16 s3 s1 s2
LSU0.LD32  i1  i18 || IAU.ADD i18 i18 4
LSU0.LD32  i2  i18 || IAU.ADD i18 i18 4
LSU0.LD32  i3  i18 || IAU.ADD i18 i18 4
LSU0.LD32  i4  i18 || IAU.ADD i18 i18 4
LSU0.LD32  i5  i18 || IAU.ADD i18 i18 4
LSU0.LD32  i6  i18 || IAU.ADD i18 i18 4
LSU0.LD32  i7  i18 || IAU.ADD i18 i18 4
LSU0.LD32  i8  i18 || IAU.ADD i18 i18 4
LSU0.LD32  i9  i18 || IAU.ADD i18 i18 4
LSU0.LD32  i10 i18 || IAU.ADD i18 i18 4
LSU0.LD32  i11 i18 || IAU.ADD i18 i18 4
LSU0.LD32  i12 i18 || IAU.ADD i18 i18 4
LSU0.LD32  i13 i18 || IAU.ADD i18 i18 4
LSU0.LD32  i14 i18 || IAU.ADD i18 i18 4
LSU0.LD32  i17 i17
CMU.CPSVR.x16 v0 s3
nop 5



lsu0.ldil i16, ___loop     ||  lsu1.ldih i16, ___loop
IAU.SHR.u32 i15 i15 3

VAU.XOR v18 v18 v18
VAU.XOR v19 v19 v19
VAU.XOR v20 v20 v20
VAU.XOR v21 v21 v21
VAU.XOR v22 v22 v22
VAU.XOR v23 v23 v23


LSU0.LD128.u8.f16 v1  i0    || IAU.ADD i0  i0  1 
LSU0.LD128.u8.f16 v2  i1    || IAU.ADD i1  i1  1
LSU0.LD128.u8.f16 v3  i2    || IAU.ADD i2  i2  1
LSU0.LD128.u8.f16 v4  i3    || IAU.ADD i3  i3  1 || VAU.XOR v18 v18 v18
LSU0.LD128.u8.f16 v5  i4    || IAU.ADD i4  i4  1 || VAU.XOR v19 v19 v19
LSU0.LD128.u8.f16 v6  i5    || IAU.ADD i5  i5  1 || VAU.XOR v20 v20 v20
LSU0.LD128.u8.f16 v7  i6    || IAU.ADD i6  i6  1 || VAU.add.f16 v18 v18 v1 
LSU0.LD128.u8.f16 v8  i7    || IAU.ADD i7  i7  1 || VAU.add.f16 v19 v19 v2 || bru.rpl i16 i15 ;one
LSU0.LD128.u8.f16 v9  i8    || IAU.ADD i8  i8  1 || VAU.add.f16 v20 v20 v3 
LSU0.LD128.u8.f16 v10 i9    || IAU.ADD i9  i9  1 || VAU.add.f16 v18 v18 v4 
LSU0.LD128.u8.f16 v11 i10   || IAU.ADD i10 i10 1 || VAU.add.f16 v19 v19 v5 
LSU0.LD128.u8.f16 v12 i11   || IAU.ADD i11 i11 1 || VAU.add.f16 v20 v20 v6 
LSU0.LD128.u8.f16 v13 i12   || IAU.ADD i12 i12 1 || VAU.add.f16 v18 v18 v7 
LSU0.LD128.u8.f16 v14 i13   || IAU.ADD i13 i13 1 || VAU.add.f16 v19 v19 v8 
LSU0.LD128.u8.f16 v15 i14   || IAU.ADD i14 i14 1 || VAU.add.f16 v20 v20 v9 
LSU0.LD128.u8.f16 v1  i0    || IAU.ADD i0  i0  1 || VAU.add.f16 v18 v18 v10 ;two
LSU0.LD128.u8.f16 v2  i1    || IAU.ADD i1  i1  1 || VAU.add.f16 v19 v19 v11
LSU0.LD128.u8.f16 v3  i2    || IAU.ADD i2  i2  1 || VAU.add.f16 v20 v20 v12
LSU0.LD128.u8.f16 v4  i3    || IAU.ADD i3  i3  1 || VAU.add.f16 v18 v18 v13
LSU0.LD128.u8.f16 v5  i4    || IAU.ADD i4  i4  1 || VAU.add.f16 v19 v19 v14
LSU0.LD128.u8.f16 v6  i5    || IAU.ADD i5  i5  1 || VAU.add.f16 v20 v20 v15
LSU0.LD128.u8.f16 v7  i6    || IAU.ADD i6  i6  1 || VAU.add.f16 v18 v18 v1 
LSU0.LD128.u8.f16 v8  i7    || IAU.ADD i7  i7  1 || VAU.add.f16 v19 v19 v2 
LSU0.LD128.u8.f16 v9  i8    || IAU.ADD i8  i8  1 || VAU.add.f16 v20 v20 v3 
LSU0.LD128.u8.f16 v10 i9    || IAU.ADD i9  i9  1 || VAU.add.f16 v18 v18 v4 
LSU0.LD128.u8.f16 v11 i10   || IAU.ADD i10 i10 1 || VAU.add.f16 v19 v19 v5 
LSU0.LD128.u8.f16 v12 i11   || IAU.ADD i11 i11 1 || VAU.add.f16 v20 v20 v6 
LSU0.LD128.u8.f16 v13 i12   || IAU.ADD i12 i12 1 || VAU.add.f16 v18 v18 v7 
LSU0.LD128.u8.f16 v14 i13   || IAU.ADD i13 i13 1 || VAU.add.f16 v19 v19 v8 
LSU0.LD128.u8.f16 v15 i14   || IAU.ADD i14 i14 1 || VAU.add.f16 v20 v20 v9 
LSU0.LD128.u8.f16 v1  i0    || IAU.ADD i0  i0  1 || VAU.add.f16 v18 v18 v10 ;three
LSU0.LD128.u8.f16 v2  i1    || IAU.ADD i1  i1  1 || VAU.add.f16 v19 v19 v11
LSU0.LD128.u8.f16 v3  i2    || IAU.ADD i2  i2  1 || VAU.add.f16 v20 v20 v12
LSU0.LD128.u8.f16 v4  i3    || IAU.ADD i3  i3  1 || VAU.add.f16 v18 v18 v13
LSU0.LD128.u8.f16 v5  i4    || IAU.ADD i4  i4  1 || VAU.add.f16 v19 v19 v14
LSU0.LD128.u8.f16 v6  i5    || IAU.ADD i5  i5  1 || VAU.add.f16 v20 v20 v15
LSU0.LD128.u8.f16 v7  i6    || IAU.ADD i6  i6  1 || VAU.add.f16 v18 v18 v1 
LSU0.LD128.u8.f16 v8  i7    || IAU.ADD i7  i7  1 || VAU.add.f16 v19 v19 v2 
LSU0.LD128.u8.f16 v9  i8    || IAU.ADD i8  i8  1 || VAU.add.f16 v20 v20 v3 
LSU0.LD128.u8.f16 v10 i9    || IAU.ADD i9  i9  1 || VAU.add.f16 v18 v18 v4 
LSU0.LD128.u8.f16 v11 i10   || IAU.ADD i10 i10 1 || VAU.add.f16 v19 v19 v5 
LSU0.LD128.u8.f16 v12 i11   || IAU.ADD i11 i11 1 || VAU.add.f16 v20 v20 v6 
LSU0.LD128.u8.f16 v13 i12   || IAU.ADD i12 i12 1 || VAU.add.f16 v21 v18 v7 
LSU0.LD128.u8.f16 v14 i13   || IAU.ADD i13 i13 1 || VAU.add.f16 v22 v19 v8 
LSU0.LD128.u8.f16 v15 i14   || IAU.ADD i14 i14 1 || VAU.add.f16 v23 v20 v9 
LSU0.LD128.u8.f16 v1  i0    || IAU.ADD i0  i0  1 || VAU.add.f16 v21 v21 v10 ;four
LSU0.LD128.u8.f16 v2  i1    || IAU.ADD i1  i1  1 || VAU.add.f16 v22 v22 v11
LSU0.LD128.u8.f16 v3  i2    || IAU.ADD i2  i2  1 || VAU.add.f16 v23 v23 v12
LSU0.LD128.u8.f16 v4  i3    || IAU.ADD i3  i3  1 || VAU.add.f16 v21 v21 v13 
LSU0.LD128.u8.f16 v5  i4    || IAU.ADD i4  i4  1 || VAU.add.f16 v22 v22 v14
LSU0.LD128.u8.f16 v6  i5    || IAU.ADD i5  i5  1 || VAU.add.f16 v23 v23 v15
LSU0.LD128.u8.f16 v7  i6    || IAU.ADD i6  i6  1 || VAU.add.f16 v21 v21 v1  
LSU0.LD128.u8.f16 v8  i7    || IAU.ADD i7  i7  1 || VAU.add.f16 v22 v22 v2 
LSU0.LD128.u8.f16 v9  i8    || IAU.ADD i8  i8  1 || VAU.add.f16 v23 v23 v3 
LSU0.LD128.u8.f16 v10 i9    || IAU.ADD i9  i9  1 || VAU.add.f16 v21 v21 v4  
LSU0.LD128.u8.f16 v11 i10   || IAU.ADD i10 i10 1 || VAU.add.f16 v22 v22 v5 
LSU0.LD128.u8.f16 v12 i11   || IAU.ADD i11 i11 1 || VAU.add.f16 v23 v23 v6 
LSU0.LD128.u8.f16 v13 i12   || IAU.ADD i12 i12 1 || VAU.add.f16 v21 v21 v7  
LSU0.LD128.u8.f16 v14 i13   || IAU.ADD i13 i13 1 || VAU.add.f16 v22 v22 v8 
LSU0.LD128.u8.f16 v15 i14   || IAU.ADD i14 i14 1 || VAU.add.f16 v23 v23 v9 
LSU0.LD128.u8.f16 v1  i0    || IAU.ADD i0  i0  1 || VAU.add.f16 v21 v21 v10  ;five
LSU0.LD128.u8.f16 v2  i1    || IAU.ADD i1  i1  1 || VAU.add.f16 v22 v22 v11
LSU0.LD128.u8.f16 v3  i2    || IAU.ADD i2  i2  1 || VAU.add.f16 v23 v23 v12
LSU0.LD128.u8.f16 v4  i3    || IAU.ADD i3  i3  1 || VAU.add.f16 v21 v21 v13 
LSU0.LD128.u8.f16 v5  i4    || IAU.ADD i4  i4  1 || VAU.add.f16 v22 v22 v14
LSU0.LD128.u8.f16 v6  i5    || IAU.ADD i5  i5  1 || VAU.add.f16 v23 v23 v15
LSU0.LD128.u8.f16 v7  i6    || IAU.ADD i6  i6  1 || VAU.add.f16 v21 v21 v1  
LSU0.LD128.u8.f16 v8  i7    || IAU.ADD i7  i7  1 || VAU.add.f16 v22 v22 v2 
LSU0.LD128.u8.f16 v9  i8    || IAU.ADD i8  i8  1 || VAU.add.f16 v23 v23 v3 
LSU0.LD128.u8.f16 v10 i9    || IAU.ADD i9  i9  1 || VAU.add.f16 v21 v21 v4  
LSU0.LD128.u8.f16 v11 i10   || IAU.ADD i10 i10 1 || VAU.add.f16 v22 v22 v5 
LSU0.LD128.u8.f16 v12 i11   || IAU.ADD i11 i11 1 || VAU.add.f16 v23 v23 v6 
LSU0.LD128.u8.f16 v13 i12   || IAU.ADD i12 i12 1 || VAU.add.f16 v21 v21 v7  
LSU0.LD128.u8.f16 v14 i13   || IAU.ADD i13 i13 1 || VAU.add.f16 v22 v22 v8 
LSU0.LD128.u8.f16 v15 i14   || IAU.ADD i14 i14 1 || VAU.add.f16 v23 v23 v9 
LSU0.LD128.u8.f16 v1  i0    || IAU.ADD i0  i0  1 || VAU.add.f16 v21 v21 v10  ;six
LSU0.LD128.u8.f16 v2  i1    || IAU.ADD i1  i1  1 || VAU.add.f16 v22 v22 v11
LSU0.LD128.u8.f16 v3  i2    || IAU.ADD i2  i2  1 || VAU.add.f16 v23 v23 v12
LSU0.LD128.u8.f16 v4  i3    || IAU.ADD i3  i3  1 || VAU.add.f16 v21 v21 v13 
LSU0.LD128.u8.f16 v5  i4    || IAU.ADD i4  i4  1 || VAU.add.f16 v22 v22 v14
LSU0.LD128.u8.f16 v6  i5    || IAU.ADD i5  i5  1 || VAU.add.f16 v23 v23 v15
LSU0.LD128.u8.f16 v7  i6    || IAU.ADD i6  i6  1 || VAU.add.f16 v21 v21 v1  
LSU0.LD128.u8.f16 v8  i7    || IAU.ADD i7  i7  1 || VAU.add.f16 v22 v22 v2 
LSU0.LD128.u8.f16 v9  i8    || IAU.ADD i8  i8  1 || VAU.add.f16 v23 v23 v3 
LSU0.LD128.u8.f16 v10 i9    || IAU.ADD i9  i9  1 || VAU.add.f16 v21 v21 v4  
LSU0.LD128.u8.f16 v11 i10   || IAU.ADD i10 i10 1 || VAU.add.f16 v22 v22 v5 
LSU0.LD128.u8.f16 v12 i11   || IAU.ADD i11 i11 1 || VAU.add.f16 v23 v23 v6 
LSU0.LD128.u8.f16 v13 i12   || IAU.ADD i12 i12 1 || VAU.add.f16 v21 v21 v7  
LSU0.LD128.u8.f16 v14 i13   || IAU.ADD i13 i13 1 || VAU.add.f16 v22 v22 v8 
LSU0.LD128.u8.f16 v15 i14   || IAU.ADD i14 i14 1 || VAU.add.f16 v23 v23 v9 
LSU0.LD128.u8.f16 v1  i0    || IAU.ADD i0  i0  1 || VAU.add.f16 v21 v21 v10  ;seven
LSU0.LD128.u8.f16 v2  i1    || IAU.ADD i1  i1  1 || VAU.add.f16 v22 v22 v11
LSU0.LD128.u8.f16 v3  i2    || IAU.ADD i2  i2  1 || VAU.add.f16 v23 v23 v12
LSU0.LD128.u8.f16 v4  i3    || IAU.ADD i3  i3  1 || VAU.add.f16 v21 v21 v13 
LSU0.LD128.u8.f16 v5  i4    || IAU.ADD i4  i4  1 || VAU.add.f16 v22 v22 v14
LSU0.LD128.u8.f16 v6  i5    || IAU.ADD i5  i5  1 || VAU.add.f16 v23 v23 v15
LSU0.LD128.u8.f16 v7  i6    || IAU.ADD i6  i6  1 || VAU.add.f16 v21 v21 v1  
LSU0.LD128.u8.f16 v8  i7    || IAU.ADD i7  i7  1 || VAU.add.f16 v22 v22 v2 
LSU0.LD128.u8.f16 v9  i8    || IAU.ADD i8  i8  1 || VAU.add.f16 v23 v23 v3 
LSU0.LD128.u8.f16 v10 i9    || IAU.ADD i9  i9  1 || VAU.add.f16 v21 v21 v4  
LSU0.LD128.u8.f16 v11 i10   || IAU.ADD i10 i10 1 || VAU.add.f16 v22 v22 v5 
LSU0.LD128.u8.f16 v12 i11   || IAU.ADD i11 i11 1 || VAU.add.f16 v23 v23 v6 
LSU0.LD128.u8.f16 v13 i12   || IAU.ADD i12 i12 1 || VAU.add.f16 v21 v21 v7  
LSU0.LD128.u8.f16 v14 i13   || IAU.ADD i13 i13 1 || VAU.add.f16 v22 v22 v8 
LSU0.LD128.u8.f16 v15 i14   || IAU.ADD i14 i14 1 || VAU.add.f16 v23 v23 v9 
LSU0.LD128.u8.f16 v1  i0    || IAU.ADD i0  i0  1 || VAU.add.f16 v21 v21 v10  ;eight
LSU0.LD128.u8.f16 v2  i1    || IAU.ADD i1  i1  1 || VAU.add.f16 v22 v22 v11
LSU0.LD128.u8.f16 v3  i2    || IAU.ADD i2  i2  1 || VAU.add.f16 v23 v23 v12
LSU0.LD128.u8.f16 v4  i3    || IAU.ADD i3  i3  1 || VAU.add.f16 v21 v21 v13 
LSU0.LD128.u8.f16 v5  i4    || IAU.ADD i4  i4  1 || VAU.add.f16 v22 v22 v14
LSU0.LD128.u8.f16 v6  i5    || IAU.ADD i5  i5  1 || VAU.add.f16 v23 v23 v15
LSU0.LD128.u8.f16 v7  i6    || IAU.ADD i6  i6  1 || VAU.add.f16 v21 v21 v1  
LSU0.LD128.u8.f16 v8  i7    || IAU.ADD i7  i7  1 || VAU.add.f16 v22 v22 v2 
LSU0.LD128.u8.f16 v9  i8    || IAU.ADD i8  i8  1 || VAU.add.f16 v23 v23 v3 
LSU0.LD128.u8.f16 v10 i9    || IAU.ADD i9  i9  1 || VAU.add.f16 v21 v21 v4  
LSU0.LD128.u8.f16 v11 i10   || IAU.ADD i10 i10 1 || VAU.add.f16 v22 v22 v5 
LSU0.LD128.u8.f16 v12 i11   || IAU.ADD i11 i11 1 || VAU.add.f16 v23 v23 v6 
LSU0.LD128.u8.f16 v13 i12   || IAU.ADD i12 i12 1 || VAU.add.f16 v21 v21 v7  
LSU0.LD128.u8.f16 v14 i13   || IAU.ADD i13 i13 1 || VAU.add.f16 v22 v22 v8 
LSU0.LD128.u8.f16 v15 i14   || IAU.ADD i14 i14 1 || VAU.add.f16 v23 v23 v9 
LSU0.LD128.u8.f16 v1  i0    || IAU.ADD i0  i0  1 || VAU.add.f16 v21 v21 v10  ;nine
LSU0.LD128.u8.f16 v2  i1    || IAU.ADD i1  i1  1 || VAU.add.f16 v22 v22 v11
LSU0.LD128.u8.f16 v3  i2    || IAU.ADD i2  i2  1 || VAU.add.f16 v23 v23 v12
LSU0.LD128.u8.f16 v4  i3    || IAU.ADD i3  i3  1 || VAU.add.f16 v21 v21 v13 
LSU0.LD128.u8.f16 v5  i4    || IAU.ADD i4  i4  1 || VAU.add.f16 v22 v22 v14
LSU0.LD128.u8.f16 v6  i5    || IAU.ADD i5  i5  1 || VAU.add.f16 v23 v23 v15
LSU0.LD128.u8.f16 v7  i6    || IAU.ADD i6  i6  1 || VAU.add.f16 v21 v21 v1  
LSU0.LD128.u8.f16 v8  i7    || IAU.ADD i7  i7  1 || VAU.add.f16 v22 v22 v2 
LSU0.LD128.u8.f16 v9  i8    || IAU.ADD i8  i8  1 || VAU.add.f16 v23 v23 v3 
LSU0.LD128.u8.f16 v10 i9    || IAU.ADD i9  i9  1 || VAU.add.f16 v21 v21 v4  
LSU0.LD128.u8.f16 v11 i10   || IAU.ADD i10 i10 1 || VAU.add.f16 v22 v22 v5 
LSU0.LD128.u8.f16 v12 i11   || IAU.ADD i11 i11 1 || VAU.add.f16 v23 v23 v6 
LSU0.LD128.u8.f16 v13 i12   || IAU.ADD i12 i12 1 || VAU.add.f16 v21 v21 v7  
LSU0.LD128.u8.f16 v14 i13   || IAU.ADD i13 i13 1 || VAU.add.f16 v22 v22 v8 
LSU0.LD128.u8.f16 v15 i14   || IAU.ADD i14 i14 1 || VAU.add.f16 v23 v23 v9 	
LSU0.LD128.u8.f16 v1  i0    || IAU.ADD i0  i0  1 || VAU.add.f16 v21 v21 v10  ;ten
LSU0.LD128.u8.f16 v2  i1    || IAU.ADD i1  i1  1 || VAU.add.f16 v22 v22 v11
LSU0.LD128.u8.f16 v3  i2    || IAU.ADD i2  i2  1 || VAU.add.f16 v23 v23 v12
LSU0.LD128.u8.f16 v4  i3    || IAU.ADD i3  i3  1 || VAU.add.f16 v21 v21 v13 
LSU0.LD128.u8.f16 v5  i4    || IAU.ADD i4  i4  1 || VAU.add.f16 v22 v22 v14
LSU0.LD128.u8.f16 v6  i5    || IAU.ADD i5  i5  1 || VAU.add.f16 v23 v23 v15
LSU0.LD128.u8.f16 v7  i6    || IAU.ADD i6  i6  1 || VAU.add.f16 v21 v21 v1  
LSU0.LD128.u8.f16 v8  i7    || IAU.ADD i7  i7  1 || VAU.add.f16 v22 v22 v2 
LSU0.LD128.u8.f16 v9  i8    || IAU.ADD i8  i8  1 || VAU.add.f16 v23 v23 v3 
LSU0.LD128.u8.f16 v10 i9    || IAU.ADD i9  i9  1 || VAU.add.f16 v21 v21 v4  
LSU0.LD128.u8.f16 v11 i10   || IAU.ADD i10 i10 1 || VAU.add.f16 v22 v22 v5 
LSU0.LD128.u8.f16 v12 i11   || IAU.ADD i11 i11 1 || VAU.add.f16 v23 v23 v6 
LSU0.LD128.u8.f16 v13 i12   || IAU.ADD i12 i12 1 || VAU.add.f16 v21 v21 v7  
LSU0.LD128.u8.f16 v14 i13   || IAU.ADD i13 i13 1 || VAU.add.f16 v22 v22 v8 
LSU0.LD128.u8.f16 v15 i14   || IAU.ADD i14 i14 1 || VAU.add.f16 v23 v23 v9 
LSU0.LD128.u8.f16 v1  i0    || IAU.ADD i0  i0  1 || VAU.add.f16 v21 v21 v10  ;eleven
LSU0.LD128.u8.f16 v2  i1    || IAU.ADD i1  i1  1 || VAU.add.f16 v22 v22 v11
LSU0.LD128.u8.f16 v3  i2    || IAU.ADD i2  i2  1 || VAU.add.f16 v23 v23 v12
LSU0.LD128.u8.f16 v4  i3    || IAU.ADD i3  i3  1 || VAU.add.f16 v21 v21 v13 
LSU0.LD128.u8.f16 v5  i4    || IAU.ADD i4  i4  1 || VAU.add.f16 v22 v22 v14
LSU0.LD128.u8.f16 v6  i5    || IAU.ADD i5  i5  1 || VAU.add.f16 v23 v23 v15
LSU0.LD128.u8.f16 v7  i6    || IAU.ADD i6  i6  1 || VAU.add.f16 v21 v21 v1  
LSU0.LD128.u8.f16 v8  i7    || IAU.ADD i7  i7  1 || VAU.add.f16 v22 v22 v2 
LSU0.LD128.u8.f16 v9  i8    || IAU.ADD i8  i8  1 || VAU.add.f16 v23 v23 v3 
LSU0.LD128.u8.f16 v10 i9    || IAU.ADD i9  i9  1 || VAU.add.f16 v21 v21 v4  
LSU0.LD128.u8.f16 v11 i10   || IAU.ADD i10 i10 1 || VAU.add.f16 v22 v22 v5 
LSU0.LD128.u8.f16 v12 i11   || IAU.ADD i11 i11 1 || VAU.add.f16 v23 v23 v6 
LSU0.LD128.u8.f16 v13 i12   || IAU.ADD i12 i12 1 || VAU.add.f16 v21 v21 v7  
LSU0.LD128.u8.f16 v14 i13   || IAU.ADD i13 i13 1 || VAU.add.f16 v22 v22 v8 
LSU0.LD128.u8.f16 v15 i14   || IAU.ADD i14 i14 1 || VAU.add.f16 v23 v23 v9 
LSU0.LD128.u8.f16 v1  i0    || IAU.ADD i0  i0  1 || VAU.add.f16 v21 v21 v10  ;twelve
LSU0.LD128.u8.f16 v2  i1    || IAU.ADD i1  i1  1 || VAU.add.f16 v22 v22 v11
LSU0.LD128.u8.f16 v3  i2    || IAU.ADD i2  i2  1 || VAU.add.f16 v23 v23 v12
LSU0.LD128.u8.f16 v4  i3    || IAU.ADD i3  i3  1 || VAU.add.f16 v21 v21 v13 
LSU0.LD128.u8.f16 v5  i4    || IAU.ADD i4  i4  1 || VAU.add.f16 v22 v22 v14
LSU0.LD128.u8.f16 v6  i5    || IAU.ADD i5  i5  1 || VAU.add.f16 v23 v23 v15
LSU0.LD128.u8.f16 v7  i6    || IAU.ADD i6  i6  1 || VAU.add.f16 v21 v21 v1  
LSU0.LD128.u8.f16 v8  i7    || IAU.ADD i7  i7  1 || VAU.add.f16 v22 v22 v2 
LSU0.LD128.u8.f16 v9  i8    || IAU.ADD i8  i8  1 || VAU.add.f16 v23 v23 v3 
LSU0.LD128.u8.f16 v10 i9    || IAU.ADD i9  i9  1 || VAU.add.f16 v21 v21 v4  
LSU0.LD128.u8.f16 v11 i10   || IAU.ADD i10 i10 1 || VAU.add.f16 v22 v22 v5 
LSU0.LD128.u8.f16 v12 i11   || IAU.ADD i11 i11 1 || VAU.add.f16 v23 v23 v6 
LSU0.LD128.u8.f16 v13 i12   || IAU.ADD i12 i12 1 || VAU.add.f16 v21 v21 v7  
LSU0.LD128.u8.f16 v14 i13   || IAU.ADD i13 i13 1 || VAU.add.f16 v22 v22 v8 
LSU0.LD128.u8.f16 v15 i14   || IAU.ADD i14 i14 1 || VAU.add.f16 v23 v23 v9 
LSU0.LD128.u8.f16 v1  i0    || IAU.ADD i0  i0  1 || VAU.add.f16 v21 v21 v10  ;thirteen
LSU0.LD128.u8.f16 v2  i1    || IAU.ADD i1  i1  1 || VAU.add.f16 v22 v22 v11
LSU0.LD128.u8.f16 v3  i2    || IAU.ADD i2  i2  1 || VAU.add.f16 v23 v23 v12
LSU0.LD128.u8.f16 v4  i3    || IAU.ADD i3  i3  1 || VAU.add.f16 v21 v21 v13 
LSU0.LD128.u8.f16 v5  i4    || IAU.ADD i4  i4  1 || VAU.add.f16 v22 v22 v14
LSU0.LD128.u8.f16 v6  i5    || IAU.ADD i5  i5  1 || VAU.add.f16 v23 v23 v15
LSU0.LD128.u8.f16 v7  i6    || IAU.ADD i6  i6  1 || VAU.add.f16 v21 v21 v1  
LSU0.LD128.u8.f16 v8  i7    || IAU.ADD i7  i7  1 || VAU.add.f16 v22 v22 v2 
LSU0.LD128.u8.f16 v9  i8    || IAU.ADD i8  i8  1 || VAU.add.f16 v23 v23 v3 
LSU0.LD128.u8.f16 v10 i9    || IAU.ADD i9  i9  1 || VAU.add.f16 v21 v21 v4  
LSU0.LD128.u8.f16 v11 i10   || IAU.ADD i10 i10 1 || VAU.add.f16 v22 v22 v5 
LSU0.LD128.u8.f16 v12 i11   || IAU.ADD i11 i11 1 || VAU.add.f16 v23 v23 v6 
LSU0.LD128.u8.f16 v13 i12   || IAU.ADD i12 i12 1 || VAU.add.f16 v21 v21 v7  
LSU0.LD128.u8.f16 v14 i13   || IAU.ADD i13 i13 1 || VAU.add.f16 v22 v22 v8 
LSU0.LD128.u8.f16 v15 i14   || IAU.ADD i14 i14 1 || VAU.add.f16 v23 v23 v9 
LSU0.LD128.u8.f16 v1  i0    || IAU.ADD i0  i0  1 || VAU.add.f16 v21 v21 v10  ;fourtheen
LSU0.LD128.u8.f16 v2  i1    || IAU.ADD i1  i1  1 || VAU.add.f16 v22 v22 v11
LSU0.LD128.u8.f16 v3  i2    || IAU.ADD i2  i2  1 || VAU.add.f16 v23 v23 v12
LSU0.LD128.u8.f16 v4  i3    || IAU.ADD i3  i3  1 || VAU.add.f16 v21 v21 v13 
LSU0.LD128.u8.f16 v5  i4    || IAU.ADD i4  i4  1 || VAU.add.f16 v22 v22 v14
LSU0.LD128.u8.f16 v6  i5    || IAU.ADD i5  i5  1 || VAU.add.f16 v23 v23 v15
LSU0.LD128.u8.f16 v7  i6    || IAU.ADD i6  i6  1 || VAU.add.f16 v21 v21 v1  
LSU0.LD128.u8.f16 v8  i7    || IAU.ADD i7  i7  1 || VAU.add.f16 v22 v22 v2 
LSU0.LD128.u8.f16 v9  i8    || IAU.ADD i8  i8  1 || VAU.add.f16 v23 v23 v3 
LSU0.LD128.u8.f16 v10 i9    || IAU.ADD i9  i9  1 || VAU.add.f16 v21 v21 v4  
LSU0.LD128.u8.f16 v11 i10   || IAU.ADD i10 i10 1 || VAU.add.f16 v22 v22 v5 
LSU0.LD128.u8.f16 v12 i11   || IAU.ADD i11 i11 1 || VAU.add.f16 v23 v23 v6 
LSU0.LD128.u8.f16 v13 i12   || IAU.ADD i12 i12 1 || VAU.add.f16 v21 v21 v7  
LSU0.LD128.u8.f16 v14 i13   || IAU.ADD i13 i13 1 || VAU.add.f16 v22 v22 v8 
LSU0.LD128.u8.f16 v15 i14   || IAU.ADD i14 i14 1 || VAU.add.f16 v23 v23 v9 
LSU0.LD128.u8.f16 v1  i0    || IAU.SUB i0  i0  6 || VAU.add.f16 v21 v21 v10  ;fiftheen
LSU0.LD128.u8.f16 v2  i1    || IAU.SUB i1  i1  6 || VAU.add.f16 v22 v22 v11
LSU0.LD128.u8.f16 v3  i2    || IAU.SUB i2  i2  6 || VAU.add.f16 v23 v23 v12
LSU0.LD128.u8.f16 v4  i3    || IAU.SUB i3  i3  6 || VAU.add.f16 v21 v21 v13 
LSU0.LD128.u8.f16 v5  i4    || IAU.SUB i4  i4  6 || VAU.add.f16 v22 v22 v14
LSU0.LD128.u8.f16 v6  i5    || IAU.SUB i5  i5  6 || VAU.add.f16 v23 v23 v15
LSU0.LD128.u8.f16 v7  i6    || IAU.SUB i6  i6  6 || VAU.add.f16 v21 v21 v1  
LSU0.LD128.u8.f16 v8  i7    || IAU.SUB i7  i7  6 || VAU.add.f16 v22 v22 v2 
LSU0.LD128.u8.f16 v9  i8    || IAU.SUB i8  i8  6 || VAU.add.f16 v23 v23 v3 
LSU0.LD128.u8.f16 v10 i9    || IAU.SUB i9  i9  6 || VAU.add.f16 v21 v21 v4  
LSU0.LD128.u8.f16 v11 i10   || IAU.SUB i10 i10 6 || VAU.add.f16 v22 v22 v5 
LSU0.LD128.u8.f16 v12 i11   || IAU.SUB i11 i11 6 || VAU.add.f16 v23 v23 v6 
LSU0.LD128.u8.f16 v13 i12   || IAU.SUB i12 i12 6 || VAU.add.f16 v21 v21 v7  
LSU0.LD128.u8.f16 v14 i13   || IAU.SUB i13 i13 6 || VAU.add.f16 v22 v22 v8 
LSU0.LD128.u8.f16 v15 i14   || IAU.SUB i14 i14 6 || VAU.add.f16 v23 v23 v9 
VAU.add.f16 v21 v21 v10
VAU.add.f16 v22 v22 v11
VAU.add.f16 v23 v23 v12
VAU.add.f16 v21 v21 v13 || LSU0.LD128.u8.f16 v1  i0    || IAU.ADD i0  i0  1 
VAU.add.f16 v22 v22 v14 || LSU0.LD128.u8.f16 v2  i1    || IAU.ADD i1  i1  1
VAU.add.f16 v23 v23 v15 || LSU0.LD128.u8.f16 v3  i2    || IAU.ADD i2  i2  1
nop 
VAU.ADD.f16 v22 v21 v22
LSU0.LD128.u8.f16 v4  i3    || IAU.ADD i3  i3  1 || VAU.XOR v18 v18 v18
nop
VAU.ADD.f16 v23 v22 v23
___loop:
LSU0.LD128.u8.f16 v5  i4    || IAU.ADD i4  i4  1 || VAU.XOR v19 v19 v19
nop
VAU.mul.f16 v23 v23 v0
LSU0.LD128.u8.f16 v6  i5    || IAU.ADD i5  i5  1 || VAU.XOR v20 v20 v20
nop
LSU0.STi128.f16.u8 v23 i17 || LSU1.LD128.u8.f16 v7  i6    || SAU.ADD.i32 i6  i6  1 || VAU.add.f16 v18 v18 v1 





bru.jmp i30
nop 5
	
	
___second:


LSU0.LD32  i0  i18 || IAU.ADD i18 i18 4
LSU0.LD32  i1  i18 || IAU.ADD i18 i18 4
LSU0.LD32  i2  i18 || IAU.ADD i18 i18 4
LSU0.LD32  i3  i18 || IAU.ADD i18 i18 4
LSU0.LD32  i4  i18 || IAU.ADD i18 i18 4
LSU0.LD32  i5  i18 || IAU.ADD i18 i18 4
LSU0.LD32  i6  i18 || IAU.ADD i18 i18 4
LSU0.LD32  i7  i18 || IAU.ADD i18 i18 4
LSU0.LD32  i8  i18 || IAU.ADD i18 i18 4
LSU0.LD32  i9  i18 || IAU.ADD i18 i18 4
LSU0.LD32  i10 i18 || IAU.ADD i18 i18 4
LSU0.LD32  i11 i18 || IAU.ADD i18 i18 4
LSU0.LD32  i12 i18 || IAU.ADD i18 i18 4
LSU0.LD32  i13 i18 || IAU.ADD i18 i18 4
LSU0.LD32  i14 i18 || IAU.ADD i18 i18 4
LSU0.LD32  i17 i17
nop 5

lsu0.ldil i16, ___loop1     ||  lsu1.ldih i16, ___loop1
IAU.SHR.u32 i15 i15 3

VAU.XOR v18 v18 v18
VAU.XOR v19 v19 v19
VAU.XOR v20 v20 v20
VAU.XOR v21 v21 v21
VAU.XOR v22 v22 v22
VAU.XOR v23 v23 v23


LSU0.LD128.u8.u16 v1  i0    || IAU.ADD i0  i0  1 
LSU0.LD128.u8.u16 v2  i1    || IAU.ADD i1  i1  1
LSU0.LD128.u8.u16 v3  i2    || IAU.ADD i2  i2  1
LSU0.LD128.u8.u16 v4  i3    || IAU.ADD i3  i3  1 || VAU.XOR v18 v18 v18
LSU0.LD128.u8.u16 v5  i4    || IAU.ADD i4  i4  1 || VAU.XOR v19 v19 v19 || bru.rpl i16 i15 ;one
LSU0.LD128.u8.u16 v6  i5    || IAU.ADD i5  i5  1 || VAU.XOR v20 v20 v20
LSU0.LD128.u8.u16 v7  i6    || IAU.ADD i6  i6  1 || VAU.add.i16 v18 v18 v1 
LSU0.LD128.u8.u16 v8  i7    || IAU.ADD i7  i7  1 || VAU.add.i16 v19 v19 v2 
LSU0.LD128.u8.u16 v9  i8    || IAU.ADD i8  i8  1 || VAU.add.i16 v20 v20 v3 
LSU0.LD128.u8.u16 v10 i9    || IAU.ADD i9  i9  1 || VAU.add.i16 v18 v18 v4 
LSU0.LD128.u8.u16 v11 i10   || IAU.ADD i10 i10 1 || VAU.add.i16 v19 v19 v5 
LSU0.LD128.u8.u16 v12 i11   || IAU.ADD i11 i11 1 || VAU.add.i16 v20 v20 v6 
LSU0.LD128.u8.u16 v13 i12   || IAU.ADD i12 i12 1 || VAU.add.i16 v18 v18 v7 
LSU0.LD128.u8.u16 v14 i13   || IAU.ADD i13 i13 1 || VAU.add.i16 v19 v19 v8 
LSU0.LD128.u8.u16 v15 i14   || IAU.ADD i14 i14 1 || VAU.add.i16 v20 v20 v9 
LSU0.LD128.u8.u16 v1  i0    || IAU.ADD i0  i0  1 || VAU.add.i16 v18 v18 v10 ;two
LSU0.LD128.u8.u16 v2  i1    || IAU.ADD i1  i1  1 || VAU.add.i16 v19 v19 v11
LSU0.LD128.u8.u16 v3  i2    || IAU.ADD i2  i2  1 || VAU.add.i16 v20 v20 v12
LSU0.LD128.u8.u16 v4  i3    || IAU.ADD i3  i3  1 || VAU.add.i16 v18 v18 v13
LSU0.LD128.u8.u16 v5  i4    || IAU.ADD i4  i4  1 || VAU.add.i16 v19 v19 v14
LSU0.LD128.u8.u16 v6  i5    || IAU.ADD i5  i5  1 || VAU.add.i16 v20 v20 v15
LSU0.LD128.u8.u16 v7  i6    || IAU.ADD i6  i6  1 || VAU.add.i16 v18 v18 v1 
LSU0.LD128.u8.u16 v8  i7    || IAU.ADD i7  i7  1 || VAU.add.i16 v19 v19 v2 
LSU0.LD128.u8.u16 v9  i8    || IAU.ADD i8  i8  1 || VAU.add.i16 v20 v20 v3 
LSU0.LD128.u8.u16 v10 i9    || IAU.ADD i9  i9  1 || VAU.add.i16 v18 v18 v4 
LSU0.LD128.u8.u16 v11 i10   || IAU.ADD i10 i10 1 || VAU.add.i16 v19 v19 v5 
LSU0.LD128.u8.u16 v12 i11   || IAU.ADD i11 i11 1 || VAU.add.i16 v20 v20 v6 
LSU0.LD128.u8.u16 v13 i12   || IAU.ADD i12 i12 1 || VAU.add.i16 v18 v18 v7 
LSU0.LD128.u8.u16 v14 i13   || IAU.ADD i13 i13 1 || VAU.add.i16 v19 v19 v8 
LSU0.LD128.u8.u16 v15 i14   || IAU.ADD i14 i14 1 || VAU.add.i16 v20 v20 v9 
LSU0.LD128.u8.u16 v1  i0    || IAU.ADD i0  i0  1 || VAU.add.i16 v18 v18 v10 ;three
LSU0.LD128.u8.u16 v2  i1    || IAU.ADD i1  i1  1 || VAU.add.i16 v19 v19 v11
LSU0.LD128.u8.u16 v3  i2    || IAU.ADD i2  i2  1 || VAU.add.i16 v20 v20 v12
LSU0.LD128.u8.u16 v4  i3    || IAU.ADD i3  i3  1 || VAU.add.i16 v18 v18 v13
LSU0.LD128.u8.u16 v5  i4    || IAU.ADD i4  i4  1 || VAU.add.i16 v19 v19 v14
LSU0.LD128.u8.u16 v6  i5    || IAU.ADD i5  i5  1 || VAU.add.i16 v20 v20 v15
LSU0.LD128.u8.u16 v7  i6    || IAU.ADD i6  i6  1 || VAU.add.i16 v18 v18 v1 
LSU0.LD128.u8.u16 v8  i7    || IAU.ADD i7  i7  1 || VAU.add.i16 v19 v19 v2 
LSU0.LD128.u8.u16 v9  i8    || IAU.ADD i8  i8  1 || VAU.add.i16 v20 v20 v3 
LSU0.LD128.u8.u16 v10 i9    || IAU.ADD i9  i9  1 || VAU.add.i16 v18 v18 v4 
LSU0.LD128.u8.u16 v11 i10   || IAU.ADD i10 i10 1 || VAU.add.i16 v19 v19 v5 
LSU0.LD128.u8.u16 v12 i11   || IAU.ADD i11 i11 1 || VAU.add.i16 v20 v20 v6 
LSU0.LD128.u8.u16 v13 i12   || IAU.ADD i12 i12 1 || VAU.add.i16 v21 v18 v7 
LSU0.LD128.u8.u16 v14 i13   || IAU.ADD i13 i13 1 || VAU.add.i16 v22 v19 v8 
LSU0.LD128.u8.u16 v15 i14   || IAU.ADD i14 i14 1 || VAU.add.i16 v23 v20 v9 
LSU0.LD128.u8.u16 v1  i0    || IAU.ADD i0  i0  1 || VAU.add.i16 v21 v21 v10 ;four
LSU0.LD128.u8.u16 v2  i1    || IAU.ADD i1  i1  1 || VAU.add.i16 v22 v22 v11
LSU0.LD128.u8.u16 v3  i2    || IAU.ADD i2  i2  1 || VAU.add.i16 v23 v23 v12
LSU0.LD128.u8.u16 v4  i3    || IAU.ADD i3  i3  1 || VAU.add.i16 v21 v21 v13 
LSU0.LD128.u8.u16 v5  i4    || IAU.ADD i4  i4  1 || VAU.add.i16 v22 v22 v14
LSU0.LD128.u8.u16 v6  i5    || IAU.ADD i5  i5  1 || VAU.add.i16 v23 v23 v15
LSU0.LD128.u8.u16 v7  i6    || IAU.ADD i6  i6  1 || VAU.add.i16 v21 v21 v1  
LSU0.LD128.u8.u16 v8  i7    || IAU.ADD i7  i7  1 || VAU.add.i16 v22 v22 v2 
LSU0.LD128.u8.u16 v9  i8    || IAU.ADD i8  i8  1 || VAU.add.i16 v23 v23 v3 
LSU0.LD128.u8.u16 v10 i9    || IAU.ADD i9  i9  1 || VAU.add.i16 v21 v21 v4  
LSU0.LD128.u8.u16 v11 i10   || IAU.ADD i10 i10 1 || VAU.add.i16 v22 v22 v5 
LSU0.LD128.u8.u16 v12 i11   || IAU.ADD i11 i11 1 || VAU.add.i16 v23 v23 v6 
LSU0.LD128.u8.u16 v13 i12   || IAU.ADD i12 i12 1 || VAU.add.i16 v21 v21 v7  
LSU0.LD128.u8.u16 v14 i13   || IAU.ADD i13 i13 1 || VAU.add.i16 v22 v22 v8 
LSU0.LD128.u8.u16 v15 i14   || IAU.ADD i14 i14 1 || VAU.add.i16 v23 v23 v9 
LSU0.LD128.u8.u16 v1  i0    || IAU.ADD i0  i0  1 || VAU.add.i16 v21 v21 v10  ;five
LSU0.LD128.u8.u16 v2  i1    || IAU.ADD i1  i1  1 || VAU.add.i16 v22 v22 v11
LSU0.LD128.u8.u16 v3  i2    || IAU.ADD i2  i2  1 || VAU.add.i16 v23 v23 v12
LSU0.LD128.u8.u16 v4  i3    || IAU.ADD i3  i3  1 || VAU.add.i16 v21 v21 v13 
LSU0.LD128.u8.u16 v5  i4    || IAU.ADD i4  i4  1 || VAU.add.i16 v22 v22 v14
LSU0.LD128.u8.u16 v6  i5    || IAU.ADD i5  i5  1 || VAU.add.i16 v23 v23 v15
LSU0.LD128.u8.u16 v7  i6    || IAU.ADD i6  i6  1 || VAU.add.i16 v21 v21 v1  
LSU0.LD128.u8.u16 v8  i7    || IAU.ADD i7  i7  1 || VAU.add.i16 v22 v22 v2 
LSU0.LD128.u8.u16 v9  i8    || IAU.ADD i8  i8  1 || VAU.add.i16 v23 v23 v3 
LSU0.LD128.u8.u16 v10 i9    || IAU.ADD i9  i9  1 || VAU.add.i16 v21 v21 v4  
LSU0.LD128.u8.u16 v11 i10   || IAU.ADD i10 i10 1 || VAU.add.i16 v22 v22 v5 
LSU0.LD128.u8.u16 v12 i11   || IAU.ADD i11 i11 1 || VAU.add.i16 v23 v23 v6 
LSU0.LD128.u8.u16 v13 i12   || IAU.ADD i12 i12 1 || VAU.add.i16 v21 v21 v7  
LSU0.LD128.u8.u16 v14 i13   || IAU.ADD i13 i13 1 || VAU.add.i16 v22 v22 v8 
LSU0.LD128.u8.u16 v15 i14   || IAU.ADD i14 i14 1 || VAU.add.i16 v23 v23 v9 
LSU0.LD128.u8.u16 v1  i0    || IAU.ADD i0  i0  1 || VAU.add.i16 v21 v21 v10  ;six
LSU0.LD128.u8.u16 v2  i1    || IAU.ADD i1  i1  1 || VAU.add.i16 v22 v22 v11
LSU0.LD128.u8.u16 v3  i2    || IAU.ADD i2  i2  1 || VAU.add.i16 v23 v23 v12
LSU0.LD128.u8.u16 v4  i3    || IAU.ADD i3  i3  1 || VAU.add.i16 v21 v21 v13 
LSU0.LD128.u8.u16 v5  i4    || IAU.ADD i4  i4  1 || VAU.add.i16 v22 v22 v14
LSU0.LD128.u8.u16 v6  i5    || IAU.ADD i5  i5  1 || VAU.add.i16 v23 v23 v15
LSU0.LD128.u8.u16 v7  i6    || IAU.ADD i6  i6  1 || VAU.add.i16 v21 v21 v1  
LSU0.LD128.u8.u16 v8  i7    || IAU.ADD i7  i7  1 || VAU.add.i16 v22 v22 v2 
LSU0.LD128.u8.u16 v9  i8    || IAU.ADD i8  i8  1 || VAU.add.i16 v23 v23 v3 
LSU0.LD128.u8.u16 v10 i9    || IAU.ADD i9  i9  1 || VAU.add.i16 v21 v21 v4  
LSU0.LD128.u8.u16 v11 i10   || IAU.ADD i10 i10 1 || VAU.add.i16 v22 v22 v5 
LSU0.LD128.u8.u16 v12 i11   || IAU.ADD i11 i11 1 || VAU.add.i16 v23 v23 v6 
LSU0.LD128.u8.u16 v13 i12   || IAU.ADD i12 i12 1 || VAU.add.i16 v21 v21 v7  
LSU0.LD128.u8.u16 v14 i13   || IAU.ADD i13 i13 1 || VAU.add.i16 v22 v22 v8 
LSU0.LD128.u8.u16 v15 i14   || IAU.ADD i14 i14 1 || VAU.add.i16 v23 v23 v9 
LSU0.LD128.u8.u16 v1  i0    || IAU.ADD i0  i0  1 || VAU.add.i16 v21 v21 v10  ;seven
LSU0.LD128.u8.u16 v2  i1    || IAU.ADD i1  i1  1 || VAU.add.i16 v22 v22 v11
LSU0.LD128.u8.u16 v3  i2    || IAU.ADD i2  i2  1 || VAU.add.i16 v23 v23 v12
LSU0.LD128.u8.u16 v4  i3    || IAU.ADD i3  i3  1 || VAU.add.i16 v21 v21 v13 
LSU0.LD128.u8.u16 v5  i4    || IAU.ADD i4  i4  1 || VAU.add.i16 v22 v22 v14
LSU0.LD128.u8.u16 v6  i5    || IAU.ADD i5  i5  1 || VAU.add.i16 v23 v23 v15
LSU0.LD128.u8.u16 v7  i6    || IAU.ADD i6  i6  1 || VAU.add.i16 v21 v21 v1  
LSU0.LD128.u8.u16 v8  i7    || IAU.ADD i7  i7  1 || VAU.add.i16 v22 v22 v2 
LSU0.LD128.u8.u16 v9  i8    || IAU.ADD i8  i8  1 || VAU.add.i16 v23 v23 v3 
LSU0.LD128.u8.u16 v10 i9    || IAU.ADD i9  i9  1 || VAU.add.i16 v21 v21 v4  
LSU0.LD128.u8.u16 v11 i10   || IAU.ADD i10 i10 1 || VAU.add.i16 v22 v22 v5 
LSU0.LD128.u8.u16 v12 i11   || IAU.ADD i11 i11 1 || VAU.add.i16 v23 v23 v6 
LSU0.LD128.u8.u16 v13 i12   || IAU.ADD i12 i12 1 || VAU.add.i16 v21 v21 v7  
LSU0.LD128.u8.u16 v14 i13   || IAU.ADD i13 i13 1 || VAU.add.i16 v22 v22 v8 
LSU0.LD128.u8.u16 v15 i14   || IAU.ADD i14 i14 1 || VAU.add.i16 v23 v23 v9 
LSU0.LD128.u8.u16 v1  i0    || IAU.ADD i0  i0  1 || VAU.add.i16 v21 v21 v10  ;eight
LSU0.LD128.u8.u16 v2  i1    || IAU.ADD i1  i1  1 || VAU.add.i16 v22 v22 v11
LSU0.LD128.u8.u16 v3  i2    || IAU.ADD i2  i2  1 || VAU.add.i16 v23 v23 v12
LSU0.LD128.u8.u16 v4  i3    || IAU.ADD i3  i3  1 || VAU.add.i16 v21 v21 v13 
LSU0.LD128.u8.u16 v5  i4    || IAU.ADD i4  i4  1 || VAU.add.i16 v22 v22 v14
LSU0.LD128.u8.u16 v6  i5    || IAU.ADD i5  i5  1 || VAU.add.i16 v23 v23 v15
LSU0.LD128.u8.u16 v7  i6    || IAU.ADD i6  i6  1 || VAU.add.i16 v21 v21 v1  
LSU0.LD128.u8.u16 v8  i7    || IAU.ADD i7  i7  1 || VAU.add.i16 v22 v22 v2 
LSU0.LD128.u8.u16 v9  i8    || IAU.ADD i8  i8  1 || VAU.add.i16 v23 v23 v3 
LSU0.LD128.u8.u16 v10 i9    || IAU.ADD i9  i9  1 || VAU.add.i16 v21 v21 v4  
LSU0.LD128.u8.u16 v11 i10   || IAU.ADD i10 i10 1 || VAU.add.i16 v22 v22 v5 
LSU0.LD128.u8.u16 v12 i11   || IAU.ADD i11 i11 1 || VAU.add.i16 v23 v23 v6 
LSU0.LD128.u8.u16 v13 i12   || IAU.ADD i12 i12 1 || VAU.add.i16 v21 v21 v7  
LSU0.LD128.u8.u16 v14 i13   || IAU.ADD i13 i13 1 || VAU.add.i16 v22 v22 v8 
LSU0.LD128.u8.u16 v15 i14   || IAU.ADD i14 i14 1 || VAU.add.i16 v23 v23 v9 
LSU0.LD128.u8.u16 v1  i0    || IAU.ADD i0  i0  1 || VAU.add.i16 v21 v21 v10  ;nine
LSU0.LD128.u8.u16 v2  i1    || IAU.ADD i1  i1  1 || VAU.add.i16 v22 v22 v11
LSU0.LD128.u8.u16 v3  i2    || IAU.ADD i2  i2  1 || VAU.add.i16 v23 v23 v12
LSU0.LD128.u8.u16 v4  i3    || IAU.ADD i3  i3  1 || VAU.add.i16 v21 v21 v13 
LSU0.LD128.u8.u16 v5  i4    || IAU.ADD i4  i4  1 || VAU.add.i16 v22 v22 v14
LSU0.LD128.u8.u16 v6  i5    || IAU.ADD i5  i5  1 || VAU.add.i16 v23 v23 v15
LSU0.LD128.u8.u16 v7  i6    || IAU.ADD i6  i6  1 || VAU.add.i16 v21 v21 v1  
LSU0.LD128.u8.u16 v8  i7    || IAU.ADD i7  i7  1 || VAU.add.i16 v22 v22 v2 
LSU0.LD128.u8.u16 v9  i8    || IAU.ADD i8  i8  1 || VAU.add.i16 v23 v23 v3 
LSU0.LD128.u8.u16 v10 i9    || IAU.ADD i9  i9  1 || VAU.add.i16 v21 v21 v4  
LSU0.LD128.u8.u16 v11 i10   || IAU.ADD i10 i10 1 || VAU.add.i16 v22 v22 v5 
LSU0.LD128.u8.u16 v12 i11   || IAU.ADD i11 i11 1 || VAU.add.i16 v23 v23 v6 
LSU0.LD128.u8.u16 v13 i12   || IAU.ADD i12 i12 1 || VAU.add.i16 v21 v21 v7  
LSU0.LD128.u8.u16 v14 i13   || IAU.ADD i13 i13 1 || VAU.add.i16 v22 v22 v8 
LSU0.LD128.u8.u16 v15 i14   || IAU.ADD i14 i14 1 || VAU.add.i16 v23 v23 v9 	
LSU0.LD128.u8.u16 v1  i0    || IAU.ADD i0  i0  1 || VAU.add.i16 v21 v21 v10  ;ten
LSU0.LD128.u8.u16 v2  i1    || IAU.ADD i1  i1  1 || VAU.add.i16 v22 v22 v11
LSU0.LD128.u8.u16 v3  i2    || IAU.ADD i2  i2  1 || VAU.add.i16 v23 v23 v12
LSU0.LD128.u8.u16 v4  i3    || IAU.ADD i3  i3  1 || VAU.add.i16 v21 v21 v13 
LSU0.LD128.u8.u16 v5  i4    || IAU.ADD i4  i4  1 || VAU.add.i16 v22 v22 v14
LSU0.LD128.u8.u16 v6  i5    || IAU.ADD i5  i5  1 || VAU.add.i16 v23 v23 v15
LSU0.LD128.u8.u16 v7  i6    || IAU.ADD i6  i6  1 || VAU.add.i16 v21 v21 v1  
LSU0.LD128.u8.u16 v8  i7    || IAU.ADD i7  i7  1 || VAU.add.i16 v22 v22 v2 
LSU0.LD128.u8.u16 v9  i8    || IAU.ADD i8  i8  1 || VAU.add.i16 v23 v23 v3 
LSU0.LD128.u8.u16 v10 i9    || IAU.ADD i9  i9  1 || VAU.add.i16 v21 v21 v4  
LSU0.LD128.u8.u16 v11 i10   || IAU.ADD i10 i10 1 || VAU.add.i16 v22 v22 v5 
LSU0.LD128.u8.u16 v12 i11   || IAU.ADD i11 i11 1 || VAU.add.i16 v23 v23 v6 
LSU0.LD128.u8.u16 v13 i12   || IAU.ADD i12 i12 1 || VAU.add.i16 v21 v21 v7  
LSU0.LD128.u8.u16 v14 i13   || IAU.ADD i13 i13 1 || VAU.add.i16 v22 v22 v8 
LSU0.LD128.u8.u16 v15 i14   || IAU.ADD i14 i14 1 || VAU.add.i16 v23 v23 v9 
LSU0.LD128.u8.u16 v1  i0    || IAU.ADD i0  i0  1 || VAU.add.i16 v21 v21 v10  ;eleven
LSU0.LD128.u8.u16 v2  i1    || IAU.ADD i1  i1  1 || VAU.add.i16 v22 v22 v11
LSU0.LD128.u8.u16 v3  i2    || IAU.ADD i2  i2  1 || VAU.add.i16 v23 v23 v12
LSU0.LD128.u8.u16 v4  i3    || IAU.ADD i3  i3  1 || VAU.add.i16 v21 v21 v13 
LSU0.LD128.u8.u16 v5  i4    || IAU.ADD i4  i4  1 || VAU.add.i16 v22 v22 v14
LSU0.LD128.u8.u16 v6  i5    || IAU.ADD i5  i5  1 || VAU.add.i16 v23 v23 v15
LSU0.LD128.u8.u16 v7  i6    || IAU.ADD i6  i6  1 || VAU.add.i16 v21 v21 v1  
LSU0.LD128.u8.u16 v8  i7    || IAU.ADD i7  i7  1 || VAU.add.i16 v22 v22 v2 
LSU0.LD128.u8.u16 v9  i8    || IAU.ADD i8  i8  1 || VAU.add.i16 v23 v23 v3 
LSU0.LD128.u8.u16 v10 i9    || IAU.ADD i9  i9  1 || VAU.add.i16 v21 v21 v4  
LSU0.LD128.u8.u16 v11 i10   || IAU.ADD i10 i10 1 || VAU.add.i16 v22 v22 v5 
LSU0.LD128.u8.u16 v12 i11   || IAU.ADD i11 i11 1 || VAU.add.i16 v23 v23 v6 
LSU0.LD128.u8.u16 v13 i12   || IAU.ADD i12 i12 1 || VAU.add.i16 v21 v21 v7  
LSU0.LD128.u8.u16 v14 i13   || IAU.ADD i13 i13 1 || VAU.add.i16 v22 v22 v8 
LSU0.LD128.u8.u16 v15 i14   || IAU.ADD i14 i14 1 || VAU.add.i16 v23 v23 v9 
LSU0.LD128.u8.u16 v1  i0    || IAU.ADD i0  i0  1 || VAU.add.i16 v21 v21 v10  ;twelve
LSU0.LD128.u8.u16 v2  i1    || IAU.ADD i1  i1  1 || VAU.add.i16 v22 v22 v11
LSU0.LD128.u8.u16 v3  i2    || IAU.ADD i2  i2  1 || VAU.add.i16 v23 v23 v12
LSU0.LD128.u8.u16 v4  i3    || IAU.ADD i3  i3  1 || VAU.add.i16 v21 v21 v13 
LSU0.LD128.u8.u16 v5  i4    || IAU.ADD i4  i4  1 || VAU.add.i16 v22 v22 v14
LSU0.LD128.u8.u16 v6  i5    || IAU.ADD i5  i5  1 || VAU.add.i16 v23 v23 v15
LSU0.LD128.u8.u16 v7  i6    || IAU.ADD i6  i6  1 || VAU.add.i16 v21 v21 v1  
LSU0.LD128.u8.u16 v8  i7    || IAU.ADD i7  i7  1 || VAU.add.i16 v22 v22 v2 
LSU0.LD128.u8.u16 v9  i8    || IAU.ADD i8  i8  1 || VAU.add.i16 v23 v23 v3 
LSU0.LD128.u8.u16 v10 i9    || IAU.ADD i9  i9  1 || VAU.add.i16 v21 v21 v4  
LSU0.LD128.u8.u16 v11 i10   || IAU.ADD i10 i10 1 || VAU.add.i16 v22 v22 v5 
LSU0.LD128.u8.u16 v12 i11   || IAU.ADD i11 i11 1 || VAU.add.i16 v23 v23 v6 
LSU0.LD128.u8.u16 v13 i12   || IAU.ADD i12 i12 1 || VAU.add.i16 v21 v21 v7  
LSU0.LD128.u8.u16 v14 i13   || IAU.ADD i13 i13 1 || VAU.add.i16 v22 v22 v8 
LSU0.LD128.u8.u16 v15 i14   || IAU.ADD i14 i14 1 || VAU.add.i16 v23 v23 v9 
LSU0.LD128.u8.u16 v1  i0    || IAU.ADD i0  i0  1 || VAU.add.i16 v21 v21 v10  ;thirteen
LSU0.LD128.u8.u16 v2  i1    || IAU.ADD i1  i1  1 || VAU.add.i16 v22 v22 v11
LSU0.LD128.u8.u16 v3  i2    || IAU.ADD i2  i2  1 || VAU.add.i16 v23 v23 v12
LSU0.LD128.u8.u16 v4  i3    || IAU.ADD i3  i3  1 || VAU.add.i16 v21 v21 v13 
LSU0.LD128.u8.u16 v5  i4    || IAU.ADD i4  i4  1 || VAU.add.i16 v22 v22 v14
LSU0.LD128.u8.u16 v6  i5    || IAU.ADD i5  i5  1 || VAU.add.i16 v23 v23 v15
LSU0.LD128.u8.u16 v7  i6    || IAU.ADD i6  i6  1 || VAU.add.i16 v21 v21 v1  
LSU0.LD128.u8.u16 v8  i7    || IAU.ADD i7  i7  1 || VAU.add.i16 v22 v22 v2 
LSU0.LD128.u8.u16 v9  i8    || IAU.ADD i8  i8  1 || VAU.add.i16 v23 v23 v3 
LSU0.LD128.u8.u16 v10 i9    || IAU.ADD i9  i9  1 || VAU.add.i16 v21 v21 v4  
LSU0.LD128.u8.u16 v11 i10   || IAU.ADD i10 i10 1 || VAU.add.i16 v22 v22 v5 
LSU0.LD128.u8.u16 v12 i11   || IAU.ADD i11 i11 1 || VAU.add.i16 v23 v23 v6 
LSU0.LD128.u8.u16 v13 i12   || IAU.ADD i12 i12 1 || VAU.add.i16 v21 v21 v7  
LSU0.LD128.u8.u16 v14 i13   || IAU.ADD i13 i13 1 || VAU.add.i16 v22 v22 v8 
LSU0.LD128.u8.u16 v15 i14   || IAU.ADD i14 i14 1 || VAU.add.i16 v23 v23 v9 
LSU0.LD128.u8.u16 v1  i0    || IAU.ADD i0  i0  1 || VAU.add.i16 v21 v21 v10  ;fourtheen
LSU0.LD128.u8.u16 v2  i1    || IAU.ADD i1  i1  1 || VAU.add.i16 v22 v22 v11
LSU0.LD128.u8.u16 v3  i2    || IAU.ADD i2  i2  1 || VAU.add.i16 v23 v23 v12
LSU0.LD128.u8.u16 v4  i3    || IAU.ADD i3  i3  1 || VAU.add.i16 v21 v21 v13 
LSU0.LD128.u8.u16 v5  i4    || IAU.ADD i4  i4  1 || VAU.add.i16 v22 v22 v14
LSU0.LD128.u8.u16 v6  i5    || IAU.ADD i5  i5  1 || VAU.add.i16 v23 v23 v15
LSU0.LD128.u8.u16 v7  i6    || IAU.ADD i6  i6  1 || VAU.add.i16 v21 v21 v1  
LSU0.LD128.u8.u16 v8  i7    || IAU.ADD i7  i7  1 || VAU.add.i16 v22 v22 v2 
LSU0.LD128.u8.u16 v9  i8    || IAU.ADD i8  i8  1 || VAU.add.i16 v23 v23 v3 
LSU0.LD128.u8.u16 v10 i9    || IAU.ADD i9  i9  1 || VAU.add.i16 v21 v21 v4  
LSU0.LD128.u8.u16 v11 i10   || IAU.ADD i10 i10 1 || VAU.add.i16 v22 v22 v5 
LSU0.LD128.u8.u16 v12 i11   || IAU.ADD i11 i11 1 || VAU.add.i16 v23 v23 v6 
LSU0.LD128.u8.u16 v13 i12   || IAU.ADD i12 i12 1 || VAU.add.i16 v21 v21 v7  
LSU0.LD128.u8.u16 v14 i13   || IAU.ADD i13 i13 1 || VAU.add.i16 v22 v22 v8 
LSU0.LD128.u8.u16 v15 i14   || IAU.ADD i14 i14 1 || VAU.add.i16 v23 v23 v9 
LSU0.LD128.u8.u16 v1  i0    || IAU.SUB i0  i0  6 || VAU.add.i16 v21 v21 v10  ;fiftheen
LSU0.LD128.u8.u16 v2  i1    || IAU.SUB i1  i1  6 || VAU.add.i16 v22 v22 v11
LSU0.LD128.u8.u16 v3  i2    || IAU.SUB i2  i2  6 || VAU.add.i16 v23 v23 v12
LSU0.LD128.u8.u16 v4  i3    || IAU.SUB i3  i3  6 || VAU.add.i16 v21 v21 v13 
LSU0.LD128.u8.u16 v5  i4    || IAU.SUB i4  i4  6 || VAU.add.i16 v22 v22 v14
LSU0.LD128.u8.u16 v6  i5    || IAU.SUB i5  i5  6 || VAU.add.i16 v23 v23 v15
LSU0.LD128.u8.u16 v7  i6    || IAU.SUB i6  i6  6 || VAU.add.i16 v21 v21 v1  
LSU0.LD128.u8.u16 v8  i7    || IAU.SUB i7  i7  6 || VAU.add.i16 v22 v22 v2 
LSU0.LD128.u8.u16 v9  i8    || IAU.SUB i8  i8  6 || VAU.add.i16 v23 v23 v3 
LSU0.LD128.u8.u16 v10 i9    || IAU.SUB i9  i9  6 || VAU.add.i16 v21 v21 v4  
LSU0.LD128.u8.u16 v11 i10   || IAU.SUB i10 i10 6 || VAU.add.i16 v22 v22 v5 
LSU0.LD128.u8.u16 v12 i11   || IAU.SUB i11 i11 6 || VAU.add.i16 v23 v23 v6 
LSU0.LD128.u8.u16 v13 i12   || IAU.SUB i12 i12 6 || VAU.add.i16 v21 v21 v7  
LSU0.LD128.u8.u16 v14 i13   || IAU.SUB i13 i13 6 || VAU.add.i16 v22 v22 v8 
LSU0.LD128.u8.u16 v15 i14   || IAU.SUB i14 i14 6 || VAU.add.i16 v23 v23 v9 
VAU.add.i16 v21 v21 v10
VAU.add.i16 v22 v22 v11
VAU.add.i16 v23 v23 v12
VAU.add.i16 v21 v21 v13
VAU.add.i16 v22 v22 v14
VAU.add.i16 v23 v23 v15
___loop1:
VAU.ADD.i16 v22 v21 v22 || LSU0.LD128.u8.u16 v1  i0    || IAU.ADD i0  i0  1 
LSU0.LD128.u8.u16 v2  i1    || IAU.ADD i1  i1  1
VAU.ADD.i16 v23 v22 v23 

LSU0.LD128.u8.u16 v3  i2    || IAU.ADD i2  i2  1
LSU0.STi64.l v23 i17 || LSU1.LD128.u8.u16 v4  i3    || SAU.ADD.i32 i3  i3  1 || VAU.XOR v18 v18 v18
LSU0.STi64.h v23 i17




bru.jmp i30
nop 5	
	
.end