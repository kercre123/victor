;-------------------------------------------------------------------------------
.version 00.51.05
; atb, 05 sep 2013
; note: yet another asm module
;------------------------------------------------------------------------------- 
.include "myriad2SippDefs.inc"

.set P_O_EPSILON               0

.code svuGenChroma

.lalign 
; i18 : pointer to SippFilter
; i17 : svuNo
; i16 : runNo
.lalign
svuGenChroma:
IAU.FEXTU    i0, i16, 0x00, 0x01  || LSU0.LDO.32 i2,  i18, F_O_PARAMS
IAU.SHL i0,  i0,  0x02            || LSU0.LDO.32 i3,  i18, F_O_PARENTS
IAU.ADD i0,  i0,  i18             || LSU1.LDIL   i1,  F_O_LINES_I
IAU.ADD i9,  i1,  i0              || LSU1.LDIL   i1,  F_O_LINES_O
IAU.ADD i12, i1,  i0              || LSU0.LDO.32 i8,  i9, 0x08
                                     LSU0.LD.32  i9,  i9
                                     LSU0.LD.32  i12, i12
                                     LSU0.LDO.32 i2,  i2,  P_O_EPSILON
                                     LSU0.LDO.32 i3,  i3,  F_O_PL_STRIDE
                                     LSU0.LDO.32 i4,  i18, F_O_PL_STRIDE
                                     LSU0.LDO.32 i15, i18, F_O_SLICE_W
LSU1.LDIL i0, (255/3) || LSU0.LD.32 i8,  i8
CMU.CPIVR.x16 v8,  i0 || LSU0.LD.32 i9,  i9
nop
CMU.CPII.f32.i16s  i2,  i2
nop
nop
nop
CMU.CPIVR.x16 v7,  i2
IAU.ADD i10, i9,  i3  || LSU0.LDI.128.u8.u16 v0,  i8
IAU.ADD i11, i10, i3  || LSU0.LDI.128.u8.u16 v1,  i9
IAU.ADD i13, i12, i4  || LSU0.LDI.128.u8.u16 v2,  i10
IAU.ADD i14, i13, i4  || LSU0.LDI.128.u8.u16 v3,  i11
nop
nop
nop
VAU.IADDS.u16 v0,  v0,  v7
nop
; takeoff
                                                              VAU.IMULS.u16  v1,  v1,  v8    || LSU1.CP i4,  v0.0
                                                                                                LSU1.CP i5,  v0.1
                                                                                                LSU1.CP i6,  v0.2
                                                              LSU0.LDI.128.u8.u16 v0,  i8    || LSU1.CP i7,  v0.3
                                                              VAU.IMULS.u16  v2,  v2,  v8    || LSU1.CP i0,  v1.0
                                 SAU.DIV.u16 i0,  i0,  i4                                    || LSU1.CP i1,  v1.1
                                 SAU.DIV.u16 i1,  i1,  i5  || IAU.SUBSU    i15, i15, 0x08    || LSU1.CP i2,  v1.2
                                 SAU.DIV.u16 i2,  i2,  i6  || LSU0.LDI.128.u8.u16  v1,  i9   || LSU1.CP i3,  v1.3
                                 SAU.DIV.u16 i3,  i3,  i7  || VAU.IMULS.u16  v3,  v3,  v8    || LSU1.CP i0,  v2.0
                                 SAU.DIV.u16 i0,  i0,  i4  || BRU.BRA  svuGenChroma_Loop     || LSU1.CP i1,  v2.1
                                 SAU.DIV.u16 i1,  i1,  i5                                    || LSU1.CP i2,  v2.2
                                 SAU.DIV.u16 i2,  i2,  i6  || LSU0.LDI.128.u8.u16  v2,  i10  || LSU1.CP i3,  v2.3
                                 SAU.DIV.u16 i3,  i3,  i7                                    || LSU1.CP i0,  v3.0
VAU.IADDS.u16  v0,  v0,  v7   || SAU.DIV.u16 i0,  i0,  i4                                    || LSU1.CP i1,  v3.1
CMU.CPIV.x32 v4.0,  i0        || SAU.DIV.u16 i1,  i1,  i5                                    || LSU1.CP i2,  v3.2
CMU.CPIV.x32 v4.1,  i1        || SAU.DIV.u16 i2,  i2,  i6  || LSU0.LDI.128.u8.u16  v3,  i11  || LSU1.CP i3,  v3.3

