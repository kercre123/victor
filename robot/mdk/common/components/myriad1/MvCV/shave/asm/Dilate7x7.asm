; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief
; ///

.version 00.51.04
.include svuCommonDefinitions.incl
.include svuCommonMacros.incl


 
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


.ifndef SVU_COMMON_MACROS
.set SVU_COMMON_MACROS

;stack manipulation macros

.macro PUSH_V_REG VREG
lsu0.sto64.h \VREG,i19,-8 || lsu1.sto64.l \VREG,i19,-16 || iau.incs i19,-16
.endm

.macro POP_V_REG VREG 
lsu0.ldo64.l \VREG,i19,0  || lsu1.ldo64.h \VREG, i19, 8 || iau.incs i19,16
.endm

.macro PUSH_2_32BIT_REG REG1, REG2
lsu0.sto32 \REG1,i19,-4   || lsu1.sto32 \REG2,i19,-8    || iau.incs i19,-8
.endm

.macro POP_2_32BIT_REG REG1, REG2
lsu0.ldo32 \REG2,i19,0    || lsu1.ldo32 \REG1,i19,4     || iau.incs i19,8
.endm

.macro PUSH_1_32BIT_REG REG
lsu0.sto32 \REG,i19,-4    || iau.incs i19,-4
.endm

.macro POP_1_32BIT_REG REG
lsu0.ldo32 \REG,i19,0     || iau.incs i19,4
.endm

.endif



.data .data.dilate
.code .text.dilate
;  void Dilate7x7(u8** src, u8** dst, u8** kernel, u32 width, u32 height)
;                   i18       i17        i16        i15         i14
Dilate7x7_asm:



PUSH_V_REG v24
PUSH_V_REG v25
PUSH_V_REG v26
PUSH_V_REG v27

PUSH_V_REG v28
PUSH_V_REG v29
PUSH_V_REG v30
PUSH_V_REG v31

PUSH_1_32BIT_REG i21
PUSH_1_32BIT_REG i24
PUSH_1_32BIT_REG i23
PUSH_1_32BIT_REG i22

;-----------------------------------------
;      Labels
;-----------------------------------------

LSU0.LDO32 i1  i18 0    || LSU1.LDIL i0   40      || CMU.CPII i22 i15         || VAU.SUB.i8  v19 v19 v19



;If width < 40 then
CMU.CMII.u32 i15 i0
PEU.PC1C LT || CMU.CPII i0 i15



LSU0.LD32  i17 i17      || LSU1.LDO32 i8  i16 0   || SAU.DIV.u32 i3 i15 i0    || CMU.CPII i6 i16 || VAU.SUB.i8  v20 v20 v20
VAU.SUB.i8  v24 v24 v24
VAU.SUB.i8  v25 v25 v25
VAU.SUB.i8  v26 v26 v26
VAU.SUB.i8  v27 v27 v27
VAU.SUB.i8  v28 v28 v28
VAU.SUB.i8  v29 v29 v29
VAU.SUB.i8  v30 v30 v30
VAU.SUB.i8  v31 v31 v31
CMU.CPII   i5 i18       || LSU0.LDIL i7 0         || LSU1.LDIL i21  3             || VAU.SUB.i8  v21 v21 v21
LSU0.LDIL i4 ___Store_Values                      || LSU1.LDIH i4 ___Store_Values || IAU.SHR.u32 i23 i0 2     || VAU.SUB.i8  v22 v22 v22
LSU0.LDIL i2 7          || VAU.SUB.i8 v27 v27 v27 
VAU.SUB.i8 v30 v30 v30  || IAU.SUB i22 i22 6      
VAU.SUB.i8 v26 v26 v26  || IAU.AND i24 i0 i21
IAU.ADD i24 i23 i24     || VAU.SUB.i8  v23 v23 v23

BRU.RPL i4 i3


___Load_Source_Values:

; Process first 10 values

