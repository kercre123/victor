; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief: applay box filter, with kernel size 3. There are 2 vesion, normalized output and unnourmalized.
; ///         input values are u8, and output are u8 for normalized version, and u16 for unnormalized version

;--------------------------------------------------------------------------------------------------------------
.version 00.50.05
.data .rodata.vect_mask 
.salign 16
___vect_mask:
		.float16    0.1111
___vect_mask2:
		.short    1


.code .text.boxfilter3x3 ;text
.salign 16
;/// boxfilter kernel that makes average on 3x3 kernel size       
;/// @param[in] in        - array of pointers to input lines      
;/// @param[out] out      - array of pointers for output lines    
;/// @param[in] normalize - parameter to check if we want to do a normalize boxfilter or not 1 for normalized values , 0 in the other case
;/// @param[in] width     - width of input line
;void boxfilter3x3(u8** in, u8** out, u32 normalize, u32 width)
;{
;    int i,x,y;
;    u8* lines[5];
;    unsigned int sum;
;    unsigned short* aux;
;    aux=(unsigned short *)(*out);
;
;    //Initialize lines pointers
;    lines[0] = *in;
;    lines[1] = *(in+1);
;    lines[2] = *(in+2);
;
;    //Go on the whole line
;    for (i = 0; i < width; i++)
;    {
;        sum = 0;
;        for (y = 0; y < 3; y++)
;        {
;            for (x = -1; x <= 1; x++)
;            {
;                sum += (lines[y][x]);
;            }
;            lines[y]++;
;        }
;
;        if(normalize == 1)
;        {
;            *(*out+i)=(u8)(((half)(float)sum)*(half)0.1111);
;        }
;        else
;        {
;            *(aux+i)=(unsigned short)sum;
;        }
;    }
;    return;
;}
boxfilter3x3_asm:
; before loop, prepare the loop values
LSU0.LDo.32  i0  i18 0x00 ||	LSU1.LDo.32  i1  i18 0x04 
LSU0.LDo.32  i2  i18 0x08 ||	LSU1.LD.32  i17  i17 ||IAU.SHR.u32 i3 i15 4
lsu0.ldil i7, ___vect_mask     ||  lsu1.ldih i7, ___vect_mask
LSU0.ldil i6, ___unormalizedFunction || LSU1.ldih i6, ___unormalizedFunction
lsu0.ld.16r v23, i7	
nop 2
iau.sub i0, i0, 1
iau.sub i1, i1, 1 ||LSU0.LDi.32.u8.u32 i8 i0 	
iau.sub i2, i2, 1	||LSU0.LDi.32.u8.u32 i9 i1
LSU0.LDi.32.u8.u32 i10 i2
LSU0.LDi.128.u8.u16 v0 i0	
LSU0.LDi.128.u8.u16 v1 i1 	
LSU0.LDi.128.u8.u16 v2 i2	
LSU0.LDi.128.u8.u16 v0 i0 
LSU0.LDi.128.u8.u16 v1 i1 	
iau.add i7, i8, i9	||LSU0.LDi.128.u8.u16 v2 i2 
iau.add i7, i7, i10	||LSU0.LDi.128.u8.u16 v3 i0 	
LSU0.LDi.128.u8.u16 v4 i1 	
VAU.ADD.i16  v6, v0, v1 
LSU0.LDi.128.u8.u16 v5 i2 
VAU.ADD.i16  v6, v6, v2
IAU.SHL i4 i3 4 	
iau.sub i4, i15, i4
VAU.ADD.i16  v7, v0, v1 ||cmu.cpvv v9, v6 		||LSU0.LDi.128.u8.u16 v0 i0 
VAU.ADD.i16  v8, v3, v4 ||CMU.SHLIV.x16 v9, v9, i7  ||LSU0.LDi.128.u8.u16 v1 i1 
VAU.ADD.i16  v7, v7, v2							||LSU0.LDi.128.u8.u16 v2 i2 
VAU.ADD.i16  v8, v8, v5							||LSU0.LDi.128.u8.u16 v3 i0 
												LSU0.LDi.128.u8.u16 v4 i1 
