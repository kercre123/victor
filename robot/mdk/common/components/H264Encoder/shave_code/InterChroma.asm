; next generation, dual shock inter chroma module

.lalign
InterChroma:
; chroma bypass code
;LSU0.LDIL i30, ENC_ColCycle || LSU1.LDIH i30, ENC_ColCycle
;BRU.JMP i30
;nop 5
                            IAU.SHR.u32 i_desc, i_desc, 0x01  || SAU.SUB.i32 i3, i_posx, i_rowStart || LSU0.LDO32 i8,  i_MBInfo,  mb_mvdL0       || LSU1.LDIL i_dst, origChromaU
CMU.CPVI.x32 i4, v_cmd.3 || IAU.SHL i6, i_desc, 0x03          || SAU.SHL.i32 i7, i_desc, 0x05       || LSU0.LDO32 i9,  i_MBInfo, (mb_mvdL0+0x04) || LSU1.LDIH i_dst, origChromaU
CMU.CPVI.x32 i5, v_cmd.2 || IAU.MUL i4, i4, i6                || SAU.SHR.u32 i3, i3, 0x01           || LSU0.LDO32 i10, i_MBInfo, (mb_mvdL0+0x08)
                            IAU.MUL i5, i5, i6                || SAU.SHL.i32 i6, i_desc, 0x02       || LSU0.LDO32 i11, i_MBInfo, (mb_mvdL0+0x0C) || LSU1.LDIL i2, refChromaU
CMU.CPIVR.x32 v27, i7                                         || SAU.ADD.i32 i_dst, i_dst, i3                                                    || LSU1.LDIH i2, refChromaU
CMU.CPIVR.x32 v25, i2    || IAU.ADD i4, i4, i3                || SAU.ADD.i32 i7, i7, i2
                            IAU.ADD i4, i4, i2                || SAU.ADD.i32 i_dst, i_dst, i5       || LSU1.LDIL i_src, sampleStore
CMU.CPII.i16.i32 i0, i8  || IAU.FEXT.i32 i12, i8,  0x13, 0xD  || SAU.ADD.i32 i6, i6, i4             || LSU1.LDIH i_src, sampleStore
CMU.CPII.i16.i32 i1, i9  || IAU.FEXT.i32 i13, i9,  0x13, 0xD  || SAU.SHR.i32 i0, i0, 0x03           || LSU0.LDI64.l v0,  i_dst, i_desc
CMU.CPII.i16.i32 i2, i10 || IAU.FEXT.i32 i14, i10, 0x13, 0xD  || SAU.SHR.i32 i1, i1, 0x03           || LSU0.LDI64.l v1,  i_dst, i_desc
CMU.CPII.i16.i32 i3, i11 || IAU.FEXT.i32 i15, i11, 0x13, 0xD  || SAU.SHR.i32 i2, i2, 0x03           || LSU0.LDI64.l v2,  i_dst, i_desc
                            IAU.MUL i12, i12, i_desc          || SAU.SHR.i32 i3, i3, 0x03           || LSU0.LDI64.l v3,  i_dst, i_desc
                            IAU.MUL i13, i13, i_desc          || SAU.ADD.i32 i5, i4, 0x04           || LSU0.LDI64.l v4,  i_dst, i_desc
                            IAU.MUL i14, i14, i_desc          || SAU.ADD.i32 i4, i4, i0             || LSU0.LDI64.l v5,  i_dst, i_desc
                            IAU.MUL i15, i15, i_desc          || SAU.ADD.i32 i5, i5, i1             || LSU0.LDI64.l v6,  i_dst, i_desc
CMU.CPIVR.x32 v26, i7                                         || SAU.ADD.i32 i7, i6, 0x04           || LSU0.LDI64.l v7,  i_dst, i_desc
                            IAU.ADD i4, i4, i12               || SAU.ADD.i32 i6, i6, i2             || LSU0.STI64.l v0,  i_src
