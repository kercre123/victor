; next generation, dual shock inter luma module

.lalign
InterLuma:
                                                                       VAU.SUB.i16 v_Residual0, v0, v24  || LSU1.LDIL i_dst, coeffStore
CMU.CPSI i18, s_YQAddr                                              || VAU.SUB.i16 v_Residual1, v1, v25  || LSU1.LDIH i_dst, coeffStore
CMU.VSZMWORD v0, v_Residual0, [DD10] || IAU.INCS i18, 0x200         || VAU.SUB.i16 v_Residual2, v2, v26
CMU.VSZMWORD v0, v_Residual1, [10DD] || IAU.ADD i_src, i_dst, 0     || VAU.SUB.i16 v_Residual3, v3, v27   
CMU.VSZMWORD v1, v_Residual2, [DD10] || IAU.XOR i_cbp, i_cbp, i_cbp || BRU.BRA forward_4x4
CMU.VSZMWORD v1, v_Residual3, [10DD] || IAU.XOR i_smb, i_smb, i_smb || SAU.SUB.i32 i_desc, i_rowEnd, i_rowStart
CMU.VSZMWORD v2, v_Residual0, [DD32] || PEU.PVEN4WORD 0x3           || VAU.SWZWORD v3, v_Residual2, [1032]
CMU.VSZMWORD v2, v_Residual1, [32DD] || PEU.PVEN4WORD 0xC           || VAU.SWZWORD v3, v_Residual3, [3210]
CMU.VILV.x16 v_Residual0, v_Residual1, v2, v0                       || LSU0.LDI64.l v_QParam, i18        || LSU1.LDIL i30, InterLuma_Store
CMU.VILV.x16 v_Residual2, v_Residual3, v3, v1                       || LSU0.LDI64.h v_QParam, i18        || LSU1.LDIH i30, InterLuma_Store

.lalign
InterLuma_Store:
                                                  IAU.SHR.u32 i1, i_smb, 0x02                                                  || LSU1.LDIL i2, 0x01
CMU.VDILV.x16 v6, v4, v_Coeff0, v_Coeff1       || IAU.INCS i_smb, 0x02        || VAU.SUB.i16 v_Residual0, v_Residual0, v_Pred0 || LSU1.LDIL i30, forward_4x4
CMU.VDILV.x16 v7, v5, v_Coeff2, v_Coeff3       || IAU.CMPI i_smb, 0x10        || VAU.SUB.i16 v_Residual1, v_Residual1, v_Pred1 || LSU1.LDIH i30, forward_4x4
PEU.PCIX.EQ  0x20 || CMU.VNZD.x16 i0, v4, 0x0                                 || VAU.SUB.i16 v_Residual2, v_Residual2, v_Pred2 || LSU1.LDIL i30, InterLuma_Hadamard
PEU.PCIX.EQ  0x20 || CMU.VNZD.x16 i0, v6, 0x2                                 || VAU.SUB.i16 v_Residual3, v_Residual3, v_Pred3 || LSU1.LDIH i30, InterLuma_Hadamard
CMU.CMZ.i32 i0                                 || IAU.SHL i2, i2, i1          || VAU.ADD.i16 v0, v_Residual0, 0                || LSU0.STI64.l v4, i_dst
PEU.PCCX.NEQ 0x01                              || IAU.OR i_cbp, i_cbp, i2     || VAU.ADD.i16 v1, v_Residual2, 0                || LSU0.STI64.h v4, i_dst
CMU.VSZMWORD v0, v_Residual1, [10DD]           || BRU.JMP i30                 || VAU.ADD.i16 v2, v_Residual1, 0                || LSU0.STI64.l v5, i_dst
CMU.VSZMWORD v1, v_Residual3, [10DD]                                          || VAU.ADD.i16 v3, v_Residual3, 0                || LSU0.STI64.h v5, i_dst
CMU.VSZMWORD v2, v_Residual0, [DD32]                                                                                           || LSU0.STI64.l v6, i_dst
CMU.VSZMWORD v3, v_Residual2, [DD32]                                                                                           || LSU0.STI64.h v6, i_dst
CMU.VILV.x16 v_Residual0, v_Residual1, v2, v0                                                                                  || LSU0.STI64.l v7, i_dst || LSU1.LDIL i30, InterLuma_Store
CMU.VILV.x16 v_Residual2, v_Residual3, v3, v1                                                                                  || LSU0.STI64.h v7, i_dst || LSU1.LDIH i30, InterLuma_Store

