;-------------------------------------------------------------------------------
.version 00.51.05
; atb, 05 sep 2013
; note: yet another asm module
;------------------------------------------------------------------------------- 
.include "myriad2SippDefs.inc"

.code svuGenDnsRef

.set P_O_LUT_GAMMA             0
.set P_O_LUT_DIST              4
.set P_O_SHIFT                 8

.lalign 
; i18 : pointer to SippFilter
; i17 : svuNo
; i16 : runNo
.lalign
svuGenDnsRef:
IAU.FEXTU i0, i16, 0x00, 0x01    || LSU0.LDO.32 i6,  i18, F_O_PARAMS
IAU.SHL  i0,  i0,  0x02          || LSU0.LDO.32 i7,  i18, F_O_COMM_INFO
IAU.ADD  i0,  i0,  i18           || LSU0.LDO.32 i2,  i18, F_O_OUT_W  || LSU1.LDIL i1, F_O_LINES_I
IAU.ADD  i13, i0,  i1            || LSU0.LDO.32 i3,  i18, F_O_OUT_H  || LSU1.LDIL i1, F_O_LINES_O
IAU.ADD  i12, i0,  i1            || LSU1.LDO.32 i4,  i18, F_O_SLICE_W
LSU0.LD.32  i13, i13
LSU1.LD.32  i12, i12
LSU0.LDO.32 i0,  i6,  P_O_SHIFT
LSU1.LDO.32 i0,  i6,  P_O_LUT_DIST
LSU0.LDO.32 i0,  i6,  P_O_LUT_GAMMA
LSU1.LDO.32 i7,  i7,  G_O_FIRST_S
nop
LSU0.LD.32  i13, i13
CMU.CPII   i14, i4
CMU.CPIVR.x32 v6,  i0
CMU.CPIVR.x32 v7,  i0 || IAU.SHR.u32 i2,  i2, 0x01
CMU.CPIVR.x32 v8,  i0 || IAU.SHR.u32 i3,  i3, 0x01

; calculate x0
IAU.SUB     i7,  i17, i7 
IAU.MUL     i1,  i7,  i4
nop
IAU.SUB     i1,  i1,  i2
CMU.CPIV.x32 v1.0,  i1  || IAU.ADD i1,  i1,  0x01
CMU.CPIV.x32 v1.1,  i1  || IAU.ADD i1,  i1,  0x01
CMU.CPIV.x32 v1.2,  i1  || IAU.ADD i1,  i1,  0x01
CMU.CPIV.x32 v1.3,  i1
; calculate yc*yc
IAU.SUB     i3,  i16, i3
IAU.MUL     i3,  i3,  i3
LSU1.LD.128.u8.u16 v0, i13
CMU.CPIVR.x32 v3,  i3

; take off
                                  VAU.MUL.i32   v2,  v1,  v1
                                  IAU.ADD       i13, i13, 0x04
nop        
nop
                                  VAU.IADDS.u32 v4,  v3,  v2
CMU.CPVV.i16.i32 v5,  v0
                                  VAU.SHR.u32   v4,  v4,  v6
                                  VAU.ADD.i32   v1,  v1,  0x04
                                  VAU.IADDS.u32 v4,  v4,  v7
                                  VAU.IADDS.u32 v5,  v5,  v8                             

