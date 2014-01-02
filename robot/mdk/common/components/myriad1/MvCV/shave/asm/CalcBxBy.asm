; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief
; ///

.version 0.50.00

; i30 : return pointer
; i19 : stack pointer
; i18 : pointer to patchI
; i17 : pointer to Ix
; i16 : pointer to Iy
; i15 : pointer to patchJ
; i14 : pointer to error
; v23 : isz
; v22 : minI
; v21 : jsz
; v20 : minJ
.code .text.MvCV_CalcBxBy
.lalign
CalcBxBy_asm:
; use jsz.x as increment, calculate { minJ.x - minI.x, minJ.y - minI.y }
CMU.CPVI.x32 i6,  v21.0 || VAU.SUB.i32 v0,  v20, v22 || LSU1.LDIL i0, CalcBxBy12
;  isz.x used as increment
CMU.CPVI.x32 i5,  v23.0 || IAU.SUB.u32s i7, i6, 0x08 || LSU1.LDIH i0, CalcBxBy12
CMU.CPVI.x32 i9,  v0.1                               || LSU1.LDIL i0, CalcBxBy8  || PEU.PCIX.EQ 0x20
; pj = patchJ; calculate (minJ.y - minI.y + 1) 
CMU.CPII i2, i15        || IAU.ADD i9, i9, 0x01      || LSU1.LDIH i0, CalcBxBy8  || PEU.PCIX.EQ 0x20
; calculate (minJ.y - minI.y + 1)*isz.x
IAU.MUL i9, i9, i5
; (isz.x-2) used as increment
CMU.CPVI.x32 i10, v0.0  || SAU.SUB.i32 i7,  i5,  0x02
; calculate (minJ.x - minI.x + 1)
IAU.ADD i10, i10, 0x01
; calculate ((minJ.y - minI.y + 1)*isz.x+(minJ.x - minI.x + 1)); shift (isz.x-2) as data type is float
CMU.CPVI.x32 i10, v0.0  || IAU.ADD i1,  i9,  i10 || SAU.SHL.i32 i7, i7, 0x02
; pi = patchI + ( minJ.y - minI.y + 1) * isz.x + minJ.x - minI.x + 1; shift (minJ.x - minI.x) as data types is float
CMU.CPVI.x32 i9,  v0.1  || IAU.ADD i1, i1, i18 || SAU.SHL.i32 i10, i10, 0x02
; calculate (minJ.y - minI.y) * (isz.x - 2)
IAU.MUL i9,  i9,  i7 || BRU.JMP i0
; jsz.y used as counter
CMU.CPVI.x32 i0,  v21.1 
; calculate (( minJ.y - minI.y) * (isz.x - 2) + minJ.x - minI.x)
IAU.ADD i3,  i9,  i10
; ix = Ix + (minJ.y - minI.y) * (isz.x - 2) + minJ.x - minI.x
IAU.ADD i3,  i3,  i17
; iy = Iy + (ix - Ix);
IAU.SUB i4,  i3,  i17
IAU.ADD i4,  i4,  i16

; handle line widths up to 8 pixels
.lalign
CalcBxBy8:
; take off
; generate mask for end of row, read pi[x] pj[x]
CMU.VSZMBYTE s4,  s4,  [ZZZZ] || IAU.FEXT.u32 i9,  i6,  0x00, 0x03 || LSU0.LDI128.u8.u16 v4, i1
CMU.VSZMBYTE s5,  s5,  [ZZZZ] || IAU.XOR      i8,  i8,  i8         || LSU0.LDI128.u8.u16 v6, i2
; read ix[x]
CMU.VSZMBYTE s6,  s6,  [ZZZZ] || IAU.ADD      i8,  i8,  0x01       || LSU0.LDI64.l v12, i3
CMU.VSZMBYTE s8,  s8,  [ZZZZ] || IAU.SHL      i8,  i8,  i9         || LSU0.LDI64.h v12, i3
CMU.VSZMBYTE s9,  s9,  [ZZZZ] || IAU.SUB      i8,  i8,  0x01       || LSU0.LDI64.l v13, i3
CMU.VSZMBYTE s12, s12, [ZZZZ] || IAU.SUB      i5,  i5,  0x08       || LSU0.LDI64.h v13, i3

; read iy[x]
CMU.VSZMBYTE s13, s13, [ZZZZ] || IAU.SUB      i6,  i6,  0x08       || LSU0.LDI64.l v16, i4  || BRU.BRA CalcBxBy8_Loop 
CMU.VSZMBYTE s16, s16, [ZZZZ] || IAU.SUB      i7,  i7,  0x20       || LSU0.LDI64.h v16, i4
CMU.VSZMBYTE s17, s17, [ZZZZ] || IAU.ADD      i1,  i1,  i5         || LSU0.LDI64.l v17, i4
                                 IAU.ADD      i2,  i2,  i6         || LSU0.LDI64.h v17, i4
