Intra4x4Luma:
IAU.XOR i_smb, i_smb, i_smb || LSU1.LDIL i0, I_4x4
IAU.XOR i_cbp, i_cbp, i_cbp || LSU0.STO8 i0, i_MBInfo, mb_type

.lalign
Intra4x4Luma_Read:
                               IAU.SUB i_desc, i_rowEnd, i_rowStart || SAU.DEALWORD i1, i_smb
                               IAU.SHL i1, i1, 0x02           || LSU0.LDI128.u8.u16 v_Residual0, i_src, i_desc || LSU1.LDIL i8, predLumaH
CMU.VSZMBYTE i2, i1, [ZZ10] || IAU.SUB i3, i_posx, i_rowStart || LSU0.LDI128.u8.u16 v_Residual1, i_src, i_desc || LSU1.LDIH i8, predLumaH
CMU.VSZMBYTE i1, i1, [ZZ32] || IAU.ADD i4, i3, i2             || LSU0.LDI128.u8.u16 v_Residual2, i_src, i_desc || LSU1.LDIL i7, predLumaV
                               IAU.ADD i8, i8, i4             || LSU0.LDI128.u8.u16 v_Residual3, i_src, i_desc || LSU1.LDIH i7, predLumaV
CMU.CPSI i9, s_width        || IAU.ADD i7, i7, i1             || LSU0.LD128.u8.u16 v_Top, i8
CMU.CPIS s26, i8            || IAU.ADD i6, i_posx, i2         || LSU0.LD128.u8.u16 v_Left, i7
CMU.CPIS s27, i7            || IAU.ADD i5, i_posy, i1         || SAU.SUB.i32 i9, i9, 0x04
CMU.CPZV v0, 0              || IAU.FEXT.u32 i0, i_smb, 0x00, 0x02                                             || LSU1.LDIL i_best, MAX_DISTORT
CMU.CMII.i32 i6, i9         || IAU.CMPI i0, 0x03              || VAU.CMBWORD v_Residual0, v_Residual1, [EEDD] || LSU0.SWZ4V [3210] [1032]
PEU.ORACC                   || IAU.CMPI i_smb, 0x0D           || VAU.CMBWORD v_Residual2, v_Residual3, [EEDD] || LSU0.SWZ4V [3210] [1032]
PEU.PCC0I.OR 0x3, 0x1                                         || VAU.ADD.i16 v_Top, v0, v_Top                 || LSU0.SWZV8 [33333210]

; try vertical
CMU.CMZ.i32 i5                               || VAU.ADIFF.i16 v20, v_Residual0, v_Top || LSU0.SWZV8 [32103210]
PEU.PCCX.EQ 0x04 || CMU.CPVV v_Top, v_Left   || VAU.ADIFF.i16 v21, v_Residual2, v_Top || LSU0.SWZV8 [32103210]
                    SAU.SUM.i16 i0, v20, 0xF || VAU.SUB.i16 v20, v_Residual0, v_Top || LSU0.SWZV8 [32103210]
CMU.CMZ.i32 i6   || SAU.SUM.i16 i1, v21, 0xF || VAU.SUB.i16 v21, v_Residual2, v_Top || LSU0.SWZV8 [32103210]
PEU.PCCX.EQ 0x04 || CMU.CPVV v_Left, v_Top   || IAU.SUB i4, i_best, i0 || SAU.SUM.i16 i3, v_Top, 0x3
CMU.CMZ.i32 i5   || IAU.SUB i4, i4, i1 || SAU.SUM.i16 i2, v_Left, 0x3
PEU.PCC0I.AND 0x6, 0x2 || CMU.CPVV v22, v20  || IAU.SUB i_best, i_best, i4 || VAU.ADD.i16 v23, v21, 0 || LSU1.LDIL i_mode, I_PRED_4x4_V

