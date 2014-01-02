; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief
; ///

.version 00.51.04

.data .data.integralimage_sqsum_f32

.code .text.integralimage_sqsum_f32
;void integralimage_sqsum_f32(u8** in, u8** out, fp32* sum, u32 width);
;                                i18      i17       i16         i15
integralimage_sqsum_f32_asm:

IAU.SUB i19 i19 4
LSU0.ST.32  i20  i19 || IAU.SUB i19 i19 4
LSU0.ST.32  i21  i19 || IAU.SUB i19 i19 4
LSU0.ST.32  i22  i19 || IAU.SUB i19 i19 4
LSU0.ST.32  i23  i19 || IAU.SUB i19 i19 4
LSU0.ST.32  i24  i19 || IAU.SUB i19 i19 4
LSU0.ST.32  i25  i19 

LSU1.LD.32  i18  i18  
LSU0.LD.32  i17  i17
LSU0.LDIL i12 ___mainloop || LSU1.LDIH i12 ___mainloop ;loop 
IAU.SHR.u32 i1  i15  2
IAU.SHL  i1  i1  2
IAU.SUB i2 i2 i2 ;contor for loop

SAU.SUB.f32 i20 i20 i20 ;sum for pixels
IAU.SUB i24 i16 0


LSU0.LD.32.u8.u32     i8  i18 || IAU.ADD i18 i18 1      
LSU0.LD.32.u8.u32     i9  i18 || IAU.ADD i18 i18 1
LSU0.LD.32.u8.u32     i10  i18 || IAU.ADD i18 i18 1
LSU0.LD.32.u8.u32     i11  i18 || IAU.ADD i18 i18 1
nop


LSU0.LD.32  i6  i16 || IAU.ADD i16 i16 4      
LSU0.LD.32  i7  i16 || IAU.ADD i16 i16 4
LSU0.LD.32  i13  i16 || IAU.ADD i16 i16 4 || CMU.CPII.i32.f32  i0 i8    
LSU0.LD.32  i14  i16 || IAU.ADD i16 i16 4 || CMU.CPII.i32.f32  i3 i9    

CMU.CPII.i32.f32  i4 i10  || SAU.MUL.f32 i0 i0 i0   
       
CMU.CPII.i32.f32  i5 i11  || SAU.MUL.f32 i3 i3 i3   
___mainloop:

SAU.MUL.f32 i4 i4 i4

SAU.MUL.f32 i5 i5 i5

SAU.ADD.f32 i21 i6 i0

SAU.ADD.f32 i22 i7 i3

SAU.ADD.f32 i23 i13 i4

SAU.ADD.f32 i25 i14 i5

SAU.ADD.f32 i20 i20 i21 || LSU0.LD.32.u8.u32     i8  i18 || IAU.ADD i18 i18 1  
LSU0.LD.32.u8.u32     i9  i18 || IAU.ADD i18 i18 1 
LSU0.LD.32.u8.u32     i10  i18 || IAU.ADD i18 i18 1
LSU0.ST.32  i20  i17 || IAU.ADD i17 i17 4 || LSU1.LD.32.u8.u32     i11  i18 || SAU.ADD.i32 i18 i18 1
SAU.ADD.f32 i20 i20 i22 || LSU0.ST.32  i21  i24 || IAU.ADD i24 i24 4    

LSU0.LD.32  i6  i16 || IAU.ADD i16 i16 4      

LSU0.LD.32  i7  i16 || IAU.ADD i16 i16 4 || SAU.ADD.i32 i2 i2 4
LSU0.ST.32  i20  i17 || IAU.ADD i17 i17 4 || LSU1.LD.32  i13  i16 || SAU.ADD.i32 i16 i16 4 || CMU.CPII.i32.f32  i0 i8 
SAU.ADD.f32 i20 i20 i23 || LSU0.ST.32  i22  i24 || IAU.ADD i24 i24 4  || CMU.CMII.i32 i2 i1 
LSU0.LD.32  i14  i16 || IAU.ADD i16 i16 4 || CMU.CPII.i32.f32  i3 i9    
CMU.CPII.i32.f32  i4 i10  || SAU.MUL.f32 i0 i0 i0  
PEU.PC1C LT || BRU.JMP i12
LSU0.ST.32  i20  i17 || IAU.ADD i17 i17 4 || CMU.CPII.i32.f32  i5 i11  || SAU.MUL.f32 i3 i3 i3   
SAU.ADD.f32 i20 i20 i25 || LSU0.ST.32  i23  i24 || IAU.ADD i24 i24 4    
nop 3
LSU0.ST.32  i20  i17 || IAU.ADD i17 i17 4 || LSU1.ST.32  i25  i24 || SAU.ADD.i32 i24 i24 4

LSU0.LD.32  i6  i16 || IAU.ADD i16 i16 4      

LSU0.LD.32  i7  i16 || IAU.ADD i16 i16 4

LSU0.LD.32  i13  i16 || IAU.ADD i16 i16 4


LSU0.LD.32.u8.u32     i8  i18 || IAU.ADD i18 i18 1      
LSU0.LD.32.u8.u32     i9  i18 || IAU.ADD i18 i18 1
LSU0.LD.32.u8.u32     i10  i18 || IAU.ADD i18 i18 1


nop 4

CMU.CPII.i32.f32  i0 i8     

CMU.CPII.i32.f32  i3 i9     
CMU.CPII.i32.f32  i4 i10 || sAU.ADD.f32 i21 i6 i0

sAU.ADD.f32 i22 i7 i3

sAU.ADD.f32 i23 i13 i4 || CMU.CMII.i32 i1 i15 

sAU.ADD.f32 i20 i20 i21
nop 2
PEU.PC1C LT || LSU0.ST.32  i20  i17 || IAU.ADD i17 i17 4 || LSU1.ST.32  i21  i24 || SAU.ADD.i32 i24 i24 4

sAU.ADD.f32 i20 i20 i22  

IAU.ADD i1 i1 1
CMU.CMII.i32 i1 i15 
PEU.PC1C LT || LSU0.ST.32  i20  i17 || IAU.ADD i17 i17 4 || LSU1.ST.32  i22  i24 || SAU.ADD.i32 i24 i24 4

sAU.ADD.f32 i20 i20 i23

IAU.ADD i1 i1 1
CMU.CMII.i32 i1 i15     
PEU.PC1C LT || LSU0.ST.32  i20  i17 || IAU.ADD i17 i17 4 || LSU1.ST.32  i23  i24 || SAU.ADD.i32 i24 i24 4



LSU0.LD.32  i25  i19 || IAU.ADD i19 i19 4
LSU0.LD.32  i24  i19 || IAU.ADD i19 i19 4
LSU0.LD.32  i23  i19 || IAU.ADD i19 i19 4
LSU0.LD.32  i22  i19 || IAU.ADD i19 i19 4
LSU0.LD.32  i21  i19 || IAU.ADD i19 i19 4
LSU0.LD.32  i20  i19 || IAU.ADD i19 i19 4
nop 6


BRU.jmp i30
nop 6

.end
