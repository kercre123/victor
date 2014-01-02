; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief
; ///

.version 00.51.04

.data .data.histogram

.code .text.histogram

histogram_asm:
;void histogram(u8** in, u32 *hist, u32 width)
;                 i18      i17        i16

IAU.SUB i1   i1    i1
IAU.SUB i2   i2    i2
IAU.SUB i3   i3    i3
IAU.SUB i4   i4    i4
IAU.SUB i5   i5    i5
IAU.SUB i6   i6    i6
IAU.SUB i7   i7    i7
IAU.SUB i8   i8    i8
IAU.SUB i9   i9    i9
IAU.SUB i10  i10   i10
IAU.SUB i11  i11   i11
IAU.SUB i12  i12   i12
IAU.SUB i13  i13   i13
IAU.SUB i14  i14   i14
IAU.SUB i15  i15   i15

LSU0.ldil i3, 0x40  || LSU1.ldih i3, 0x0
;LSU0.ldil i4, ___second  || LSU1.ldih i4, ___second
LSU0.ldil i5, ___loop  || LSU1.ldih i5, ___loop
LSU1.LD32  i18  i18  ;load adress for line
nop 5
IAU.SHR.u32 i1  i16  6 ;calculate how many pixels will remain 
IAU.SHL  i1  i1  6
IAU.SUB i2 i16 i1   ;width mod 64
IAU.SUB i15 i15 i15 ;contor for loop

IAU.SUB i19 i19 4
LSU0.ST32  i20  i19 || IAU.SUB i19 i19 4
LSU0.ST32  i21  i19 || IAU.SUB i19 i19 4
LSU0.ST32  i22  i19 || IAU.SUB i19 i19 4
LSU0.ST32  i23  i19 || IAU.SUB i19 i19 4
LSU0.ST32  i24  i19 || IAU.SUB i19 i19 4
LSU0.ST32  i25  i19 || IAU.SUB i19 i19 4
LSU0.ST32  i26  i19 || IAU.SUB i19 i19 4
LSU0.ST32  i27  i19 || IAU.SUB i19 i19 4
LSU0.ST32  i28  i19 || IAU.SUB i19 i19 4
LSU0.ST32  i29  i19 


CMU.CMII.i32 i16 i3
PEU.PC1C LT ||  BRU.BRA _____compensation || IAU.SUB  i2 i16 0
nop 5


	LSU0.LDi128.u8.u16 v1 i18 
	LSU0.LDi128.u8.u16 v2 i18 
	LSU0.LDi128.u8.u16 v3 i18 
	LSU0.LDi128.u8.u16 v4 i18 
	LSU0.LDi128.u8.u16 v5 i18 
	LSU0.LDi128.u8.u16 v6 i18 
	LSU0.LDi128.u8.u16 v7 i18 || CMU.CPVI.x16      i6.l   v1.0
	LSU0.LDi128.u8.u16 v8 i18 || CMU.CPVI.x16      i7.l   v1.1 || IAU.SHL i6 i6 2
	
	CMU.CPVI.x16      i8.l   v1.2 || IAU.SHL i7 i7 2 || SAU.ADD.i32 i20 i17 i6
	CMU.CPVI.x16      i9.l   v1.3 || IAU.SHL i8 i8 2 || SAU.ADD.i32 i21 i17 i7
	CMU.CPVI.x16      i10.l   v1.4 || IAU.SHL i9 i9 2 || SAU.ADD.i32 i22 i17 i8   
	CMU.CPVI.x16      i12.l   v1.5 || IAU.SHL i10 i10 2 || SAU.ADD.i32 i23 i17 i9  
	CMU.CPVI.x16      i13.l   v1.6 || IAU.SHL i12 i12 2 || SAU.ADD.i32 i24 i17 i10 
	CMU.CPVI.x16      i14.l   v1.7 || IAU.SHL i13 i13 2 || SAU.ADD.i32 i25 i17 i12 
	
