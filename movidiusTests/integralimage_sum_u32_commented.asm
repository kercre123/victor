; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief
; ///

.version 00.51.04

.data .data.integralimage_sum_u32

.code .text.integralimage_sum_u32

integralimage_sum_u32_asm:

;/// integral image kernel - this kernel makes the sum of all pixels before it and on the left of it's column  ( this particular case makes sum of pixels in u32 format)
;/// @param[in] in         - array of pointers to input lines      
;/// @param[out]out        - array of pointers for output lines    
;/// @param[in] sum        - sum of previous pixels . for this parameter we must have an array of u32 declared as global and having the width of the line
;/// @param[in] width      - width of input line
;void integralimage_sum_u32(u8** in, u8** out, u32* sum, u32 width)
;{
;    int i, x, y;
;    u8* in_line;
;    u32* out_line;
;    u32 s=0;
;    //initialize a line pointer for one of u32 or f32
;    in_line = *in;
;    out_line = (u32 *)(*out);
;    //go on the whole line and sum the pixels
;    for(i=0; i<width; i++)
;    {
;        s = s + in_line[i] + sum[i];
;        out_line[i] = s;
;        sum[i] = sum[i] + in_line[i];
;    }
;    return;
;}

; extern unsigned int integralImageSumCycleCount;
; void integralImageSum_asm_test(unsigned char** in, unsigned char** out, long unsigned int* sum, unsigned int width);

; TODO: figure out what this is doing
; Store the first 20 bytes above the stack pointer in registers i20-i24 ???
;lsu0.st32 irf:Src irf:Base
;rf_data[31:0] = irf(src)[31:0]
IAU.SUB i19 i19 4
LSU0.ST32  i20  i19 || IAU.SUB i19 i19 4
LSU0.ST32  i21  i19 || IAU.SUB i19 i19 4
LSU0.ST32  i22  i19 || IAU.SUB i19 i19 4
LSU0.ST32  i23  i19 || IAU.SUB i19 i19 4
LSU0.ST32  i24  i19 

LSU1.LD32  i18  i18 ; address of first input line into i18
LSU0.LD32  i17  i17 ; address of first output line into i17

LSU0.LDIL i12 ___mainloop || LSU1.LDIH i12 ___mainloop ;loop 

; i1 = 4 * (width/4);
IAU.SHR.u32 i1  i15  2
IAU.SHL  i1  i1  2 

; Is this setting i2 and i3 to zero?
IAU.SUB i2 i2 i2 ;counter for loop
IAU.SUB i3 i3 i3 ;sum for pixels

; Is this setting i24 to i16 (sum)?
IAU.SUB i24 i16 0

; Load the first four sum[i] into i4-i7
LSU0.LD32  i4  i16 || IAU.ADD i16 i16 4      
LSU0.LD32  i5  i16 || IAU.ADD i16 i16 4
LSU0.LD32  i6  i16 || IAU.ADD i16 i16 4
LSU0.LD32  i7  i16 || IAU.ADD i16 i16 4

; Load the first four pixels from image[i] into i8-i11
LSU0.LD32.u8.u32     i8  i18 || IAU.ADD i18 i18 1      
LSU0.LD32.u8.u32     i9  i18 || IAU.ADD i18 i18 1
LSU0.LD32.u8.u32     i10  i18 || IAU.ADD i18 i18 1
LSU0.LD32.u8.u32     i11  i18 || IAU.ADD i18 i18 1

nop 2
IAU.ADD i20 i4 i8 ; i20 = sum[0] + image[0]


___mainloop:

IAU.ADD i2 i2 4 ; Increment loop counter by 4

IAU.ADD i21 i5 i9 || ; i21 = sum[n+1] + image[n+1]
    LSU0.LD32  i4  i16 || ; sum[n] = sum[n+4]
 	SAU.ADD.i32 i16 i16 4 || ; n += 4
	CMU.CMII.i32 i2 i1 ; Have we proccessed all pixels?


IAU.ADD i22 i6 i10 ||  ; i22 = sum[n+2] + image[n+2]
    LSU0.LD32.u8.u32     i8  i18 ||  ; im[0] = im[n+4]
	SAU.ADD.i32 i18 i18 1 ; n++


IAU.ADD i23 i7 i11 || LSU0.LD32  i5  i16 || SAU.ADD.i32 i16 i16 4

IAU.ADD i3 i3 i20 || LSU0.LD32.u8.u32     i9  i18 || SAU.ADD.i32 i18 i18 1
LSU0.ST32  i3  i17 || IAU.ADD i17 i17 4 || LSU1.LD32  i6  i16 || SAU.ADD.i32 i16 i16 4
IAU.ADD i3 i3 i21 || LSU0.LD32.u8.u32     i10  i18 || SAU.ADD.i32 i18 i18 1   
LSU0.ST32  i3  i17 || IAU.ADD i17 i17 4 || LSU1.LD32  i7  i16 || SAU.ADD.i32 i16 i16 4
IAU.ADD i3 i3 i22  || LSU0.LD32.u8.u32     i11  i18 || SAU.ADD.i32 i18 i18 1   
LSU0.ST32  i3  i17 || IAU.ADD i17 i17 4 || LSU1.ST32  i20  i24 || SAU.ADD.i32 i24 i24 4
PEU.PC1C LT || BRU.JMP i12 ; if we haven't processed all pixels, goto ___mainloop
IAU.ADD i3 i3 i23    
LSU0.ST32  i3  i17 || IAU.ADD i17 i17 4 || LSU1.ST32  i21  i24 || SAU.ADD.i32 i24 i24 4
IAU.ADD i20 i4 i8
LSU0.ST32  i22  i24 || IAU.ADD i24 i24 4
LSU0.ST32  i23  i24 || IAU.ADD i24 i24 4



LSU0.LD32  i4  i16 || IAU.ADD i16 i16 4      
LSU0.LD32  i5  i16 || IAU.ADD i16 i16 4
LSU0.LD32  i6  i16 || IAU.ADD i16 i16 4


LSU0.LD32.u8.u32     i8  i18 || IAU.ADD i18 i18 1      
LSU0.LD32.u8.u32     i9  i18 || IAU.ADD i18 i18 1
LSU0.LD32.u8.u32     i10  i18 || IAU.ADD i18 i18 1


nop 5

IAU.ADD i20 i4 i8
IAU.ADD i21 i5 i9
IAU.ADD i22 i6 i10


CMU.CMII.i32 i1 i15 

; ?? Maybe finish up the last few?
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
 

