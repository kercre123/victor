; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief
; ///

.version 00.51.04

.data .data.equalizeHist

.code .text.equalizeHist
;void equalizeHist(u8** in, u8** out, u32 *cum_hist, u32 width)
;                    i18       i17        i16            i15
equalizeHist_asm:


IAU.SUB i19 i19 4
LSU0.ST32  i20  i19 || IAU.SUB i19 i19 4
LSU0.ST32  i21  i19 || IAU.SUB i19 i19 4
LSU0.ST32  i22  i19 || IAU.SUB i19 i19 4
LSU0.ST32  i23  i19 || IAU.SUB i19 i19 4
LSU0.ST32  i24  i19 || IAU.SUB i19 i19 4
LSU0.ST32  i25  i19 || IAU.SUB i19 i19 4
LSU0.ST32  i26  i19 || IAU.SUB i19 i19 4
LSU0.ST32  i27  i19 || IAU.SUB i19 i19 4
LSU0.ST32  i28  i19 

LSU0.ldil i2, 0xff  || LSU1.ldih i2, 0x00 ;load 255
IAU.SHL i2 i2 2
IAU.ADD i3 i16 i2
LSU0.LD32 i4 i3 ;i4 is the w*h dimension
IAU.SHR.u32 i2 i2 2
nop 4
CMU.CPIS.i32.f32  s1 i2 ;255 to f32
CMU.CPIS.i32.f32  s2 i4 ;w*h to s2
nop
SAU.DIV.f32  s3  s1 s2 ;255/(w*h)
LSU1.LD32  i18  i18 || LSU0.LD32  i17  i17 ;load input and output line address
nop 5
LSU0.ldil i1, ___loop  || LSU1.ldih i1, ___loop ;loop for processing loop
nop 4
IAU.SUB i10 i16 0 ;copy of histogram address
IAU.SUB i11 i16 0 ;copy of histogram address
IAU.SUB i12 i16 0 ;copy of histogram address
IAU.SUB i13 i16 0 ;copy of histogram address
IAU.SUB i14 i16 0 ;copy of histogram address
IAU.SUB i20 i16 0 ;copy of histogram address
IAU.SUB i21 i16 0 ;copy of histogram address
IAU.SUB i22 i16 0 ;copy of histogram address

IAU.SUB i2 i2 i2
IAU.SUB i3 i3 i3
IAU.SUB i4 i4 i4
IAU.SUB i5 i5 i5
IAU.SUB i6 i6 i6
IAU.SUB i7 i7 i7
IAU.SUB i8 i8 i8
IAU.SUB i9 i9 i9

LSU0.ldil i27, 0x30  || LSU1.ldih i27, 0x0 ;48 pixels are processed
LSU0.ldil i23, 0x31  || LSU1.ldih i23, 0x0 ;49

LSU0.ldil i24, ___small  || LSU1.ldih i24, ___small ;jump if width is smaller then 49
IAU.SUB i19 i19 i27 ;making place for compensation pixels
IAU.SUB i26 i19 0 ;copy of stack address



CMU.CMII.i32 i15 i23
PEU.PC1C LT || BRU.JMP i24
nop 5


IAU.SUB i28 i28 i28 ;contor for loop
IAU.ADD i28 i28 i27

___loop:
LSU0.LDi128.u8.u16 v1 i18 
LSU0.LDi128.u8.u16 v2 i18 
LSU0.LDi128.u8.u16 v3 i18 
LSU0.LDi128.u8.u16 v4 i18 
LSU0.LDi128.u8.u16 v5 i18 
LSU0.LDi128.u8.u16 v6 i18 

;processing pixels for v1
CMU.CPVI.x16 i2.l   v1.0
CMU.CPVI.x16 i3.l   v1.1  || IAU.SHL i2 i2 2
CMU.CPVI.x16 i4.l   v1.2  || IAU.SHL i3 i3 2 || SAU.ADD.i32 i2 i16 i2
CMU.CPVI.x16 i5.l   v1.3  || IAU.SHL i4 i4 2 || SAU.ADD.i32 i3 i16 i3
CMU.CPVI.x16 i6.l   v1.4  || IAU.SHL i5 i5 2 || SAU.ADD.i32 i4 i16 i4 || LSU0.LD32   i10  i2  
CMU.CPVI.x16 i7.l   v1.5  || IAU.SHL i6 i6 2 || SAU.ADD.i32 i5 i16 i5 || LSU1.LD32   i11  i3  
CMU.CPVI.x16 i8.l   v1.6  || IAU.SHL i7 i7 2 || SAU.ADD.i32 i6 i16 i6 || LSU0.LD32   i12  i4  
CMU.CPVI.x16 i9.l   v1.7  || IAU.SHL i8 i8 2 || SAU.ADD.i32 i7 i16 i7 || LSU1.LD32   i13  i5 
IAU.SHL i9 i9 2 || SAU.ADD.i32 i8 i16 i8 || LSU0.LD32   i14  i6  
SAU.ADD.i32 i9 i16 i9 || LSU1.LD32   i20  i7  


LSU0.LD32   i21  i8 || CMU.CPIS.i32.f32 s4  i10  
LSU1.LD32   i22  i9 || CMU.CPIS.i32.f32 s5  i11 

CMU.CPIS.i32.f32 s6  i12 || SAU.MUL.f32  s12  s4  s3
CMU.CPIS.i32.f32 s7  i13 || SAU.MUL.f32  s13  s5  s3
CMU.CPIS.i32.f32 s8  i14 || SAU.MUL.f32  s14  s6  s3
CMU.CPIS.i32.f32 s9  i20 || SAU.MUL.f32  s15  s7  s3
CMU.CPIS.i32.f32 s10 i21 || SAU.MUL.f32  s16  s8  s3
CMU.CPIS.i32.f32 s11 i22 || SAU.MUL.f32  s17  s9  s3

