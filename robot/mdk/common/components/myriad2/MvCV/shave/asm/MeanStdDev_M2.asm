; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief
; ///
.version 00.70.00

.code .text.meanstddef_asm
.salign 16

;void meanstddev(u8** in, float *mean, float *stddev, u32 width)
;                    i18          i17            i16        i15
meanstddev_asm:

IAU.SUB i19 i19 4
LSU0.ST.32  i20  i19 || IAU.SUB i19 i19 4
LSU0.ST.32  i21  i19 || IAU.SUB i19 i19 4
LSU0.ST.32  i22  i19 || IAU.SUB i19 i19 4
LSU0.ST.32  i23  i19 || IAU.SUB i19 i19 4
LSU0.ST.32  i24  i19 || IAU.SUB i19 i19 4
LSU0.ST.32  i25  i19 
    ; i9- mean i13- stdev
    LSU0.LD.32 i18, i18        || LSU1.LDIL i0, 0                                            || VAU.XOR v1, v1, v1
	nop
; (Myriad2 COMPATIBILITY ISSUE): CMU.CPIS.I32.F32: SRF version no longer available at line number 23, in MeanStdDev.asm
    CMU.CPII.i32.f32 i24, i15 || LSU1.LDIL i1, 1 || IAU.ADD i6, i0, 6                       || VAU.XOR v2, v2, v2
; (Myriad2 COMPATIBILITY ISSUE): CMU.CPII.I32.F32: SRF version no longer available at line number 24, in MeanStdDev.asm
    CMU.CPII.i32.f32 i25, i1  || LSU1.LDIL i2, 2 || IAU.ADD i7, i0, 7                       || VAU.XOR v3, v3, v3

    ; contor for line loop (i15 = width)
; (Myriad2 COMPATIBILITY ISSUE): CMU.CPII.I32.F32: SRF version no longer available at line number 27, in MeanStdDev.asm
    IAU.SHR.u32 i15 i15 3     || LSU1.LDIL i3, 3 || CMU.CPII.i32.f32 i9, i0                 || VAU.XOR v4, v4, v4
; (Myriad2 COMPATIBILITY ISSUE): CMU.CPII.I32.F32: SRF version no longer available at line number 28, in MeanStdDev.asm
; (Myriad2 COMPATIBILITY ISSUE): SAU.DIV.F32: SRF version no longer available at line number 28, in MeanStdDev.asm
    SAU.DIV.f32 i24, i25, i24 || LSU1.LDIL i4, 4 || CMU.CPII.i32.f32 i13, i0                 || VAU.XOR v5, v5, v5
                                 LSU1.LDIL i5, 5 || IAU.ADD i8, i0, 8                       || VAU.XOR v6, v6, v6

                   LSU0.LD.128.U8.U16 v0, i18 || IAU.ADD i18, i18, 8 || CMU.CMII.i32 i15, i1 || VAU.XOR v7, v7, v7
    PEU.PC1C GT || LSU0.LD.128.U8.U16 v1, i18 || IAU.ADD i18, i18, 8 || CMU.CMII.i32 i15, i2
    PEU.PC1C GT || LSU0.LD.128.U8.U16 v2, i18 || IAU.ADD i18, i18, 8 || CMU.CMII.i32 i15, i3
    PEU.PC1C GT || LSU0.LD.128.U8.U16 v3, i18 || IAU.ADD i18, i18, 8 || CMU.CMII.i32 i15, i4
    PEU.PC1C GT || LSU0.LD.128.U8.U16 v4, i18 || IAU.ADD i18, i18, 8 || CMU.CMII.i32 i15, i5
    PEU.PC1C GT || LSU0.LD.128.U8.U16 v5, i18 || IAU.ADD i18, i18, 8 || CMU.CMII.i32 i15, i6
    PEU.PC1C GT || LSU0.LD.128.U8.U16 v6, i18 || IAU.ADD i18, i18, 8 || CMU.CMII.i32 i15, i7
    PEU.PC1C GT || LSU0.LD.128.U8.U16 v7, i18 || IAU.ADD i18, i18, 8
    IAU.INCS i15, -8