CMU.CPVI.x32 i0,  v4.0                                            || LSU0.LD.128.u8.u16 v0,  i13
CMU.CPVI.x32 i1,  v4.1                                            || LSU1.LD.32.u8.u32  i0,  i0
CMU.CPVI.x32 i2,  v4.2                                            || LSU0.LD.32.u8.u32  i1,  i1
CMU.CPVI.x32 i3,  v4.3                                            || LSU1.LD.32.u8.u32  i2,  i2
CMU.CPVI.x32 i4,  v5.0         || VAU.MUL.i32   v2,  v1,  v1      || LSU0.LD.32.u8.u32  i3,  i3
CMU.CPVI.x32 i5,  v5.1         || IAU.ADD       i13, i13, 0x04    || LSU1.LD.32.u8.u32  i4,  i4
CMU.CPVI.x32 i6,  v5.2         || IAU.SUBSU     i14, i14, 0x04    || LSU0.LD.32.u8.u32  i5,  i5
CMU.CPVI.x32 i7,  v5.3         || BRU.BRA svuGenDnsRef_Loop       || LSU1.LD.32.u8.u32  i6,  i6
CMU.CPVV.i16.i32  v5,  v0      || VAU.IADDS.u32 v4,  v3,  v2      || LSU0.LD.32.u8.u32  i7,  i7 
CMU.VSZM.BYTE i8,  i8,  [ZZZ1] 
CMU.VSZM.BYTE i8,  i9,  [ZZ1D] || VAU.SHR.u32   v4,  v4,  v6
CMU.VSZM.BYTE i8,  i10, [Z1DD] || VAU.ADD.i32   v1,  v1,  0x04
CMU.VSZM.BYTE i8,  i11, [1DDD] || VAU.IADDS.u32 v4,  v4,  v7
                                  VAU.IADDS.u32 v5,  v5,  v8

; warp zone
.lalign
svuGenDnsRef_Loop:
CMU.CPVI.x32 i0,  v4.0         || IAU.MUL       i8,  i4,  i0      || LSU1.LD.128.u8.u16 v0,  i13
CMU.CPVI.x32 i1,  v4.1         || IAU.MUL       i9,  i5,  i1      || LSU0.LD.32.u8.u32  i0,  i0
CMU.CPVI.x32 i2,  v4.2         || IAU.MUL       i10, i6,  i2      || LSU1.LD.32.u8.u32  i1,  i1
CMU.CPVI.x32 i3,  v4.3         || IAU.MUL       i11, i7,  i3      || LSU0.LD.32.u8.u32  i2,  i2
CMU.CPVI.x32 i4,  v5.0         || VAU.MUL.i32   v2,  v1,  v1      || LSU1.LD.32.u8.u32  i3,  i3
CMU.CPVI.x32 i5,  v5.1         || IAU.ADD       i13, i13, 0x04    || LSU0.LD.32.u8.u32  i4,  i4
CMU.CPVI.x32 i6,  v5.2         || IAU.SUBSU     i14, i14, 0x04    || LSU1.LD.32.u8.u32  i5,  i5
CMU.CPVI.x32 i7,  v5.3         || BRU.BRA svuGenDnsRef_Loop       || LSU0.LD.32.u8.u32  i6,  i6   || PEU.PCIX.NEQ 0
CMU.CPVV.i16.i32  v5,  v0      || VAU.IADDS.u32 v4,  v3,  v2      || LSU1.LD.32.u8.u32  i7,  i7 
CMU.VSZM.BYTE i8,  i8,  [ZZZ1] 
CMU.VSZM.BYTE i8,  i9,  [ZZ1D] || VAU.SHR.u32   v4,  v4,  v6
CMU.VSZM.BYTE i8,  i10, [Z1DD] || VAU.ADD.i32   v1,  v1,  0x04
CMU.VSZM.BYTE i8,  i11, [1DDD] || VAU.IADDS.u32 v4,  v4,  v7
                                  VAU.IADDS.u32 v5,  v5,  v8      || LSU0.STI.32   i8,  i12
                                  
; landing
                                  IAU.MUL       i8,  i4,  i0
                                  IAU.MUL       i9,  i5,  i1
BRU.JMP i30                    || IAU.MUL       i10, i6,  i2
                                  IAU.MUL       i11, i7,  i3
CMU.VSZM.BYTE i8,  i8,  [ZZZ1]
CMU.VSZM.BYTE i8,  i9,  [ZZ1D]
CMU.VSZM.BYTE i8,  i10, [Z1DD]
CMU.VSZM.BYTE i8,  i11, [1DDD]
LSU0.ST.32    i8,  i12

.end
