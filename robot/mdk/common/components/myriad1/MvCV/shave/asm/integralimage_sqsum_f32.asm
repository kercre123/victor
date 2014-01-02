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
LSU0.ST32  i24  i19 

LSU1.LD32  i18  i18  
LSU0.LD32  i17  i17
LSU0.LDIL i12 ___mainloop || LSU1.LDIH i12 ___mainloop ;loop 
IAU.SHR.u32 i1  i15  2
IAU.SHL  i1  i1  2
IAU.SUB i2 i2 i2 ;contor for loop
SAU.SUB.f32 s10 s10 s10 ;sum for pixels
IAU.SUB i24 i16 0


LSU0.LD32.u8.u32     i8  i18 || IAU.ADD i18 i18 1      
LSU0.LD32.u8.u32     i9  i18 || IAU.ADD i18 i18 1
LSU0.LD32.u8.u32     i10  i18 || IAU.ADD i18 i18 1
LSU0.LD32.u8.u32     i11  i18 || IAU.ADD i18 i18 1


LSU0.LD32  s4  i16 || IAU.ADD i16 i16 4      
LSU0.LD32  s5  i16 || IAU.ADD i16 i16 4
LSU0.LD32  s6  i16 || IAU.ADD i16 i16 4 || CMU.CPIS.i32.f32  s0 i8    
LSU0.LD32  s7  i16 || IAU.ADD i16 i16 4 || CMU.CPIS.i32.f32  s1 i9    


CMU.CPIS.i32.f32  s2 i10  || SAU.MUL.f32 s0 s0 s0   
CMU.CPIS.i32.f32  s3 i11  || SAU.MUL.f32 s1 s1 s1   
  ___mainloop:
SAU.MUL.f32 s2 s2 s2
SAU.MUL.f32 s3 s3 s3


SAU.ADD.f32 s20 s4 s0
SAU.ADD.f32 s21 s5 s1
SAU.ADD.f32 s22 s6 s2
SAU.ADD.f32 s23 s7 s3



SAU.ADD.f32 s10 s10 s20 || LSU0.LD32.u8.u32     i8  i18 || IAU.ADD i18 i18 1  
LSU0.LD32.u8.u32     i9  i18 || IAU.ADD i18 i18 1 
LSU0.LD32.u8.u32     i10  i18 || IAU.ADD i18 i18 1
LSU0.ST32  s10  i17 || IAU.ADD i17 i17 4 || LSU1.LD32.u8.u32     i11  i18 || SAU.ADD.i32 i18 i18 1
SAU.ADD.f32 s10 s10 s21 || LSU0.ST32  s20  i24 || IAU.ADD i24 i24 4    
LSU0.LD32  s4  i16 || IAU.ADD i16 i16 4      
LSU0.LD32  s5  i16 || IAU.ADD i16 i16 4 || SAU.ADD.i32 i2 i2 4
LSU0.ST32  s10  i17 || IAU.ADD i17 i17 4 || LSU1.LD32  s6  i16 || SAU.ADD.i32 i16 i16 4 || CMU.CPIS.i32.f32  s0 i8 
SAU.ADD.f32 s10 s10 s22 || LSU0.ST32  s21  i24 || IAU.ADD i24 i24 4  || CMU.CMII.i32 i2 i1 
LSU0.LD32  s7  i16 || IAU.ADD i16 i16 4 || CMU.CPIS.i32.f32  s1 i9    
CMU.CPIS.i32.f32  s2 i10  || SAU.MUL.f32 s0 s0 s0  
PEU.PC1C LT || BRU.JMP i12
LSU0.ST32  s10  i17 || IAU.ADD i17 i17 4 || CMU.CPIS.i32.f32  s3 i11  || SAU.MUL.f32 s1 s1 s1   
SAU.ADD.f32 s10 s10 s23 || LSU0.ST32  s22  i24 || IAU.ADD i24 i24 4    
nop 2  
LSU0.ST32  s10  i17 || IAU.ADD i17 i17 4 || LSU1.ST32  s23  i24 || SAU.ADD.i32 i24 i24 4




LSU0.LD32  s4  i16 || IAU.ADD i16 i16 4      
LSU0.LD32  s5  i16 || IAU.ADD i16 i16 4
LSU0.LD32  s6  i16 || IAU.ADD i16 i16 4


LSU0.LD32.u8.u32     i8  i18 || IAU.ADD i18 i18 1      
LSU0.LD32.u8.u32     i9  i18 || IAU.ADD i18 i18 1
LSU0.LD32.u8.u32     i10  i18 || IAU.ADD i18 i18 1


nop 3
CMU.CPIS.i32.f32  s0 i8     
CMU.CPIS.i32.f32  s1 i9     
CMU.CPIS.i32.f32  s2 i10 || sAU.ADD.f32 s20 s4 s0




sAU.ADD.f32 s21 s5 s1
sAU.ADD.f32 s22 s6 s2 || CMU.CMII.i32 i1 i15 


sAU.ADD.f32 s10 s10 s20
nop 2  
PEU.PC1C LT || LSU0.ST32  s10  i17 || IAU.ADD i17 i17 4 || LSU1.ST32  s20  i24 || SAU.ADD.i32 i24 i24 4
sAU.ADD.f32 s10 s10 s21  

IAU.ADD i1 i1 1
CMU.CMII.i32 i1 i15 
PEU.PC1C LT || LSU0.ST32  s10  i17 || IAU.ADD i17 i17 4 || LSU1.ST32  s21  i24 || SAU.ADD.i32 i24 i24 4
sAU.ADD.f32 s10 s10 s22

IAU.ADD i1 i1 1
CMU.CMII.i32 i1 i15     
PEU.PC1C LT || LSU0.ST32  s10  i17 || IAU.ADD i17 i17 4 || LSU1.ST32  s22  i24 || SAU.ADD.i32 i24 i24 4



LSU0.LD32  i24  i19 || IAU.ADD i19 i19 4
nop 5


BRU.jmp i30
nop 5

.end
 