; lock mask for end of row in condition codes of CMU								 
CMU.CMASK v0, i8, 0x1         || IAU.ADD      i3,  i3,  i7          
CMU.CMZ.i16 v0                || IAU.ADD      i4,  i4,  i7         

.lalign
CalcBxBy8_Loop:
; warp zone
; calculate t0 = pi[x] - pj[x], read pi[x] pj[x]
                              VAU.SUB.i16 v8,  v4,  v6  || LSU0.LDI128.u8.u16 v4, i1
                              IAU.SUB i0,  i0,  0x01    || LSU0.LDI128.u8.u16 v6, i2
; accumulate partial results, convert to float
SAU.ADD.f32 s4,  s4,  s12  || VAU.XOR     v8,  v8,  v8  || PEU.PVV16 0x1 
SAU.ADD.f32 s5,  s5,  s16  || CMU.CPVV.i16.f32 v8,  v8  || LSU0.LDI64.l v12, i3
SAU.ADD.f32 s6,  s6,  s8   || CMU.CPVV.i16.f32 v9,  v8  || LSU0.LDI64.h v12, i3  || LSU1.SWZC4WORD [1032]
; calculate t0 * ix[x]
SAU.ADD.f32 s4,  s4,  s13  || VAU.MUL.f32 v12, v12, v8  || LSU0.LDI64.l v13, i3
SAU.ADD.f32 s5,  s5,  s17  || VAU.MUL.f32 v13, v13, v9  || LSU0.LDI64.h v13, i3
; calculate t0 * iy[x]
SAU.ADD.f32 s6,  s6,  s9   || VAU.MUL.f32 v16, v16, v8  || LSU0.LDI64.l v16, i4
SAU.SUM.f32 s12, v12, 0xF  || VAU.MUL.f32 v17, v17, v9  || LSU0.LDI64.h v16, i4 						  
PEU.PCIX.NEQ 0             || BRU.BRA CalcBxBy8_Loop 	|| LSU0.LDI64.l v17, i4 
; column add to get partial results, calculate t0 * t0
SAU.SUM.f32 s13, v13, 0xF  || VAU.MUL.f32 v8,  v8,  v8  || LSU0.LDI64.h v17, i4
SAU.SUM.f32 s16, v16, 0xF  || VAU.MUL.f32 v9,  v9,  v9  || IAU.ADD i1,  i1,  i5 
SAU.SUM.f32 s17, v17, 0xF  || IAU.ADD i2,  i2,  i6 
SAU.SUM.f32 s8,  v8,  0xF  || IAU.ADD i3,  i3,  i7
SAU.SUM.f32 s9,  v9,  0xF  || IAU.ADD i4,  i4,  i7

; landing
nop
nop
SAU.ADD.f32 s4,  s4,  s12 
SAU.ADD.f32 s5,  s5,  s16 
SAU.ADD.f32 s6,  s6,  s8 
SAU.ADD.f32 s4,  s4,  s13 || BRU.JMP i30
SAU.ADD.f32 s5,  s5,  s17
SAU.ADD.f32 s6,  s6,  s9
CMU.CPSV.x32 v23.0, s4
CMU.CPSV.x32 v23.1, s5
LSU0.ST32 s6, i14


; handle line widths up to 12 pixels
.lalign
CalcBxBy12:
; take off
; generate mask for end of row, read pi[x]
CMU.VSZMBYTE s4,  s4,  [ZZZZ] || IAU.FEXT.u32 i9,  i6,  0x00, 0x02 || LSU0.LDI128.u8.u16 v4, i1
CMU.VSZMBYTE s5,  s5,  [ZZZZ]                                      || LSU0.LDI128.u8.u16 v5, i1     
; read pj[x]
CMU.VSZMBYTE s6,  s6,  [ZZZZ] || IAU.XOR      i8,  i8,  i8         || LSU0.LDI128.u8.u16 v6, i2  
CMU.VSZMBYTE s8,  s8,  [ZZZZ] || IAU.ADD      i8,  i8,  0x01       || LSU0.LDI128.u8.u16 v7, i2
; read ix[x]
CMU.VSZMBYTE s9,  s9,  [ZZZZ] || IAU.SHL      i8,  i8,  i9         || LSU0.LDI64.l v12, i3
CMU.VSZMBYTE s10, s10, [ZZZZ] || IAU.SUB      i8,  i8,  0x01       || LSU0.LDI64.h v12, i3
CMU.VSZMBYTE s12, s12, [ZZZZ] || IAU.SUB      i5,  i5,  0x10       || LSU0.LDI64.l v13, i3
CMU.VSZMBYTE s13, s13, [ZZZZ] || IAU.SUB      i6,  i6,  0x10       || LSU0.LDI64.h v13, i3
CMU.VSZMBYTE s14, s14, [ZZZZ] || IAU.SUB      i7,  i7,  0x30       || LSU0.LDI64.l v14, i3
CMU.VSZMBYTE s16, s16, [ZZZZ]                                      || LSU0.LDI64.h v14, i3
; read iy[x], 
CMU.VSZMBYTE s17, s17, [ZZZZ] || LSU0.LDI64.l v16, i4  
CMU.VSZMBYTE s18, s18, [ZZZZ] || LSU0.LDI64.h v16, i4
                                 LSU0.LDI64.l v17, i4  