IAU.ADD      i1  i1  i7
LSU0.LDO64.l v1  i1  0   || LSU1.LDO64.h  v1  i1 8 || IAU.ADD i1 i1 i24
LSU0.LDO64.l v0  i1  0   || LSU1.LDO64.h  v0  i1 8 || IAU.ADD i1 i1 i23
LSU0.LDO64.l v25 i1  0   || LSU1.LDO64.h  v25 i1 8 || IAU.ADD i1 i1 i23
LSU0.LDO64.l v29 i1  0   || LSU1.LDO64.h  v29 i1 8 || IAU.ADD i1 i1 i23

; Load kernel values
LSU0.LDO8    i9  i8  0   || LSU1.LDO8 i10 i8 1 
LSU0.LDO8    i11 i8  2   || LSU1.LDO8 i12 i8 3 




LSU0.LDO8    i13 i8  4   || LSU1.LDO8 i14 i8 5     
LSU0.LDO8    i15 i8  6   || CMU.VROT  v2  v1 1 
CMU.VROT     v4  v1 3 
CMU.VROT     v3  v1  2 



; Load source values
CMU.VROT     v5  v1  4 
CMU.VROT     v6  v1  5 
CMU.VROT     v7  v1  6 


; Create kernel vrfs
CMU.CPIVR.x8 v8  i9 
CMU.CPIVR.x8 v9  i10 
CMU.CPIVR.x8 v10 i11 
CMU.CPIVR.x8 v11 i12     || VAU.MUL.i8 v16 v1 v8
CMU.CPIVR.x8 v12 i13     || VAU.MUL.i8 v17 v2 v9
CMU.CPIVR.x8 v13 i14     || VAU.MUL.i8 v18 v3 v10
CMU.CPIVR.x8 v14 i15     || VAU.MUL.i8 v19 v4 v11


; Process the multiplication on the first 10 values
VAU.MUL.i8 v20 v5 v12    || CMU.MAX.u8 v23 v16 v17
VAU.MUL.i8 v21 v6 v13
VAU.MUL.i8 v22 v7 v14


; Find maximum per line
CMU.MAX.u8 v24 v18 v19
CMU.MAX.u8 v31 v20 v21
CMU.MAX.u8 v23 v23 v22
    NOP
CMU.VROT   v2  v0  1 
CMU.MAX.u8 v31 v31 v24
    NOP



CMU.VROT   v3  v0  2 
CMU.MAX.u8 v31 v31 v23
    NOP 
CMU.VROT v4 v0 3 
CMU.VROT v5 v0 4        || VAU.MUL.i8 v16 v0 v8
CMU.MAX.u8 v26 v26 v31  || VAU.MUL.i8 v17 v2 v9
    NOP
    
    
; Load source values
CMU.VROT v6 v0 5        || VAU.MUL.i8 v18 v3 v10
CMU.VROT v7 v0 6      


; Process the multiplication
VAU.MUL.i8 v19 v4 v11   || CMU.MAX.u8 v23 v16 v17
VAU.MUL.i8 v20 v5 v12   
VAU.MUL.i8 v21 v6 v13
VAU.MUL.i8 v22 v7 v14   || CMU.MAX.u8 v24 v18 v19


; Find maximum per line
    NOP
CMU.VROT   v2  v25 1 
CMU.MAX.u8 v31 v20 v21
CMU.MAX.u8 v23 v23 v22
    NOP
CMU.MAX.u8 v31 v31 v24
    NOP


CMU.VROT   v3  v25 2 
CMU.MAX.u8 v31 v31 v23
IAU.ADD    i18 i18 4
LSU0.LDO32 i1  i18 0


; Compute the kernel maximum
CMU.MAX.u8 v27 v27 v31
    NOP


; Load source values
CMU.VROT v4 v25 3 
CMU.VROT v5 v25 4         || VAU.MUL.i8 v16 v25 v8
CMU.VROT v6 v25 5         || VAU.MUL.i8 v17 v2  v9
CMU.VROT v7 v25 6         || VAU.MUL.i8 v18 v3  v10


