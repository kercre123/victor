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

.set StackPtr	 	i19

.ifndef SVU_COMMON_MACROS
.set SVU_COMMON_MACROS

;stack manipulation macros

.macro PUSH_V_REG VREG
lsu0.sto64.h \VREG,StackPtr,-8 || lsu1.sto64.l \VREG,StackPtr,-16 || iau.incs StackPtr,-16
.endm

.macro POP_V_REG VREG 
lsu0.ldo64.l \VREG,StackPtr,0 || lsu1.ldo64.h \VREG, StackPtr, 8 || iau.incs StackPtr,16
.endm

.macro PUSH_2_32BIT_REG REG1, REG2
lsu0.sto32 \REG1,StackPtr,-4 || lsu1.sto32 \REG2,StackPtr,-8 || iau.incs StackPtr,-8
.endm

.macro POP_2_32BIT_REG REG1, REG2
lsu0.ldo32 \REG2,StackPtr,0 || lsu1.ldo32 \REG1,StackPtr,4 || iau.incs StackPtr,8
.endm

.macro PUSH_1_32BIT_REG REG
lsu0.sto32 \REG,StackPtr,-4 || iau.incs StackPtr,-4
.endm

.macro POP_1_32BIT_REG REG
lsu0.ldo32 \REG,StackPtr,0 || iau.incs StackPtr,4
.endm

.endif


.data .data.dilate
.code .text.dilate

;  void Dilate5x5(u8** src, u8** dst, u8** kernel, u32 width, u32 height)
;                   i18       i17        i16        i15         i14
Dilate5x5_asm:

PUSH_V_REG v19
PUSH_V_REG v20
PUSH_V_REG v21

PUSH_V_REG v22
PUSH_V_REG v23

PUSH_V_REG v24
PUSH_V_REG v25

PUSH_V_REG v26
PUSH_V_REG v27

PUSH_V_REG v28
PUSH_V_REG v29

PUSH_V_REG v30
PUSH_V_REG v31

;-----------------------------------------
;      Labels
;-----------------------------------------
LSU0.LDIL i7 ___Store_Values || LSU1.LDIH i7 ___Store_Values

; Set the src addreses lines || Set some counters
LSU0.LDO32  i1  i18 0  || VAU.SUB.i8  v22 v22 v22 || LSU1.LDIL i4 48 
VAU.SUB.i8  v19 v19 v19
VAU.SUB.i8  v20 v20 v20
VAU.SUB.i8  v21 v21 v21
VAU.SUB.i8  v22 v22 v22
VAU.SUB.i8  v23 v23 v23

SAU.SHR.U8 i0 i3 2
 
;If width < 48 then
CMU.CMII.u32 i15 i4
PEU.PC1C LT || CMU.CPII i15 i4 

LSU0.LD32  i17 i17   || CMU.CPII i5 i15    || SAU.DIV.u32 i3  i15  i4

;i8: k repeat for 5 line processing
LSU0.LDIL i8 5   || LSU1.LDIL i2 0  || SAU.SHR.U8 i0 i4 2
VAU.SUB.i8  v24 v24 v24
VAU.SUB.i8  v25 v25 v25
VAU.SUB.i8  v26 v26 v26
VAU.SUB.i8  v27 v27 v27
VAU.SUB.i8  v28 v28 v28
VAU.SUB.i8  v29 v29 v29
VAU.SUB.i8  v30 v30 v30
VAU.SUB.i8  v31 v31 v31
NOP 2

CMU.CPII i9 i16  

; Set the kernel addreses lines
CMU.CPII i10 i18 || LSU0.LDO32 i6 i16 0 
; Add the relevant displacemet

; Main Loop 
BRU.RPL i7 i3


___Load_Source_Values:    
; Load source values 
LSU0.LDO64.l v1  i1  0 || LSU1.LDO64.h v1 i1 8 || IAU.ADD i1 i1 i0
LSU0.LDO64.l v2  i1  0 || LSU1.LDO64.h v2 i1 8 || IAU.ADD i1 i1 i0
LSU0.LDO64.l v3  i1  0 || LSU1.LDO64.h v3 i1 8 || IAU.ADD i1 i1 i0
LSU0.LDO64.l v4  i1  0 || LSU1.LDO64.h v4 i1 8 || IAU.ADD i1 i1 i0
    NOP


___Load_Kernel_Values:
; Load first kernel line values || Rotate source values
LSU0.LDO8 i11 i6 0 
LSU0.LDO8 i12 i6 1 || CMU.VROT v11 v1 1 
LSU0.LDO8 i13 i6 2 || CMU.VROT v12 v1 2 || IAU.ADD    i18 i18 4
LSU0.LDO8 i14 i6 3 || CMU.VROT v13 v1 3 || LSU1.LDO32 i1  i18 0
LSU0.LDO8 i15 i6 4 || CMU.VROT v14 v1 4

NOP 

; Create vectors using the first kernel line 
CMU.CPIVR.x8 v6  i11 
CMU.CPIVR.x8 v7  i12 
CMU.CPIVR.x8 v8  i13
CMU.CPIVR.x8 v9  i14
CMU.CPIVR.x8 v10 i15 || IAU.ADD i1 i1 i2