CMU.CPIV.x32 v28.0, i4   || IAU.ADD i5, i5, i13               || SAU.ADD.i32 i7, i7, i3             || LSU0.STI64.l v1,  i_src
CMU.CPIV.x32 v28.1, i5   || IAU.ADD i6, i6, i14                                                     || LSU0.STI64.l v2,  i_src
CMU.CPIV.x32 v28.2, i6   || IAU.ADD i7, i7, i15                                                     || LSU0.STI64.l v3,  i_src
CMU.CPIV.x32 v28.3, i7   || IAU.SHL i0, i_desc, 0x03                                                || LSU0.STI64.l v4,  i_src
CMU.CMVV.i32 v28, v25    || IAU.SUB i_dst, i_dst, i0                                                || LSU0.STI64.l v5,  i_src
PEU.PVV32 0x4            || VAU.ADD.i32 v28, v28, v27                                               || LSU0.STI64.l v6,  i_src
CMU.CMVV.i32 v28, v26    || IAU.INCS i_dst, (origChromaV-origChromaU)                               || LSU0.STI64.l v7,  i_src
PEU.PVV32 0x3            || VAU.SUB.i32 v28, v28, v27                                               || LSU0.LDI64.h v0,  i_dst, i_desc
                    CMU.CPSI i18, s_UVQAddr                                                         || LSU0.LDI64.h v1,  i_dst, i_desc
                    CMU.CPVI.x32 i4, v28.0     || IAU.INCS i18, 0x200                               || LSU0.LDI64.h v2,  i_dst, i_desc
                    CMU.CPVI.x32 i2, v26.0     || IAU.ADD i5, i4, i_desc                            || LSU0.LDI64.h v3,  i_dst, i_desc
                    CMU.CPVI.x32 i1, v27.0     || IAU.ADD i6, i5, i_desc                            || LSU0.LDI64.h v4,  i_dst, i_desc
CMU.CPVV.u8.u16 v_Residual0, v0                || IAU.ADD i7, i6, i_desc                            || LSU0.LDI64.h v5,  i_dst, i_desc
CMU.CPVV.u8.u16 v_Residual1, v1                || IAU.SUB.u32s i0, i2, i5                           || LSU0.LDI64.h v6,  i_dst, i_desc
PEU.PCIX.EQ 0x02                               || IAU.SUB.u32s i0, i2, i6 || SAU.SUB.i32 i5, i5, i1 || LSU0.LDI64.h v7,  i_dst, i_desc
PEU.PCIX.EQ 0x02                               || IAU.SUB.u32s i0, i2, i7 || SAU.SUB.i32 i6, i6, i1 || LSU0.LD32 i_flag, i4
PEU.PCIX.EQ 0x02                                                          || SAU.SUB.i32 i7, i7, i1 || LSU0.LD32 i_flag, i5 || LSU1.LDIL i0, (refChromaV-refChromaU)
                                                  IAU.ADD i4, i4, i0                                || LSU0.LD32 i_flag, i6
CMU.CPVV.u8.u16 v_Residual2, v2                || IAU.ADD i5, i5, i0                                || LSU0.LD32 i_flag, i7
CMU.CPVV.u8.u16 v_Residual3, v3                || IAU.ADD i6, i6, i0                                || LSU0.LD32 i_flag, i4
                    CMU.CPVI.x32 i4, v28.1     || IAU.ADD i7, i7, i0                                || LSU0.LD32 i_flag, i5
                    CMU.CPIV.x32 v8.0,  i_flag || IAU.ADD i5, i4, i_desc                            || LSU0.LD32 i_flag, i6
                    CMU.CPIV.x32 v9.0,  i_flag || IAU.ADD i6, i5, i_desc                            || LSU0.LD32 i_flag, i7
                    CMU.CPIV.x32 v10.0, i_flag || IAU.ADD i7, i6, i_desc                            || LSU0.STI64.h v0,  i_src
                    CMU.CPIV.x32 v11.0, i_flag || IAU.SUB.u32s i0, i2, i5                           || LSU0.STI64.h v1,  i_src
