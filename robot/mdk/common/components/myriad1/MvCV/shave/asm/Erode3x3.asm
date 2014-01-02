; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief
; ///

.version 00.51.04

; Local defines
; Function arguments
.set srcImage1 i18
.set destImage i17
.set kernel i16
.set widthImage i15
.set heightImage i14
.set k i13

; Others registers

.set zero i11

.ifndef SVU_COMMON_MACROS_
.set SVU_COMMON_MACROS_

;stack manipulation macros

.macro PUSH_V_REG VREG
lsu0.sto64.h \VREG,i19,-8 || lsu1.sto64.l \VREG,i19,-16 || iau.incs i19,-16
.endm

.macro POP_V_REG VREG
lsu0.ldo64.l \VREG,i19,0 || lsu1.ldo64.h \VREG, i19, 8 || iau.incs i19,16
.endm

.macro PUSH_1_32BIT_REG REG
lsu0.sto32 \REG,i19,-4 || iau.incs i19,-4
.endm

.macro POP_1_32BIT_REG REG
lsu0.ldo32 \REG,i19,0 || iau.incs i19,4
.endm

.endif


.data .data.dilate
.code .text.dilate

;  void erode3x3(u8** src, u8** dst, u8** kernel, u32 width, u32 height)
;                   i18       i17        i16        i15         i14
Erode3x3_asm:


PUSH_V_REG v24
PUSH_V_REG v25
PUSH_V_REG v26

PUSH_V_REG v27
PUSH_V_REG v28
PUSH_V_REG v29
PUSH_1_32BIT_REG i24
PUSH_1_32BIT_REG i25
PUSH_1_32BIT_REG i26


LSU0.LDIL i14 16
LSU0.LDIL i13 -1 ; this is used to change in the masc the 0 values with -1 and not taking into account for min value

; Set the src address lines
LSU0.LDO32 i4 i16 0 || LSU1.LDO32 i1 i18 0
LSU0.LDO32 i5 i16 4 || LSU1.LDO32 i2 i18 4
LSU0.LDO32 i6 i16 8 || LSU1.LDO32 i3 i18 8
LSU0.LD32  i17 i17
NOP 3

; Load first kernel line || ; Load the second kernel line
LSU0.LDO8 i7 i4 0   || LSU1.LDO8 i10 i5 0
LSU0.LDO8 i8 i4 1   || LSU1.LDO8 i11 i5 1
LSU0.LDO8 i9 i4 2   || LSU1.LDO8 i12 i5 2


; Load source values
LSU0.LDO64.l v1  i1 0  || LSU1.LDO64.h v1 i1 8 || IAU.ADD i1 i1 i14
LSU0.LDO64.l v24 i1 0  

LSU0.LDO64.l v2  i2 0  || LSU1.LDO64.h v2 i2 8 || IAU.ADD i2 i2 i14
LSU0.LDO64.l v25 i2 0  

LSU0.LDO64.l v3  i3 0  || LSU1.LDO64.h v3 i3 8 || IAU.ADD i3 i3 i14
LSU0.LDO64.l v26 i3 0  

; Create vectors using the first kernel line
CMU.CPIVR.x8 v4 i7
CMU.CPIVR.x8 v5 i8
CMU.CPIVR.x8 v6 i9   || VAU.ALIGNVEC v14 v1 v24  1

;Create vectors using the second kernel line
CMU.CPIVR.x8 v27 i10 || VAU.ALIGNVEC v15 v1 v24  2
CMU.CPIVR.x8 v28 i11 || VAU.ALIGNVEC v17 v2 v25  1
CMU.CPIVR.x8 v29 i12 || VAU.ALIGNVEC v18 v2 v25  2

LSU0.LDO8 i7 i6 0   
LSU0.LDO8 i8 i6 1   
LSU0.LDO8 i9 i6 2

NOP 3

