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


.data .data.erode
.code .text.erode
;  void Erode7x7(u8** src, u8** dst, u8** kernel, u32 width, u32 height)
;                   i18       i17        i16        i15         i14 
Erode7x7_asm:

lsu0.sto.32 I21,i19,-4 || iau.incs i19,-4
lsu0.sto.32 I24,i19,-4 || iau.incs i19,-4
lsu0.sto.32 I23,i19,-4 || iau.incs i19,-4
lsu0.sto.32 I22,i19,-4 || iau.incs i19,-4

lsu0.sto.64.h V24,i19,-8 || lsu1.sto.64.l V24,i19,-16 || iau.incs i19,-16
lsu0.sto.64.h V25,i19,-8 || lsu1.sto.64.l V25,i19,-16 || iau.incs i19,-16
lsu0.sto.64.h V26,i19,-8 || lsu1.sto.64.l V26,i19,-16 || iau.incs i19,-16
lsu0.sto.64.h V27,i19,-8 || lsu1.sto.64.l V27,i19,-16 || iau.incs i19,-16

lsu0.sto.64.h V28,i19,-8 || lsu1.sto.64.l V28,i19,-16 || iau.incs i19,-16
lsu0.sto.64.h V29,i19,-8 || lsu1.sto.64.l V29,i19,-16 || iau.incs i19,-16
lsu0.sto.64.h V30,i19,-8 || lsu1.sto.64.l V30,i19,-16 || iau.incs i19,-16
lsu0.sto.64.h V31,i19,-8 || lsu1.sto.64.l V31,i19,-16 || iau.incs i19,-16
;-----------------------------------------
;      Labels
;-----------------------------------------




LSU0.LDIL    i3 0xFF     || LSU1.LDIL i0   40  || CMU.CPII i22 i15 || VAU.SUB.i8  v19 v19 v19
CMU.CPIVR.x8 v31  i3     || VAU.SUB.i8  v20 v20 v20

LSU0.LDO.32 i1  i18 0    || VAU.SUB.i8  v21 v21 v21   

;If width < 40 then
CMU.CMII.u32 i15 i0
PEU.PC1C LT || CMU.CPII i0 i15 

LSU0.LD.32  i17 i17      || LSU1.LDO.32 i8  i16 0   || SAU.DIV.u32 i3 i15 i0    || CMU.CPII i6 i16 || VAU.SUB.i8  v22 v22 v22
VAU.SUB.i8  v24 v24 v24
VAU.SUB.i8  v25 v25 v25
VAU.SUB.i8  v26 v26 v26
VAU.SUB.i8  v27 v27 v27
VAU.SUB.i8  v28 v28 v28
VAU.SUB.i8  v29 v29 v29
VAU.SUB.i8  v30 v30 v30

CMU.CPII   i5 i18       || LSU0.LDIL i7 0         || LSU1.LDIL i21  3 || VAU.SUB.i8  v23 v23 v23
LSU0.LDIL  i4 ___Store_Values                     || LSU1.LDIH i4 ___Store_Values
LSU0.LDIL i2 7          || VAU.SUB.i8 v27 v27 v27 
VAU.SUB.i8 v30 v30 v30  
VAU.SUB.i8 v26 v26 v26 


IAU.SHR.u32 i23 i0 2
IAU.AND i24 i0 i21
IAU.ADD i24 i23 i24

BRU.RPL i4 i3

___Load_Source_Values:

; Process first 10 values
IAU.ADD      i1  i1  i7
LSU0.LDO.64.L v1  i1  0   || LSU1.LDO.64.H  v1  i1 8 || IAU.ADD i1 i1 i24
LSU0.LDO.64.L v0  i1  0   || LSU1.LDO.64.H  v0  i1 8 || IAU.ADD i1 i1 i23
LSU0.LDO.64.L v25 i1  0   || LSU1.LDO.64.H  v25 i1 8 || IAU.ADD i1 i1 i23
LSU0.LDO.64.L v29 i1  0   || LSU1.LDO.64.H  v29 i1 8 || IAU.ADD i1 i1 i23