.lalign
InterLuma_Hadamard:
; there is no hadmard for inter just load coeffcients
LSU0.LDI64.l v12, i_src
LSU0.LDI64.h v12, i_src
LSU0.LDI64.l v13, i_src
LSU0.LDI64.h v13, i_src
LSU0.LDI64.l v14, i_src
LSU0.LDI64.h v14, i_src
VAU.XOR v_DC0, v_DC0, v_DC0  || LSU0.LDI64.l v15, i_src
VAU.XOR v_DC1, v_DC1, v_DC1  || LSU0.LDI64.h v15, i_src
BRU.BRA inverse_4x4          || LSU1.LDIL i30, InterChroma
IAU.XOR i_smb, i_smb, i_smb  || LSU1.LDIH i30, InterChroma
CMU.CPIS s30, i30                         || LSU1.LDIL i_dst, codec_offset
LSU0.STO16 i_smb, i_MBInfo, mb_lumaDCMap  || LSU1.LDIH i_dst, codec_offset
CMU.VILV.x16 v_Coeff0, v_Coeff1, v14, v12 || LSU1.LDIL i30, InterLuma_Write
CMU.VILV.x16 v_Coeff2, v_Coeff3, v15, v13 || LSU1.LDIH i30, InterLuma_Write

.lalign
InterLuma_Write:
CMU.VDILV.x16 v2, v0, v_Residual0, v_Residual1 || SAU.DEALWORD i1, i_smb                     || LSU0.LD32 i8, i_dst  || LSU1.LDIL i30, inverse_4x4
CMU.VDILV.x16 v3, v1, v_Residual2, v_Residual3 || IAU.SHL i1, i1, 0x02                                               || LSU1.LDIH i30, inverse_4x4
CMU.VSZMBYTE i2, i1, [ZZ32]                    || VAU.ADD.i16 v_Residual0, v0, 0                                     || LSU1.LDIL i0, 0xFF
CMU.CPIVR.x16 v8, i0                           || VAU.ADD.i16 v_Residual1, v2, 0                                     || LSU1.LDIH i1, 0
CMU.VSZMWORD v_Residual0, v2, [10DD]           || VAU.ADD.i16 v_Residual2, v1, 0             || IAU.INCS i_smb, 0x02 || LSU1.LDIL i4, predLumaH
CMU.VSZMWORD v_Residual1, v0, [DD32]           || VAU.ADD.i16 v_Residual3, v3, 0             || IAU.CMPI i_smb, 0x10 || LSU1.LDIH i4, predLumaH
CMU.VSZMWORD v_Residual2, v3, [10DD]           || VAU.ADD.i16 v_Residual0, v_Residual0, v24  || PEU.PCIX.EQ  0x20    || LSU1.LDIL i30, RLE_Luma
CMU.VSZMWORD v_Residual3, v1, [DD32]           || VAU.ADD.i16 v_Residual1, v_Residual1, v25  || PEU.PCIX.EQ  0x20    || LSU1.LDIH i30, RLE_Luma
CMU.CLAMP0.i16 v_Residual0, v_Residual0, v8    || VAU.ADD.i16 v_Residual2, v_Residual2, v26  || IAU.ADD i9, i8, 0x10 || LSU1.LDIL i5, predLumaV_lo
CMU.CLAMP0.i16 v_Residual1, v_Residual1, v8    || VAU.ADD.i16 v_Residual3, v_Residual3, v27  || LSU0.STI32 i9, i_dst || LSU1.LDIH i5, predLumaV_lo
CMU.CLAMP0.i16 v_Residual2, v_Residual2, v8    || IAU.SUB i0, i_posx, i_rowStart             || LSU0.STI128.u16.u8 v_Residual0, i8, i_desc
CMU.CLAMP0.i16 v_Residual3, v_Residual3, v8    || IAU.ADD i4, i4, i0                         || LSU0.STI128.u16.u8 v_Residual1, i8, i_desc || BRU.JMP i30
                                                  IAU.ADD i4, i4, i1                         || LSU0.STI128.u16.u8 v_Residual2, i8, i_desc || LSU1.LDIL i30, InterLuma_Write
VAU.XOR v8, v8, v8                                                                           || LSU0.STI128.u16.u8 v_Residual3, i8, i_desc || LSU1.LDIH i30, InterLuma_Write
CMU.CPVCR.x16 v9.l, v_Residual0.7
VAU.OR  v8, v8, v9                             || IAU.ADD i5, i5, i2                         || LSU0.ST128.u16.u8  v_Residual3, i4         || LSU1.SWZV8 [01230123]
                                                                                                LSU0.ST128.u16.u8  v8, i5
