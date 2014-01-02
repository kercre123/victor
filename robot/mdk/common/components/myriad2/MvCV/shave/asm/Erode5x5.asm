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


.data .data.erode5x5
.code .text.erode5x5


;  void Erode5x5(u8** src, u8** dst, u8** kernel, u32 width, u32 height)
;                   i18       i17        i16        i15         i14     
Erode5x5_asm:
LSU0.STO.32 I20,i19,-4 || iau.incs i19,-4

LSU0.STO.64.H V24,i19,-8 || LSU1.STO.64.L V24,i19,-16 || iau.incs i19,-16
LSU0.STO.64.H V25,i19,-8 || LSU1.STO.64.L V25,i19,-16 || iau.incs i19,-16

LSU0.STO.64.H V26,i19,-8 || LSU1.STO.64.L V26,i19,-16 || iau.incs i19,-16
LSU0.STO.64.H V27,i19,-8 || LSU1.STO.64.L V27,i19,-16 || iau.incs i19,-16

LSU0.STO.64.H V28,i19,-8 || LSU1.STO.64.L V28,i19,-16 || iau.incs i19,-16
LSU0.STO.64.H V29,i19,-8 || LSU1.STO.64.L V29,i19,-16 || iau.incs i19,-16

LSU0.STO.64.H V30,i19,-8 || LSU1.STO.64.L V30,i19,-16 || iau.incs i19,-16
LSU0.STO.64.H V31,i19,-8 || LSU1.STO.64.L V31,i19,-16 || iau.incs i19,-16


;      Labels
;-----------------------------------------
LSU0.LDIL i7 ___Store_Values || LSU1.LDIH i7 ___Store_Values



; Set the src addreses lines || Set some counters
LSU0.LDO.32 i1  i18 0    || VAU.SUB.i8  v22 v22 v22 || LSU1.LDIL i3 48
VAU.SUB.i8  v19 v19 v19
VAU.SUB.i8  v20 v20 v20
VAU.SUB.i8  v21 v21 v21
VAU.SUB.i8  v22 v22 v22
VAU.SUB.i8  v23 v23 v23  || SAU.SHR.U8 i0 i3 2

 
;If width < 48 then
CMU.CMII.u32 i15 i3
PEU.PC1C LT || CMU.CPII i3 i15 

LSU0.LD.32  i17 i17   || CMU.CPII i5 i15    || SAU.DIV.u32 i3  i15  i3

;i8: k repeat for 5 line processing
LSU0.LDIL i8 5   || LSU1.LDIL i2 0

VAU.SUB.i8  v24 v24 v24
VAU.SUB.i8  v25 v25 v25
VAU.SUB.i8  v26 v26 v26
VAU.SUB.i8  v27 v27 v27
VAU.SUB.i8  v28 v28 v28
VAU.SUB.i8  v29 v29 v29
VAU.SUB.i8  v30 v30 v30
VAU.SUB.i8  v31 v31 v31

LSU1.LDIL i20 0xFF
CMU.CPII i9 i16  
cmu.cpii i4 i3 ;LSU1.LDIL  i4  48 

; Set the kernel address lines
CMU.CPII i10 i18 || LSU0.LDO.32 i6 i16 0 
; Add the relevant displacement
CMU.CPIVR.x8 v0  i20

; Main Loop 
; (Myriad2 COMPATIBILITY ISSUE): BRU.RPL: changed order of parameters at line number 125, in Erode5x5.asm
BRU.RPL i7 i3


___Load_Source_Values:
; Load source values 
LSU0.LDO.64.L v1  i1  0 || LSU1.LDO.64.H v1 i1 8 || IAU.ADD i1 i1 i0
LSU0.LDO.64.L v2  i1  0 || LSU1.LDO.64.H v2 i1 8 || IAU.ADD i1 i1 i0
LSU0.LDO.64.L v3  i1  0 || LSU1.LDO.64.H v3 i1 8 || IAU.ADD i1 i1 i0
LSU0.LDO.64.L v4  i1  0 || LSU1.LDO.64.H v4 i1 8 || IAU.ADD i1 i1 i0
NOP 2


___Load_Kernel_Values:
; Load first kernel line values || Rotate source values
LSU0.LDO.8 i11 i6 0 

VAU.SUB.i8 v1 v0 v1
VAU.SUB.i8 v2 v0 v2


LSU0.LDO.8 i12 i6 1 || VAU.SUB.i8 v3  v0  v3
LSU0.LDO.8 i13 i6 2 || VAU.SUB.i8 v4  v0  v4
LSU0.LDO.8 i14 i6 3 || CMU.ALIGNVEC   v11 V1, v1  1 
LSU0.LDO.8 i15 i6 4 || CMU.ALIGNVEC   v12 V1, v1  2    || IAU.ADD    i18 i18 4 
CMU.ALIGNVEC  v13 V1, v1 3 || LSU1.LDO.32 i1  i18 0
CMU.ALIGNVEC  v14 V1, v1 4


; Create vectors using the first kernel line 
CMU.CPIVR.x8 v6  i11 
CMU.CPIVR.x8 v7  i12 
CMU.CPIVR.x8 v8  i13
CMU.CPIVR.x8 v9  i14
CMU.CPIVR.x8 v10 i15   