SAU.MUL.f32  s18  s10 s3 || CMU.CPSI.f32.i32s i2 s12
SAU.MUL.f32  s19  s11 s3 || CMU.CPSI.f32.i32s i3 s13

CMU.CPSI.f32.i32s i4 s14
CMU.CPSI.f32.i32s i5 s15
CMU.CPSI.f32.i32s i6 s16
CMU.CPSI.f32.i32s i7 s17
CMU.CPSI.f32.i32s i8 s18
CMU.CPSI.f32.i32s i9 s19


CMU.CPIV.x16  v11.0  i2.l
CMU.CPIV.x16  v11.1  i3.l
CMU.CPIV.x16  v11.2  i4.l
CMU.CPIV.x16  v11.3  i5.l
CMU.CPIV.x16  v11.4  i6.l
CMU.CPIV.x16  v11.5  i7.l
CMU.CPIV.x16  v11.6  i8.l
CMU.CPIV.x16  v11.7  i9.l

;processing pixels for v2
CMU.CPVI.x16 i2.l   v2.0
CMU.CPVI.x16 i3.l   v2.1 || IAU.SHL i2 i2 2
CMU.CPVI.x16 i4.l   v2.2 || IAU.SHL i3 i3 2 || SAU.ADD.i32 i2 i16 i2
CMU.CPVI.x16 i5.l   v2.3 || IAU.SHL i4 i4 2 || SAU.ADD.i32 i3 i16 i3
CMU.CPVI.x16 i6.l   v2.4 || IAU.SHL i5 i5 2 || SAU.ADD.i32 i4 i16 i4 || LSU0.LD32   i10  i2  
CMU.CPVI.x16 i7.l   v2.5 || IAU.SHL i6 i6 2 || SAU.ADD.i32 i5 i16 i5 || LSU0.LD32   i11  i3  
CMU.CPVI.x16 i8.l   v2.6 || IAU.SHL i7 i7 2 || SAU.ADD.i32 i6 i16 i6 || LSU0.LD32   i12  i4  
CMU.CPVI.x16 i9.l   v2.7 || IAU.SHL i8 i8 2 || SAU.ADD.i32 i7 i16 i7 || LSU0.LD32   i13  i5  
IAU.SHL i9 i9 2 || SAU.ADD.i32 i8 i16 i8 || LSU0.LD32   i14  i6  
SAU.ADD.i32 i9 i16 i9 || LSU0.LD32   i20  i7 
 
LSU0.LD32   i21  i8  || CMU.CPIS.i32.f32 s4  i10
LSU0.LD32   i22  i9  || CMU.CPIS.i32.f32 s5  i11

CMU.CPIS.i32.f32 s6  i12 || SAU.MUL.f32  s12  s4  s3
CMU.CPIS.i32.f32 s7  i13 || SAU.MUL.f32  s13  s5  s3
CMU.CPIS.i32.f32 s8  i14 || SAU.MUL.f32  s14  s6  s3
CMU.CPIS.i32.f32 s9  i20 || SAU.MUL.f32  s15  s7  s3
CMU.CPIS.i32.f32 s10 i21 || SAU.MUL.f32  s16  s8  s3
CMU.CPIS.i32.f32 s11 i22 || SAU.MUL.f32  s17  s9  s3

SAU.MUL.f32  s18  s10 s3  || CMU.CPSI.f32.i32s i2 s12
SAU.MUL.f32  s19  s11 s3  || CMU.CPSI.f32.i32s i3 s13

CMU.CPSI.f32.i32s i4 s14
CMU.CPSI.f32.i32s i5 s15
CMU.CPSI.f32.i32s i6 s16
CMU.CPSI.f32.i32s i7 s17
CMU.CPSI.f32.i32s i8 s18
CMU.CPSI.f32.i32s i9 s19


CMU.CPIV.x16  v12.0  i2.l
CMU.CPIV.x16  v12.1  i3.l
CMU.CPIV.x16  v12.2  i4.l
CMU.CPIV.x16  v12.3  i5.l
CMU.CPIV.x16  v12.4  i6.l
CMU.CPIV.x16  v12.5  i7.l
CMU.CPIV.x16  v12.6  i8.l
CMU.CPIV.x16  v12.7  i9.l

;processing pixels for v3
CMU.CPVI.x16 i2.l   v3.0
CMU.CPVI.x16 i3.l   v3.1 || IAU.SHL i2 i2 2
CMU.CPVI.x16 i4.l   v3.2 || IAU.SHL i3 i3 2 || SAU.ADD.i32 i2 i16 i2
CMU.CPVI.x16 i5.l   v3.3 || IAU.SHL i4 i4 2 || SAU.ADD.i32 i3 i16 i3
CMU.CPVI.x16 i6.l   v3.4 || IAU.SHL i5 i5 2 || SAU.ADD.i32 i4 i16 i4 || LSU0.LD32   i10  i2 
CMU.CPVI.x16 i7.l   v3.5 || IAU.SHL i6 i6 2 || SAU.ADD.i32 i5 i16 i5 || LSU0.LD32   i11  i3 
CMU.CPVI.x16 i8.l   v3.6 || IAU.SHL i7 i7 2 || SAU.ADD.i32 i6 i16 i6 || LSU0.LD32   i12  i4 
CMU.CPVI.x16 i9.l   v3.7 || IAU.SHL i8 i8 2 || SAU.ADD.i32 i7 i16 i7 || LSU0.LD32   i13  i5 
IAU.SHL i9 i9 2  || SAU.ADD.i32 i8 i16 i8   || LSU0.LD32   i14  i6 
SAU.ADD.i32 i9 i16 i9   || LSU0.LD32   i20  i7 
LSU0.LD32   i21  i8 || CMU.CPIS.i32.f32 s4  i10
LSU0.LD32   i22  i9 || CMU.CPIS.i32.f32 s5  i11
 
