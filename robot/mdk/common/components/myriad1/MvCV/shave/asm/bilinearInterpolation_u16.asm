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
; i18 : pointer to source
; i17 : step (stride) for source
; i16 : pointer to destination
; i15 : step (stride) for destination
; v23 : window size
; v22 : center coordinates

.code MvCV_BilinearInterpolation_u16
.lalign
bilinearInterpolation16u_asm:
CMU.CPVI.x32 i3,  v23.0 || VAU.SHL.u32  v0,  v23, 0x1F || LSU1.LDIL i0, biInter8
CMU.CPVI.x32 i11, v23.1 || IAU.SUB.u32s i10, i3,  0x08 || LSU1.LDIH i0, biInter8
CMU.CMZ.i32 v0 || LSU1.LDIL i1, 0x01
PEU.PVV32 0x1 || CMU.CMZ.i32 i10 || VAU.SUB.f32 v22, v22, 0.5
PEU.PCCX.NEQ 0x20 || IAU.ADD i8,  i18, 0x01 || LSU1.LDIL i0, biInter16
PEU.PCCX.NEQ 0x20 || IAU.ADD i9,  i8,  i17  || LSU1.LDIH i0, biInter16
CMU.CPIVR.i32.f32 v1,  i1  || VAU.FRAC.f32 v0, v22 || LSU1.SWZ4V [1100] [1100]
IAU.ADD i10, i16, 0 
PEU.PVEN4WORD 0xA || VAU.SUB.f32 v0, v1, v0 
IAU.FEXT.u32 i3, i3, 0x00, 0x03 || LSU1.LDIL i2, 0x100
CMU.CPIVR.i32.f32 v2, i2 || IAU.SHL i3, i1, i3 
VAU.MUL.f32 v0, v0, v0 || IAU.SUB i3, i3, 0x01 || LSU1.SWZ4V [3310] [1022]
CMU.CMASK v3, i3, 0x0 
CMU.CMZ.i8 v3 || BRU.JMP i0 
VAU.MUL.f32 v0, v0, v2
CMU.CPVI.x32 i2, v23.1
CMU.CPIVR.x16 v1, i1 || LSU1.LDIL i0, 0x80
; f rounding
VAU.ADD.f32 v0, v0, 0.5
CMU.CPIVR.x16 v12, i0

; handle line widths up to 8 pixels
.lalign
biInter8:
; take off
                                  IAU.SUB i13, i17, 0                                         || LSU0.LDO8          i4,  i8,  -1   
CMU.CPVV.f32.i32s v0,  v0      || IAU.SUB i12, i15, 0                                         || LSU0.LDI128.u8.u16 v4,  i8,  i13   
                                  IAU.SUB i11, i11, 0x01                                                                          || LSU1.LDIL i0, bilInter8_LoopEnd
								  IAU.SHR.u32 i11, i11, 0x01                                  || LSU0.LDO8          i5,  i8,  -1  || LSU1.LDIH i0, bilInter8_LoopEnd
CMU.VSZMBYTE i4, i4, [ZZZZ]                                   || VAU.SWZBYTE v8,  v0, [1010]  || LSU0.LDI128.u8.u16 v5,  i8,  i13 || LSU1.SWZ4V [2222] [2222]  
CMU.VSZMBYTE i5, i5, [ZZZZ]                                   || VAU.SWZBYTE v9,  v0, [1010]                                      || LSU1.SWZ4V [3333] [3333]
CMU.VSZMBYTE i6, i6, [ZZZZ]                                   || VAU.SWZBYTE v10, v0, [1010]  || LSU0.LDO8          i6,  i8,  -1  || LSU1.SWZ4V [0000] [0000] 
                                                                 VAU.SWZBYTE v11, v0, [1010]  || LSU0.LDI128.u8.u16 v6,  i8,  i13 || LSU1.SWZ4V [1111] [1111]  
																 nop