; try horizontal
CMU.CPSI i18, s29                            || VAU.ADIFF.i16 v20, v_Residual0, v_Left || LSU0.SWZV8 [11110000]
IAU.ADD i2, i2, i3                           || VAU.ADIFF.i16 v21, v_Residual2, v_Left || LSU0.SWZV8 [33332222]
IAU.ADD i2, i2, 0x04    || SAU.SUM.i16 i0, v20, 0xF  || VAU.SUB.i16 v20, v_Residual0, v_Left || LSU0.SWZV8 [11110000]
IAU.SHR.u32 i2, i2, 0x3 || SAU.SUM.i16 i1, v21, 0xF  || VAU.SUB.i16 v21, v_Residual2, v_Left || LSU0.SWZV8 [33332222]
CMU.CPIVR.x16 v_DC, i2  || IAU.SUB i4, i_best, i0    || VAU.AVG.u16 v0, v_Top,  v_Top        || LSU0.SWZV8 [07654321]
CMU.CMZ.i32 i6          || IAU.SUB i4, i4, i1        || VAU.AVG.u16 v1, v_Left, v_Left       || LSU0.SWZV8 [07654321]
PEU.PCC0I.AND 0x6, 0x2 || CMU.CPVV v22, v20  || IAU.SUB i_best, i_best, i4 || VAU.ADD.i16 v23, v21, 0 || LSU1.LDIL i_mode, I_PRED_4x4_H
; try DC
                            VAU.ADIFF.i16 v20, v_Residual0, v_DC || LSU1.LDIL i30, Intra4x4Luma_StoreAC
                            VAU.ADIFF.i16 v21, v_Residual2, v_DC || LSU1.LDIH i30, Intra4x4Luma_StoreAC
SAU.SUM.i16 i0, v20, 0xF || VAU.SUB.i16 v20, v_Residual0, v_DC
SAU.SUM.i16 i1, v21, 0xF || VAU.SUB.i16 v21, v_Residual2, v_DC
IAU.SUB i4, i_best, i0   || VAU.ADD.i16 v2, v_Top,  v_Top  || LSU0.SWZV8 [07654321]
IAU.SUB i4, i4, i1       || VAU.ADD.i16 v3, v_Left, v_Left || LSU0.SWZV8 [07654321]
PEU.PC1I 0x3 || CMU.CPVV v22, v20  || IAU.SUB i_best, i_best, i4 || VAU.ADD.i16 v23, v21, 0 || LSU1.LDIL i_mode, I_PRED_4x4_DC
; calculate interpolated values for VL and HUP
CMU.CPVI.x16 i0.l, v_Left.3  || VAU.ADD.i16 v2, v2, v2   || LSU0.SWZV8 [07654321]
CMU.CPVI.x16 i1.l, v_Left.2  || VAU.ADD.i16 v3, v3, v3   || LSU0.SWZV8 [07654321]
CMU.VSZMWORD v24, v0, [DD10] || VAU.ADD.i16 v2, v2, 0x02 || IAU.MUL i2, i0, 0x03
CMU.VROT v0, v0, 0x02        || VAU.ADD.i16 v3, v3, 0x02
CMU.VSZMWORD v25, v0, [DD10] || VAU.SHR.i16 v2, v2, 0x02 || IAU.ADD i2, i2, i1
CMU.VSZMWORD v26, v1, [DD10] || VAU.SHR.i16 v3, v3, 0x02 || IAU.ADD i2, i2, 0x02
CMU.VSZMWORD v24, v2, [10DD]                             || IAU.SHR.u32 i2, i2, 0x02
CMU.VROT v2, v2, 0x02                                    || IAU.SUB i1, i_rowEnd, 0x04
; try VL
CMU.VSZMWORD v25, v2, [10DD] || VAU.ADIFF.i16 v20, v_Residual0, v24  || IAU.SUB.u32s i1, i1, i6
CMU.VSZMWORD v26, v3, [10DD] || VAU.ADIFF.i16 v21, v_Residual2, v25  || IAU.XOR i5, i5, i5        || PEU.PCIX.EQ 0x01
CMU.CPIV.x16 v26.3, i0.l     || VAU.SUB.i16 v20, v_Residual0, v24    || SAU.SUM.i16 i0, v20, 0xF
CMU.CPIV.x16 v26.7, i2.l     || VAU.SUB.i16 v21, v_Residual2, v25    || SAU.SUM.i16 i1, v21, 0xF
                                IAU.SUB i4, i_best, i0  || LSU0.LD64.l v_QParam, i18
