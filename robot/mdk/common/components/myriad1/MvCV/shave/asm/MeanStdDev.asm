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
    ; s0- mean s1- stdev
    LSU0.LD32 i18, i18        || LSU1.LDIL i0, 0                                            || VAU.XOR v1, v1, v1
    CMU.CPIS.i32.f32 s15, i15 || LSU1.LDIL i1, 1 || IAU.ADD i6, i0, 6                       || VAU.XOR v2, v2, v2
    CMU.CPIS.i32.f32 s16, i1  || LSU1.LDIL i2, 2 || IAU.ADD i7, i0, 7                       || VAU.XOR v3, v3, v3

    ; contor for line loop (i15 = width)
    IAU.SHR.u32 i15 i15 3     || LSU1.LDIL i3, 3 || CMU.CPIS.i32.f32 s0, i0                 || VAU.XOR v4, v4, v4
    SAU.DIV.f32 s15, s16, s15 || LSU1.LDIL i4, 4 || CMU.CPIS.i32.f32 s1, i0                 || VAU.XOR v5, v5, v5
                                 LSU1.LDIL i5, 5 || IAU.ADD i8, i0, 8                       || VAU.XOR v6, v6, v6

                   LSU0.LD128.u8.u16 v0, i18 || IAU.ADD i18, i18, 8 || CMU.CMII.i32 i15, i1 || VAU.XOR v7, v7, v7
    PEU.PC1C GT || LSU0.LD128.u8.u16 v1, i18 || IAU.ADD i18, i18, 8 || CMU.CMII.i32 i15, i2
    PEU.PC1C GT || LSU0.LD128.u8.u16 v2, i18 || IAU.ADD i18, i18, 8 || CMU.CMII.i32 i15, i3
    PEU.PC1C GT || LSU0.LD128.u8.u16 v3, i18 || IAU.ADD i18, i18, 8 || CMU.CMII.i32 i15, i4
    PEU.PC1C GT || LSU0.LD128.u8.u16 v4, i18 || IAU.ADD i18, i18, 8 || CMU.CMII.i32 i15, i5
    PEU.PC1C GT || LSU0.LD128.u8.u16 v5, i18 || IAU.ADD i18, i18, 8 || CMU.CMII.i32 i15, i6
    PEU.PC1C GT || LSU0.LD128.u8.u16 v6, i18 || IAU.ADD i18, i18, 8 || CMU.CMII.i32 i15, i7
    PEU.PC1C GT || LSU0.LD128.u8.u16 v7, i18 || IAU.ADD i18, i18, 8
    IAU.INCS i15, -8
__mainloop:
    ; Computations for mean value ___________________ || Computations for stddev value ____________________ || Load pixel values
    SAU.SUM.u16 i20, v0, 0xF                          || VAU.MUL.u16s v10, v0, v0
    SAU.SUM.u16 i21, v1, 0xF                          || VAU.MUL.u16s v11, v1, v1
    SAU.SUM.u16 i22, v2, 0xF                          || VAU.MUL.u16s v12, v2, v2
    SAU.SUM.u16 i20, v3, 0xF || IAU.ADD i21, i20, i21 || VAU.MUL.u16s v13, v3, v3
    SAU.SUM.u16 i21, v4, 0xF || IAU.ADD i22, i21, i22 || VAU.MUL.u16s v14, v4, v4
    SAU.SUM.u16 i22, v5, 0xF || IAU.ADD i20, i22, i20 || VAU.MUL.u16s v15, v5, v5
    SAU.SUM.u16 i20, v6, 0xF || IAU.ADD i21, i20, i21 || VAU.MUL.u16s v16, v6, v6
    SAU.SUM.u16 i21, v7, 0xF || IAU.ADD i22, i21, i22 || VAU.MUL.u16s v17, v7, v7
                                IAU.ADD i20, i22, i20 || SAU.SUM.u16 i10, v10, 0xF                          || CMU.CMII.i32 i15, i0
                                                                                                               PEU.PC1C GT || LSU0.LDI128.u8.u16 v0, i18
                                IAU.ADD i21, i20, i21 || SAU.SUM.u16 i11, v11, 0xF                          || CMU.CMII.i32 i15, i1 || VAU.XOR v0, v0, v0
                                                                                                               PEU.PC1C GT || LSU0.LDI128.u8.u16 v1, i18
                                                         SAU.SUM.u16 i12, v12, 0xF                          || CMU.CMII.i32 i15, i2 || VAU.XOR v1, v1, v1
                                                                                                               PEU.PC1C GT || LSU0.LDI128.u8.u16 v2, i18
                                                         SAU.SUM.u16 i10, v13, 0xF || IAU.ADD i11, i10, i11 || CMU.CMII.i32 i15, i3 || VAU.XOR v2, v2, v2
                                                                                                               PEU.PC1C GT || LSU0.LDI128.u8.u16 v3, i18
                                                         SAU.SUM.u16 i11, v14, 0xF || IAU.ADD i12, i11, i12 || CMU.CMII.i32 i15, i4 || VAU.XOR v3, v3, v3
                                                                                                               PEU.PC1C GT || LSU0.LDI128.u8.u16 v4, i18
                                                         SAU.SUM.u16 i12, v15, 0xF || IAU.ADD i10, i12, i10 || CMU.CMII.i32 i15, i5 || VAU.XOR v4, v4, v4
                                                                                                               PEU.PC1C GT || LSU0.LDI128.u8.u16 v5, i18
                                                         SAU.SUM.u16 i10, v16, 0xF || IAU.ADD i11, i10, i11 || CMU.CMII.i32 i15, i6 || VAU.XOR v5, v5, v5
                                                                                                               PEU.PC1C GT || LSU0.LDI128.u8.u16 v6, i18
                                                         SAU.SUM.u16 i11, v17, 0xF || IAU.ADD i12, i11, i12 || CMU.CMII.i32 i15, i7 || VAU.XOR v6, v6, v6
                                                                                                               PEU.PC1C GT || LSU0.LDI128.u8.u16 v7, i18
                                                                                      IAU.ADD i10, i12, i10 || CMU.CMII.i32 i15, i0 || VAU.XOR v7, v7, v7
    PEU.PC1C GT || BRU.BRA __mainloop
    CMU.CPIS.i32.f32 s2, i21                                                       || IAU.ADD i11, i10, i11
                                                         CMU.CPIS.i32.f32 s3, i11                           || IAU.INCS i15, -8
    SAU.ADD.f32 s0, s0, s2
                                                         SAU.ADD.f32 s1, s1, s3
    NOP

    SAU.MUL.f32 s0, s0, s15
    SAU.MUL.f32 s1, s1, s15
    NOP
    SAU.MUL.f32 s2, s0, s0
    NOP 2
    SAU.SUB.f32 s1, s1, s2
    NOP 2
    CMU.CPSS.f32.f16 s1, s1
    NOP
    SAU.SQT.f16.l s2, s1
    NOP 5
    CMU.CPSS.f16l.f32 s1, s2
    LSU0.ST32 s0, i17 ; Store mean
    LSU0.ST32 s1, i16 ; Store stddev

    BRU.JMP i30
    NOP 5

.end