BRU.BRA CalcBxBy12_Loop      || LSU0.LDI64.h v17, i4  
IAU.ADD i1,  i1,  i5          || LSU0.LDI64.l v18, i4  
IAU.ADD i2,  i2,  i6          || LSU0.LDI64.h v18, i4 
; lock mask for end of row in condition codes of CMU
IAU.ADD i3,  i3,  i7  || CMU.CMASK v0, i8, 0x1  
IAU.ADD i4,  i4,  i7  || CMU.CMZ.i16 v0
IAU.SUB i0,  i0,  0x01

.lalign
CalcBxBy12_Loop:
; warp zone
; accumulate partial results, calculate t0 = pi[x] - pj[x], read pi[x]
SAU.ADD.f32 s4,  s4,  s12  || VAU.SUB.i16 v10, v5,  v7   || LSU0.LDI128.u8.u16 v4, i1
SAU.ADD.f32 s5,  s5,  s16  || VAU.SUB.i16 v8,  v4,  v6   || LSU0.LDI128.u8.u16 v5, i1 
SAU.ADD.f32 s6,  s6,  s8   || VAU.SUB.i16 v10, v10, v10  || PEU.PVV16 0x1     
; convert to float, read pj[x]
SAU.ADD.f32 s4,  s4,  s13  || CMU.CPVV.i16.f32 v9,  v8   || LSU1.SWZC4WORD [1032]
SAU.ADD.f32 s5,  s5,  s17  || CMU.CPVV.i16.f32 v8,  v8   || LSU0.LDI128.u8.u16 v6, i2
SAU.ADD.f32 s6,  s6,  s9   || CMU.CPVV.i16.f32 v10, v10  || LSU0.LDI128.u8.u16 v7, i2
; calculate t0 * ix[x], read ix[x]
SAU.ADD.f32 s4,  s4,  s14  || VAU.MUL.f32 v12, v12, v8   || LSU0.LDI64.l v12, i3
SAU.ADD.f32 s5,  s5,  s18  || VAU.MUL.f32 v13, v13, v9   || LSU0.LDI64.h v12, i3
SAU.ADD.f32 s6,  s6,  s10  || VAU.MUL.f32 v14, v14, v10  || LSU0.LDI64.l v13, i3
; column add to get partial results, calculate bx += t0 * ix[x], calculate t0 * iy[x]
SAU.SUM.f32 s12, v12, 0xF  || VAU.MUL.f32 v16, v16, v8   || LSU0.LDI64.h v13, i3
SAU.SUM.f32 s13, v13, 0xF  || VAU.MUL.f32 v17, v17, v9   || LSU0.LDI64.l v14, i3
SAU.SUM.f32 s14, v14, 0xF  || VAU.MUL.f32 v18, v18, v10  || LSU0.LDI64.h v14, i3
; calculate by += t0 * iy[x], calculate t0 * t0
SAU.SUM.f32 s16, v16, 0xF  || VAU.MUL.f32 v8,  v8,  v8   || LSU0.LDI64.l v16, i4 
SAU.SUM.f32 s17, v17, 0xF  || VAU.MUL.f32 v9,  v9,  v9   || LSU0.LDI64.h v16, i4
SAU.SUM.f32 s18, v18, 0xF  || VAU.MUL.f32 v10, v10, v10  || LSU0.LDI64.l v17, i4
; calculate error += t0 * t0, read iy[x]
PEU.PCIX.NEQ 0             || BRU.BRA CalcBxBy12_Loop   || LSU0.LDI64.h v17, i4
SAU.SUM.f32 s8,  v8,  0xF  || IAU.ADD i1,  i1,  i5       || LSU0.LDI64.l v18, i4   
SAU.SUM.f32 s9,  v9,  0xF  || IAU.ADD i2,  i2,  i6       || LSU0.LDI64.h v18, i4
SAU.SUM.f32 s10, v10, 0xF  || IAU.ADD i3,  i3,  i7                               
                              IAU.ADD i4,  i4,  i7                                                                        
                              IAU.SUB i0,  i0,  0x01    						  

; landing
SAU.ADD.f32 s4,  s4,  s12 
SAU.ADD.f32 s5,  s5,  s16
SAU.ADD.f32 s6,  s6,  s8
SAU.ADD.f32 s4,  s4,  s13
SAU.ADD.f32 s5,  s5,  s17
SAU.ADD.f32 s6,  s6,  s9
SAU.ADD.f32 s4,  s4,  s14 || BRU.JMP i30
SAU.ADD.f32 s5,  s5,  s18
SAU.ADD.f32 s6,  s6,  s10
CMU.CPSV.x32 v23.0, s4
CMU.CPSV.x32 v23.1, s5
LSU0.ST32 s6, i14