___loop:
	LSU0.LD32 i6 i20
	IAU.SHL i14 i14 2 || SAU.ADD.i32 i26 i17 i13 
	SAU.ADD.i32 i27 i17 i14 
	nop 3

	IAU.ADD i6 i6 1
	LSU0.ST32 i6 i20
	CMU.CPVI.x16      i6.l   v2.0 
	
	LSU0.LD32 i7 i21
	nop 5
	IAU.ADD i7 i7 1
	LSU0.ST32 i7 i21
	CMU.CPVI.x16      i7.l   v2.1 || IAU.SHL i6 i6 2
	
	LSU0.LD32 i8 i22
	nop 5
	IAU.ADD i8 i8 1
	LSU0.ST32 i8 i22
	CMU.CPVI.x16      i8.l   v2.2 || IAU.SHL i7 i7 2 || SAU.ADD.i32 i20 i17 i6

	
	LSU0.LD32 i9 i23
	nop 5
	IAU.ADD i9 i9 1
	LSU0.ST32 i9 i23
	CMU.CPVI.x16      i9.l   v2.3 || IAU.SHL i8 i8 2 || SAU.ADD.i32 i21 i17 i7
	
	
	LSU0.LD32 i10 i24
	nop 5
	IAU.ADD i10 i10 1
	LSU0.ST32 i10 i24
	CMU.CPVI.x16      i10.l   v2.4 || IAU.SHL i9 i9 2 || SAU.ADD.i32 i22 i17 i8  
	
	LSU0.LD32 i12 i25
	nop 5
	IAU.ADD i12 i12 1
	LSU0.ST32 i12 i25
	CMU.CPVI.x16      i12.l   v2.5 || IAU.SHL i10 i10 2 || SAU.ADD.i32 i23 i17 i9 
	
	
	LSU0.LD32 i13 i26
	nop 5
	IAU.ADD i13 i13 1
	LSU0.ST32 i13 i26
	CMU.CPVI.x16      i13.l   v2.6 || IAU.SHL i12 i12 2 || SAU.ADD.i32 i24 i17 i10 
	
	
	LSU0.LD32 i14 i27
	nop 5
	IAU.ADD i14 i14 1
	LSU0.ST32 i14 i27
	CMU.CPVI.x16      i14.l   v2.7 || IAU.SHL i13 i13 2 || SAU.ADD.i32 i25 i17 i12 
	

	LSU0.LD32 i6 i20
	IAU.SHL i14 i14 2 || SAU.ADD.i32 i26 i17 i13 
	SAU.ADD.i32 i27 i17 i14 
	nop 3

	IAU.ADD i6 i6 1
	LSU0.ST32 i6 i20
	CMU.CPVI.x16      i6.l   v3.0 
	
	LSU0.LD32 i7 i21
	nop 5
	IAU.ADD i7 i7 1
	LSU0.ST32 i7 i21
	CMU.CPVI.x16      i7.l   v3.1 || IAU.SHL i6 i6 2
	
	LSU0.LD32 i8 i22
	nop 5
	IAU.ADD i8 i8 1
	LSU0.ST32 i8 i22
	CMU.CPVI.x16      i8.l   v3.2 || IAU.SHL i7 i7 2 || SAU.ADD.i32 i20 i17 i6
	
	LSU0.LD32 i9 i23
	nop 5
	IAU.ADD i9 i9 1
	LSU0.ST32 i9 i23
	CMU.CPVI.x16      i9.l   v3.3 || IAU.SHL i8 i8 2 || SAU.ADD.i32 i21 i17 i7
	
	LSU0.LD32 i10 i24
	nop 5
	IAU.ADD i10 i10 1
	LSU0.ST32 i10 i24
	CMU.CPVI.x16      i10.l   v3.4 || IAU.SHL i9 i9 2 || SAU.ADD.i32 i22 i17 i8  
	
	LSU0.LD32 i12 i25
	nop 5
	IAU.ADD i12 i12 1
	LSU0.ST32 i12 i25
	CMU.CPVI.x16      i12.l   v3.5 || IAU.SHL i10 i10 2 || SAU.ADD.i32 i23 i17 i9  
	
	LSU0.LD32 i13 i26
	nop 5
	IAU.ADD i13 i13 1
	LSU0.ST32 i13 i26
	CMU.CPVI.x16      i13.l   v3.6 || IAU.SHL i12 i12 2 || SAU.ADD.i32 i24 i17 i10 
	
	LSU0.LD32 i14 i27
	nop 5
	IAU.ADD i14 i14 1
	LSU0.ST32 i14 i27
	CMU.CPVI.x16      i14.l   v3.7 || IAU.SHL i13 i13 2 || SAU.ADD.i32 i25 i17 i12 
	LSU0.LD32 i6 i20
	IAU.SHL i14 i14 2 || SAU.ADD.i32 i26 i17 i13 
	SAU.ADD.i32 i27 i17 i14 
	nop 3
	IAU.ADD i6 i6 1
	LSU0.ST32 i6 i20
	CMU.CPVI.x16      i6.l   v4.0 
	
	LSU0.LD32 i7 i21
	nop 5
	IAU.ADD i7 i7 1
	LSU0.ST32 i7 i21
	CMU.CPVI.x16      i7.l   v4.1 || IAU.SHL i6 i6 2
	
	LSU0.LD32 i8 i22
	nop 5
	IAU.ADD i8 i8 1
	LSU0.ST32 i8 i22
	CMU.CPVI.x16      i8.l   v4.2 || IAU.SHL i7 i7 2 || SAU.ADD.i32 i20 i17 i6
	
	LSU0.LD32 i9 i23
	nop 5
	IAU.ADD i9 i9 1
	LSU0.ST32 i9 i23
	CMU.CPVI.x16      i9.l   v4.3 || IAU.SHL i8 i8 2 || SAU.ADD.i32 i21 i17 i7
	
	
	LSU0.LD32 i10 i24
	nop 5
	IAU.ADD i10 i10 1
	LSU0.ST32 i10 i24
	CMU.CPVI.x16      i10.l   v4.4 || IAU.SHL i9 i9 2 || SAU.ADD.i32 i22 i17 i8  
	
	LSU0.LD32 i12 i25
	nop 5
	IAU.ADD i12 i12 1
	LSU0.ST32 i12 i25
	CMU.CPVI.x16      i12.l   v4.5 || IAU.SHL i10 i10 2 || SAU.ADD.i32 i23 i17 i9 
	
	
	LSU0.LD32 i13 i26
	nop 5
	IAU.ADD i13 i13 1
	LSU0.ST32 i13 i26
	CMU.CPVI.x16      i13.l   v4.6 || IAU.SHL i12 i12 2 || SAU.ADD.i32 i24 i17 i10 
	
	
	LSU0.LD32 i14 i27
	nop 5
	IAU.ADD i14 i14 1
	LSU0.ST32 i14 i27
	CMU.CPVI.x16      i14.l   v4.7 || IAU.SHL i13 i13 2 || SAU.ADD.i32 i25 i17 i12 
		
	LSU0.LD32 i6 i20
	IAU.SHL i14 i14 2 || SAU.ADD.i32 i26 i17 i13 
	SAU.ADD.i32 i27 i17 i14 
	nop 3
	IAU.ADD i6 i6 1
	LSU0.ST32 i6 i20
	CMU.CPVI.x16      i6.l   v5.0 
	
	LSU0.LD32 i7 i21
	nop 5
	IAU.ADD i7 i7 1
	LSU0.ST32 i7 i21
	CMU.CPVI.x16      i7.l   v5.1 || IAU.SHL i6 i6 2
	
	LSU0.LD32 i8 i22
	nop 5
	IAU.ADD i8 i8 1
	LSU0.ST32 i8 i22
	CMU.CPVI.x16      i8.l   v5.2 || IAU.SHL i7 i7 2 || SAU.ADD.i32 i20 i17 i6
	
	LSU0.LD32 i9 i23
	nop 5
	IAU.ADD i9 i9 1
	LSU0.ST32 i9 i23
	CMU.CPVI.x16      i9.l   v5.3 || IAU.SHL i8 i8 2 || SAU.ADD.i32 i21 i17 i7
	
	
	LSU0.LD32 i10 i24
	nop 5
	IAU.ADD i10 i10 1
	LSU0.ST32 i10 i24
	CMU.CPVI.x16      i10.l   v5.4 || IAU.SHL i9 i9 2 || SAU.ADD.i32 i22 i17 i8  
	
	LSU0.LD32 i12 i25
	nop 5
	IAU.ADD i12 i12 1
	LSU0.ST32 i12 i25
	CMU.CPVI.x16      i12.l   v5.5 || IAU.SHL i10 i10 2 || SAU.ADD.i32 i23 i17 i9  
	
	LSU0.LD32 i13 i26
	nop 5
	IAU.ADD i13 i13 1
	LSU0.ST32 i13 i26
	CMU.CPVI.x16      i13.l   v5.6 || IAU.SHL i12 i12 2 || SAU.ADD.i32 i24 i17 i10 
	
	LSU0.LD32 i14 i27
	nop 5
	IAU.ADD i14 i14 1
	LSU0.ST32 i14 i27
	CMU.CPVI.x16      i14.l   v5.7 || IAU.SHL i13 i13 2 || SAU.ADD.i32 i25 i17 i12 
	
	LSU0.LD32 i6 i20
	IAU.SHL i14 i14 2 || SAU.ADD.i32 i26 i17 i13 
	SAU.ADD.i32 i27 i17 i14 
	nop 3
	
	IAU.ADD i6 i6 1
	LSU0.ST32 i6 i20
	CMU.CPVI.x16      i6.l   v6.0 
	
	LSU0.LD32 i7 i21
	nop 5
	IAU.ADD i7 i7 1
	LSU0.ST32 i7 i21
	CMU.CPVI.x16      i7.l   v6.1 || IAU.SHL i6 i6 2
	
	LSU0.LD32 i8 i22
	nop 5
	IAU.ADD i8 i8 1
	LSU0.ST32 i8 i22
	CMU.CPVI.x16      i8.l   v6.2 || IAU.SHL i7 i7 2 || SAU.ADD.i32 i20 i17 i6
	
	LSU0.LD32 i9 i23
	nop 5
	IAU.ADD i9 i9 1
	LSU0.ST32 i9 i23
	CMU.CPVI.x16      i9.l   v6.3 || IAU.SHL i8 i8 2 || SAU.ADD.i32 i21 i17 i7
	
	
	LSU0.LD32 i10 i24
	nop 5
	IAU.ADD i10 i10 1
	LSU0.ST32 i10 i24
	CMU.CPVI.x16      i10.l   v6.4 || IAU.SHL i9 i9 2 || SAU.ADD.i32 i22 i17 i8  
	
	LSU0.LD32 i12 i25
	nop 5
	IAU.ADD i12 i12 1
	LSU0.ST32 i12 i25
	CMU.CPVI.x16      i12.l   v6.5 || IAU.SHL i10 i10 2 || SAU.ADD.i32 i23 i17 i9  
	
	LSU0.LD32 i13 i26
	nop 5
	IAU.ADD i13 i13 1
	LSU0.ST32 i13 i26
	CMU.CPVI.x16      i13.l   v6.6 || IAU.SHL i12 i12 2 || SAU.ADD.i32 i24 i17 i10 
	
	
	LSU0.LD32 i14 i27
	nop 5
	IAU.ADD i14 i14 1
	LSU0.ST32 i14 i27
	CMU.CPVI.x16      i14.l   v6.7 || IAU.SHL i13 i13 2 || SAU.ADD.i32 i25 i17 i12 
	
	LSU0.LD32 i6 i20
	IAU.SHL i14 i14 2 || SAU.ADD.i32 i26 i17 i13 
	SAU.ADD.i32 i27 i17 i14 
	nop 3

	IAU.ADD i6 i6 1
	LSU0.ST32 i6 i20
	CMU.CPVI.x16      i6.l   v7.0 
	
	LSU0.LD32 i7 i21
	nop 5
	IAU.ADD i7 i7 1
	LSU0.ST32 i7 i21
	CMU.CPVI.x16      i7.l   v7.1 || IAU.SHL i6 i6 2
	
	LSU0.LD32 i8 i22
	nop 5
	IAU.ADD i8 i8 1
	LSU0.ST32 i8 i22
	CMU.CPVI.x16      i8.l   v7.2 || IAU.SHL i7 i7 2 || SAU.ADD.i32 i20 i17 i6
	
	LSU0.LD32 i9 i23
	nop 5
	IAU.ADD i9 i9 1
	LSU0.ST32 i9 i23
	CMU.CPVI.x16      i9.l   v7.3 || IAU.SHL i8 i8 2 || SAU.ADD.i32 i21 i17 i7
	
	LSU0.LD32 i10 i24
	nop 5
	IAU.ADD i10 i10 1
	LSU0.ST32 i10 i24
	CMU.CPVI.x16      i10.l   v7.4 || IAU.SHL i9 i9 2 || SAU.ADD.i32 i22 i17 i8  
	
	LSU0.LD32 i12 i25
	nop 5
	IAU.ADD i12 i12 1
	LSU0.ST32 i12 i25
	CMU.CPVI.x16      i12.l   v7.5 || IAU.SHL i10 i10 2 || SAU.ADD.i32 i23 i17 i9  
	
	LSU0.LD32 i13 i26
	nop 5
	IAU.ADD i13 i13 1
	LSU0.ST32 i13 i26
	CMU.CPVI.x16      i13.l   v7.6 || IAU.SHL i12 i12 2 || SAU.ADD.i32 i24 i17 i10 
	
	LSU0.LD32 i14 i27
	nop 5
	IAU.ADD i14 i14 1
	LSU0.ST32 i14 i27
	CMU.CPVI.x16      i14.l   v7.7 || IAU.SHL i13 i13 2 || SAU.ADD.i32 i25 i17 i12 
	
	LSU0.LD32 i6 i20
	IAU.SHL i14 i14 2 || SAU.ADD.i32 i26 i17 i13 
	SAU.ADD.i32 i27 i17 i14 
	nop 3

	IAU.ADD i6 i6 1
	LSU0.ST32 i6 i20
	CMU.CPVI.x16      i6.l   v8.0 
	
	LSU0.LD32 i7 i21
	nop 5
	IAU.ADD i7 i7 1
	LSU0.ST32 i7 i21
	CMU.CPVI.x16      i7.l   v8.1 || IAU.SHL i6 i6 2
	
	LSU0.LD32 i8 i22
	nop 5
	IAU.ADD i8 i8 1
	LSU0.ST32 i8 i22
	CMU.CPVI.x16      i8.l   v8.2 || IAU.SHL i7 i7 2 || SAU.ADD.i32 i20 i17 i6
	
	LSU0.LD32 i9 i23
	nop 5
	IAU.ADD i9 i9 1
	LSU0.ST32 i9 i23
	CMU.CPVI.x16      i9.l   v8.3 || IAU.SHL i8 i8 2 || SAU.ADD.i32 i21 i17 i7
	
	LSU0.LD32 i10 i24
	nop 5
	IAU.ADD i10 i10 1
	LSU0.ST32 i10 i24
	CMU.CPVI.x16      i10.l   v8.4 || IAU.SHL i9 i9 2 || SAU.ADD.i32 i22 i17 i8  
	
	LSU0.LD32 i12 i25
	nop 5
	IAU.ADD i12 i12 1
	LSU0.ST32 i12 i25
	CMU.CPVI.x16      i12.l   v8.5 || IAU.SHL i10 i10 2 || SAU.ADD.i32 i23 i17 i9  
	
	LSU0.LD32 i13 i26
	nop 5
	IAU.ADD i13 i13 1
	LSU0.ST32 i13 i26
	CMU.CPVI.x16      i13.l   v8.6 || IAU.SHL i12 i12 2 || SAU.ADD.i32 i24 i17 i10 
	
	LSU0.LD32 i14 i27
	LSU0.LDi128.u8.u16 v1 i18 
	LSU0.LDi128.u8.u16 v2 i18 
	LSU0.LDi128.u8.u16 v3 i18 
	LSU0.LDi128.u8.u16 v4 i18 
	LSU0.LDi128.u8.u16 v5 i18 
	IAU.ADD i14 i14 1
	LSU0.ST32 i14 i27
	CMU.CPVI.x16      i14.l   v8.7 || IAU.SHL i13 i13 2 || SAU.ADD.i32 i25 i17 i12 
	
	LSU0.LD32 i6 i20
	IAU.SHL i14 i14 2 || SAU.ADD.i32 i26 i17 i13 
	SAU.ADD.i32 i27 i17 i14 
	LSU0.LDi128.u8.u16 v6 i18 
	LSU0.LDi128.u8.u16 v7 i18 
	LSU0.LDi128.u8.u16 v8 i18 
	
	IAU.ADD i6 i6 1
	LSU0.ST32 i6 i20
	CMU.CPVI.x16      i6.l   v1.0
	
	LSU0.LD32 i7 i21
	nop 5
	IAU.ADD i7 i7 1
	LSU0.ST32 i7 i21
	CMU.CPVI.x16      i7.l   v1.1 || IAU.SHL i6 i6 2
	
	LSU0.LD32 i8 i22
	nop 5
	IAU.ADD i8 i8 1
	LSU0.ST32 i8 i22
	CMU.CPVI.x16      i8.l   v1.2 || IAU.SHL i7 i7 2 || SAU.ADD.i32 i20 i17 i6
	
	LSU0.LD32 i9 i23
	nop 5
	IAU.ADD i9 i9 1
	LSU0.ST32 i9 i23
	CMU.CPVI.x16      i9.l   v1.3 || IAU.SHL i8 i8 2 || SAU.ADD.i32 i21 i17 i7
	
	LSU0.LD32 i10 i24
	nop 5
	IAU.ADD i10 i10 1
	LSU0.ST32 i10 i24
	CMU.CPVI.x16      i10.l   v1.4 || IAU.SHL i9 i9 2 || SAU.ADD.i32 i22 i17 i8   
	
	LSU0.LD32 i12 i25
	nop 5
	IAU.ADD i12 i12 1
	LSU0.ST32 i12 i25
	CMU.CPVI.x16      i12.l   v1.5 || IAU.SHL i10 i10 2 || SAU.ADD.i32 i23 i17 i9  
	
	LSU0.LD32 i13 i26
	nop 5
	IAU.ADD i13 i13 1
	LSU0.ST32 i13 i26
	CMU.CPVI.x16      i13.l   v1.6 || IAU.SHL i12 i12 2 || SAU.ADD.i32 i24 i17 i10
	
	LSU0.LD32 i14 i27
	IAU.ADD i15 i15 i3
	CMU.CMII.i32 i15 i1
	PEU.PC1C LT || BRU.JMP i5
	nop 2
	IAU.ADD i14 i14 1
	LSU0.ST32 i14 i27
	CMU.CPVI.x16      i14.l   v1.7 || IAU.SHL i13 i13 2 || SAU.ADD.i32 i25 i17 i12 
	