__mainloop:
    ; Computations for mean value ___________________ || Computations for stddev value ____________________ || Load pixel values
    SAU.SUMx.u16 i20, v0                           || VAU.IMULS.U16 v10, v0, v0
    SAU.SUMx.u16 i21, v1                           || VAU.IMULS.U16 v11, v1, v1
    SAU.SUMx.u16 i22, v2                           || VAU.IMULS.U16 v12, v2, v2
    SAU.SUMx.u16 i20, v3  || IAU.ADD i21, i20, i21 || VAU.IMULS.U16 v13, v3, v3
    SAU.SUMx.u16 i21, v4  || IAU.ADD i22, i21, i22 || VAU.IMULS.U16 v14, v4, v4
    SAU.SUMx.u16 i22, v5  || IAU.ADD i20, i22, i20 || VAU.IMULS.U16 v15, v5, v5
    SAU.SUMx.u16 i20, v6  || IAU.ADD i21, i20, i21 || VAU.IMULS.U16 v16, v6, v6
    SAU.SUMx.u16 i21, v7  || IAU.ADD i22, i21, i22 || VAU.IMULS.U16 v17, v7, v7
                                IAU.ADD i20, i22, i20 || SAU.SUMx.u16 i10, v10                          || CMU.CMII.i32 i15, i0
                                                                                                          PEU.PC1C GT || LSU0.LDI.128.U8.U16 v0, i18
                                IAU.ADD i21, i20, i21 || SAU.SUMx.u16 i11, v11                          || CMU.CMII.i32 i15, i1 || VAU.XOR v0, v0, v0
                                                                                                          PEU.PC1C GT || LSU0.LDI.128.U8.U16 v1, i18
                                                         SAU.SUMx.u16 i12, v12                          || CMU.CMII.i32 i15, i2 || VAU.XOR v1, v1, v1
                                                                                                          PEU.PC1C GT || LSU0.LDI.128.U8.U16 v2, i18
                                                         SAU.SUMx.u16 i10, v13 || IAU.ADD i11, i10, i11 || CMU.CMII.i32 i15, i3 || VAU.XOR v2, v2, v2
                                                                                                          PEU.PC1C GT || LSU0.LDI.128.U8.U16 v3, i18
                                                         SAU.SUMx.u16 i11, v14 || IAU.ADD i12, i11, i12 || CMU.CMII.i32 i15, i4 || VAU.XOR v3, v3, v3
                                                                                                          PEU.PC1C GT || LSU0.LDI.128.U8.U16 v4, i18
                                                         SAU.SUMx.u16 i12, v15 || IAU.ADD i10, i12, i10 || CMU.CMII.i32 i15, i5 || VAU.XOR v4, v4, v4
                                                                                                          PEU.PC1C GT || LSU0.LDI.128.U8.U16 v5, i18
                                                         SAU.SUMx.u16 i10, v16 || IAU.ADD i11, i10, i11 || CMU.CMII.i32 i15, i6 || VAU.XOR v5, v5, v5
                                                                                                          PEU.PC1C GT || LSU0.LDI.128.U8.U16 v6, i18
                                                         SAU.SUMx.u16 i11, v17 || IAU.ADD i12, i11, i12 || CMU.CMII.i32 i15, i7 || VAU.XOR v6, v6, v6
                                                                                                               PEU.PC1C GT || LSU0.LDI.128.U8.U16 v7, i18
                                                                                      IAU.ADD i10, i12, i10 || CMU.CMII.i32 i15, i0 || VAU.XOR v7, v7, v7
    PEU.PC1C GT || BRU.BRA __mainloop
	nop
; (Myriad2 COMPATIBILITY ISSUE): CMU.CPII.I32.F32: SRF version no longer available at line number 68, in MeanStdDev.asm
    CMU.CPII.i32.f32 i14, i21                                                       || IAU.ADD i11, i10, i11
; (Myriad2 COMPATIBILITY ISSUE): CMU.CPII.I32.F32: SRF version no longer available at line number 69, in MeanStdDev.asm
                                                         CMU.CPII.i32.f32 i23, i11                           || IAU.INCS i15, -8
; (Myriad2 COMPATIBILITY ISSUE): SAU.ADD.F32: SRF version no longer available at line number 70, in MeanStdDev.asm
    SAU.ADD.f32 i9, i9, i14
; (Myriad2 COMPATIBILITY ISSUE): SAU.ADD.F32: SRF version no longer available at line number 71, in MeanStdDev.asm
                                                         SAU.ADD.f32 i13, i13, i23
NOP

; (Myriad2 COMPATIBILITY ISSUE): SAU.MUL.F32: SRF version no longer available at line number 74, in MeanStdDev.asm
    SAU.MUL.f32 i9, i9, i24
; (Myriad2 COMPATIBILITY ISSUE): SAU.MUL.F32: SRF version no longer available at line number 75, in MeanStdDev.asm
    SAU.MUL.f32 i13, i13, i24
NOP
; (Myriad2 COMPATIBILITY ISSUE): SAU.MUL.F32: SRF version no longer available at line number 77, in MeanStdDev.asm
    SAU.MUL.f32 i14, i9, i9
NOP 2
; (Myriad2 COMPATIBILITY ISSUE): SAU.SUB.F32: SRF version no longer available at line number 79, in MeanStdDev.asm
    SAU.SUB.f32 i13, i13, i14
NOP 2
; (Myriad2 COMPATIBILITY ISSUE): CMU.CPII.F32.F16: SRF version no longer available at line number 81, in MeanStdDev.asm
    CMU.CPII.f32.f16 i13, i13
NOP
    SAU.SQT i14, i13.0
NOP 5
; (Myriad2 COMPATIBILITY ISSUE): CMU.CPII.F16L.F32: SRF version no longer available at line number 85, in MeanStdDev.asm
    CMU.CPII.f16l.f32 i13, i14
    LSU0.ST.32 i9, i17 ; Store mean
; (Myriad2 COMPATIBILITY ISSUE): LSU0.ST32: SRF version no longer available at line number 87, in MeanStdDev.asm
    LSU0.ST.32 i13, i16 ; Store stddev

LSU0.LD.32  i25  i19 || IAU.ADD i19 i19 4
LSU0.LD.32  i24  i19 || IAU.ADD i19 i19 4
LSU0.LD.32  i23  i19 || IAU.ADD i19 i19 4
LSU0.LD.32  i22  i19 || IAU.ADD i19 i19 4
LSU0.LD.32  i21  i19 || IAU.ADD i19 i19 4
LSU0.LD.32  i20  i19 || IAU.ADD i19 i19 4
nop 5
    BRU.JMP i30
NOP 5

.end
