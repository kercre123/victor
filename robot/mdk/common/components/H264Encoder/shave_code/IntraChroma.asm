; next generation, dual shock intra chroma module

.lalign
IntraChroma:
                             IAU.SHR.u32 i_desc, i_desc, 0x01 || LSU0.LD32  i10, i_dst         || LSU1.LDIL i8, predChromaH
CMU.CMZ.i32 i_posx        || IAU.SUB i0, i_posx, i_rowStart   || LSU0.LDO32 i11, i_dst, 0x08   || LSU1.LDIH i8, predChromaH
PEU.PCCX.NEQ 0x10         || IAU.ADD i8, i8, i0               || LSU0.LDA64.l v_Left, 0, predChromaV_lo
PEU.PCCX.NEQ 0x10         || CMU.CMZ.i32 i_posy               || LSU0.LDA64.h v_Left, 0, predChromaV_hi || LSU1.LDIL i0, 0x80
PEU.PCCX.NEQ 0x10         || CMU.CPIVR.x8 v_Left, i0          || LSU0.LDI64.l v_Top, i8
PEU.PCCX.NEQ 0x10         || CMU.CPIVR.x8 v_Top,  i0          || LSU0.LDI64.h v_Top, i8
                                                                 LSU0.LDI64.l v0,  i10, i_desc
CMU.CPSI i18, s_UVQAddr                                       || LSU0.LDI64.l v1,  i10, i_desc
                                                                 LSU0.LDI64.l v2,  i10, i_desc || LSU1.LDIL i_src, sampleStore
                                                                 LSU0.LDI64.l v3,  i10, i_desc || LSU1.LDIH i_src, sampleStore