CMU.CPIS.i32.f32 s6  i12 || SAU.MUL.f32  s12  s4  s3
CMU.CPIS.i32.f32 s7  i13 || SAU.MUL.f32  s13  s5  s3
CMU.CPIS.i32.f32 s8  i14 || SAU.MUL.f32  s14  s6  s3
CMU.CPIS.i32.f32 s9  i20 || SAU.MUL.f32  s15  s7  s3
CMU.CPIS.i32.f32 s10 i21 || SAU.MUL.f32  s16  s8  s3
CMU.CPIS.i32.f32 s11 i22 || SAU.MUL.f32  s17  s9  s3
SAU.MUL.f32  s18  s10 s3 || CMU.CPSI.f32.i32s i2 s12
SAU.MUL.f32  s19  s11 s3 || CMU.CPSI.f32.i32s i3 s13

CMU.CPSI.f32.i32s i4 s14
CMU.CPSI.f32.i32s i5 s15
CMU.CPSI.f32.i32s i6 s16
CMU.CPSI.f32.i32s i7 s17
CMU.CPSI.f32.i32s i8 s18
CMU.CPSI.f32.i32s i9 s19


CMU.CPIV.x16  v13.0  i2.l
CMU.CPIV.x16  v13.1  i3.l
CMU.CPIV.x16  v13.2  i4.l
CMU.CPIV.x16  v13.3  i5.l
CMU.CPIV.x16  v13.4  i6.l
CMU.CPIV.x16  v13.5  i7.l
CMU.CPIV.x16  v13.6  i8.l
CMU.CPIV.x16  v13.7  i9.l

;processing pixels for v4
CMU.CPVI.x16 i2.l   v4.0
CMU.CPVI.x16 i3.l   v4.1 || IAU.SHL i2 i2 2
CMU.CPVI.x16 i4.l   v4.2 || IAU.SHL i3 i3 2  || SAU.ADD.i32 i2 i16 i2
CMU.CPVI.x16 i5.l   v4.3 || IAU.SHL i4 i4 2  || SAU.ADD.i32 i3 i16 i3
CMU.CPVI.x16 i6.l   v4.4 || IAU.SHL i5 i5 2  || SAU.ADD.i32 i4 i16 i4 || LSU0.LD32   i10  i2 
CMU.CPVI.x16 i7.l   v4.5 || IAU.SHL i6 i6 2  || SAU.ADD.i32 i5 i16 i5 || LSU0.LD32   i11  i3 
CMU.CPVI.x16 i8.l   v4.6 || IAU.SHL i7 i7 2  || SAU.ADD.i32 i6 i16 i6 || LSU0.LD32   i12  i4 
CMU.CPVI.x16 i9.l   v4.7 || IAU.SHL i8 i8 2  || SAU.ADD.i32 i7 i16 i7 || LSU0.LD32   i13  i5 
IAU.SHL i9 i9 2 || SAU.ADD.i32 i8 i16 i8     || LSU0.LD32   i14  i6 
SAU.ADD.i32 i9 i16 i9   || LSU0.LD32   i20  i7 
LSU0.LD32   i21  i8 || CMU.CPIS.i32.f32 s4  i10
LSU0.LD32   i22  i9 || CMU.CPIS.i32.f32 s5  i11

CMU.CPIS.i32.f32 s6  i12 || SAU.MUL.f32  s12  s4  s3
CMU.CPIS.i32.f32 s7  i13 || SAU.MUL.f32  s13  s5  s3
CMU.CPIS.i32.f32 s8  i14 || SAU.MUL.f32  s14  s6  s3
CMU.CPIS.i32.f32 s9  i20 || SAU.MUL.f32  s15  s7  s3
CMU.CPIS.i32.f32 s10 i21 || SAU.MUL.f32  s16  s8  s3
CMU.CPIS.i32.f32 s11 i22 || SAU.MUL.f32  s17  s9  s3
SAU.MUL.f32  s18  s10 s3 || CMU.CPSI.f32.i32s i2 s12
SAU.MUL.f32  s19  s11 s3 || CMU.CPSI.f32.i32s i3 s13

CMU.CPSI.f32.i32s i4 s14
CMU.CPSI.f32.i32s i5 s15
CMU.CPSI.f32.i32s i6 s16
CMU.CPSI.f32.i32s i7 s17
CMU.CPSI.f32.i32s i8 s18
CMU.CPSI.f32.i32s i9 s19

CMU.CPIV.x16  v14.0  i2.l
CMU.CPIV.x16  v14.1  i3.l
CMU.CPIV.x16  v14.2  i4.l
CMU.CPIV.x16  v14.3  i5.l
CMU.CPIV.x16  v14.4  i6.l
CMU.CPIV.x16  v14.5  i7.l
CMU.CPIV.x16  v14.6  i8.l
CMU.CPIV.x16  v14.7  i9.l