CMU.SHLIV.x16 v4,  i4                                         || VAU.MACPZ.u16 v4,  v8         
CMU.CPVV      v4,  v5          || IAU.ADD     i4,  i5,  0     || VAU.MACP.u16  v4,  v9
CMU.SHLIV.x16 v5,  i5                                         || VAU.MACP.u16  v5,  v10       || LSU0.LDO8          i5,  i8,  -1
                                                                 VAU.MACP.u16  v5,  v11       || LSU0.LDI128.u8.u16 v5,  i8,  i13
CMU.CPVV      v5,  v4                                         || VAU.MACPW.u16 v16, v12, v1   || BRU.BRA biInter8_Loop 
CMU.SHLIV.x16 v5,  i5                                         || VAU.MACPZ.u16 v5,  v8        || LSU0.LDO8          i6,  i8,  -1
CMU.CPVV      v4,  v6          || IAU.ADD     i4,  i6,  0     || VAU.MACP.u16  v5,  v9        || LSU0.LDI128.u8.u16 v6,  i8,  i13 
CMU.SHLIV.x16 v6,  i6                                         || VAU.MACP.u16  v6,  v10       
                                                                 VAU.MACP.u16  v6,  v11
                                                                 VAU.MACPW.u16 v17, v12, v1  

.lalign
biInter8_Loop:
; warp zone
CMU.SHLIV.x16 v4,  i4                                         || VAU.MACPZ.u16 v4,  v8        || BRU.RPL i0,  i11
CMU.CPVV      v4,  v5          || IAU.ADD     i4,  i5,  0     || VAU.MACP.u16  v4,  v9
CMU.SHLIV.x16 v5,  i5                                         || VAU.MACP.u16  v5,  v10       || LSU0.LDO8          i5,  i8,  -1
CMU.VSZMBYTE  v16, v16, [Z3Z1]                                || VAU.MACP.u16  v5,  v11       || LSU0.LDI128.u8.u16 v5,  i8,  i13
bilInter8_LoopEnd:
CMU.CPVV      v5,  v4          || PEU.PVL0S8 0x6              || VAU.MACPW.u16 v16, v12, v1   || LSU0.STI128.u16.u8 v16, i10, i12
CMU.SHLIV.x16 v5,  i5                                         || VAU.MACPZ.u16 v5,  v8        || LSU0.LDO8          i6,  i8,  -1
CMU.CPVV      v4,  v6          || IAU.ADD     i4,  i6,  0     || VAU.MACP.u16  v5,  v9        || LSU0.LDI128.u8.u16 v6,  i8,  i13 
CMU.SHLIV.x16 v6,  i6                                         || VAU.MACP.u16  v6,  v10       
CMU.VSZMBYTE  v17, v17, [Z3Z1]                                || VAU.MACP.u16  v6,  v11
IAU.FEXT.u32  i3,  i2, 0, 1    || PEU.PVL0S8 0x6              || VAU.MACPW.u16 v17, v12, v1   || LSU0.STI128.u16.u8 v17, i10, i12

; landing
PEU.PC1I 0x6 || BRU.JMP i30 
PEU.PC1I 0x1 || BRU.JMP i30 
CMU.VSZMBYTE  v16, v16, [Z3Z1] 
PEU.PVL0S8 0x6 || LSU0.STI128.u16.u8 v16, i10, i12 
CMU.VSZMBYTE  v17, v17, [Z3Z1]
PEU.PVL0S8 0x6 || LSU0.ST128.u16.u8  v17, i10 
nop


; handle line widths up to 16 pixels
.lalign
biInter16:
; take off
                                  IAU.SUB i13, i17, 0x08                                      || LSU0.LDO8          i4,  i8,  -1
CMU.CPVV.f32.i32s v0,  v0      || IAU.SUB i12, i15, 0x08                                      || LSU0.LDI128.u8.u16 v4,  i8
                                  IAU.SUB i11, i11, 0x01                                      || LSU0.LDO8          i6,  i9,  -1   || LSU1.LDIL i0, bilInter16_LoopEnd
                                                                                                 LSU0.LDI128.u8.u16 v6,  i9        || LSU1.LDIH i0, bilInter16_LoopEnd
CMU.VSZMBYTE i4, i4, [ZZZZ]                                   || VAU.SWZBYTE v8,  v0, [1010]                                       || LSU1.SWZ4V [2222] [2222]
                                                                 VAU.SWZBYTE v9,  v0, [1010]                                       || LSU1.SWZ4V [3333] [3333]