; First line: multiply 
VAU.MUL.i8 v15 v6  v1  || IAU.ADD i1 i1 i2
VAU.MUL.i8 v16 v7  v11 
VAU.MUL.i8 v17 v8  v12
VAU.MUL.i8 v18 v9  v13
VAU.MUL.i8 v19 v10 v14 || CMU.ALIGNVEC v11 V2, v2 1

; FIRST LINE MAX
CMU.MAX.u8 v20 v15 v16
CMU.MAX.u8 v21 v17 v18
NOP
CMU.ALIGNVEC v12 V2, v2 2
CMU.ALIGNVEC v13 V2, v2 3

; Second line: multiply || Rotate source values
VAU.MUL.i8 v15 v6  v2  || CMU.ALIGNVEC   v14 V2, v2  4
VAU.MUL.i8 v16 v7  v11 || CMU.MAX.u8 v20 v20 v19  || IAU.ADD i16 i16 4
VAU.MUL.i8 v17 v8  v12
VAU.MUL.i8 v18 v9  v13
VAU.MUL.i8 v19 v10 v14 || CMU.ALIGNVEC v11 V3, v3 1

; Third line
CMU.ALIGNVEC v12 V3, v3 2
CMU.ALIGNVEC v13 V3, v3 3
CMU.ALIGNVEC v14 V3, v3 4

; SECOND LINE MAX
CMU.MAX.u8 v23 v15 v16
CMU.MAX.u8 v24 v17 v18

; Third line: multiply || Rotate source values
VAU.MUL.i8 v15 v6  v3  
VAU.MUL.i8 v16 v7  v11 || CMU.MAX.u8 v23 v23 v19 
VAU.MUL.i8 v17 v8  v12
VAU.MUL.i8 v18 v9  v13
VAU.MUL.i8 v19 v10 v14 || CMU.ALIGNVEC v11 V4, v4 1

; 4th line
CMU.ALIGNVEC v12 V4, v4 2
CMU.ALIGNVEC v13 V4, v4 3
CMU.ALIGNVEC v14 V4, v4 4

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
CMU.MAX.u8 v30 v30 v29

IAU.SUB i8 i8 1         || CMU.MAX.u8 v21 v21 v20
PEU.PC1I GT             || BRU.BRA ___Load_Source_Values
CMU.MAX.u8 v27 v27 v26
NOP 
LSU0.LDO.32 i6 i16 0     || CMU.MAX.u8 v31 v31 v30 
CMU.MAX.u8 v25 v25 v24  
CMU.MAX.u8 v28 v28 v27 
CMU.MAX.u8 v22 v22 v21 

CMU.CPII   i18 i10 
LSU0.LDO.32 i1  i18 0    || CMU.CPII i16 i9        ||  IAU.ADD i2 i2 i4   || VAU.SUB.i8 v28 v0 v28



___Store_Values:
VAU.SUB.i8 v22 v0 v22
LSU0.LDIL   i8 5        || LSU1.LDO.32 i6 i16 0    ||  IAU.SUB i5 i5 i4   || VAU.SUB.i8 v25 v0 v25
LSU0.ST.64.L v22 i17     || LSU1.STO.64.H v22 i17 8 ||  IAU.ADD i17 i17 i0 || VAU.SUB.i8 v31 v0 v31
LSU0.ST.64.L v25 i17     || LSU1.STO.64.H v25 i17 8 ||  IAU.ADD i17 i17 i0 || VAU.SUB.i8 v22 v22 v22 
LSU0.ST.64.L v28 i17     || LSU1.STO.64.H v28 i17 8 ||  IAU.ADD i17 i17 i0 || VAU.SUB.i8 v25 v25 v25
LSU0.ST.64.L v31 i17     || LSU1.STO.64.H v31 i17 8 ||  IAU.ADD i17 i17 i0 || VAU.SUB.i8 v28 v28 v28
VAU.SUB.i8  v31 v31 v31 || IAU.ADD i1 i1 i2


; Compensate the remaining pixels
___Compensate_Last_48_Bits:
CMU.CMZ.i8 i5 
PEU.PC1C GT      || CMU.CPII  i4 i5      || BRU.BRA ___Load_Source_Values
IAU.SUB i5 i5 i5 || SAU.SHR.U8 i0 i4 2
NOP 5

LSU0.LDO.64.L V31,i19,0 || LSU1.LDO.64.H V31, i19, 8 || iau.incs i19,16
LSU0.LDO.64.L V30,i19,0 || LSU1.LDO.64.H V30, i19, 8 || iau.incs i19,16
LSU0.LDO.64.L V29,i19,0 || LSU1.LDO.64.H V29, i19, 8 || iau.incs i19,16
LSU0.LDO.64.L V28,i19,0 || LSU1.LDO.64.H V28, i19, 8 || iau.incs i19,16
LSU0.LDO.64.L V27,i19,0 || LSU1.LDO.64.H V27, i19, 8 || iau.incs i19,16
LSU0.LDO.64.L V26,i19,0 || LSU1.LDO.64.H V26, i19, 8 || iau.incs i19,16
LSU0.LDO.64.L V25,i19,0 || LSU1.LDO.64.H V25, i19, 8 || iau.incs i19,16
LSU0.LDO.64.L V24,i19,0 || LSU1.LDO.64.H V24, i19, 8 || iau.incs i19,16

LSU0.LDO.32 I20,i19,0 || iau.incs i19,4

BRU.JMP i30
NOP 6


.end