;processing pixels for v5
CMU.CPVI.x16 i2.l   v5.0
CMU.CPVI.x16 i3.l   v5.1  || IAU.SHL i2 i2 2
CMU.CPVI.x16 i4.l   v5.2  || IAU.SHL i3 i3 2  || SAU.ADD.i32 i2 i16 i2
CMU.CPVI.x16 i5.l   v5.3  || IAU.SHL i4 i4 2  || SAU.ADD.i32 i3 i16 i3
CMU.CPVI.x16 i6.l   v5.4  || IAU.SHL i5 i5 2  || SAU.ADD.i32 i4 i16 i4 || LSU0.LD32   i10  i2
CMU.CPVI.x16 i7.l   v5.5  || IAU.SHL i6 i6 2  || SAU.ADD.i32 i5 i16 i5 || LSU0.LD32   i11  i3
CMU.CPVI.x16 i8.l   v5.6  || IAU.SHL i7 i7 2  || SAU.ADD.i32 i6 i16 i6 || LSU0.LD32   i12  i4
CMU.CPVI.x16 i9.l   v5.7  || IAU.SHL i8 i8 2  || SAU.ADD.i32 i7 i16 i7 || LSU0.LD32   i13  i5
IAU.SHL i9 i9 2  || SAU.ADD.i32 i8 i16 i8    || LSU0.LD32   i14  i6
SAU.ADD.i32 i9 i16 i9  || LSU0.LD32   i20  i7
LSU0.LD32   i21  i8 || CMU.CPIS.i32.f32 s4  i10
LSU0.LD32   i22  i9 || CMU.CPIS.i32.f32 s5  i11

CMU.CPIS.i32.f32 s6  i12  || SAU.MUL.f32  s12  s4  s3
CMU.CPIS.i32.f32 s7  i13  || SAU.MUL.f32  s13  s5  s3
CMU.CPIS.i32.f32 s8  i14  || SAU.MUL.f32  s14  s6  s3
CMU.CPIS.i32.f32 s9  i20  || SAU.MUL.f32  s15  s7  s3
CMU.CPIS.i32.f32 s10 i21  || SAU.MUL.f32  s16  s8  s3
CMU.CPIS.i32.f32 s11 i22  || SAU.MUL.f32  s17  s9  s3
SAU.MUL.f32  s18  s10 s3  || CMU.CPSI.f32.i32s i2 s12
SAU.MUL.f32  s19  s11 s3  || CMU.CPSI.f32.i32s i3 s13

CMU.CPSI.f32.i32s i4 s14
CMU.CPSI.f32.i32s i5 s15
CMU.CPSI.f32.i32s i6 s16
CMU.CPSI.f32.i32s i7 s17
CMU.CPSI.f32.i32s i8 s18
CMU.CPSI.f32.i32s i9 s19


CMU.CPIV.x16  v15.0  i2.l
CMU.CPIV.x16  v15.1  i3.l
CMU.CPIV.x16  v15.2  i4.l
CMU.CPIV.x16  v15.3  i5.l
CMU.CPIV.x16  v15.4  i6.l
CMU.CPIV.x16  v15.5  i7.l
CMU.CPIV.x16  v15.6  i8.l
CMU.CPIV.x16  v15.7  i9.l

;processing pixels for v6
CMU.CPVI.x16 i2.l   v6.0
CMU.CPVI.x16 i3.l   v6.1 || IAU.SHL i2 i2 2
CMU.CPVI.x16 i4.l   v6.2 || IAU.SHL i3 i3 2 || SAU.ADD.i32 i2 i16 i2
CMU.CPVI.x16 i5.l   v6.3 || IAU.SHL i4 i4 2 || SAU.ADD.i32 i3 i16 i3
CMU.CPVI.x16 i6.l   v6.4 || IAU.SHL i5 i5 2 || SAU.ADD.i32 i4 i16 i4 || LSU0.LD32   i10  i2
CMU.CPVI.x16 i7.l   v6.5 || IAU.SHL i6 i6 2 || SAU.ADD.i32 i5 i16 i5 || LSU0.LD32   i11  i3
CMU.CPVI.x16 i8.l   v6.6 || IAU.SHL i7 i7 2 || SAU.ADD.i32 i6 i16 i6 || LSU0.LD32   i12  i4
CMU.CPVI.x16 i9.l   v6.7 || IAU.SHL i8 i8 2 || SAU.ADD.i32 i7 i16 i7 || LSU0.LD32   i13  i5
IAU.SHL i9 i9 2  || SAU.ADD.i32 i8 i16 i8   || LSU0.LD32   i14  i6
SAU.ADD.i32 i9 i16 i9   || LSU0.LD32   i20  i7
LSU0.LD32   i21  i8 || CMU.CPIS.i32.f32 s4  i10
LSU0.LD32   i22  i9 || CMU.CPIS.i32.f32 s5  i11
  

CMU.CPIS.i32.f32 s6  i12   || SAU.MUL.f32  s12  s4  s3
CMU.CPIS.i32.f32 s7  i13   || SAU.MUL.f32  s13  s5  s3
CMU.CPIS.i32.f32 s8  i14   || SAU.MUL.f32  s14  s6  s3
CMU.CPIS.i32.f32 s9  i20   || SAU.MUL.f32  s15  s7  s3
CMU.CPIS.i32.f32 s10 i21   || SAU.MUL.f32  s16  s8  s3
CMU.CPIS.i32.f32 s11 i22   || SAU.MUL.f32  s17  s9  s3
SAU.MUL.f32  s18  s10 s3  || CMU.CPSI.f32.i32s i2 s12
SAU.MUL.f32  s19  s11 s3  || CMU.CPSI.f32.i32s i3 s13