; Create vectors using the third kernel line
CMU.CPIVR.x8 v10 i7 || VAU.ALIGNVEC v18 v2 v25  2
CMU.CPIVR.x8 v11 i8 || VAU.ALIGNVEC v20 v3 v26  1
CMU.CPIVR.x8 v12 i9 || VAU.ALIGNVEC v21 v3 v26  2 || LSU0.LDIL i16 0

;If width < 16 then
CMU.CMII.u32 i15 i14
PEU.PC1C LT || CMU.CPII i14 i15 || BRU.BRA ___Compensate_Last_8_Bits
NOP 5

SAU.SHR.u32 i0 i15 4
LSU0.LDIL i4 ___Process  || LSU1.LDIH i4 ___Process
NOP 2 

; Main Loop
BRU.RPL i4 i0

___Start_Computing:

;

; FIRST LINE
; Multiply the first source input values with the kernel mask
VAU.MUL.i8 v13 v1  v4 
VAU.MUL.i8 v14 v14 v5
VAU.MUL.i8 v15 v15 v6
CMU.CMZ.i8 v4
; If v4 is made of 0 values we have to change just to make sure that we do not take it into account in computing the min val
PEU.PC1C EQ || CMU.CPIVR.x8 v13 i13

CMU.CMZ.i8 v5 
; If v5 is made of 0 values we have to change it 
PEU.PC1C EQ || CMU.CPIVR.x8 v14 i13 

CMU.CMZ.i8 v6
; If v6 is made of 0 values we have to change it 
PEU.PC1C EQ || CMU.CPIVR.x8 v15 i13


; SECOND LINE
; Multiply the second source input values with the kernel mask || Add the necessary shift(line1)
VAU.MUL.i8 v16  v2 v27  || LSU1.LD64.l  v1  i1
VAU.MUL.i8 v17 v17 v28  || LSU1.LDO64.h v1  i1 8 || IAU.ADD i1 i1 i14
VAU.MUL.i8 v18 v18 v29  || LSU1.LDO64.l v24 i1 0
CMU.CMZ.i8 v27 
; If v7 is made of 0 values we have to change it
PEU.PC1C EQ || CMU.CPIVR.x8 v16 i13


CMU.CMZ.i8 v28 
; If v8 is made of 0 values we have to change it 
PEU.PC1C EQ || CMU.CPIVR.x8 v17 i13

CMU.CMZ.i8 v29 
; If v9 is made of 0 values we have to change it 
PEU.PC1C EQ || CMU.CPIVR.x8 v18 i13


; THIRD LINE
; Multiply the third source input values with the kernel mask || Add the necessary shift(line2)
VAU.MUL.i8 v19 v3  v10 || LSU1.LD64.l  v2   i2
VAU.MUL.i8 v20 v20 v11 || LSU1.LDO64.h v2  i2 8 || IAU.ADD i2 i2 i14
VAU.MUL.i8 v21 v21 v12 || LSU1.LDO64.l v25 i2 0

CMU.CMZ.i8 v10 
; If v10 is made of 0 values we have to change it 
PEU.PC1C EQ || CMU.CPIVR.x8 v19 i13


; If v11 is made of 0 values we have to change it 
CMU.CMZ.i8 v11 
PEU.PC1C EQ || CMU.CPIVR.x8 v20 i13


CMU.CMZ.i8 v12 
; If v12 is made of 0 values we have to change it 
PEU.PC1C EQ || CMU.CPIVR.x8 v21 i13


; Compute the MINimum value between the first two values on each line
CMU.MIN.u8 v13 v13 v14  || LSU0.LD64.l v3 i3 || LSU1.LDO64.h v3 i3 8 || IAU.ADD i3 i3 i14
CMU.MIN.u8 v16 v16 v17  || LSU0.LDO64.l v26 i3 0
CMU.MIN.u8 v19 v19 v20

; Compute the MINimum on each line
CMU.MIN.u8 v13 v13 v15 || VAU.ALIGNVEC v14 v1 v24  1

;Compute the MINimum value on a kernel
CMU.MIN.u8 v16 v16 v18 || VAU.ALIGNVEC v15 v1 v24  2

 ___Process:
CMU.MIN.u8 v19 v19 v21
CMU.MIN.u8 v13 v13 v16 ||  VAU.ALIGNVEC v17  v2    v25  1 
VAU.ALIGNVEC v20 v3 v26  1
CMU.MIN.u8 v13 v13 v19 || VAU.ALIGNVEC v18  v2     v25  2
VAU.ALIGNVEC v21 v3 v26  2
; Place the result in the destination file
LSU0.ST64.l v13 i17    || LSU1.STO64.h v13  i17 8         || IAU.ADD i17 i17 i14

SAU.SHL.u32 i0  i0 4
NOP 2
IAU.SUB i15 i15 i0
PEU.PC1C EQ || BRU.JMP i30
NOP 5


___Compensate_Last_8_Bits:

; FIRST LINE
; Multiply the first source input values with the kernel mask
VAU.MUL.i8 v13 v1  v4 
VAU.MUL.i8 v14 v14 v5
VAU.MUL.i8 v15 v15 v6

CMU.CMZ.i8 v4
; If v4 is made of 0 values we have to change just to make sure that we do not take it into account in computing the min val
PEU.PC1C EQ || CMU.CPIVR.x8 v13 i13

CMU.CMZ.i8 v5 
; If v5 is made of 0 values we have to change it 
PEU.PC1C EQ || CMU.CPIVR.x8 v14 i13 

CMU.CMZ.i8 v6
; If v6 is made of 0 values we have to change it 
PEU.PC1C EQ || CMU.CPIVR.x8 v15 i13

; SECOND LINE
; Multiply the second source input values with the kernel mask || Add the necessary shift(line1)
VAU.MUL.i8 v16  v2 v27 
VAU.MUL.i8 v17 v17 v28  
VAU.MUL.i8 v18 v18 v29 

CMU.CMZ.i8 v27 
; If v7 is made of 0 values we have to change it
PEU.PC1C EQ || CMU.CPIVR.x8 v16 i13


CMU.CMZ.i8 v28 
; If v8 is made of 0 values we have to change it 
PEU.PC1C EQ || CMU.CPIVR.x8 v17 i13

CMU.CMZ.i8 v29 
; If v9 is made of 0 values we have to change it 
PEU.PC1C EQ || CMU.CPIVR.x8 v18 i13

; THIRD LINE
; Multiply the third source input values with the kernel mask || Add the necessary shift(line2)
VAU.MUL.i8 v19 v3  v10 
VAU.MUL.i8 v20 v20 v11 
VAU.MUL.i8 v21 v21 v12 

CMU.CMZ.i8 v10 
; If v10 is made of 0 values we have to change it 
PEU.PC1C EQ || CMU.CPIVR.x8 v19 i13


; If v11 is made of 0 values we have to change it 
CMU.CMZ.i8 v11 
PEU.PC1C EQ || CMU.CPIVR.x8 v20 i13


CMU.CMZ.i8 v12 
; If v12 is made of 0 values we have to change it 
PEU.PC1C EQ || CMU.CPIVR.x8 v21 i13

; Compute the MINimum value between the first two values on each line
CMU.MIN.u8 v13 v13 v14  
CMU.MIN.u8 v16 v16 v17  
CMU.MIN.u8 v19 v19 v20

; Compute the MINimum on each line
CMU.MIN.u8 v13 v13 v15 


___Process_Last_8_:
;Compute the MINimum value on a kernel
CMU.MIN.u8 v16 v16 v18 
CMU.MIN.u8 v19 v19 v21
CMU.MIN.u8 v13 v13 v16 
NOP
CMU.MIN.u8 v13 v13 v19 
NOP
; Place the result in the destination file
LSU0.ST64.l v13 i17 
   
   
   
POP_1_32BIT_REG i26
POP_1_32BIT_REG i25
POP_1_32BIT_REG i24

POP_V_REG v29
POP_V_REG v28
POP_V_REG v27

POP_V_REG v26
POP_V_REG v25
POP_V_REG v24


BRU.JMP i30
NOP 5

.end