PEU.PCIX.EQ 0x02 || CMU.CPIV.x32 v8.2,  i_flag || IAU.SUB.u32s i0, i2, i6 || SAU.SUB.i32 i5, i5, i1 || LSU0.LDI64.l v_QParam, i18
PEU.PCIX.EQ 0x02 || CMU.CPIV.x32 v9.2,  i_flag || IAU.SUB.u32s i0, i2, i7 || SAU.SUB.i32 i6, i6, i1 || LSU0.LD32 i_flag, i4
PEU.PCIX.EQ 0x02 || CMU.CPIV.x32 v10.2, i_flag                            || SAU.SUB.i32 i7, i7, i1 || LSU0.LD32 i_flag, i5 || LSU1.LDIL i0, (refChromaV-refChromaU)
                    CMU.CPIV.x32 v11.2, i_flag || IAU.ADD i4, i4, i0                                || LSU0.LD32 i_flag, i6
                                                  IAU.ADD i5, i5, i0                                || LSU0.LD32 i_flag, i7
                                                  IAU.ADD i6, i6, i0                                || LSU0.LD32 i_flag, i4
                    CMU.CPVI.x32 i4, v28.2     || IAU.ADD i7, i7, i0                                || LSU0.LD32 i_flag, i5
                    CMU.CPIV.x32 v8.1,  i_flag || IAU.ADD i5, i4, i_desc                            || LSU0.LD32 i_flag, i6
                    CMU.CPIV.x32 v9.1,  i_flag || IAU.ADD i6, i5, i_desc                            || LSU0.LD32 i_flag, i7
                    CMU.CPIV.x32 v10.1, i_flag || IAU.ADD i7, i6, i_desc                            || LSU0.STI64.h v2,  i_src
                    CMU.CPIV.x32 v11.1, i_flag || IAU.SUB.u32s i0, i2, i5                           || LSU0.STI64.h v3,  i_src
PEU.PCIX.EQ 0x02 || CMU.CPIV.x32 v8.3,  i_flag || IAU.SUB.u32s i0, i2, i6 || SAU.SUB.i32 i5, i5, i1 || LSU0.LDI64.h v_QParam, i18
PEU.PCIX.EQ 0x02 || CMU.CPIV.x32 v9.3,  i_flag || IAU.SUB.u32s i0, i2, i7 || SAU.SUB.i32 i6, i6, i1 || LSU0.LD32 i_flag, i4
PEU.PCIX.EQ 0x02 || CMU.CPIV.x32 v10.3, i_flag                            || SAU.SUB.i32 i7, i7, i1 || LSU0.LD32 i_flag, i5 || LSU1.LDIL i0, (refChromaV-refChromaU)
                    CMU.CPIV.x32 v11.3, i_flag || IAU.ADD i4, i4, i0                                || LSU0.LD32 i_flag, i6
                    CMU.CPVV.u8.u16 v24, v8    || IAU.ADD i5, i5, i0                                || LSU0.LD32 i_flag, i7
                    CMU.CPVV.u8.u16 v25, v9    || IAU.ADD i6, i6, i0                                || LSU0.LD32 i_flag, i4
                    CMU.CPVI.x32 i4, v28.3     || IAU.ADD i7, i7, i0                                || LSU0.LD32 i_flag, i5
                    CMU.CPIV.x32 v12.0, i_flag || IAU.ADD i5, i4, i_desc                            || LSU0.LD32 i_flag, i6
                    CMU.CPIV.x32 v13.0, i_flag || IAU.ADD i6, i5, i_desc                            || LSU0.LD32 i_flag, i7
                    CMU.CPIV.x32 v14.0, i_flag || IAU.ADD i7, i6, i_desc                            || LSU0.STI64.h v4,  i_src
                    CMU.CPIV.x32 v15.0, i_flag || IAU.SUB.u32s i0, i2, i5                           || LSU0.STI64.h v5,  i_src
