; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief
; ///

;--------------------------------------------------------------------------------------------------------------
.version 00.50.05
.data .rodata.boxfilter13x13_asm 
.salign 16
	___multiply:
		.float16    0.005917F16


.code .text.boxfilter13x13_asm ;text
.salign 16

boxfilter13x13_asm:
;void boxfilter13x13(u8** in, u8** out, u8 normalize, u32 width);
;                     i18      i17        i16          i15

	LSU0.ldil i5, ___second || LSU1.ldih i5, ___second
	LSU0.ldil i13, 0x1 || LSU1.ldih i13, 0x0
	CMU.CMII.U8 i16 i13
	PEU.PC1C NEQ || BRU.JMP i5
	nop 5



lsu0.ldil i13, ___multiply     ||  lsu1.ldih i13, ___multiply
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
LSU0.LD32  i17 i17
lsu0.ld16r v0, i13
nop 5

lsu0.ldil i13, ___loop     ||  lsu1.ldih i13, ___loop
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
LSU0.LD128.u8.f16 v7  i6    || IAU.ADD i6  i6  1 || VAU.ADD.f16 v18 v18 v1 
LSU0.LD128.u8.f16 v8  i7    || IAU.ADD i7  i7  1 || VAU.ADD.f16 v19 v19 v2 || bru.rpl i13 i15 ;first 
LSU0.LD128.u8.f16 v9  i8    || IAU.ADD i8  i8  1 || VAU.ADD.f16 v20 v20 v3 
LSU0.LD128.u8.f16 v10 i9    || IAU.ADD i9  i9  1 || VAU.ADD.f16 v18 v18 v4 
LSU0.LD128.u8.f16 v11 i10   || IAU.ADD i10 i10 1 || VAU.ADD.f16 v19 v19 v5  
LSU0.LD128.u8.f16 v12 i11   || IAU.ADD i11 i11 1 || VAU.ADD.f16 v20 v20 v6 
LSU0.LD128.u8.f16 v13 i12   || IAU.ADD i12 i12 1 || VAU.ADD.f16 v18 v18 v7 
LSU0.LD128.u8.f16 v1  i0    || IAU.ADD i0  i0  1 || VAU.ADD.f16 v19 v19 v8    ;second
LSU0.LD128.u8.f16 v2  i1    || IAU.ADD i1  i1  1 || VAU.ADD.f16 v20 v20 v9 
LSU0.LD128.u8.f16 v3  i2    || IAU.ADD i2  i2  1 || VAU.ADD.f16 v18 v18 v10
LSU0.LD128.u8.f16 v4  i3    || IAU.ADD i3  i3  1 || VAU.ADD.f16 v19 v19 v11 
LSU0.LD128.u8.f16 v5  i4    || IAU.ADD i4  i4  1 || VAU.ADD.f16 v20 v20 v12
LSU0.LD128.u8.f16 v6  i5    || IAU.ADD i5  i5  1 || VAU.ADD.f16 v18 v18 v13
LSU0.LD128.u8.f16 v7  i6    || IAU.ADD i6  i6  1 || VAU.ADD.f16 v19 v19 v1  
LSU0.LD128.u8.f16 v8  i7    || IAU.ADD i7  i7  1 || VAU.ADD.f16 v20 v20 v2 
LSU0.LD128.u8.f16 v9  i8    || IAU.ADD i8  i8  1 || VAU.ADD.f16 v18 v18 v3 
LSU0.LD128.u8.f16 v10 i9    || IAU.ADD i9  i9  1 || VAU.ADD.f16 v19 v19 v4  
LSU0.LD128.u8.f16 v11 i10   || IAU.ADD i10 i10 1 || VAU.ADD.f16 v20 v20 v5 
LSU0.LD128.u8.f16 v12 i11   || IAU.ADD i11 i11 1 || VAU.ADD.f16 v18 v18 v6 
LSU0.LD128.u8.f16 v13 i12   || IAU.ADD i12 i12 1 || VAU.ADD.f16 v19 v19 v7  
LSU0.LD128.u8.f16 v1  i0    || IAU.ADD i0  i0  1 || VAU.ADD.f16 v20 v20 v8   ;third
LSU0.LD128.u8.f16 v2  i1    || IAU.ADD i1  i1  1 || VAU.ADD.f16 v18 v18 v9 
LSU0.LD128.u8.f16 v3  i2    || IAU.ADD i2  i2  1 || VAU.ADD.f16 v19 v19 v10 
LSU0.LD128.u8.f16 v4  i3    || IAU.ADD i3  i3  1 || VAU.ADD.f16 v20 v20 v11
LSU0.LD128.u8.f16 v5  i4    || IAU.ADD i4  i4  1 || VAU.ADD.f16 v18 v18 v12
LSU0.LD128.u8.f16 v6  i5    || IAU.ADD i5  i5  1 || VAU.ADD.f16 v19 v19 v13 
LSU0.LD128.u8.f16 v7  i6    || IAU.ADD i6  i6  1 || VAU.ADD.f16 v20 v20 v1 
LSU0.LD128.u8.f16 v8  i7    || IAU.ADD i7  i7  1 || VAU.ADD.f16 v21 v18 v2 
LSU0.LD128.u8.f16 v9  i8    || IAU.ADD i8  i8  1 || VAU.ADD.f16 v22 v19 v3  
LSU0.LD128.u8.f16 v10 i9    || IAU.ADD i9  i9  1 || VAU.ADD.f16 v23 v20 v4 
LSU0.LD128.u8.f16 v11 i10   || IAU.ADD i10 i10 1 || VAU.ADD.f16 v21 v21 v5 
LSU0.LD128.u8.f16 v12 i11   || IAU.ADD i11 i11 1 || VAU.ADD.f16 v22 v22 v6  
LSU0.LD128.u8.f16 v13 i12   || IAU.ADD i12 i12 1 || VAU.ADD.f16 v23 v23 v7 
LSU0.LD128.u8.f16 v1  i0    || IAU.ADD i0  i0  1 || VAU.ADD.f16 v21 v21 v8   ;four
LSU0.LD128.u8.f16 v2  i1    || IAU.ADD i1  i1  1 || VAU.ADD.f16 v22 v22 v9 
LSU0.LD128.u8.f16 v3  i2    || IAU.ADD i2  i2  1 || VAU.ADD.f16 v23 v23 v10
LSU0.LD128.u8.f16 v4  i3    || IAU.ADD i3  i3  1 || VAU.ADD.f16 v21 v21 v11
LSU0.LD128.u8.f16 v5  i4    || IAU.ADD i4  i4  1 || VAU.ADD.f16 v22 v22 v12
LSU0.LD128.u8.f16 v6  i5    || IAU.ADD i5  i5  1 || VAU.ADD.f16 v23 v23 v13
LSU0.LD128.u8.f16 v7  i6    || IAU.ADD i6  i6  1 || VAU.ADD.f16 v21 v21 v1 
LSU0.LD128.u8.f16 v8  i7    || IAU.ADD i7  i7  1 || VAU.ADD.f16 v22 v22 v2 
LSU0.LD128.u8.f16 v9  i8    || IAU.ADD i8  i8  1 || VAU.ADD.f16 v23 v23 v3 
LSU0.LD128.u8.f16 v10 i9    || IAU.ADD i9  i9  1 || VAU.ADD.f16 v21 v21 v4 
LSU0.LD128.u8.f16 v11 i10   || IAU.ADD i10 i10 1 || VAU.ADD.f16 v22 v22 v5 
LSU0.LD128.u8.f16 v12 i11   || IAU.ADD i11 i11 1 || VAU.ADD.f16 v23 v23 v6 
LSU0.LD128.u8.f16 v13 i12   || IAU.ADD i12 i12 1 || VAU.ADD.f16 v21 v21 v7 
LSU0.LD128.u8.f16 v1  i0    || IAU.ADD i0  i0  1 || VAU.ADD.f16 v22 v22 v8   ;five
LSU0.LD128.u8.f16 v2  i1    || IAU.ADD i1  i1  1 || VAU.ADD.f16 v23 v23 v9 
LSU0.LD128.u8.f16 v3  i2    || IAU.ADD i2  i2  1 || VAU.ADD.f16 v21 v21 v10
LSU0.LD128.u8.f16 v4  i3    || IAU.ADD i3  i3  1 || VAU.ADD.f16 v22 v22 v11
LSU0.LD128.u8.f16 v5  i4    || IAU.ADD i4  i4  1 || VAU.ADD.f16 v23 v23 v12
LSU0.LD128.u8.f16 v6  i5    || IAU.ADD i5  i5  1 || VAU.ADD.f16 v21 v21 v13
LSU0.LD128.u8.f16 v7  i6    || IAU.ADD i6  i6  1 || VAU.ADD.f16 v22 v22 v1 
LSU0.LD128.u8.f16 v8  i7    || IAU.ADD i7  i7  1 || VAU.ADD.f16 v23 v23 v2 
LSU0.LD128.u8.f16 v9  i8    || IAU.ADD i8  i8  1 || VAU.ADD.f16 v21 v21 v3 
LSU0.LD128.u8.f16 v10 i9    || IAU.ADD i9  i9  1 || VAU.ADD.f16 v22 v22 v4 
LSU0.LD128.u8.f16 v11 i10   || IAU.ADD i10 i10 1 || VAU.ADD.f16 v23 v23 v5 
LSU0.LD128.u8.f16 v12 i11   || IAU.ADD i11 i11 1 || VAU.ADD.f16 v21 v21 v6 
LSU0.LD128.u8.f16 v13 i12   || IAU.ADD i12 i12 1 || VAU.ADD.f16 v22 v22 v7 
LSU0.LD128.u8.f16 v1  i0    || IAU.ADD i0  i0  1 || VAU.ADD.f16 v23 v23 v8   ;six
LSU0.LD128.u8.f16 v2  i1    || IAU.ADD i1  i1  1 || VAU.ADD.f16 v21 v21 v9 
LSU0.LD128.u8.f16 v3  i2    || IAU.ADD i2  i2  1 || VAU.ADD.f16 v22 v22 v10
LSU0.LD128.u8.f16 v4  i3    || IAU.ADD i3  i3  1 || VAU.ADD.f16 v23 v23 v11
LSU0.LD128.u8.f16 v5  i4    || IAU.ADD i4  i4  1 || VAU.ADD.f16 v21 v21 v12
LSU0.LD128.u8.f16 v6  i5    || IAU.ADD i5  i5  1 || VAU.ADD.f16 v22 v22 v13
LSU0.LD128.u8.f16 v7  i6    || IAU.ADD i6  i6  1 || VAU.ADD.f16 v23 v23 v1 
LSU0.LD128.u8.f16 v8  i7    || IAU.ADD i7  i7  1 || VAU.ADD.f16 v21 v21 v2 
LSU0.LD128.u8.f16 v9  i8    || IAU.ADD i8  i8  1 || VAU.ADD.f16 v22 v22 v3 
LSU0.LD128.u8.f16 v10 i9    || IAU.ADD i9  i9  1 || VAU.ADD.f16 v23 v23 v4 
LSU0.LD128.u8.f16 v11 i10   || IAU.ADD i10 i10 1 || VAU.ADD.f16 v21 v21 v5 
LSU0.LD128.u8.f16 v12 i11   || IAU.ADD i11 i11 1 || VAU.ADD.f16 v22 v22 v6 
LSU0.LD128.u8.f16 v13 i12   || IAU.ADD i12 i12 1 || VAU.ADD.f16 v23 v23 v7 
LSU0.LD128.u8.f16 v1  i0    || IAU.ADD i0  i0  1 || VAU.ADD.f16 v21 v21 v8   ;seven
LSU0.LD128.u8.f16 v2  i1    || IAU.ADD i1  i1  1 || VAU.ADD.f16 v22 v22 v9 
LSU0.LD128.u8.f16 v3  i2    || IAU.ADD i2  i2  1 || VAU.ADD.f16 v23 v23 v10
LSU0.LD128.u8.f16 v4  i3    || IAU.ADD i3  i3  1 || VAU.ADD.f16 v21 v21 v11
LSU0.LD128.u8.f16 v5  i4    || IAU.ADD i4  i4  1 || VAU.ADD.f16 v22 v22 v12
LSU0.LD128.u8.f16 v6  i5    || IAU.ADD i5  i5  1 || VAU.ADD.f16 v23 v23 v13
LSU0.LD128.u8.f16 v7  i6    || IAU.ADD i6  i6  1 || VAU.ADD.f16 v21 v21 v1 
LSU0.LD128.u8.f16 v8  i7    || IAU.ADD i7  i7  1 || VAU.ADD.f16 v22 v22 v2 
LSU0.LD128.u8.f16 v9  i8    || IAU.ADD i8  i8  1 || VAU.ADD.f16 v23 v23 v3 
LSU0.LD128.u8.f16 v10 i9    || IAU.ADD i9  i9  1 || VAU.ADD.f16 v21 v21 v4 
LSU0.LD128.u8.f16 v11 i10   || IAU.ADD i10 i10 1 || VAU.ADD.f16 v22 v22 v5 
LSU0.LD128.u8.f16 v12 i11   || IAU.ADD i11 i11 1 || VAU.ADD.f16 v23 v23 v6 
LSU0.LD128.u8.f16 v13 i12   || IAU.ADD i12 i12 1 || VAU.ADD.f16 v21 v21 v7 
LSU0.LD128.u8.f16 v1  i0    || IAU.ADD i0  i0  1 || VAU.ADD.f16 v22 v22 v8   ;eight
LSU0.LD128.u8.f16 v2  i1    || IAU.ADD i1  i1  1 || VAU.ADD.f16 v23 v23 v9 
LSU0.LD128.u8.f16 v3  i2    || IAU.ADD i2  i2  1 || VAU.ADD.f16 v21 v21 v10
LSU0.LD128.u8.f16 v4  i3    || IAU.ADD i3  i3  1 || VAU.ADD.f16 v22 v22 v11
LSU0.LD128.u8.f16 v5  i4    || IAU.ADD i4  i4  1 || VAU.ADD.f16 v23 v23 v12
LSU0.LD128.u8.f16 v6  i5    || IAU.ADD i5  i5  1 || VAU.ADD.f16 v21 v21 v13
LSU0.LD128.u8.f16 v7  i6    || IAU.ADD i6  i6  1 || VAU.ADD.f16 v22 v22 v1 
LSU0.LD128.u8.f16 v8  i7    || IAU.ADD i7  i7  1 || VAU.ADD.f16 v23 v23 v2 
LSU0.LD128.u8.f16 v9  i8    || IAU.ADD i8  i8  1 || VAU.ADD.f16 v21 v21 v3 
LSU0.LD128.u8.f16 v10 i9    || IAU.ADD i9  i9  1 || VAU.ADD.f16 v22 v22 v4 
LSU0.LD128.u8.f16 v11 i10   || IAU.ADD i10 i10 1 || VAU.ADD.f16 v23 v23 v5 
LSU0.LD128.u8.f16 v12 i11   || IAU.ADD i11 i11 1 || VAU.ADD.f16 v21 v21 v6 
LSU0.LD128.u8.f16 v13 i12   || IAU.ADD i12 i12 1 || VAU.ADD.f16 v22 v22 v7 
LSU0.LD128.u8.f16 v1  i0    || IAU.ADD i0  i0  1 || VAU.ADD.f16 v23 v23 v8   ;nine
LSU0.LD128.u8.f16 v2  i1    || IAU.ADD i1  i1  1 || VAU.ADD.f16 v21 v21 v9 
LSU0.LD128.u8.f16 v3  i2    || IAU.ADD i2  i2  1 || VAU.ADD.f16 v22 v22 v10
LSU0.LD128.u8.f16 v4  i3    || IAU.ADD i3  i3  1 || VAU.ADD.f16 v23 v23 v11
LSU0.LD128.u8.f16 v5  i4    || IAU.ADD i4  i4  1 || VAU.ADD.f16 v21 v21 v12
LSU0.LD128.u8.f16 v6  i5    || IAU.ADD i5  i5  1 || VAU.ADD.f16 v22 v22 v13
LSU0.LD128.u8.f16 v7  i6    || IAU.ADD i6  i6  1 || VAU.ADD.f16 v23 v23 v1 
LSU0.LD128.u8.f16 v8  i7    || IAU.ADD i7  i7  1 || VAU.ADD.f16 v21 v21 v2 
LSU0.LD128.u8.f16 v9  i8    || IAU.ADD i8  i8  1 || VAU.ADD.f16 v22 v22 v3 
LSU0.LD128.u8.f16 v10 i9    || IAU.ADD i9  i9  1 || VAU.ADD.f16 v23 v23 v4 
LSU0.LD128.u8.f16 v11 i10   || IAU.ADD i10 i10 1 || VAU.ADD.f16 v21 v21 v5 
LSU0.LD128.u8.f16 v12 i11   || IAU.ADD i11 i11 1 || VAU.ADD.f16 v22 v22 v6 
LSU0.LD128.u8.f16 v13 i12   || IAU.ADD i12 i12 1 || VAU.ADD.f16 v23 v23 v7 
LSU0.LD128.u8.f16 v1  i0    || IAU.ADD i0  i0  1 || VAU.ADD.f16 v21 v21 v8   ;ten
LSU0.LD128.u8.f16 v2  i1    || IAU.ADD i1  i1  1 || VAU.ADD.f16 v22 v22 v9 
LSU0.LD128.u8.f16 v3  i2    || IAU.ADD i2  i2  1 || VAU.ADD.f16 v23 v23 v10
LSU0.LD128.u8.f16 v4  i3    || IAU.ADD i3  i3  1 || VAU.ADD.f16 v21 v21 v11
LSU0.LD128.u8.f16 v5  i4    || IAU.ADD i4  i4  1 || VAU.ADD.f16 v22 v22 v12
LSU0.LD128.u8.f16 v6  i5    || IAU.ADD i5  i5  1 || VAU.ADD.f16 v23 v23 v13
LSU0.LD128.u8.f16 v7  i6    || IAU.ADD i6  i6  1 || VAU.ADD.f16 v21 v21 v1 
LSU0.LD128.u8.f16 v8  i7    || IAU.ADD i7  i7  1 || VAU.ADD.f16 v22 v22 v2 
LSU0.LD128.u8.f16 v9  i8    || IAU.ADD i8  i8  1 || VAU.ADD.f16 v23 v23 v3 
LSU0.LD128.u8.f16 v10 i9    || IAU.ADD i9  i9  1 || VAU.ADD.f16 v21 v21 v4 
LSU0.LD128.u8.f16 v11 i10   || IAU.ADD i10 i10 1 || VAU.ADD.f16 v22 v22 v5 
LSU0.LD128.u8.f16 v12 i11   || IAU.ADD i11 i11 1 || VAU.ADD.f16 v23 v23 v6 
LSU0.LD128.u8.f16 v13 i12   || IAU.ADD i12 i12 1 || VAU.ADD.f16 v21 v21 v7 
LSU0.LD128.u8.f16 v1  i0    || IAU.ADD i0  i0  1 || VAU.ADD.f16 v22 v22 v8   ;eleven
LSU0.LD128.u8.f16 v2  i1    || IAU.ADD i1  i1  1 || VAU.ADD.f16 v23 v23 v9 
LSU0.LD128.u8.f16 v3  i2    || IAU.ADD i2  i2  1 || VAU.ADD.f16 v21 v21 v10
LSU0.LD128.u8.f16 v4  i3    || IAU.ADD i3  i3  1 || VAU.ADD.f16 v22 v22 v11
LSU0.LD128.u8.f16 v5  i4    || IAU.ADD i4  i4  1 || VAU.ADD.f16 v23 v23 v12
LSU0.LD128.u8.f16 v6  i5    || IAU.ADD i5  i5  1 || VAU.ADD.f16 v21 v21 v13
LSU0.LD128.u8.f16 v7  i6    || IAU.ADD i6  i6  1 || VAU.ADD.f16 v22 v22 v1 
LSU0.LD128.u8.f16 v8  i7    || IAU.ADD i7  i7  1 || VAU.ADD.f16 v23 v23 v2 
LSU0.LD128.u8.f16 v9  i8    || IAU.ADD i8  i8  1 || VAU.ADD.f16 v21 v21 v3 
LSU0.LD128.u8.f16 v10 i9    || IAU.ADD i9  i9  1 || VAU.ADD.f16 v22 v22 v4 
LSU0.LD128.u8.f16 v11 i10   || IAU.ADD i10 i10 1 || VAU.ADD.f16 v23 v23 v5 
LSU0.LD128.u8.f16 v12 i11   || IAU.ADD i11 i11 1 || VAU.ADD.f16 v21 v21 v6 
LSU0.LD128.u8.f16 v13 i12   || IAU.ADD i12 i12 1 || VAU.ADD.f16 v22 v22 v7 
LSU0.LD128.u8.f16 v1  i0    || IAU.ADD i0  i0  1 || VAU.ADD.f16 v23 v23 v8   ;twelve
LSU0.LD128.u8.f16 v2  i1    || IAU.ADD i1  i1  1 || VAU.ADD.f16 v21 v21 v9 
LSU0.LD128.u8.f16 v3  i2    || IAU.ADD i2  i2  1 || VAU.ADD.f16 v22 v22 v10
LSU0.LD128.u8.f16 v4  i3    || IAU.ADD i3  i3  1 || VAU.ADD.f16 v23 v23 v11
LSU0.LD128.u8.f16 v5  i4    || IAU.ADD i4  i4  1 || VAU.ADD.f16 v21 v21 v12
LSU0.LD128.u8.f16 v6  i5    || IAU.ADD i5  i5  1 || VAU.ADD.f16 v22 v22 v13
LSU0.LD128.u8.f16 v7  i6    || IAU.ADD i6  i6  1 || VAU.ADD.f16 v23 v23 v1 
LSU0.LD128.u8.f16 v8  i7    || IAU.ADD i7  i7  1 || VAU.ADD.f16 v21 v21 v2 
LSU0.LD128.u8.f16 v9  i8    || IAU.ADD i8  i8  1 || VAU.ADD.f16 v22 v22 v3 
LSU0.LD128.u8.f16 v10 i9    || IAU.ADD i9  i9  1 || VAU.ADD.f16 v23 v23 v4 
LSU0.LD128.u8.f16 v11 i10   || IAU.ADD i10 i10 1 || VAU.ADD.f16 v21 v21 v5 
LSU0.LD128.u8.f16 v12 i11   || IAU.ADD i11 i11 1 || VAU.ADD.f16 v22 v22 v6 
LSU0.LD128.u8.f16 v13 i12   || IAU.ADD i12 i12 1 || VAU.ADD.f16 v23 v23 v7 
LSU0.LD128.u8.f16 v1  i0    || IAU.SUB i0  i0  4 || VAU.ADD.f16 v21 v21 v8   ;thirteen
LSU0.LD128.u8.f16 v2  i1    || IAU.SUB i1  i1  4 || VAU.ADD.f16 v22 v22 v9 
LSU0.LD128.u8.f16 v3  i2    || IAU.SUB i2  i2  4 || VAU.ADD.f16 v23 v23 v10
LSU0.LD128.u8.f16 v4  i3    || IAU.SUB i3  i3  4 || VAU.ADD.f16 v21 v21 v11
LSU0.LD128.u8.f16 v5  i4    || IAU.SUB i4  i4  4 || VAU.ADD.f16 v22 v22 v12
LSU0.LD128.u8.f16 v6  i5    || IAU.SUB i5  i5  4 || VAU.ADD.f16 v23 v23 v13
LSU0.LD128.u8.f16 v7  i6    || IAU.SUB i6  i6  4 || VAU.ADD.f16 v21 v21 v1 
LSU0.LD128.u8.f16 v8  i7    || IAU.SUB i7  i7  4 || VAU.ADD.f16 v22 v22 v2 
LSU0.LD128.u8.f16 v9  i8    || IAU.SUB i8  i8  4 || VAU.ADD.f16 v23 v23 v3 
LSU0.LD128.u8.f16 v10 i9    || IAU.SUB i9  i9  4 || VAU.ADD.f16 v21 v21 v4 
LSU0.LD128.u8.f16 v11 i10   || IAU.SUB i10 i10 4 || VAU.ADD.f16 v22 v22 v5 
LSU0.LD128.u8.f16 v12 i11   || IAU.SUB i11 i11 4 || VAU.ADD.f16 v23 v23 v6 
LSU0.LD128.u8.f16 v13 i12   || IAU.SUB i12 i12 4 || VAU.ADD.f16 v21 v21 v7 
VAU.ADD.f16 v22 v22 v8 
VAU.ADD.f16 v23 v23 v9 
VAU.ADD.f16 v21 v21 v10
VAU.ADD.f16 v22 v22 v11 || LSU0.LD128.u8.f16 v1  i0    || IAU.ADD i0  i0  1 
VAU.ADD.f16 v23 v23 v12 || LSU0.LD128.u8.f16 v2  i1    || IAU.ADD i1  i1  1
VAU.ADD.f16 v21 v21 v13 || LSU0.LD128.u8.f16 v3  i2    || IAU.ADD i2  i2  1
nop 
VAU.add.f16 v23 v22 v23
LSU0.LD128.u8.f16 v4  i3    || IAU.ADD i3  i3  1 || VAU.XOR v18 v18 v18
nop
VAU.ADD.f16 v23 v21 v23
___loop:
LSU0.LD128.u8.f16 v5  i4    || IAU.ADD i4  i4  1 || VAU.XOR v19 v19 v19
nop
VAU.MUL.f16 v23 v23 v0
LSU0.LD128.u8.f16 v6  i5    || IAU.ADD i5  i5  1 || VAU.XOR v20 v20 v20
nop
LSU0.STi128.f16.u8 v23 i17 || LSU1.LD128.u8.f16 v7  i6    || SAU.ADD.i32 i6  i6  1 || VAU.ADD.f16 v18 v18 v1 
 

												 
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
LSU0.LD32  i17 i17
nop 5