; Load kernel values
LSU0.LDO.8    i9  i8  0   || LSU1.LDO.8 i10 i8 1 
LSU0.LDO.8    i11 i8  2   || LSU1.LDO.8 i12 i8 3 
NOP

LSU0.LDO.8    i13 i8  4   || LSU1.LDO.8 i14 i8 5  || VAU.SUB.i8 v1 v31 v1

LSU0.LDO.8    i15 i8  6   
CMU.ALIGNVEC     v2  V1, v1  1   
CMU.ALIGNVEC     v3  V1, v1  2 
CMU.ALIGNVEC     v4  V1, v1  3 


; Load source values
CMU.ALIGNVEC     v5  v1  v1 4   || VAU.SUB.i8 v0  v31  v0 
CMU.ALIGNVEC     v6  v1  v1 5   || VAU.SUB.i8 v25 v31  v25 
CMU.ALIGNVEC     v7  v1  v1 6   || VAU.SUB.i8 v29 v31  v29

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
CMU.MAX.u8 v1 v20 v21
CMU.MAX.u8 v23 v23 v22
NOP
CMU.ALIGNVEC   v2  V0, v0  1 
CMU.MAX.u8 v1 v1 v24
NOP


CMU.ALIGNVEC   v3  V0, v0  2 
CMU.MAX.u8 v1 v1 v23
NOP
CMU.ALIGNVEC v4 V0, v0 3 
CMU.ALIGNVEC v5 v0 v0 4         || VAU.MUL.i8 v16 v0 v8
CMU.MAX.u8 v26 v26 v1        || VAU.MUL.i8 v17 v2 v9
NOP
    
    
; Load source values
CMU.ALIGNVEC v6 v0 v0 5         || VAU.MUL.i8 v18 v3 v10
CMU.ALIGNVEC v7 V0, v0 6      


; Process the multiplication
VAU.MUL.i8 v19 v4 v11    || CMU.MAX.u8 v23 v16 v17
VAU.MUL.i8 v20 v5 v12   
VAU.MUL.i8 v21 v6 v13
VAU.MUL.i8 v22 v7 v14    || CMU.MAX.u8 v24 v18 v19


; Find maximum per line
NOP
CMU.ALIGNVEC   v2  V25, v25 1 
CMU.MAX.u8 v0 v20 v21
CMU.MAX.u8 v23 v23 v22
NOP
CMU.MAX.u8 v0 v0 v24
NOP


CMU.ALIGNVEC   v3  V25, v25 2 
CMU.MAX.u8 v0 v0 v23
IAU.ADD    i18 i18 4
LSU0.LDO.32 i1  i18 0


; Compute the kernel maximum
CMU.MAX.u8 v27 v27 v0
NOP


; Load source values
CMU.ALIGNVEC v4 V25, v25 3 
CMU.ALIGNVEC v5 v25 v25 4         || VAU.MUL.i8 v16 v25 v8
CMU.ALIGNVEC v6 v25 v25 5         || VAU.MUL.i8 v17 v2  v9
CMU.ALIGNVEC v7 v25 v25 6         || VAU.MUL.i8 v18 v3  v10


; Process the multiplication
VAU.MUL.i8 v19 v4 v11
VAU.MUL.i8 v20 v5 v12     || CMU.MAX.u8 v23 v16 v17
VAU.MUL.i8 v21 v6 v13
VAU.MUL.i8 v22 v7 v14

CMU.MAX.u8 v24 v18 v19
CMU.MAX.u8 v25 v20 v21
CMU.MAX.u8 v23 v23 v22
NOP
CMU.MAX.u8 v25  v25  v24
NOP
CMU.ALIGNVEC v2 V29  v29 1 
CMU.ALIGNVEC v3 V29  v29 2 
CMU.MAX.u8 v25  v25  v23
NOP
CMU.ALIGNVEC v4 V29, v29 3 
CMU.ALIGNVEC v5 v29  v29 4         || VAU.MUL.i8 v16 v29 v8
CMU.MAX.u8 v28 v28 v25
NOP


; Load source values
CMU.ALIGNVEC v6 v29 v29 5         || VAU.MUL.i8 v17 v2 v9
CMU.ALIGNVEC v7 v29 v29 6         || VAU.MUL.i8 v18 v3 v10