CMU.CMZ.i32 i_posx                                            || LSU0.LDI64.l v4,  i10, i_desc || LSU1.LDIL i_dst, coeffStore
PEU.PCCX.EQ 0x04                || CMU.CPVV v_Left, v_Top     || LSU0.LDI64.l v5,  i10, i_desc || LSU1.LDIH i_dst, coeffStore
CMU.CMZ.i32 i_posy                                            || LSU0.LDI64.l v6,  i10, i_desc
PEU.PCCX.EQ 0x04                || CMU.CPVV v_Top,  v_Left    || LSU0.LDI64.l v7,  i10, i_desc
CMU.CPVV.u8.u16 v_Residual0, v0 || SAU.SUM.u8 i2, v_Left, 0x1 || LSU1.LDI64.h v0,  i11, i_desc || LSU0.STI64.l v0,  i_src
CMU.CPVV.u8.u16 v_Residual1, v1 || SAU.SUM.u8 i3, v_Left, 0x2 || LSU1.LDI64.h v1,  i11, i_desc || LSU0.STI64.l v1,  i_src
IAU.INCS i2, 0x02               || SAU.SUM.u8 i4, v_Left, 0x4 || LSU1.LDI64.h v2,  i11, i_desc || LSU0.STI64.l v2,  i_src
IAU.INCS i3, 0x02               || SAU.SUM.u8 i5, v_Left, 0x8 || LSU1.LDI64.h v3,  i11, i_desc || LSU0.STI64.l v3,  i_src
IAU.INCS i4, 0x02               || SAU.SUM.u8 i6, v_Top,  0x1 || LSU1.LDI64.h v4,  i11, i_desc || LSU0.STI64.l v4,  i_src
IAU.INCS i5, 0x02               || SAU.SUM.u8 i7, v_Top,  0x2 || LSU1.LDI64.h v5,  i11, i_desc || LSU0.STI64.l v5,  i_src
IAU.INCS i6, 0x02               || SAU.SUM.u8 i8, v_Top,  0x4 || LSU1.LDI64.h v6,  i11, i_desc || LSU0.STI64.l v6,  i_src
IAU.INCS i7, 0x02               || SAU.SUM.u8 i9, v_Top,  0x8 || LSU1.LDI64.h v7,  i11, i_desc || LSU0.STI64.l v7,  i_src
CMU.CPVV.u8.u16 v_Residual2, v2 || IAU.INCS i8, 0x02                                           || LSU0.STI64.h v0,  i_src
CMU.CPVV.u8.u16 v_Residual3, v3 || IAU.INCS i9, 0x02                                           || LSU0.STI64.h v1,  i_src
CMU.CMZ.i32 i_posy              || IAU.ADD  i12, i2, i6       || SAU.SHR.u32 i13, i7, 0x02     || LSU0.STI64.h v2,  i_src
PEU.PCCX.EQ 0x02                || IAU.ADD  i15, i3, i7       || SAU.SHR.u32 i13, i2, 0x02     || LSU0.STI64.h v3,  i_src
CMU.CMZ.i32 i_posx              || IAU.SHR.u32 i12, i12, 0x03 || SAU.SHR.u32 i14, i3, 0x02     || LSU0.STI64.h v4,  i_src
PEU.PCCX.EQ 0x02                || IAU.SHR.u32 i15, i15, 0x03 || SAU.SHR.u32 i14, i6, 0x02     || LSU0.STI64.h v5,  i_src
CMU.CPIV.x32 v24.0, i12                                                                        || LSU0.STI64.h v6,  i_src
CMU.CPIV.x32 v24.1, i13                                                                        || LSU0.STI64.h v7,  i_src
CMU.VSZMBYTE v24, v24, [0000]                                                                  || LSU0.LDI64.l v_QParam, i18
CMU.CPVV.u8.u16 v28, v24        || IAU.INCS i_src, 0x80                                        || LSU0.LDI64.h v_QParam, i18
CMU.CPIV.x32 v25.0, i14         || VAU.SUB.i16 v_Residual0, v_Residual0, v28                   || LSU0.STI64.l v24, i_src
CMU.CPIV.x32 v25.1, i15         || VAU.SUB.i16 v_Residual1, v_Residual1, v28                   || LSU0.STI64.l v24, i_src
CMU.VSZMBYTE v25, v25, [0000]                                                                  || LSU0.STI64.l v24, i_src
CMU.CMZ.i32 i_posy              || IAU.ADD  i12, i4, i8       || SAU.SHR.u32 i13, i9, 0x02     || LSU0.STI64.l v24, i_src
PEU.PCCX.EQ 0x02                || IAU.ADD  i15, i5, i9       || SAU.SHR.u32 i13, i4, 0x02     || LSU0.STI64.l v25, i_src
CMU.CMZ.i32 i_posx              || IAU.SHR.u32 i12, i12, 0x03 || SAU.SHR.u32 i14, i5, 0x02     || LSU0.STI64.l v25, i_src
PEU.PCCX.EQ 0x02                || IAU.SHR.u32 i15, i15, 0x03 || SAU.SHR.u32 i14, i8, 0x02     || LSU0.STI64.l v25, i_src
CMU.CPIV.x32 v26.0, i12         || VAU.SUB.i16 v_Residual2, v_Residual2, v28                   || LSU0.STI64.l v25, i_src
CMU.CPIV.x32 v26.1, i13         || VAU.SUB.i16 v_Residual3, v_Residual3, v28
CMU.VSZMBYTE v26, v26, [0000]   || VAU.ADD.i16 v0, v_Residual0, 0
CMU.CPIV.x32 v27.0, i14         || VAU.ADD.i16 v1, v_Residual2, 0                              || LSU0.STI64.l v26, i_src
CMU.CPIV.x32 v27.1, i15         || VAU.ADD.i16 v2, v_Residual1, 0                              || LSU0.STI64.l v26, i_src
CMU.VSZMBYTE v27, v27, [0000]   || VAU.ADD.i16 v3, v_Residual3, 0                              || LSU0.STI64.l v26, i_src
CMU.VSZMWORD v0, v_Residual1, [10DD] || BRU.BRA forward_4x4                                    || LSU0.STI64.l v26, i_src
CMU.VSZMWORD v1, v_Residual3, [10DD] || IAU.XOR i_smb, i_smb, i_smb                            || LSU0.STI64.l v27, i_src
CMU.VSZMWORD v2, v_Residual0, [DD32]                                                           || LSU0.STI64.l v27, i_src
CMU.VSZMWORD v3, v_Residual2, [DD32]                                                           || LSU0.STI64.l v27, i_src
CMU.VILV.x16 v_Residual0, v_Residual1, v2, v0                                                  || LSU0.STI64.l v27, i_src  || LSU1.LDIL i30, IntraChroma_Store
CMU.VILV.x16 v_Residual2, v_Residual3, v3, v1                                                                              || LSU1.LDIH i30, IntraChroma_Store

