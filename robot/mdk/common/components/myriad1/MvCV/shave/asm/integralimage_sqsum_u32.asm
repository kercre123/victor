; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief
; ///

.version 00.51.04

.data .data.integralimage_sqsum_u32

.code .text.integralimage_sqsum_u32

integralimage_sqsum_u32_asm:

IAU.SUB i19 i19 4
LSU0.ST32  i20  i19 || IAU.SUB i19 i19 4
LSU0.ST32  i21  i19 || IAU.SUB i19 i19 4
LSU0.ST32  i22  i19 || IAU.SUB i19 i19 4
LSU0.ST32  i23  i19 || IAU.SUB i19 i19 4
LSU0.ST32  i24  i19 

LSU1.LD32  i18  i18  
LSU0.LD32  i17  i17
LSU0.LDIL i12 ___mainloop || LSU1.LDIH i12 ___mainloop ;loop 
IAU.SHR.u32 i1  i15  2
IAU.SHL  i1  i1  2
IAU.SUB i2 i2 i2 ;contor for loop
IAU.SUB i3 i3 i3 ;sum for pixels
IAU.SUB i24 i16 0


LSU0.LD32.u8.u32     i8  i18 || IAU.ADD i18 i18 1      
LSU0.LD32.u8.u32     i9  i18 || IAU.ADD i18 i18 1
LSU0.LD32.u8.u32     i10  i18 || IAU.ADD i18 i18 1
LSU0.LD32.u8.u32     i11  i18 || IAU.ADD i18 i18 1


LSU0.LD32  i4  i16 || IAU.ADD i16 i16 4      
LSU0.LD32  i5  i16 || IAU.ADD i16 i16 4



___mainloop:

LSU0.LD32  i6  i16 || IAU.ADD i16 i16 4 || SAU.MUL.i32 i8 i8 i8
LSU0.LD32  i7  i16 || IAU.ADD i16 i16 4 || SAU.MUL.i32 i9 i9 i9
SAU.MUL.i32 i10 i10 i10 || IAU.ADD i2 i2 4
SAU.MUL.i32 i11 i11 i11 || CMU.CMII.i32 i2 i1 


IAU.ADD i20 i4 i8
IAU.ADD i21 i5 i9 || LSU0.ST32  i20  i24 || SAU.ADD.i32 i24 i24 4
IAU.ADD i22 i6 i10 || LSU0.LD32.u8.u32     i8  i18 || SAU.ADD.i32 i18 i18 1    
IAU.ADD i23 i7 i11 || LSU0.ST32  i21  i24 || SAU.ADD.i32 i24 i24 4

IAU.ADD i3 i3 i20 || LSU0.LD32.u8.u32     i9  i18 || SAU.ADD.i32 i18 i18 1
LSU0.ST32  i3  i17 || IAU.ADD i17 i17 4 || LSU1.ST32  i22  i24 || SAU.ADD.i32 i24 i24 4
IAU.ADD i3 i3 i21   || LSU0.LD32.u8.u32     i10  i18 || SAU.ADD.i32 i18 i18 1 

LSU0.ST32  i3  i17 || IAU.ADD i17 i17 4 || LSU1.ST32  i23  i24 || SAU.ADD.i32 i24 i24 4
PEU.PC1C LT || BRU.JMP i12
IAU.ADD i3 i3 i22    || LSU0.LD32.u8.u32     i11  i18 || SAU.ADD.i32 i18 i18 1
LSU0.ST32  i3  i17 || IAU.ADD i17 i17 4 || LSU1.LD32  i4  i16 || SAU.ADD.i32 i16 i16 4      
IAU.ADD i3 i3 i23    
LSU0.ST32  i3  i17 || IAU.ADD i17 i17 4 || LSU1.LD32  i5  i16 || SAU.ADD.i32 i16 i16 4
nop 


LSU0.LD32.u8.u32     i8  i18 || IAU.ADD i18 i18 1      
LSU0.LD32.u8.u32     i9  i18 || IAU.ADD i18 i18 1
LSU0.LD32.u8.u32     i10  i18 || IAU.ADD i18 i18 1



LSU0.LD32  i4  i16 || IAU.ADD i16 i16 4      
LSU0.LD32  i5  i16 || IAU.ADD i16 i16 4
LSU0.LD32  i6  i16 || IAU.ADD i16 i16 4




SAU.MUL.i32 i8 i8 i8
SAU.MUL.i32 i9 i9 i9
SAU.MUL.i32 i10 i10 i10


IAU.ADD i20 i4 i8
IAU.ADD i21 i5 i9
IAU.ADD i22 i6 i10


CMU.CMII.i32 i1 i15 


IAU.ADD i3 i3 i20
PEU.PC1C LT || LSU0.ST32  i3  i17 || IAU.ADD i17 i17 4 || LSU1.ST32  i20  i24 || SAU.ADD.i32 i24 i24 4
IAU.ADD i3 i3 i21  
IAU.ADD i1 i1 1
CMU.CMII.i32 i1 i15 
PEU.PC1C LT || LSU0.ST32  i3  i17 || IAU.ADD i17 i17 4 || LSU1.ST32  i21  i24 || SAU.ADD.i32 i24 i24 4
IAU.ADD i3 i3 i22
IAU.ADD i1 i1 1
CMU.CMII.i32 i1 i15     
PEU.PC1C LT || LSU0.ST32  i3  i17 || IAU.ADD i17 i17 4 || LSU1.ST32  i22  i24 || SAU.ADD.i32 i24 i24 4



LSU0.LD32  i24  i19 || IAU.ADD i19 i19 4
LSU0.LD32  i23  i19 || IAU.ADD i19 i19 4
LSU0.LD32  i22  i19 || IAU.ADD i19 i19 4
LSU0.LD32  i21  i19 || IAU.ADD i19 i19 4
LSU0.LD32  i20  i19 || IAU.ADD i19 i19 4
nop 5


BRU.jmp i30
nop 5

.end
 