PEU.PCIX.EQ 0x02 || CMU.CPIV.x32 v12.2, i_flag || IAU.SUB.u32s i0, i2, i6 || SAU.SUB.i32 i5, i5, i1
PEU.PCIX.EQ 0x02 || CMU.CPIV.x32 v13.2, i_flag || IAU.SUB.u32s i0, i2, i7 || SAU.SUB.i32 i6, i6, i1 || LSU0.LD32 i_flag, i4
PEU.PCIX.EQ 0x02 || CMU.CPIV.x32 v14.2, i_flag                            || SAU.SUB.i32 i7, i7, i1 || LSU0.LD32 i_flag, i5 || LSU1.LDIL i0, (refChromaV-refChromaU)
                    CMU.CPIV.x32 v15.2, i_flag || IAU.ADD i4, i4, i0                                || LSU0.LD32 i_flag, i6
                    CMU.CPVV.u8.u16 v26, v10   || IAU.ADD i5, i5, i0                                || LSU0.LD32 i_flag, i7
                    CMU.CPVV.u8.u16 v27, v11   || IAU.ADD i6, i6, i0                                || LSU0.LD32 i_flag, i4
                                                  IAU.ADD i7, i7, i0                                || LSU0.LD32 i_flag, i5
                    CMU.CPIV.x32 v12.1, i_flag                                                      || LSU0.LD32 i_flag, i6
                    CMU.CPIV.x32 v13.1, i_flag                                                      || LSU0.LD32 i_flag, i7
                    CMU.CPIV.x32 v14.1, i_flag || IAU.INCS i_src, 0x08                              || LSU0.ST64.h  v6,  i_src
                    CMU.CPIV.x32 v15.1, i_flag || IAU.INCS i_src, 0x88                              || LSU0.ST64.h  v7,  i_src
PEU.PCIX.EQ 0x02 || CMU.CPIV.x32 v12.3, i_flag                                                      || LSU0.STI64.l v8,  i_src
PEU.PCIX.EQ 0x02 || CMU.CPIV.x32 v13.3, i_flag                                                      || LSU0.STI64.l v9,  i_src
PEU.PCIX.EQ 0x02 || CMU.CPIV.x32 v14.3, i_flag || VAU.SUB.i16 v_Residual0, v_Residual0, v24         || LSU0.STI64.l v10, i_src
                    CMU.CPIV.x32 v15.3, i_flag || VAU.SUB.i16 v_Residual1, v_Residual1, v25         || LSU0.STI64.l v11, i_src
                                                  VAU.SUB.i16 v_Residual2, v_Residual2, v26         || LSU0.STI64.l v12, i_src
                                                  VAU.SUB.i16 v_Residual3, v_Residual3, v27         || LSU0.STI64.l v13, i_src
                                                  VAU.ADD.i16 v0, v_Residual0, 0                    || LSU0.STI64.l v14, i_src
                                                  VAU.ADD.i16 v1, v_Residual2, 0                    || LSU0.STI64.l v15, i_src
                                                  VAU.ADD.i16 v2, v_Residual1, 0                    || LSU0.STI64.h v8,  i_src
                                                  VAU.ADD.i16 v3, v_Residual3, 0                    || LSU0.STI64.h v9,  i_src
CMU.VSZMWORD v0, v_Residual1, [10DD]           || BRU.BRA forward_4x4                               || LSU0.STI64.h v10, i_src
CMU.VSZMWORD v1, v_Residual3, [10DD]           || IAU.XOR i_smb, i_smb, i_smb                       || LSU0.STI64.h v11, i_src
CMU.VSZMWORD v2, v_Residual0, [DD32]           || LSU0.STI64.h v12, i_src || LSU1.LDIL i_dst, coeffStore
CMU.VSZMWORD v3, v_Residual2, [DD32]           || LSU0.STI64.h v13, i_src || LSU1.LDIH i_dst, coeffStore
CMU.VILV.x16 v_Residual0, v_Residual1, v2, v0  || LSU0.STI64.h v14, i_src || LSU1.LDIL i30, IntraChroma_Store
CMU.VILV.x16 v_Residual2, v_Residual3, v3, v1  || LSU0.STI64.h v15, i_src || LSU1.LDIH i30, IntraChroma_Store