CMU.CPSI.f32.i32s i4 s14
CMU.CPSI.f32.i32s i5 s15
CMU.CPSI.f32.i32s i6 s16
CMU.CPSI.f32.i32s i7 s17
CMU.CPSI.f32.i32s i8 s18
CMU.CPSI.f32.i32s i9 s19


CMU.CPIV.x16  v16.0  i2.l
CMU.CPIV.x16  v16.1  i3.l
CMU.CPIV.x16  v16.2  i4.l
CMU.CPIV.x16  v16.3  i5.l
CMU.CPIV.x16  v16.4  i6.l
CMU.CPIV.x16  v16.5  i7.l
CMU.CPIV.x16  v16.6  i8.l
CMU.CPIV.x16  v16.7  i9.l || IAU.ADD i28 i28 i27

LSU0.STi128.u16.u8  v11  i17  || CMU.CMII.i32 i28 i15 
PEU.PC1C LT   || BRU.JMP i1 
LSU0.STi128.u16.u8  v12  i17    
LSU0.STi128.u16.u8  v13  i17    
LSU0.STi128.u16.u8  v14  i17    
LSU0.STi128.u16.u8  v15  i17    
LSU0.STi128.u16.u8  v16  i17    


___small:
LSU0.LDi128.u8.u16 v1 i18 
LSU0.LDi128.u8.u16 v2 i18 
LSU0.LDi128.u8.u16 v3 i18 
LSU0.LDi128.u8.u16 v4 i18 
LSU0.LDi128.u8.u16 v5 i18 
LSU0.LDi128.u8.u16 v6 i18 

;processing pixels for v1
CMU.CPVI.x16 i2.l   v1.0
CMU.CPVI.x16 i3.l   v1.1  || IAU.SHL i2 i2 2
CMU.CPVI.x16 i4.l   v1.2  || IAU.SHL i3 i3 2 || SAU.ADD.i32 i2 i16 i2
CMU.CPVI.x16 i5.l   v1.3  || IAU.SHL i4 i4 2 || SAU.ADD.i32 i3 i16 i3
CMU.CPVI.x16 i6.l   v1.4  || IAU.SHL i5 i5 2 || SAU.ADD.i32 i4 i16 i4 || LSU0.LD32   i10  i2  
CMU.CPVI.x16 i7.l   v1.5  || IAU.SHL i6 i6 2 || SAU.ADD.i32 i5 i16 i5 || LSU1.LD32   i11  i3  
CMU.CPVI.x16 i8.l   v1.6  || IAU.SHL i7 i7 2 || SAU.ADD.i32 i6 i16 i6 || LSU0.LD32   i12  i4  
CMU.CPVI.x16 i9.l   v1.7  || IAU.SHL i8 i8 2 || SAU.ADD.i32 i7 i16 i7 || LSU1.LD32   i13  i5 
IAU.SHL i9 i9 2 || SAU.ADD.i32 i8 i16 i8 || LSU0.LD32   i14  i6  
SAU.ADD.i32 i9 i16 i9 || LSU1.LD32   i20  i7  


LSU0.LD32   i21  i8 || CMU.CPIS.i32.f32 s4  i10  
LSU1.LD32   i22  i9 || CMU.CPIS.i32.f32 s5  i11 



CMU.CPIS.i32.f32 s6  i12 || SAU.MUL.f32  s12  s4  s3
CMU.CPIS.i32.f32 s7  i13 || SAU.MUL.f32  s13  s5  s3
CMU.CPIS.i32.f32 s8  i14 || SAU.MUL.f32  s14  s6  s3
CMU.CPIS.i32.f32 s9  i20 || SAU.MUL.f32  s15  s7  s3
CMU.CPIS.i32.f32 s10 i21 || SAU.MUL.f32  s16  s8  s3
CMU.CPIS.i32.f32 s11 i22 || SAU.MUL.f32  s17  s9  s3

SAU.MUL.f32  s18  s10 s3 || CMU.CPSI.f32.i32s i2 s12
SAU.MUL.f32  s19  s11 s3 || CMU.CPSI.f32.i32s i3 s13

CMU.CPSI.f32.i32s i4 s14
CMU.CPSI.f32.i32s i5 s15
CMU.CPSI.f32.i32s i6 s16
CMU.CPSI.f32.i32s i7 s17
CMU.CPSI.f32.i32s i8 s18
CMU.CPSI.f32.i32s i9 s19


CMU.CPIV.x16  v11.0  i2.l
CMU.CPIV.x16  v11.1  i3.l
CMU.CPIV.x16  v11.2  i4.l
CMU.CPIV.x16  v11.3  i5.l
CMU.CPIV.x16  v11.4  i6.l
CMU.CPIV.x16  v11.5  i7.l
CMU.CPIV.x16  v11.6  i8.l
CMU.CPIV.x16  v11.7  i9.l