; First line: multiply || Rotate source values
VAU.MUL.i8 v15 v6  v1  
VAU.MUL.i8 v16 v7  v11 
VAU.MUL.i8 v17 v8  v12
VAU.MUL.i8 v18 v9  v13
VAU.MUL.i8 v19 v10 v14 || CMU.VROT v11 v2 1

; FIRST LINE MAX
CMU.MAX.u8 v20 v15 v16
CMU.MAX.u8 v21 v17 v18
NOP
CMU.VROT v12 v2 2
CMU.VROT v13 v2 3

; Second line: multiply || Rotate source values
VAU.MUL.i8 v15 v6  v2  || CMU.VROT   v14 v2  4
VAU.MUL.i8 v16 v7  v11 || CMU.MAX.u8 v20 v20 v19  || IAU.ADD i16 i16 4
VAU.MUL.i8 v17 v8  v12
VAU.MUL.i8 v18 v9  v13
VAU.MUL.i8 v19 v10 v14 || CMU.VROT v11 v3 1

; Third line
CMU.VROT v12 v3 2
CMU.VROT v13 v3 3
CMU.VROT v14 v3 4

; SECOND LINE MAX
CMU.MAX.u8 v23 v15 v16
CMU.MAX.u8 v24 v17 v18

; Third line: multiply || Rotate source values
VAU.MUL.i8 v15 v6  v3  
VAU.MUL.i8 v16 v7  v11 || CMU.MAX.u8 v23 v23 v19 
VAU.MUL.i8 v17 v8  v12
VAU.MUL.i8 v18 v9  v13
VAU.MUL.i8 v19 v10 v14 || CMU.VROT v11 v4 1

; 4th line
CMU.VROT v12 v4 2
CMU.VROT v13 v4 3
CMU.VROT v14 v4 4

; Third LINE MAX
CMU.MAX.u8 v26 v15 v16
CMU.MAX.u8 v27 v17 v18

; 4th line: multiply || Rotate source values
VAU.MUL.i8 v15 v6  v4 
VAU.MUL.i8 v16 v7  v11 || CMU.MAX.u8 v26 v26 v19 
VAU.MUL.i8 v17 v8  v12
VAU.MUL.i8 v18 v9  v13
VAU.MUL.i8 v19 v10 v14

; 4th LINE MAX
CMU.MAX.u8 v29 v15 v16
CMU.MAX.u8 v30 v17 v18
CMU.MAX.u8 v24 v24 v23
CMU.MAX.u8 v29 v29 v19
NOP  
CMU.MAX.u8 v27 v27 v26
CMU.MAX.u8 v30 v30 v29

IAU.SUB i8 i8 1         || CMU.MAX.u8 v21 v21 v20
PEU.PC1I GT             || BRU.BRA ___Load_Source_Values
NOP
LSU0.LDO32 i6 i16 0     || CMU.MAX.u8 v31 v31 v30 
CMU.MAX.u8 v25 v25 v24  
CMU.MAX.u8 v28 v28 v27 
CMU.MAX.u8 v22 v22 v21 

CMU.CPII   i18 i10 
LSU0.LDO32 i1  i18 0    || CMU.CPII i16 i9        ||  IAU.ADD i2 i2 i4
NOP
___Store_Values:
LSU0.LDIL   i8 5        || LSU1.LDO32 i6 i16 0    ||  IAU.SUB i5 i5 i4
LSU0.ST64.l v22 i17     || LSU1.STO64.h v22 i17 8 ||  IAU.ADD i17 i17 i0   
LSU0.ST64.l v25 i17     || LSU1.STO64.h v25 i17 8 ||  IAU.ADD i17 i17 i0 || VAU.SUB.i8 v22 v22 v22 
LSU0.ST64.l v28 i17     || LSU1.STO64.h v28 i17 8 ||  IAU.ADD i17 i17 i0 || VAU.SUB.i8 v25 v25 v25
LSU0.ST64.l v31 i17     || LSU1.STO64.h v31 i17 8 ||  IAU.ADD i17 i17 i0 || VAU.SUB.i8 v28 v28 v28
VAU.SUB.i8  v31 v31 v31 || IAU.ADD i1 i1 i2

; Compensate the remaining pixels
___Compensate_Last_48_Bits:
CMU.CMZ.i8 i5 
PEU.PC1C GT      || CMU.CPII  i4 i5      || BRU.BRA ___Load_Source_Values
IAU.SUB i5 i5 i5 || SAU.SHR.U8 i0 i4 2
NOP 4

POP_V_REG v31
POP_V_REG v30
POP_V_REG v29
POP_V_REG v28
POP_V_REG v27
POP_V_REG v26
POP_V_REG v25
POP_V_REG v24
POP_V_REG v23
POP_V_REG v22
POP_V_REG v21
POP_V_REG v20
POP_V_REG v19


BRU.JMP i30
NOP 5

.end
 