; Process the multiplication
VAU.MUL.i8 v19 v4 v11
VAU.MUL.i8 v20 v5 v12     || CMU.MAX.u8 v23 v16 v17
VAU.MUL.i8 v21 v6 v13
VAU.MUL.i8 v22 v7 v14

CMU.MAX.u8 v24 v18 v19
CMU.MAX.u8 v29 v20 v21
CMU.MAX.u8 v23 v23 v22
NOP
CMU.MAX.u8 v29  v29  v24
NOP


IAU.SUB i2 i2 1
PEU.PC1I GT               || BRU.BRA ___Load_Source_Values
NOP 2
   CMU.MAX.u8 v29  v29  v23
   IAU.ADD    i16  i16  4 
   LSU0.LDO.32 i8   i16  0 
   CMU.MAX.u8 v30  v30  v29


VAU.SUB.i8 v26 v31 v26
LSU0.LDO.32   i1  i5    0  || VAU.SUB.i8 v27 v31 v27
LSU0.STO.64.L v26 i17   0  || LSU1.STO.64.H v26 i17  8     || IAU.ADD    i17 i17 i24  || CMU.CPII   i18 i5  || VAU.SUB.i8 v28 v31 v28

; Store the kernel maximum
___Store_Values:
NOP
LSU0.STO.64.L v27 i17   0  || LSU1.STO.64.H v27 i17  8     || IAU.ADD    i17 i17 i23  || VAU.SUB.i8 v30 v31 v30
LSU0.STO.64.L v28 i17   0  || LSU1.STO.64.H v28 i17  8     || IAU.ADD    i17 i17 i23  
LSU0.STO.64.L v30 i17   0  || LSU1.STO.64.H v30 i17  8     || IAU.ADD    i17 i17 i23  || CMU.CPII   i18 i5       || VAU.SUB.i8 v27 v27 v27 
CMU.CPII     i16 i6        || IAU.ADD      i7  i7   i0    || LSU1.LDIL  i2  7        || VAU.SUB.i8 v30 v30 v30  
LSU1.LDO.32   i8  i16   0  || IAU.SUB      i22 i22  i0    || VAU.SUB.i8 v28 v28 v28 
VAU.SUB.i8   v26 v26  v26 


; Compensate last values
CMU.CMZ.i8 i22
PEU.PC1I GT                || BRU.BRA ___Load_Source_Values 
CMU.CPII i0 i22 
IAU.SUB i22 i22 i22  
IAU.SHR.u32 i23 i0 2
IAU.AND i24 i0 i21
IAU.ADD i24 i23 i24
NOP

lsu0.ldo.64.l V31,i19,0 || lsu1.ldo.64.h V31, i19, 8 || iau.incs i19,16
lsu0.ldo.64.l V30,i19,0 || lsu1.ldo.64.h V30, i19, 8 || iau.incs i19,16
lsu0.ldo.64.l V29,i19,0 || lsu1.ldo.64.h V29, i19, 8 || iau.incs i19,16
lsu0.ldo.64.l V28,i19,0 || lsu1.ldo.64.h V28, i19, 8 || iau.incs i19,16

lsu0.ldo.64.l V27,i19,0 || lsu1.ldo.64.h V27, i19, 8 || iau.incs i19,16
lsu0.ldo.64.l V26,i19,0 || lsu1.ldo.64.h V26, i19, 8 || iau.incs i19,16
lsu0.ldo.64.l V25,i19,0 || lsu1.ldo.64.h V25, i19, 8 || iau.incs i19,16
lsu0.ldo.64.l V24,i19,0 || lsu1.ldo.64.h V24, i19, 8 || iau.incs i19,16


lsu0.ldo.32 I22,i19,0 || iau.incs i19,4
lsu0.ldo.32 I23,i19,0 || iau.incs i19,4
lsu0.ldo.32 I24,i19,0 || iau.incs i19,4
lsu0.ldo.32 I21,i19,0 || iau.incs i19,4

BRU.JMP i30
NOP 6

.end