IAU.SUB i18 i18 i3


_____compensation:
	CMU.CMZ.i32 i2
    PEU.PC1C EQ ||  BRU.BRA _____final
_____compensation_loop:

        LSU0.LDI32.u8.u32 i9 i18
        IAU.INCS i2 -1
        NOP 4
        IAU.SHL i9 i9 2
        IAU.ADD i9 i17 i9
        LSU0.LD32 i8 i9
        CMU.CMZ.i32 i2
        PEU.PC1C NEQ
            ||  BRU.BRA _____compensation_loop
        NOP 3
        IAU.ADD i8 i8 1
        LSU0.ST32 i8 i9
_____final:	
LSU0.LD32  i29  i19 || IAU.ADD i19 i19 4
LSU0.LD32  i28  i19 || IAU.ADD i19 i19 4	
LSU0.LD32  i27  i19 || IAU.ADD i19 i19 4
LSU0.LD32  i26  i19 || IAU.ADD i19 i19 4
LSU0.LD32  i25  i19 || IAU.ADD i19 i19 4
LSU0.LD32  i24  i19 || IAU.ADD i19 i19 4
LSU0.LD32  i23  i19 || IAU.ADD i19 i19 4
LSU0.LD32  i22  i19 || IAU.ADD i19 i19 4
LSU0.LD32  i21  i19 || IAU.ADD i19 i19 4
LSU0.LD32  i20  i19 || IAU.ADD i19 i19 4
nop 5


BRU.JMP i30
NOP 5
.end
 