;processing pixels for v2
CMU.CPVI.x16 i2.l   v2.0
CMU.CPVI.x16 i3.l   v2.1 || IAU.SHL i2 i2 2
CMU.CPVI.x16 i4.l   v2.2 || IAU.SHL i3 i3 2 || SAU.ADD.i32 i2 i16 i2
CMU.CPVI.x16 i5.l   v2.3 || IAU.SHL i4 i4 2 || SAU.ADD.i32 i3 i16 i3
CMU.CPVI.x16 i6.l   v2.4 || IAU.SHL i5 i5 2 || SAU.ADD.i32 i4 i16 i4 || LSU0.LD32   i10  i2  
CMU.CPVI.x16 i7.l   v2.5 || IAU.SHL i6 i6 2 || SAU.ADD.i32 i5 i16 i5 || LSU0.LD32   i11  i3  
CMU.CPVI.x16 i8.l   v2.6 || IAU.SHL i7 i7 2 || SAU.ADD.i32 i6 i16 i6 || LSU0.LD32   i12  i4  
CMU.CPVI.x16 i9.l   v2.7 || IAU.SHL i8 i8 2 || SAU.ADD.i32 i7 i16 i7 || LSU0.LD32   i13  i5  
IAU.SHL i9 i9 2 || SAU.ADD.i32 i8 i16 i8 || LSU0.LD32   i14  i6  
SAU.ADD.i32 i9 i16 i9 || LSU0.LD32   i20  i7 
 
LSU0.LD32   i21  i8  || CMU.CPIS.i32.f32 s4  i10
LSU0.LD32   i22  i9  || CMU.CPIS.i32.f32 s5  i11



CMU.CPIS.i32.f32 s6  i12 || SAU.MUL.f32  s12  s4  s3
CMU.CPIS.i32.f32 s7  i13 || SAU.MUL.f32  s13  s5  s3
CMU.CPIS.i32.f32 s8  i14 || SAU.MUL.f32  s14  s6  s3
CMU.CPIS.i32.f32 s9  i20 || SAU.MUL.f32  s15  s7  s3
CMU.CPIS.i32.f32 s10 i21 || SAU.MUL.f32  s16  s8  s3
CMU.CPIS.i32.f32 s11 i22 || SAU.MUL.f32  s17  s9  s3

SAU.MUL.f32  s18  s10 s3  || CMU.CPSI.f32.i32s i2 s12
SAU.MUL.f32  s19  s11 s3  || CMU.CPSI.f32.i32s i3 s13

CMU.CPSI.f32.i32s i4 s14
CMU.CPSI.f32.i32s i5 s15
CMU.CPSI.f32.i32s i6 s16
CMU.CPSI.f32.i32s i7 s17
CMU.CPSI.f32.i32s i8 s18
CMU.CPSI.f32.i32s i9 s19


CMU.CPIV.x16  v12.0  i2.l
CMU.CPIV.x16  v12.1  i3.l
CMU.CPIV.x16  v12.2  i4.l
CMU.CPIV.x16  v12.3  i5.l
CMU.CPIV.x16  v12.4  i6.l
CMU.CPIV.x16  v12.5  i7.l
CMU.CPIV.x16  v12.6  i8.l
CMU.CPIV.x16  v12.7  i9.l

;processing pixels for v3
CMU.CPVI.x16 i2.l   v3.0
CMU.CPVI.x16 i3.l   v3.1 || IAU.SHL i2 i2 2
CMU.CPVI.x16 i4.l   v3.2 || IAU.SHL i3 i3 2 || SAU.ADD.i32 i2 i16 i2
CMU.CPVI.x16 i5.l   v3.3 || IAU.SHL i4 i4 2 || SAU.ADD.i32 i3 i16 i3
CMU.CPVI.x16 i6.l   v3.4 || IAU.SHL i5 i5 2 || SAU.ADD.i32 i4 i16 i4 || LSU0.LD32   i10  i2 
CMU.CPVI.x16 i7.l   v3.5 || IAU.SHL i6 i6 2 || SAU.ADD.i32 i5 i16 i5 || LSU0.LD32   i11  i3 
CMU.CPVI.x16 i8.l   v3.6 || IAU.SHL i7 i7 2 || SAU.ADD.i32 i6 i16 i6 || LSU0.LD32   i12  i4 
CMU.CPVI.x16 i9.l   v3.7 || IAU.SHL i8 i8 2 || SAU.ADD.i32 i7 i16 i7 || LSU0.LD32   i13  i5 
IAU.SHL i9 i9 2  || SAU.ADD.i32 i8 i16 i8   || LSU0.LD32   i14  i6 
SAU.ADD.i32 i9 i16 i9   || LSU0.LD32   i20  i7 
LSU0.LD32   i21  i8 || CMU.CPIS.i32.f32 s4  i10
LSU0.LD32   i22  i9 || CMU.CPIS.i32.f32 s5  i11
 



CMU.CPIS.i32.f32 s6  i12 || SAU.MUL.f32  s12  s4  s3
CMU.CPIS.i32.f32 s7  i13 || SAU.MUL.f32  s13  s5  s3
CMU.CPIS.i32.f32 s8  i14 || SAU.MUL.f32  s14  s6  s3
CMU.CPIS.i32.f32 s9  i20 || SAU.MUL.f32  s15  s7  s3
CMU.CPIS.i32.f32 s10 i21 || SAU.MUL.f32  s16  s8  s3
CMU.CPIS.i32.f32 s11 i22 || SAU.MUL.f32  s17  s9  s3
SAU.MUL.f32  s18  s10 s3 || CMU.CPSI.f32.i32s i2 s12
SAU.MUL.f32  s19  s11 s3 || CMU.CPSI.f32.i32s i3 s13

CMU.CPSI.f32.i32s i4 s14
CMU.CPSI.f32.i32s i5 s15
CMU.CPSI.f32.i32s i6 s16
CMU.CPSI.f32.i32s i7 s17
CMU.CPSI.f32.i32s i8 s18
CMU.CPSI.f32.i32s i9 s19