.lalign
IntraChroma_Store:
CMU.VSZMWORD v_Coeff0, v_Coeff0, [321Z]                                                                                        || LSU1.LDIL i1, 0x20
CMU.VDILV.x16 v6, v4, v_Coeff0, v_Coeff1       || IAU.INCS i_smb, 0x02        || VAU.SUB.i16 v_Residual0, v_Residual0, v_Pred0 || LSU1.LDIL i30, forward_4x4
CMU.VDILV.x16 v7, v5, v_Coeff2, v_Coeff3       || IAU.CMPI i_smb, 0x08        || VAU.SUB.i16 v_Residual1, v_Residual1, v_Pred1 || LSU1.LDIH i30, forward_4x4
PEU.PCIX.EQ  0x20 || CMU.VNZD.x16 i0, v4, 0x0                                 || VAU.SUB.i16 v_Residual2, v_Residual2, v_Pred2 || LSU1.LDIL i30, IntraChroma_Hadamard
PEU.PCIX.EQ  0x20 || CMU.VNZD.x16 i0, v6, 0x2                                 || VAU.SUB.i16 v_Residual3, v_Residual3, v_Pred3 || LSU1.LDIH i30, IntraChroma_Hadamard
CMU.CMZ.i32 i0                                                                || VAU.ADD.i16 v0, v_Residual0, 0                || LSU0.STI64.l v4, i_dst
PEU.PCCX.NEQ 0x01                              || IAU.OR i_cbp, i_cbp, i1     || VAU.ADD.i16 v1, v_Residual2, 0                || LSU0.STI64.h v4, i_dst
CMU.VSZMWORD v0, v_Residual1, [10DD]           || BRU.JMP i30                 || VAU.ADD.i16 v2, v_Residual1, 0                || LSU0.STI64.l v5, i_dst
CMU.VSZMWORD v1, v_Residual3, [10DD]                                          || VAU.ADD.i16 v3, v_Residual3, 0                || LSU0.STI64.h v5, i_dst
CMU.VSZMWORD v2, v_Residual0, [DD32]                                                                                           || LSU0.STI64.l v6, i_dst
CMU.VSZMWORD v3, v_Residual2, [DD32]                                                                                           || LSU0.STI64.h v6, i_dst
CMU.VILV.x16 v_Residual0, v_Residual1, v2, v0                                                                                  || LSU0.STI64.l v7, i_dst || LSU1.LDIL i30, IntraChroma_Store
CMU.VILV.x16 v_Residual2, v_Residual3, v3, v1                                                                                  || LSU0.STI64.h v7, i_dst || LSU1.LDIH i30, IntraChroma_Store

.lalign
IntraChroma_Hadamard:
CMU.CPVV.i16.i32 v_DC0, v_DC0 || VAU.SWZWORD v_DC1, v_DC0, [1032]            || LSU1.LDIL i_src, coeffStore
CMU.CPVV.i16.i32 v_DC1, v_DC1                                                || LSU1.LDIH i_src, coeffStore
PEU.PVEN4WORD 0x3 || VAU.ADD.i32 v0, v_DC0, v_DC0 || LSU0.LDI64.l v12, i_src || LSU1.SWZ4V [3210] [1032]
PEU.PVEN4WORD 0xC || VAU.SUB.i32 v0, v_DC0, v_DC0 || LSU0.LDI64.h v12, i_src || LSU1.SWZ4V [1032] [3210]
PEU.PVEN4WORD 0x3 || VAU.ADD.i32 v1, v_DC1, v_DC1 || LSU0.LDI64.l v13, i_src || LSU1.SWZ4V [3210] [1032]
PEU.PVEN4WORD 0xC || VAU.SUB.i32 v1, v_DC1, v_DC1 || LSU0.LDI64.h v13, i_src || LSU1.SWZ4V [1032] [3210]
PEU.PVEN4WORD 0x5 || VAU.ADD.i32 v_DC0, v0, v0    || LSU0.LDI64.l v14, i_src || LSU1.SWZ4V [3210] [2301]
PEU.PVEN4WORD 0xA || VAU.SUB.i32 v_DC0, v0, v0    || LSU0.LDI64.h v14, i_src || LSU1.SWZ4V [2301] [3210]
PEU.PVEN4WORD 0x5 || VAU.ADD.i32 v_DC1, v1, v1    || LSU0.LDI64.l v15, i_src || LSU1.SWZ4V [3210] [2301]
PEU.PVEN4WORD 0xA || VAU.SUB.i32 v_DC1, v1, v1    || LSU0.LDI64.h v15, i_src || LSU1.SWZ4V [2301] [3210]
CMU.CPVV.i16.i32 v9, v_QParam
                                             VAU.ABS.i32 v0, v_DC0      || LSU1.LDIL i30, ENC_ColCycle
                                             VAU.ABS.i32 v1, v_DC1      || LSU1.LDIH i30, ENC_ColCycle