CMU.VSZMBYTE i6, i6, [ZZZZ]                                   || VAU.SWZBYTE v10, v0, [1010]  || LSU0.LDI128.u8.u16 v5,  i8,  i13  || LSU1.SWZ4V [0000] [0000] 
                                                                 VAU.SWZBYTE v11, v0, [1010]  || LSU0.LDI128.u8.u16 v7,  i9,  i13  || LSU1.SWZ4V [1111] [1111]

CMU.SHLIV.x16 v4,  i4          || SAU.SUM.u32 i5,  v4,  0x8   || VAU.MACPZ.u16 v4,  v8        
CMU.CPVV      v4,  v6                                         || VAU.MACP.u16  v4,  v9
CMU.SHLIV.x16 v4,  i6          || SAU.SUM.u32 i7,  v4,  0x8   || VAU.MACP.u16  v4,  v10       || LSU0.LDO8          i6,  i9,  -1
                                                                 VAU.MACP.u16  v4,  v11       || LSU0.LDI128.u8.u16 v6,  i9
CMU.CPVV      v4,  v6          || IAU.SHR.u32 i5,  i5,  0x10  || VAU.MACPW.u16 v16, v12, v1   || BRU.BRA biInter16_Loop
CMU.SHLIV.x16 v5,  i5          || IAU.ADD     i4,  i6,  0     || VAU.MACPZ.u16 v5,  v8        || LSU0.LDI128.u8.u16 v7,  i9,  i13
CMU.CPVV      v5,  v7          || IAU.SHR.u32 i7,  i7,  0x10  || VAU.MACP.u16  v5,  v9         
CMU.SHLIV.x16 v5,  i7                                         || VAU.MACP.u16  v5,  v10       
                                                                 VAU.MACP.u16  v5,  v11
CMU.CPVV      v5,  v7                                         || VAU.MACPW.u16 v17, v12, v1 

.lalign
biInter16_Loop:
; warp zone
CMU.SHLIV.x16 v4,  i4          || SAU.SUM.u32 i5,  v4,  0x8   || VAU.MACPZ.u16 v4,  v8        || BRU.RPL i0,  i11
CMU.CPVV      v4,  v6                                         || VAU.MACP.u16  v4,  v9
CMU.SHLIV.x16 v4,  i6          || SAU.SUM.u32 i7,  v4,  0x8   || VAU.MACP.u16  v4,  v10       || LSU0.LDO8          i6,  i9,  -1
CMU.VSZMBYTE  v16, v16, [Z3Z1]                                || VAU.MACP.u16  v4,  v11       || LSU0.LDI128.u8.u16 v6,  i9
bilInter16_LoopEnd:
CMU.CPVV      v4,  v6          || IAU.SHR.u32 i5,  i5,  0x10  || VAU.MACPW.u16 v16, v12, v1   || LSU0.STI128.u16.u8 v16, i10
CMU.SHLIV.x16 v5,  i5          || IAU.ADD     i4,  i6,  0     || VAU.MACPZ.u16 v5,  v8        || LSU0.LDI128.u8.u16 v7,  i9,  i13
CMU.CPVV      v5,  v7          || IAU.SHR.u32 i7,  i7,  0x10  || VAU.MACP.u16  v5,  v9         
CMU.SHLIV.x16 v5,  i7                                         || VAU.MACP.u16  v5,  v10       
CMU.VSZMBYTE  v17, v17, [Z3Z1]                                || VAU.MACP.u16  v5,  v11
CMU.CPVV      v5,  v7          || PEU.PVL0S8 0x6              || VAU.MACPW.u16 v17, v12, v1   || LSU0.STI128.u16.u8 v17, i10, i12  


; landing
BRU.JMP i30
CMU.VSZMBYTE  v16, v16, [Z3Z1]
LSU0.STI128.u16.u8 v16, i10
CMU.VSZMBYTE  v17, v17, [Z3Z1]
PEU.PVL0S8 0x6 || LSU0.ST128.u16.u8 v17, i10
nop