CMU.CPIV.x16  v13.0  i2.l
CMU.CPIV.x16  v13.1  i3.l
CMU.CPIV.x16  v13.2  i4.l
CMU.CPIV.x16  v13.3  i5.l
CMU.CPIV.x16  v13.4  i6.l
CMU.CPIV.x16  v13.5  i7.l
CMU.CPIV.x16  v13.6  i8.l
CMU.CPIV.x16  v13.7  i9.l

;processing pixels for v4
CMU.CPVI.x16 i2.l   v4.0
CMU.CPVI.x16 i3.l   v4.1 || IAU.SHL i2 i2 2
CMU.CPVI.x16 i4.l   v4.2 || IAU.SHL i3 i3 2  || SAU.ADD.i32 i2 i16 i2
CMU.CPVI.x16 i5.l   v4.3 || IAU.SHL i4 i4 2  || SAU.ADD.i32 i3 i16 i3
CMU.CPVI.x16 i6.l   v4.4 || IAU.SHL i5 i5 2  || SAU.ADD.i32 i4 i16 i4 || LSU0.LD32   i10  i2 
CMU.CPVI.x16 i7.l   v4.5 || IAU.SHL i6 i6 2  || SAU.ADD.i32 i5 i16 i5 || LSU0.LD32   i11  i3 
CMU.CPVI.x16 i8.l   v4.6 || IAU.SHL i7 i7 2  || SAU.ADD.i32 i6 i16 i6 || LSU0.LD32   i12  i4 
CMU.CPVI.x16 i9.l   v4.7 || IAU.SHL i8 i8 2  || SAU.ADD.i32 i7 i16 i7 || LSU0.LD32   i13  i5 
IAU.SHL i9 i9 2 || SAU.ADD.i32 i8 i16 i8     || LSU0.LD32   i14  i6 
SAU.ADD.i32 i9 i16 i9   || LSU0.LD32   i20  i7 
LSU0.LD32   i21  i8 || CMU.CPIS.i32.f32 s4  i10
LSU0.LD32   i22  i9 || CMU.CPIS.i32.f32 s5  i11




CMU.CPIS.i32.f32 s6  i12 || SAU.MUL.f32  s12  s4  s3
CMU.CPIS.i32.f32 s7  i13 || SAU.MUL.f32  s13  s5  s3
CMU.CPIS.i32.f32 s8  i14 || SAU.MUL.f32  s14  s6  s3
CMU.CPIS.i32.f32 s9  i20 || SAU.MUL.f32  s15  s7  s3
CMU.CPIS.i32.f32 s10 i21 || SAU.MUL.f32  s16  s8  s3
CMU.CPIS.i32.f32 s11 i22 || SAU.MUL.f32  s17  s9  s3
SAU.MUL.f32  s18  s10 s3 || CMU.CPSI.f32.i32s i2 s12
SAU.MUL.f32  s19  s11 s3 || CMU.CPSI.f32.i32s i3 s13

CMU.CPSI.f32.i32s i4 s14
CMU.CPSI.f32.i32s i5 s15
CMU.CPSI.f32.i32s i6 s16
CMU.CPSI.f32.i32s i7 s17
CMU.CPSI.f32.i32s i8 s18
CMU.CPSI.f32.i32s i9 s19

CMU.CPIV.x16  v14.0  i2.l
CMU.CPIV.x16  v14.1  i3.l
CMU.CPIV.x16  v14.2  i4.l
CMU.CPIV.x16  v14.3  i5.l
CMU.CPIV.x16  v14.4  i6.l
CMU.CPIV.x16  v14.5  i7.l
CMU.CPIV.x16  v14.6  i8.l
CMU.CPIV.x16  v14.7  i9.l

;processing pixels for v5
CMU.CPVI.x16 i2.l   v5.0
CMU.CPVI.x16 i3.l   v5.1  || IAU.SHL i2 i2 2
CMU.CPVI.x16 i4.l   v5.2  || IAU.SHL i3 i3 2  || SAU.ADD.i32 i2 i16 i2
CMU.CPVI.x16 i5.l   v5.3  || IAU.SHL i4 i4 2  || SAU.ADD.i32 i3 i16 i3
CMU.CPVI.x16 i6.l   v5.4  || IAU.SHL i5 i5 2  || SAU.ADD.i32 i4 i16 i4 || LSU0.LD32   i10  i2
CMU.CPVI.x16 i7.l   v5.5  || IAU.SHL i6 i6 2  || SAU.ADD.i32 i5 i16 i5 || LSU0.LD32   i11  i3
CMU.CPVI.x16 i8.l   v5.6  || IAU.SHL i7 i7 2  || SAU.ADD.i32 i6 i16 i6 || LSU0.LD32   i12  i4
CMU.CPVI.x16 i9.l   v5.7  || IAU.SHL i8 i8 2  || SAU.ADD.i32 i7 i16 i7 || LSU0.LD32   i13  i5
IAU.SHL i9 i9 2  || SAU.ADD.i32 i8 i16 i8    || LSU0.LD32   i14  i6
SAU.ADD.i32 i9 i16 i9  || LSU0.LD32   i20  i7
LSU0.LD32   i21  i8 || CMU.CPIS.i32.f32 s4  i10
LSU0.LD32   i22  i9 || CMU.CPIS.i32.f32 s5  i11