CMU.CMZ.i32 i5               || IAU.SUB i4, i4, i1      || LSU0.LDO64.h v_QParam, i18, 0x08
PEU.PCC0I.AND 0x6, 0x2 || CMU.CPVV v22, v20  || IAU.SUB i_best, i_best, i4 || VAU.ADD.i16 v23, v21, 0 || LSU1.LDIL i_mode, I_PRED_4x4_VL
; try HUP
                                VAU.ADIFF.i16 v20, v_Residual0, v26  || LSU0.SWZV8 [72515140]
                                VAU.ADIFF.i16 v21, v_Residual2, v26  || LSU0.SWZV8 [33333372]
                                VAU.SUB.i16 v20, v_Residual0, v26    || SAU.SUM.i16 i0, v20, 0xF  || LSU0.SWZV8 [72515140]
                                VAU.SUB.i16 v21, v_Residual2, v26    || SAU.SUM.i16 i1, v21, 0xF  || LSU0.SWZV8 [33333372]
                                IAU.SUB i4, i_best, i0
CMU.CMZ.i32 i6               || IAU.SUB i4, i4, i1                   || BRU.BRA forward_4x4
;nop
PEU.PCC0I.AND 0x6, 0x2 || CMU.CPVV v22, v20  || IAU.SUB i_best, i_best, i4 || VAU.ADD.i16 v23, v21, 0 || LSU1.LDIL i_mode, I_PRED_4x4_HUP
CMU.CPVV.i16.i32 v_Residual0, v22
CMU.CPVV.i16.i32 v_Residual1, v22 || LSU0.SWZC4WORD [1032]
CMU.CPVV.i16.i32 v_Residual2, v23
CMU.CPVV.i16.i32 v_Residual3, v23 || LSU0.SWZC4WORD [1032]


; return point from forward
.lalign
Intra4x4Luma_StoreAC:
CMU.VNZD.x32 i0, v_Residual0, 0x0 || IAU.SHR.u32 i2, i_smb, 0x02 || LSU0.LDIL i1, 0x01 || LSU1.LDIL i4, Intra4x4Luma_Write
CMU.VNZD.x32 i0, v_Residual2, 0x1 || IAU.SHL i_flag, i1, i2      || LSU1.LDIH i4, Intra4x4Luma_Write
CMU.CMZ.i16 i0    || IAU.INCS i_smb, 0x01 || SAU.ADD.i32 i30, i4, 0 || LSU1.LDIL i1, 0x03
PEU.PCCX.NEQ 0x21 || CMU.CPIS s31, i4  || IAU.OR i_cbp, i_cbp, i_flag  || LSU1.LDIL i4, inverse_4x4
PEU.PCCX.NEQ 0x20 || CMU.CPSI i18, s29 || IAU.AND i0, i1, i_smb   || LSU1.LDIH i4, inverse_4x4
PEU.PC1I 0x1      || CMU.CPII i30, i4  || LSU0.LDIL i4, Coeff_RLE || LSU1.LDIH i4, Coeff_RLE
BRU.JMP i4  || LSU0.LDO64.l v_QParam, i18, 0x400
PEU.PVEN4WORD 0x3 || VAU.SWZWORD v0, v_Residual0, [3210] || CMU.VSZMWORD v1, v_Residual1, [2DD1] || LSU0.LDO64.h v_QParam, i18, 0x408
PEU.PVEN4WORD 0x6 || VAU.SWZWORD v1, v_Residual0, [0321] || CMU.VSZMWORD v0, v_Residual1, [D0DD] || IAU.XOR i18, i18, i18
PEU.PVEN4WORD 0xC || VAU.SWZWORD v3, v_Residual3, [3210] || CMU.VSZMWORD v0, v_Residual2, [0DDD]
PEU.PVEN4WORD 0x6 || VAU.SWZWORD v2, v_Residual3, [2103] || CMU.VSZMWORD v3, v_Residual1, [DDD3] || IAU.AND i_flag, i_flag, i_cbp
PEU.PVEN4WORD 0x9 || VAU.SWZWORD v2, v_Residual2, [2031] || CMU.VSZMWORD v3, v_Residual2, [DD3D] || BRU.RFB16I 0x04