; Process the multiplication
VAU.MUL.i8 v19 v4 v11
VAU.MUL.i8 v20 v5 v12     || CMU.MAX.u8 v23 v16 v17
VAU.MUL.i8 v21 v6 v13
VAU.MUL.i8 v22 v7 v14

CMU.MAX.u8 v24 v18 v19
CMU.MAX.u8 v31 v20 v21
CMU.MAX.u8 v23 v23 v22
    NOP
CMU.MAX.u8 v31  v31  v24
    NOP
CMU.VROT v2 v29 1 
CMU.VROT v3 v29 2 
CMU.MAX.u8 v31  v31  v23
    NOP 
CMU.VROT v4 v29 3 
CMU.VROT v5 v29 4         || VAU.MUL.i8 v16 v29 v8
CMU.MAX.u8 v28 v28 v31
    NOP


; Load source values
CMU.VROT v6 v29 5         || VAU.MUL.i8 v17 v2 v9
CMU.VROT v7 v29 6         || VAU.MUL.i8 v18 v3 v10

; Process the multiplication
VAU.MUL.i8 v19 v4 v11
VAU.MUL.i8 v20 v5 v12     || CMU.MAX.u8 v23 v16 v17
VAU.MUL.i8 v21 v6 v13
VAU.MUL.i8 v22 v7 v14

CMU.MAX.u8 v24 v18 v19
CMU.MAX.u8 v31 v20 v21
CMU.MAX.u8 v23 v23 v22
    NOP
CMU.MAX.u8 v31  v31  v24
    NOP






IAU.SUB i2 i2 1
PEU.PC1I GT               || BRU.BRA ___Load_Source_Values
   NOP
   CMU.MAX.u8 v31  v31  v23
   IAU.ADD i16 i16 4 
   LSU0.LDO32 i8 i16 0 
   CMU.MAX.u8 v30 v30 v31



LSU0.LDO32   i1  i5    0 
LSU0.STO64.l v26 i17   0  || LSU1.STO64.h v26 i17  8     || IAU.ADD    i17 i17 i24 || CMU.CPII   i18 i5


; Store the kernel maximum
___Store_Values:
LSU0.STO64.l v27 i17   0  || LSU1.STO64.h v27 i17  8     || IAU.ADD    i17 i17 i23
LSU0.STO64.l v28 i17   0  || LSU1.STO64.h v28 i17  8     || IAU.ADD    i17 i17 i23
LSU0.STO64.l v30 i17   0  || LSU1.STO64.h v30 i17  8     || IAU.ADD    i17 i17 i23  || CMU.CPII   i18 i5       || VAU.SUB.i8 v27 v27 v27 
CMU.CPII     i16 i6       || IAU.ADD      i7  i7   i0    || LSU1.LDIL  i2  7        || VAU.SUB.i8 v30 v30 v30  
LSU1.LDO32   i8  i16   0  || IAU.SUB      i22 i22  i0    || VAU.SUB.i8 v28 v28 v28 
VAU.SUB.i8   v26 v26   v26 


; Compensate last values
CMU.CMZ.i8 i22
PEU.PC1I GT               || BRU.BRA ___Load_Source_Values 
CMU.CPII i0 i22 
IAU.SUB i22 i22 i22  
IAU.SHR.u32 i23 i0 2
IAU.AND i24 i0 i21
IAU.ADD i24 i23 i24


POP_1_32BIT_REG i22
POP_1_32BIT_REG i23
POP_1_32BIT_REG i24
POP_1_32BIT_REG i21

POP_V_REG v31
POP_V_REG v30
POP_V_REG v29
POP_V_REG v28
POP_V_REG v27
POP_V_REG v26
POP_V_REG v25
POP_V_REG v24





BRU.JMP i30
NOP 5

.end