CMU.CPIS s30, i30                         || VAU.MUL.i32 v0, v0, v9     || LSU0.SWZ4V [3210] [0000]
CMU.VSZMWORD v8, v_QParam, [2222]         || VAU.MUL.i32 v1, v1, v9     || LSU0.SWZ4V [3210] [0000]
CMU.VSZMBYTE v8, v8, [Z100]                                             || LSU1.LDIL i1, 0x10
CMU.VSZMWORD v9, v9, [3333]               || VAU.SHL.i32 v8, v8, 0x01
                                             VAU.ADD.i32 v9, v9, 0x01
CMU.VILV.x16 v_Coeff0, v_Coeff1, v14, v12 || VAU.ADD.i32 v0, v0, v8
CMU.VILV.x16 v_Coeff2, v_Coeff3, v15, v13 || VAU.ADD.i32 v1, v1, v8
CMU.VSZMWORD v8, v8, [ZZZZ]               || VAU.SHR.i32 v0, v0, v9
                  CMU.CMZ.i32 v_DC0       || VAU.SHR.i32 v1, v1, v9
PEU.PVV32 0x4  || CMU.CMZ.i32 v_DC1       || VAU.SUB.i32 v0, v8, v0
PEU.PVV32 0x4  || CMU.VNZD.x32 i0, v0, 0  || VAU.SUB.i32 v1, v8, v1
CMU.CMZ.i16 i0 || IAU.ADD i18, i_MBInfo, mb_chromaDCMap
CMU.CPVV.i16.i32 v8, v_QParam || LSU0.SWZC4WORD [1032]    || LSU1.LDIL i30, IntraChroma_Coeff
BRU.BRA Coeff2x2              || VAU.SHR.i32 v8, v8, 0x01 || LSU1.LDIH i30, IntraChroma_Coeff
PEU.PC1C 0x6   || IAU.OR i_cbp, i_cbp, i1
IntraChroma_Ihadamard:
PEU.PVEN4WORD 0x3   || VAU.ADD.i32 v_DC0, v0, v0    || LSU1.SWZ4V [3210] [1032]
PEU.PVEN4WORD 0xC   || VAU.SUB.i32 v_DC0, v0, v0    || LSU1.SWZ4V [1032] [3210]
PEU.PVEN4WORD 0x3   || VAU.ADD.i32 v_DC1, v1, v1    || LSU1.SWZ4V [3210] [1032]
PEU.PVEN4WORD 0xC   || VAU.SUB.i32 v_DC1, v1, v1    || LSU1.SWZ4V [1032] [3210]
.lalign
IntraChroma_Coeff:
PEU.PVEN4WORD 0x5   || VAU.ADD.i32 v0, v_DC0, v_DC0 || LSU1.SWZ4V [3210] [0321]
PEU.PVEN4WORD 0xA   || VAU.SUB.i32 v0, v_DC0, v_DC0 || LSU1.SWZ4V [2103] [3210]
PEU.PVEN4WORD 0x5   || VAU.ADD.i32 v1, v_DC1, v_DC1 || LSU1.SWZ4V [3210] [0321]
PEU.PVEN4WORD 0xA   || VAU.SUB.i32 v1, v_DC1, v_DC1 || LSU1.SWZ4V [2103] [3210]
BRU.BRA inverse_4x4 || VAU.MUL.i32 v_DC0, v0, v8    || LSU1.SWZ4V [3210] [1111]
                       VAU.MUL.i32 v_DC1, v1, v8    || LSU1.SWZ4V [3210] [1111]