; warp zone
.lalign
svuGenChroma_Loop:
CMU.CPIV.x32 v4.2,  i2        || SAU.DIV.u16 i3,  i3,  i7  || VAU.IMULS.u16  v1,  v1,  v8    || LSU1.CP i4,  v0.0
CMU.CPIV.x32 v4.3,  i3                                                                       || LSU1.CP i5,  v0.1
CMU.CPIV.x32 v5.0,  i0                                                                       || LSU1.CP i6,  v0.2
CMU.CPIV.x32 v5.1,  i1                                     || LSU0.LDI.128.u8.u16 v0,  i8    || LSU1.CP i7,  v0.3
CMU.CPIV.x32 v5.2,  i2                                     || VAU.IMULS.u16  v2,  v2,  v8    || LSU1.CP i0,  v1.0
CMU.CPIV.x32 v5.3,  i3        || SAU.DIV.u16 i0,  i0,  i4                                    || LSU1.CP i1,  v1.1
CMU.CPIV.x32 v6.0,  i0        || SAU.DIV.u16 i1,  i1,  i5  || IAU.SUBSU    i15, i15, 0x08    || LSU1.CP i2,  v1.2
CMU.CPIV.x32 v6.1,  i1        || SAU.DIV.u16 i2,  i2,  i6  || LSU0.LDI.128.u8.u16  v1,  i9   || LSU1.CP i3,  v1.3
CMU.CPIV.x32 v6.2,  i2        || SAU.DIV.u16 i3,  i3,  i7  || VAU.IMULS.u16  v3,  v3,  v8    || LSU1.CP i0,  v2.0
CMU.CPIV.x32 v6.3,  i3        || SAU.DIV.u16 i0,  i0,  i4  || BRU.BRA svuGenChroma_Loop      || LSU1.CP i1,  v2.1  || PEU.PCIX.NEQ 0
CMU.VSZM.BYTE v4,  v4, [Z2Z0] || SAU.DIV.u16 i1,  i1,  i5                                    || LSU1.CP i2,  v2.2  
CMU.VSZM.BYTE v5,  v5, [Z2Z0] || SAU.DIV.u16 i2,  i2,  i6  || LSU0.LDI.128.u8.u16  v2,  i10  || LSU1.CP i3,  v2.3
CMU.VSZM.BYTE v6,  v6, [Z2Z0] || SAU.DIV.u16 i3,  i3,  i7  || LSU0.STI.128.u16.u8  v4,  i12  || LSU1.CP i0,  v3.0
VAU.IADDS.u16  v0,  v0,  v7   || SAU.DIV.u16 i0,  i0,  i4  || LSU0.STI.128.u16.u8  v5,  i13  || LSU1.CP i1,  v3.1
CMU.CPIV.x32 v4.0,  i0        || SAU.DIV.u16 i1,  i1,  i5  || LSU0.STI.128.u16.u8  v6,  i14  || LSU1.CP i2,  v3.2
CMU.CPIV.x32 v4.1,  i1        || SAU.DIV.u16 i2,  i2,  i6  || LSU0.LDI.128.u8.u16  v3,  i11  || LSU1.CP i3,  v3.3

; landing
CMU.CPIV.x32 v4.2,  i2        || SAU.DIV.u16 i3,  i3,  i7
CMU.CPIV.x32 v4.3,  i3
CMU.CPIV.x32 v5.0,  i0
CMU.CPIV.x32 v5.1,  i1
CMU.CPIV.x32 v5.2,  i2
CMU.CPIV.x32 v5.3,  i3
CMU.CPIV.x32 v6.0,  i0
CMU.CPIV.x32 v6.1,  i1        || BRU.JMP i30
CMU.CPIV.x32 v6.2,  i2        
CMU.CPIV.x32 v6.3,  i3
CMU.VSZM.BYTE v4,  v4, [Z2Z0]
CMU.VSZM.BYTE v5,  v5, [Z2Z0]                              || LSU0.ST.128.u16.u8  v4,  i12
CMU.VSZM.BYTE v6,  v6, [Z2Z0]                              || LSU0.ST.128.u16.u8  v5,  i13
                                                              LSU0.ST.128.u16.u8  v6,  i14