; return point from inverse
.lalign
Intra4x4Luma_Write:
CMU.CPVV.i32.i16s v_Residual0, v_Residual0                                     || LSU1.LDIL i18, (codec_offset-0x04)
CMU.CPVV.i32.i16s v_Residual1, v_Residual1         || IAU.SHL i0, i_smb, 0x02  || LSU1.LDIH i18, (codec_offset-0x04)
CMU.CPVV.i32.i16s v_Residual2, v_Residual2         || IAU.ADD i18, i18, i0     || LSU1.LDIL i0, 0xFF
CMU.CPVV.i32.i16s v_Residual3, v_Residual3         || VAU.CMBWORD v_Residual0, v_Residual1, [EEDD] || LSU0.LD32 i_dst, i18 || LSU1.SWZ4V [3210] [1032]
; default to DC
CMU.VSZMWORD  v_Residual2, v_Residual3, [10DD]         || VAU.ADD.i16 v_Residual1, v_Residual0, v_DC   || LSU1.LDIL i30, Intra4x4Luma_Read
CMU.CPIVR.x16 v20, i0 || IAU.CMPI i_mode, I_PRED_4x4_V || VAU.ADD.i16 v_Residual3, v_Residual2, v_DC   || LSU1.LDIH i30, Intra4x4Luma_Read
; try V
PEU.PCIX.EQ 0x18 || CMU.CPSI i7, s27                || VAU.ADD.i16 v_Residual1, v_Residual0, v_Top  || LSU0.SWZV8 [32103210]
PEU.PCIX.EQ 0x18 || IAU.CMPI i_mode, I_PRED_4x4_H   || VAU.ADD.i16 v_Residual3, v_Residual2, v_Top  || LSU0.SWZV8 [32103210]
; try H
PEU.PCIX.EQ 0x18 || CMU.CPSI i8, s26                || VAU.ADD.i16 v_Residual1, v_Residual0, v_Left || LSU0.SWZV8 [11110000]
PEU.PCIX.EQ 0x18 || IAU.CMPI i_mode, I_PRED_4x4_VL  || VAU.ADD.i16 v_Residual3, v_Residual2, v_Left || LSU0.SWZV8 [33332222]
; try VL
PEU.PCIX.EQ 0x18                                    || VAU.ADD.i16 v_Residual1, v_Residual0, v24
PEU.PCIX.EQ 0x18 || IAU.CMPI i_mode, I_PRED_4x4_HUP || VAU.ADD.i16 v_Residual3, v_Residual2, v25
; try HUP
PEU.PCIX.EQ 0x18                                    || VAU.ADD.i16 v_Residual1, v_Residual0, v26    || LSU0.SWZV8 [72515140]
PEU.PCIX.EQ 0x18 || IAU.ADD i4, i_dst, 0x10         || VAU.ADD.i16 v_Residual3, v_Residual2, v26    || LSU0.SWZV8 [33333372]
; clamp to u8 result
CMU.CLAMP0.i16 v_Residual1, v_Residual1, v20       || IAU.ADD i0, i_MBInfo, i_smb || LSU0.ST32 i4, i18
CMU.CLAMP0.i16 v_Residual3, v_Residual3, v20       || IAU.CMPI i_smb, 0x10        || LSU0.LDO32 i_src, i18, 0x04
PEU.PC1I 0x3 || LSU0.LDIL i30, IntraChroma         || LSU1.LDIH i30, IntraChroma
CMU.CPVV.u16.u8s v_Residual1, v_Residual1          || IAU.INCS i0, (mb_pred-1)
CMU.CPVV.u16.u8s v_Residual3, v_Residual3  || SAU.SUM.u32 i0, v_Residual1, 0x1 || LSU0.ST8 i_mode, i0
CMU.CPVI.x32 i6, v_Residual3.1             || SAU.SUM.u32 i1, v_Residual1, 0x2 || IAU.SUB i_desc, i_rowEnd,i_rowStart
LSU0.ST32 i6, i8                           || BRU.JMP i30
CMU.VSZMBYTE i6, i0, [DDD3]                || SAU.SUM.u32 i2, v_Residual3, 0x1 || LSU0.STI32 i0, i_dst, i_desc 
CMU.VSZMBYTE i6, i1, [DD3D]                || SAU.SUM.u32 i3, v_Residual3, 0x2 || LSU0.STI32 i1, i_dst, i_desc
CMU.VSZMBYTE i6, i2, [D3DD]                                                    || LSU0.STI32 i2, i_dst, i_desc
                                                                                  LSU0.STI32 i3, i_dst, i_desc
LSU0.ST32 i6, i7 