lsu0.ldil i13, ___loop1     ||  lsu1.ldih i13, ___loop1
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
LSU0.LD128.u8.u16 v5  i4    || IAU.ADD i4  i4  1 || VAU.XOR v19 v19 v19 || bru.rpl i13 i15 ;first
LSU0.LD128.u8.u16 v6  i5    || IAU.ADD i5  i5  1 || VAU.XOR v20 v20 v20
LSU0.LD128.u8.u16 v7  i6    || IAU.ADD i6  i6  1 || VAU.ADD.i16 v18 v18 v1 
LSU0.LD128.u8.u16 v8  i7    || IAU.ADD i7  i7  1 || VAU.ADD.i16 v19 v19 v2  
LSU0.LD128.u8.u16 v9  i8    || IAU.ADD i8  i8  1 || VAU.ADD.i16 v20 v20 v3 
LSU0.LD128.u8.u16 v10 i9    || IAU.ADD i9  i9  1 || VAU.ADD.i16 v18 v18 v4 
LSU0.LD128.u8.u16 v11 i10   || IAU.ADD i10 i10 1 || VAU.ADD.i16 v19 v19 v5  
LSU0.LD128.u8.u16 v12 i11   || IAU.ADD i11 i11 1 || VAU.ADD.i16 v20 v20 v6 
LSU0.LD128.u8.u16 v13 i12   || IAU.ADD i12 i12 1 || VAU.ADD.i16 v18 v18 v7 
LSU0.LD128.u8.u16 v1  i0    || IAU.ADD i0  i0  1 || VAU.ADD.i16 v19 v19 v8    ;second
LSU0.LD128.u8.u16 v2  i1    || IAU.ADD i1  i1  1 || VAU.ADD.i16 v20 v20 v9 
LSU0.LD128.u8.u16 v3  i2    || IAU.ADD i2  i2  1 || VAU.ADD.i16 v18 v18 v10
LSU0.LD128.u8.u16 v4  i3    || IAU.ADD i3  i3  1 || VAU.ADD.i16 v19 v19 v11 
LSU0.LD128.u8.u16 v5  i4    || IAU.ADD i4  i4  1 || VAU.ADD.i16 v20 v20 v12
LSU0.LD128.u8.u16 v6  i5    || IAU.ADD i5  i5  1 || VAU.ADD.i16 v18 v18 v13
LSU0.LD128.u8.u16 v7  i6    || IAU.ADD i6  i6  1 || VAU.ADD.i16 v19 v19 v1  
LSU0.LD128.u8.u16 v8  i7    || IAU.ADD i7  i7  1 || VAU.ADD.i16 v20 v20 v2 
LSU0.LD128.u8.u16 v9  i8    || IAU.ADD i8  i8  1 || VAU.ADD.i16 v18 v18 v3 
LSU0.LD128.u8.u16 v10 i9    || IAU.ADD i9  i9  1 || VAU.ADD.i16 v19 v19 v4  
LSU0.LD128.u8.u16 v11 i10   || IAU.ADD i10 i10 1 || VAU.ADD.i16 v20 v20 v5 
LSU0.LD128.u8.u16 v12 i11   || IAU.ADD i11 i11 1 || VAU.ADD.i16 v18 v18 v6 
LSU0.LD128.u8.u16 v13 i12   || IAU.ADD i12 i12 1 || VAU.ADD.i16 v19 v19 v7  
LSU0.LD128.u8.u16 v1  i0    || IAU.ADD i0  i0  1 || VAU.ADD.i16 v20 v20 v8   ;third
LSU0.LD128.u8.u16 v2  i1    || IAU.ADD i1  i1  1 || VAU.ADD.i16 v18 v18 v9 
LSU0.LD128.u8.u16 v3  i2    || IAU.ADD i2  i2  1 || VAU.ADD.i16 v19 v19 v10 
LSU0.LD128.u8.u16 v4  i3    || IAU.ADD i3  i3  1 || VAU.ADD.i16 v20 v20 v11
LSU0.LD128.u8.u16 v5  i4    || IAU.ADD i4  i4  1 || VAU.ADD.i16 v18 v18 v12
LSU0.LD128.u8.u16 v6  i5    || IAU.ADD i5  i5  1 || VAU.ADD.i16 v19 v19 v13 
LSU0.LD128.u8.u16 v7  i6    || IAU.ADD i6  i6  1 || VAU.ADD.i16 v20 v20 v1 
LSU0.LD128.u8.u16 v8  i7    || IAU.ADD i7  i7  1 || VAU.ADD.i16 v21 v18 v2 
LSU0.LD128.u8.u16 v9  i8    || IAU.ADD i8  i8  1 || VAU.ADD.i16 v22 v19 v3  
LSU0.LD128.u8.u16 v10 i9    || IAU.ADD i9  i9  1 || VAU.ADD.i16 v23 v20 v4 
LSU0.LD128.u8.u16 v11 i10   || IAU.ADD i10 i10 1 || VAU.ADD.i16 v21 v21 v5 
LSU0.LD128.u8.u16 v12 i11   || IAU.ADD i11 i11 1 || VAU.ADD.i16 v22 v22 v6  
LSU0.LD128.u8.u16 v13 i12   || IAU.ADD i12 i12 1 || VAU.ADD.i16 v23 v23 v7 
LSU0.LD128.u8.u16 v1  i0    || IAU.ADD i0  i0  1 || VAU.ADD.i16 v21 v21 v8   ;four
LSU0.LD128.u8.u16 v2  i1    || IAU.ADD i1  i1  1 || VAU.ADD.i16 v22 v22 v9 
LSU0.LD128.u8.u16 v3  i2    || IAU.ADD i2  i2  1 || VAU.ADD.i16 v23 v23 v10
LSU0.LD128.u8.u16 v4  i3    || IAU.ADD i3  i3  1 || VAU.ADD.i16 v21 v21 v11
LSU0.LD128.u8.u16 v5  i4    || IAU.ADD i4  i4  1 || VAU.ADD.i16 v22 v22 v12
LSU0.LD128.u8.u16 v6  i5    || IAU.ADD i5  i5  1 || VAU.ADD.i16 v23 v23 v13
LSU0.LD128.u8.u16 v7  i6    || IAU.ADD i6  i6  1 || VAU.ADD.i16 v21 v21 v1 
LSU0.LD128.u8.u16 v8  i7    || IAU.ADD i7  i7  1 || VAU.ADD.i16 v22 v22 v2 
LSU0.LD128.u8.u16 v9  i8    || IAU.ADD i8  i8  1 || VAU.ADD.i16 v23 v23 v3 
LSU0.LD128.u8.u16 v10 i9    || IAU.ADD i9  i9  1 || VAU.ADD.i16 v21 v21 v4 
LSU0.LD128.u8.u16 v11 i10   || IAU.ADD i10 i10 1 || VAU.ADD.i16 v22 v22 v5 
LSU0.LD128.u8.u16 v12 i11   || IAU.ADD i11 i11 1 || VAU.ADD.i16 v23 v23 v6 
LSU0.LD128.u8.u16 v13 i12   || IAU.ADD i12 i12 1 || VAU.ADD.i16 v21 v21 v7 
LSU0.LD128.u8.u16 v1  i0    || IAU.ADD i0  i0  1 || VAU.ADD.i16 v22 v22 v8   ;five
LSU0.LD128.u8.u16 v2  i1    || IAU.ADD i1  i1  1 || VAU.ADD.i16 v23 v23 v9 
LSU0.LD128.u8.u16 v3  i2    || IAU.ADD i2  i2  1 || VAU.ADD.i16 v21 v21 v10
LSU0.LD128.u8.u16 v4  i3    || IAU.ADD i3  i3  1 || VAU.ADD.i16 v22 v22 v11
LSU0.LD128.u8.u16 v5  i4    || IAU.ADD i4  i4  1 || VAU.ADD.i16 v23 v23 v12
LSU0.LD128.u8.u16 v6  i5    || IAU.ADD i5  i5  1 || VAU.ADD.i16 v21 v21 v13
LSU0.LD128.u8.u16 v7  i6    || IAU.ADD i6  i6  1 || VAU.ADD.i16 v22 v22 v1 
LSU0.LD128.u8.u16 v8  i7    || IAU.ADD i7  i7  1 || VAU.ADD.i16 v23 v23 v2 
LSU0.LD128.u8.u16 v9  i8    || IAU.ADD i8  i8  1 || VAU.ADD.i16 v21 v21 v3 
LSU0.LD128.u8.u16 v10 i9    || IAU.ADD i9  i9  1 || VAU.ADD.i16 v22 v22 v4 
LSU0.LD128.u8.u16 v11 i10   || IAU.ADD i10 i10 1 || VAU.ADD.i16 v23 v23 v5 
LSU0.LD128.u8.u16 v12 i11   || IAU.ADD i11 i11 1 || VAU.ADD.i16 v21 v21 v6 
LSU0.LD128.u8.u16 v13 i12   || IAU.ADD i12 i12 1 || VAU.ADD.i16 v22 v22 v7 
LSU0.LD128.u8.u16 v1  i0    || IAU.ADD i0  i0  1 || VAU.ADD.i16 v23 v23 v8   ;six
LSU0.LD128.u8.u16 v2  i1    || IAU.ADD i1  i1  1 || VAU.ADD.i16 v21 v21 v9 
LSU0.LD128.u8.u16 v3  i2    || IAU.ADD i2  i2  1 || VAU.ADD.i16 v22 v22 v10
LSU0.LD128.u8.u16 v4  i3    || IAU.ADD i3  i3  1 || VAU.ADD.i16 v23 v23 v11
LSU0.LD128.u8.u16 v5  i4    || IAU.ADD i4  i4  1 || VAU.ADD.i16 v21 v21 v12
LSU0.LD128.u8.u16 v6  i5    || IAU.ADD i5  i5  1 || VAU.ADD.i16 v22 v22 v13
LSU0.LD128.u8.u16 v7  i6    || IAU.ADD i6  i6  1 || VAU.ADD.i16 v23 v23 v1 
LSU0.LD128.u8.u16 v8  i7    || IAU.ADD i7  i7  1 || VAU.ADD.i16 v21 v21 v2 
LSU0.LD128.u8.u16 v9  i8    || IAU.ADD i8  i8  1 || VAU.ADD.i16 v22 v22 v3 
LSU0.LD128.u8.u16 v10 i9    || IAU.ADD i9  i9  1 || VAU.ADD.i16 v23 v23 v4 
LSU0.LD128.u8.u16 v11 i10   || IAU.ADD i10 i10 1 || VAU.ADD.i16 v21 v21 v5 
LSU0.LD128.u8.u16 v12 i11   || IAU.ADD i11 i11 1 || VAU.ADD.i16 v22 v22 v6 
LSU0.LD128.u8.u16 v13 i12   || IAU.ADD i12 i12 1 || VAU.ADD.i16 v23 v23 v7 
LSU0.LD128.u8.u16 v1  i0    || IAU.ADD i0  i0  1 || VAU.ADD.i16 v21 v21 v8   ;seven
LSU0.LD128.u8.u16 v2  i1    || IAU.ADD i1  i1  1 || VAU.ADD.i16 v22 v22 v9 
LSU0.LD128.u8.u16 v3  i2    || IAU.ADD i2  i2  1 || VAU.ADD.i16 v23 v23 v10
LSU0.LD128.u8.u16 v4  i3    || IAU.ADD i3  i3  1 || VAU.ADD.i16 v21 v21 v11
LSU0.LD128.u8.u16 v5  i4    || IAU.ADD i4  i4  1 || VAU.ADD.i16 v22 v22 v12
LSU0.LD128.u8.u16 v6  i5    || IAU.ADD i5  i5  1 || VAU.ADD.i16 v23 v23 v13
LSU0.LD128.u8.u16 v7  i6    || IAU.ADD i6  i6  1 || VAU.ADD.i16 v21 v21 v1 
LSU0.LD128.u8.u16 v8  i7    || IAU.ADD i7  i7  1 || VAU.ADD.i16 v22 v22 v2 
LSU0.LD128.u8.u16 v9  i8    || IAU.ADD i8  i8  1 || VAU.ADD.i16 v23 v23 v3 
LSU0.LD128.u8.u16 v10 i9    || IAU.ADD i9  i9  1 || VAU.ADD.i16 v21 v21 v4 
LSU0.LD128.u8.u16 v11 i10   || IAU.ADD i10 i10 1 || VAU.ADD.i16 v22 v22 v5 
LSU0.LD128.u8.u16 v12 i11   || IAU.ADD i11 i11 1 || VAU.ADD.i16 v23 v23 v6 
LSU0.LD128.u8.u16 v13 i12   || IAU.ADD i12 i12 1 || VAU.ADD.i16 v21 v21 v7 
LSU0.LD128.u8.u16 v1  i0    || IAU.ADD i0  i0  1 || VAU.ADD.i16 v22 v22 v8   ;eight
LSU0.LD128.u8.u16 v2  i1    || IAU.ADD i1  i1  1 || VAU.ADD.i16 v23 v23 v9 
LSU0.LD128.u8.u16 v3  i2    || IAU.ADD i2  i2  1 || VAU.ADD.i16 v21 v21 v10
LSU0.LD128.u8.u16 v4  i3    || IAU.ADD i3  i3  1 || VAU.ADD.i16 v22 v22 v11
LSU0.LD128.u8.u16 v5  i4    || IAU.ADD i4  i4  1 || VAU.ADD.i16 v23 v23 v12
LSU0.LD128.u8.u16 v6  i5    || IAU.ADD i5  i5  1 || VAU.ADD.i16 v21 v21 v13
LSU0.LD128.u8.u16 v7  i6    || IAU.ADD i6  i6  1 || VAU.ADD.i16 v22 v22 v1 
LSU0.LD128.u8.u16 v8  i7    || IAU.ADD i7  i7  1 || VAU.ADD.i16 v23 v23 v2 
LSU0.LD128.u8.u16 v9  i8    || IAU.ADD i8  i8  1 || VAU.ADD.i16 v21 v21 v3 
LSU0.LD128.u8.u16 v10 i9    || IAU.ADD i9  i9  1 || VAU.ADD.i16 v22 v22 v4 
LSU0.LD128.u8.u16 v11 i10   || IAU.ADD i10 i10 1 || VAU.ADD.i16 v23 v23 v5 
LSU0.LD128.u8.u16 v12 i11   || IAU.ADD i11 i11 1 || VAU.ADD.i16 v21 v21 v6 
LSU0.LD128.u8.u16 v13 i12   || IAU.ADD i12 i12 1 || VAU.ADD.i16 v22 v22 v7 
LSU0.LD128.u8.u16 v1  i0    || IAU.ADD i0  i0  1 || VAU.ADD.i16 v23 v23 v8   ;nine
LSU0.LD128.u8.u16 v2  i1    || IAU.ADD i1  i1  1 || VAU.ADD.i16 v21 v21 v9 
LSU0.LD128.u8.u16 v3  i2    || IAU.ADD i2  i2  1 || VAU.ADD.i16 v22 v22 v10
LSU0.LD128.u8.u16 v4  i3    || IAU.ADD i3  i3  1 || VAU.ADD.i16 v23 v23 v11
LSU0.LD128.u8.u16 v5  i4    || IAU.ADD i4  i4  1 || VAU.ADD.i16 v21 v21 v12
LSU0.LD128.u8.u16 v6  i5    || IAU.ADD i5  i5  1 || VAU.ADD.i16 v22 v22 v13
LSU0.LD128.u8.u16 v7  i6    || IAU.ADD i6  i6  1 || VAU.ADD.i16 v23 v23 v1 
LSU0.LD128.u8.u16 v8  i7    || IAU.ADD i7  i7  1 || VAU.ADD.i16 v21 v21 v2 
LSU0.LD128.u8.u16 v9  i8    || IAU.ADD i8  i8  1 || VAU.ADD.i16 v22 v22 v3 
LSU0.LD128.u8.u16 v10 i9    || IAU.ADD i9  i9  1 || VAU.ADD.i16 v23 v23 v4 
LSU0.LD128.u8.u16 v11 i10   || IAU.ADD i10 i10 1 || VAU.ADD.i16 v21 v21 v5 
LSU0.LD128.u8.u16 v12 i11   || IAU.ADD i11 i11 1 || VAU.ADD.i16 v22 v22 v6 
LSU0.LD128.u8.u16 v13 i12   || IAU.ADD i12 i12 1 || VAU.ADD.i16 v23 v23 v7 
LSU0.LD128.u8.u16 v1  i0    || IAU.ADD i0  i0  1 || VAU.ADD.i16 v21 v21 v8   ;ten
LSU0.LD128.u8.u16 v2  i1    || IAU.ADD i1  i1  1 || VAU.ADD.i16 v22 v22 v9 
LSU0.LD128.u8.u16 v3  i2    || IAU.ADD i2  i2  1 || VAU.ADD.i16 v23 v23 v10
LSU0.LD128.u8.u16 v4  i3    || IAU.ADD i3  i3  1 || VAU.ADD.i16 v21 v21 v11
LSU0.LD128.u8.u16 v5  i4    || IAU.ADD i4  i4  1 || VAU.ADD.i16 v22 v22 v12
LSU0.LD128.u8.u16 v6  i5    || IAU.ADD i5  i5  1 || VAU.ADD.i16 v23 v23 v13
LSU0.LD128.u8.u16 v7  i6    || IAU.ADD i6  i6  1 || VAU.ADD.i16 v21 v21 v1 
LSU0.LD128.u8.u16 v8  i7    || IAU.ADD i7  i7  1 || VAU.ADD.i16 v22 v22 v2 
LSU0.LD128.u8.u16 v9  i8    || IAU.ADD i8  i8  1 || VAU.ADD.i16 v23 v23 v3 
LSU0.LD128.u8.u16 v10 i9    || IAU.ADD i9  i9  1 || VAU.ADD.i16 v21 v21 v4 
LSU0.LD128.u8.u16 v11 i10   || IAU.ADD i10 i10 1 || VAU.ADD.i16 v22 v22 v5 
LSU0.LD128.u8.u16 v12 i11   || IAU.ADD i11 i11 1 || VAU.ADD.i16 v23 v23 v6 
LSU0.LD128.u8.u16 v13 i12   || IAU.ADD i12 i12 1 || VAU.ADD.i16 v21 v21 v7 
LSU0.LD128.u8.u16 v1  i0    || IAU.ADD i0  i0  1 || VAU.ADD.i16 v22 v22 v8   ;eleven
LSU0.LD128.u8.u16 v2  i1    || IAU.ADD i1  i1  1 || VAU.ADD.i16 v23 v23 v9 
LSU0.LD128.u8.u16 v3  i2    || IAU.ADD i2  i2  1 || VAU.ADD.i16 v21 v21 v10
LSU0.LD128.u8.u16 v4  i3    || IAU.ADD i3  i3  1 || VAU.ADD.i16 v22 v22 v11
LSU0.LD128.u8.u16 v5  i4    || IAU.ADD i4  i4  1 || VAU.ADD.i16 v23 v23 v12
LSU0.LD128.u8.u16 v6  i5    || IAU.ADD i5  i5  1 || VAU.ADD.i16 v21 v21 v13
LSU0.LD128.u8.u16 v7  i6    || IAU.ADD i6  i6  1 || VAU.ADD.i16 v22 v22 v1 
LSU0.LD128.u8.u16 v8  i7    || IAU.ADD i7  i7  1 || VAU.ADD.i16 v23 v23 v2 
LSU0.LD128.u8.u16 v9  i8    || IAU.ADD i8  i8  1 || VAU.ADD.i16 v21 v21 v3 
LSU0.LD128.u8.u16 v10 i9    || IAU.ADD i9  i9  1 || VAU.ADD.i16 v22 v22 v4 
LSU0.LD128.u8.u16 v11 i10   || IAU.ADD i10 i10 1 || VAU.ADD.i16 v23 v23 v5 
LSU0.LD128.u8.u16 v12 i11   || IAU.ADD i11 i11 1 || VAU.ADD.i16 v21 v21 v6 
LSU0.LD128.u8.u16 v13 i12   || IAU.ADD i12 i12 1 || VAU.ADD.i16 v22 v22 v7 
LSU0.LD128.u8.u16 v1  i0    || IAU.ADD i0  i0  1 || VAU.ADD.i16 v23 v23 v8   ;twelve
LSU0.LD128.u8.u16 v2  i1    || IAU.ADD i1  i1  1 || VAU.ADD.i16 v21 v21 v9 
LSU0.LD128.u8.u16 v3  i2    || IAU.ADD i2  i2  1 || VAU.ADD.i16 v22 v22 v10
LSU0.LD128.u8.u16 v4  i3    || IAU.ADD i3  i3  1 || VAU.ADD.i16 v23 v23 v11
LSU0.LD128.u8.u16 v5  i4    || IAU.ADD i4  i4  1 || VAU.ADD.i16 v21 v21 v12
LSU0.LD128.u8.u16 v6  i5    || IAU.ADD i5  i5  1 || VAU.ADD.i16 v22 v22 v13
LSU0.LD128.u8.u16 v7  i6    || IAU.ADD i6  i6  1 || VAU.ADD.i16 v23 v23 v1 
LSU0.LD128.u8.u16 v8  i7    || IAU.ADD i7  i7  1 || VAU.ADD.i16 v21 v21 v2 
LSU0.LD128.u8.u16 v9  i8    || IAU.ADD i8  i8  1 || VAU.ADD.i16 v22 v22 v3 
LSU0.LD128.u8.u16 v10 i9    || IAU.ADD i9  i9  1 || VAU.ADD.i16 v23 v23 v4 
LSU0.LD128.u8.u16 v11 i10   || IAU.ADD i10 i10 1 || VAU.ADD.i16 v21 v21 v5 
LSU0.LD128.u8.u16 v12 i11   || IAU.ADD i11 i11 1 || VAU.ADD.i16 v22 v22 v6 
LSU0.LD128.u8.u16 v13 i12   || IAU.ADD i12 i12 1 || VAU.ADD.i16 v23 v23 v7 
LSU0.LD128.u8.u16 v1  i0    || IAU.SUB i0  i0  4 || VAU.ADD.i16 v21 v21 v8   ;thirteen
LSU0.LD128.u8.u16 v2  i1    || IAU.SUB i1  i1  4 || VAU.ADD.i16 v22 v22 v9 
LSU0.LD128.u8.u16 v3  i2    || IAU.SUB i2  i2  4 || VAU.ADD.i16 v23 v23 v10
LSU0.LD128.u8.u16 v4  i3    || IAU.SUB i3  i3  4 || VAU.ADD.i16 v21 v21 v11
LSU0.LD128.u8.u16 v5  i4    || IAU.SUB i4  i4  4 || VAU.ADD.i16 v22 v22 v12
LSU0.LD128.u8.u16 v6  i5    || IAU.SUB i5  i5  4 || VAU.ADD.i16 v23 v23 v13
LSU0.LD128.u8.u16 v7  i6    || IAU.SUB i6  i6  4 || VAU.ADD.i16 v21 v21 v1 
LSU0.LD128.u8.u16 v8  i7    || IAU.SUB i7  i7  4 || VAU.ADD.i16 v22 v22 v2 
LSU0.LD128.u8.u16 v9  i8    || IAU.SUB i8  i8  4 || VAU.ADD.i16 v23 v23 v3 
LSU0.LD128.u8.u16 v10 i9    || IAU.SUB i9  i9  4 || VAU.ADD.i16 v21 v21 v4 
LSU0.LD128.u8.u16 v11 i10   || IAU.SUB i10 i10 4 || VAU.ADD.i16 v22 v22 v5 
LSU0.LD128.u8.u16 v12 i11   || IAU.SUB i11 i11 4 || VAU.ADD.i16 v23 v23 v6 
LSU0.LD128.u8.u16 v13 i12   || IAU.SUB i12 i12 4 || VAU.ADD.i16 v21 v21 v7 
VAU.ADD.i16 v22 v22 v8 
VAU.ADD.i16 v23 v23 v9 
VAU.ADD.i16 v21 v21 v10
VAU.ADD.i16 v22 v22 v11
VAU.ADD.i16 v23 v23 v12
VAU.ADD.i16 v21 v21 v13
___loop1:
VAU.add.i16 v23 v22 v23 || LSU0.LD128.u8.u16 v1  i0    || IAU.ADD i0  i0  1 
LSU0.LD128.u8.u16 v2  i1    || IAU.ADD i1  i1  1
VAU.ADD.i16 v23 v21 v23
LSU0.LD128.u8.u16 v3  i2    || IAU.ADD i2  i2  1
LSU0.STi64.l v23 i17 || LSU1.LD128.u8.u16 v4  i3    || SAU.ADD.i32 i3  i3  1 || VAU.XOR v18 v18 v18
LSU0.STi64.h v23 i17
 

												 
bru.jmp i30
nop 5
	
	
.end