cmu.alignvec v10, v6, v7, 2						||LSU0.LDi.128.u8.u16 v5 i2 
cmu.alignvec v11, v6, v7, 14
CMU.CMZ.i32 i16 ||lsu0.ldil i5, ___loop0     ||  lsu1.ldih i5, ___loop0	
PEU.PC1C EQ || BRU.JMP i6	||lsu0.ldil i5, ___loop1     ||  lsu1.ldih i5, ___loop1
cmu.alignvec v12, v7, v8, 2
vau.add.i16 v13, v6, v9
vau.add.i16 v14, v7, v11   ||cmu.cpvv v6, v8
vau.add.i16 v13, v13, v10 
vau.add.i16 v14, v14, v12  ||cmu.cpvi.x16 i7.l, v7.7 
nop
___normalizedFunction:
;// start loop  
bru.rpl i5 i3  		      ||VAU.ADD.i16  v7, v0, v1      ||cmu.cpvv v9, v6             ||LSU0.LDi.128.u8.u16 v0 i0 
VAU.ADD.i16  v8, v3, v4   ||CMU.SHLIV.x16 v9, v9,i7      ||LSU0.LDi.128.u8.u16 v1 i1 
VAU.ADD.i16  v7, v7, v2   ||LSU0.LDi.128.u8.u16 v2 i2    ||cmu.cpvv.i16.f16 v15, v13
VAU.ADD.i16  v8, v8, v5   ||LSU0.LDi.128.u8.u16 v3 i0    ||cmu.cpvv.i16.f16 v16, v14
___loop0:    
vau.mul.f16 v15, v15, v23 ||LSU0.LDi.128.u8.u16 v4 i1 
vau.mul.f16 v16, v16, v23 ||cmu.alignvec v10, v6, v7, 2  ||LSU0.LDi.128.u8.u16 v5 i2 
cmu.cpvi.x16 i7.l, v7.7
vau.add.i16 v13, v6, v9   ||LSU0.STi.128.f16.u8 v15 i17  ||cmu.alignvec v11, v6, v7, 14
vau.add.i16 v14, v7, v11  ||LSU0.STi.128.f16.u8 v16 i17  ||cmu.alignvec v12, v7, v8, 2
vau.add.i16 v13, v13, v10 ||cmu.cpvv v6, v8    
vau.add.i16 v14, v14, v12 
;// end loop
; post loop sector
cmu.cpvv.i16.f16 v15, v13
cmu.cpvv.i16.f16 v16, v14
bru.jmp i30  ||vau.mul.f16 v15, v15, v23
vau.mul.f16 v16, v16, v23
CMU.CMZ.i32 i4 
PEU.PC1C GT || LSU0.STi.128.f16.u8 v15 i17 
iau.sub i4, i4, 0x08 
CMU.CMZ.i32 i4
PEU.PC1C GT || LSU0.STi.128.f16.u8 v16 i17

___unormalizedFunction:
;// start loop  
bru.rpl i5 i3                  ||VAU.ADD.i16  v7, v0, v1   ||cmu.cpvv v9, v6                ||LSU0.LDi.128.u8.u16 v0 i0 
VAU.ADD.i16  v8, v3, v4        ||CMU.SHLIV.x16 v9, v9, i7  ||LSU0.LDi.128.u8.u16 v1 i1 
___loop1:
VAU.ADD.i16  v7, v7, v2        ||LSU0.LDi.128.u8.u16 v2 i2  ||cmu.cpvv v15, v13
VAU.ADD.i16  v8, v8, v5        ||LSU0.LDi.128.u8.u16 v3 i0  ||cmu.cpvv v16, v14             ||LSU1.LDi.128.u8.u16 v4 i1 
cmu.alignvec v10, v6, v7, 2    ||LSU0.LDi.128.u8.u16 v5 i2 
vau.add.i16 v13, v6, v9        ||LSU0.STi.64.l v15 i17      ||cmu.alignvec v11, v6, v7, 14
vau.add.i16 v14, v7, v11       ||LSU0.STi.64.h v15 i17      ||cmu.alignvec v12, v7, v8, 2    
vau.add.i16 v13, v13, v10      ||cmu.cpvv v6, v8            ||LSU0.STi.64.l v16 i17
vau.add.i16 v14, v14, v12      ||LSU0.STi.64.h v16 i17      ||cmu.cpvi.x16 i7.l, v7.7
;// end loop
; post loop sector
cmu.cpvv v15, v13 || iau.add i4, i4, 0x03
bru.jmp i30 
cmu.cpvv v16, v14 ||iau.sub i4, i4, 0x04  
PEU.PC1I GTE || iau.sub i4, i4, 0x04 || LSU0.STi.64.l v15 i17 
PEU.PC1I GTE || iau.sub i4, i4, 0x04 || LSU0.STi.64.h v15 i17
PEU.PC1I GTE || iau.sub i4, i4, 0x04 || LSU0.STi.64.l v16 i17
PEU.PC1I GTE || LSU0.STi.64.h v16 i17
    
    
.end