CMU.CPIS.i32.f32 s6  i12  || SAU.MUL.f32  s12  s4  s3
CMU.CPIS.i32.f32 s7  i13  || SAU.MUL.f32  s13  s5  s3
CMU.CPIS.i32.f32 s8  i14  || SAU.MUL.f32  s14  s6  s3
CMU.CPIS.i32.f32 s9  i20  || SAU.MUL.f32  s15  s7  s3
CMU.CPIS.i32.f32 s10 i21  || SAU.MUL.f32  s16  s8  s3
CMU.CPIS.i32.f32 s11 i22  || SAU.MUL.f32  s17  s9  s3
SAU.MUL.f32  s18  s10 s3  || CMU.CPSI.f32.i32s i2 s12
SAU.MUL.f32  s19  s11 s3  || CMU.CPSI.f32.i32s i3 s13

CMU.CPSI.f32.i32s i4 s14
CMU.CPSI.f32.i32s i5 s15
CMU.CPSI.f32.i32s i6 s16
CMU.CPSI.f32.i32s i7 s17
CMU.CPSI.f32.i32s i8 s18
CMU.CPSI.f32.i32s i9 s19


CMU.CPIV.x16  v15.0  i2.l
CMU.CPIV.x16  v15.1  i3.l
CMU.CPIV.x16  v15.2  i4.l
CMU.CPIV.x16  v15.3  i5.l
CMU.CPIV.x16  v15.4  i6.l
CMU.CPIV.x16  v15.5  i7.l
CMU.CPIV.x16  v15.6  i8.l
CMU.CPIV.x16  v15.7  i9.l

;processing pixels for v6
CMU.CPVI.x16 i2.l   v6.0
CMU.CPVI.x16 i3.l   v6.1 || IAU.SHL i2 i2 2
CMU.CPVI.x16 i4.l   v6.2 || IAU.SHL i3 i3 2 || SAU.ADD.i32 i2 i16 i2
CMU.CPVI.x16 i5.l   v6.3 || IAU.SHL i4 i4 2 || SAU.ADD.i32 i3 i16 i3
CMU.CPVI.x16 i6.l   v6.4 || IAU.SHL i5 i5 2 || SAU.ADD.i32 i4 i16 i4 || LSU0.LD32   i10  i2
CMU.CPVI.x16 i7.l   v6.5 || IAU.SHL i6 i6 2 || SAU.ADD.i32 i5 i16 i5 || LSU0.LD32   i11  i3
CMU.CPVI.x16 i8.l   v6.6 || IAU.SHL i7 i7 2 || SAU.ADD.i32 i6 i16 i6 || LSU0.LD32   i12  i4
CMU.CPVI.x16 i9.l   v6.7 || IAU.SHL i8 i8 2 || SAU.ADD.i32 i7 i16 i7 || LSU0.LD32   i13  i5
IAU.SHL i9 i9 2  || SAU.ADD.i32 i8 i16 i8   || LSU0.LD32   i14  i6
SAU.ADD.i32 i9 i16 i9   || LSU0.LD32   i20  i7
LSU0.LD32   i21  i8 || CMU.CPIS.i32.f32 s4  i10
LSU0.LD32   i22  i9 || CMU.CPIS.i32.f32 s5  i11
  



CMU.CPIS.i32.f32 s6  i12   || SAU.MUL.f32  s12  s4  s3
CMU.CPIS.i32.f32 s7  i13   || SAU.MUL.f32  s13  s5  s3
CMU.CPIS.i32.f32 s8  i14   || SAU.MUL.f32  s14  s6  s3
CMU.CPIS.i32.f32 s9  i20   || SAU.MUL.f32  s15  s7  s3
CMU.CPIS.i32.f32 s10 i21   || SAU.MUL.f32  s16  s8  s3
CMU.CPIS.i32.f32 s11 i22   || SAU.MUL.f32  s17  s9  s3
SAU.MUL.f32  s18  s10 s3  || CMU.CPSI.f32.i32s i2 s12
SAU.MUL.f32  s19  s11 s3  || CMU.CPSI.f32.i32s i3 s13

CMU.CPSI.f32.i32s i4 s14
CMU.CPSI.f32.i32s i5 s15
CMU.CPSI.f32.i32s i6 s16
CMU.CPSI.f32.i32s i7 s17
CMU.CPSI.f32.i32s i8 s18
CMU.CPSI.f32.i32s i9 s19


CMU.CPIV.x16  v16.0  i2.l
CMU.CPIV.x16  v16.1  i3.l
CMU.CPIV.x16  v16.2  i4.l
CMU.CPIV.x16  v16.3  i5.l
CMU.CPIV.x16  v16.4  i6.l
CMU.CPIV.x16  v16.5  i7.l
CMU.CPIV.x16  v16.6  i8.l
CMU.CPIV.x16  v16.7  i9.l

LSU0.STi128.u16.u8  v11  i26    
LSU0.STi128.u16.u8  v12  i26    
LSU0.STi128.u16.u8  v13  i26    
LSU0.STi128.u16.u8  v14  i26    
LSU0.STi128.u16.u8  v15  i26    
LSU0.STi128.u16.u8  v16  i26 



IAU.SUB i28 i28 i27 ; calculating remaining pixels
IAU.SUB i28 i15 i28

LSU0.LDIL i1 ___compensation || LSU1.LDIH i1 ___compensation ;loop for R-plane
IAU.SUB i10 i10 i10

CMU.CMII.i32 i15 i23 ;see if the width is smaller then 49
PEU.PC1C LT || IAU.SUB i28  i15 0

___compensation:
LSU0.LD8    i2  i19  || IAU.ADD i19 i19 1
nop 5
LSU0.ST8    i2  i17  || IAU.ADD i17 i17 1
IAU.ADD i10 i10 1
CMU.CMII.i32 i10 i28
PEU.PC1C LT || BRU.JMP i1
nop 5


IAU.SUB i19 i26 0

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
 