IAU.XOR i_smb, i_smb, i_smb        || LSU1.LDIL i_dst, codec_offset
CMU.CPVV.i32.i16s v_DC0, v_DC0     || LSU1.LDIH i_dst, codec_offset
CMU.CPVV.i32.i16s v_DC1, v_DC1                             || LSU1.LDIL i30, IntraChroma_Write
CMU.VSZMWORD v_DC0, v_DC1, [10DD]  || IAU.INCS i_dst, 0x20 || LSU1.LDIH i30, IntraChroma_Write

.lalign
IntraChroma_Write:
CMU.VDILV.x16 v2, v0, v_Residual0, v_Residual1 || SAU.DEALWORD i1, i_smb                     || LSU0.LD32 i8, i_dst  || LSU1.LDIL i30, inverse_4x4
CMU.VDILV.x16 v3, v1, v_Residual2, v_Residual3 || IAU.SHL i1, i1, 0x02                                               || LSU1.LDIH i30, inverse_4x4
CMU.VSZMBYTE i2, i1, [ZZ32]                    || VAU.ADD.i16 v_Residual0, v0, 0                                     || LSU1.LDIL i0, 0xFF
CMU.CPIVR.x16 v8, i0                           || VAU.ADD.i16 v_Residual1, v2, 0                                     || LSU1.LDIH i1, 0
CMU.VSZMWORD v_Residual0, v2, [10DD]           || VAU.ADD.i16 v_Residual2, v1, 0             || IAU.INCS i_smb, 0x02 || LSU1.LDIL i4, predChromaH
CMU.VSZMWORD v_Residual1, v0, [DD32]           || VAU.ADD.i16 v_Residual3, v3, 0             || IAU.CMPI i_smb, 0x08 || LSU1.LDIH i4, predChromaH
CMU.VSZMWORD v_Residual2, v3, [10DD]           || VAU.ADD.i16 v_Residual0, v_Residual0, v24  || PEU.PCIX.EQ  0x20    || LSU1.LDIL i30, RLE_Chroma
CMU.VSZMWORD v_Residual3, v1, [DD32]           || VAU.ADD.i16 v_Residual1, v_Residual1, v25  || PEU.PCIX.EQ  0x20    || LSU1.LDIH i30, RLE_Chroma
CMU.CLAMP0.i16 v_Residual0, v_Residual0, v8    || VAU.ADD.i16 v_Residual2, v_Residual2, v26  || IAU.ADD i9, i8, 0x08 || LSU1.LDIL i5, predChromaV_lo
CMU.CLAMP0.i16 v_Residual1, v_Residual1, v8    || VAU.ADD.i16 v_Residual3, v_Residual3, v27  || LSU0.STI32 i9, i_dst || LSU1.LDIH i5, predChromaV_lo
CMU.CLAMP0.i16 v_Residual2, v_Residual2, v8    || IAU.SUB i0, i_posx, i_rowStart             || LSU0.STI128.u16.u8 v_Residual0, i8, i_desc
CMU.CLAMP0.i16 v_Residual3, v_Residual3, v8    || IAU.ADD i4, i4, i0                         || LSU0.STI128.u16.u8 v_Residual1, i8, i_desc || BRU.JMP i30
                                                  IAU.ADD i4, i4, i1                         || LSU0.STI128.u16.u8 v_Residual2, i8, i_desc || LSU1.LDIL i30, IntraChroma_Write
VAU.XOR v8, v8, v8                             || IAU.SUB.u32s i0, i_smb, 0x4                || LSU0.STI128.u16.u8 v_Residual3, i8, i_desc || LSU1.LDIH i30, IntraChroma_Write
PEU.PCIX.NEQ 0x01                              || IAU.ADD i5, i5, 0x08                       || CMU.CPVCR.x16 v9.l, v_Residual0.7
VAU.OR  v8, v8, v9                             || IAU.ADD i5, i5, i2                         || LSU0.ST128.u16.u8  v_Residual3, i4         || LSU1.SWZV8 [01230123]
                                                                                                LSU0.ST128.u16.u8  v8, i5
