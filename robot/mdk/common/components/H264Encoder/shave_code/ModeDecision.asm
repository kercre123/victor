.version 00.40.00

.code .text.MD_Intra
.lalign
MD_Intra:
; do intra prediction
CMU.CMZ.i32 i_posx || IAU.SHL i1, i_desc, 0x04              || LSU0.LDIL i0, 0x80                    || LSU1.LDIL i4, predLumaH
PEU.PCCX.NEQ 0x10  || IAU.SUB i2, i_posx, i_rowStart        || LSU0.LDA64.l v_Left, 0, predLumaV_lo  || LSU1.LDIH i4, predLumaH
PEU.PCCX.NEQ 0x10  || IAU.ADD i4, i4, i2                    || LSU0.LDA64.h v_Left, 0, predLumaV_hi  || CMU.CMZ.i32 i_posy
PEU.PCCX.NEQ 0x10  || CMU.CPIVR.x8 v_Left, i0               || LSU0.LDI64.l v_Top, i4                || LSU1.LDIL i_src, origLuma
PEU.PCCX.NEQ 0x10  || CMU.CPIVR.x8 v_Top,  i0               || LSU0.LDI64.h v_Top, i4                || LSU1.LDIH i_src, origLuma
CMU.CMZ.i32 v_cmd  || IAU.ADD i_src, i_src, i2
PEU.PCXC 0x6, 0x2  || IAU.ADD i_src, i_src, i1
; calculate SAD for supported i16x16 modes, calculate mean for AQ
IAU.ADD i_adr, i_src, 0x08                                        || LSU1.LDIL i_mode, I_16x16_2_0_0
SAU.SUM.u8 i2, v_Left, 0xF                                        || LSU0.LDI64.l v0,  i_src, i_desc || LSU1.LDIL i_dst, sampleStore
                                                                     LSU0.LDI64.h v0,  i_adr, i_desc || LSU1.LDIH i_dst, sampleStore
SAU.SUM.u8 i3, v_Top, 0xF                                         || LSU0.LDI64.l v1,  i_src, i_desc || LSU1.LDIL i6, 0
CMU.CMZ.i32 i_posx                                                || LSU0.LDI64.h v1,  i_adr, i_desc || LSU1.LDIL i5, 0
PEU.PCCX.EQ 0x21 || CMU.CMZ.i32 i_posy || IAU.ADD i2, i3, 0       || LSU1.LDIL i6, 0xFFFF
PEU.PCCX.EQ 0x24 || CMU.CPII i3, i2    || IAU.ADD i2, i2, 0x10    || LSU1.LDIL i5, 0xFFFF
IAU.ADD i2, i2, i3                                                || LSU0.LDI64.l v2,  i_src, i_desc
IAU.SHR.u32 i2, i2, 0x05                                          || LSU0.LDI64.h v2,  i_adr, i_desc
CMU.CPIVR.x8 v_Mean, i2                                           || SAU.SUM.u8 i3, v0,  0xF
CMU.VSZMBYTE v_Temp, v_Left, [0000]                                                          || VAU.ADIFF.u8 v20, v0,  v_Mean  || LSU1.SWZC4WORD [0000]
                                       IAU.ADD i4, i3, 0          || SAU.SUM.u8 i3, v1,  0xF || VAU.ADIFF.u8 v21, v0,  v_Top   || LSU0.LDI64.l v3,  i_src, i_desc
                                                                     SAU.SUM.u8 i0, v20, 0xF || VAU.ADIFF.u8 v22, v0,  v_Temp  || LSU0.LDI64.h v3,  i_adr, i_desc
CMU.VSZMBYTE v_Temp, v_Left, [1111] || IAU.ADD i4, i4, i3         || SAU.SUM.u8 i1, v21, 0xF                                   || LSU1.SWZC4WORD [0000]
                                       IAU.ADD i_best, i0, 0      || SAU.SUM.u8 i2, v22, 0xF || VAU.ADIFF.u8 v20, v1,  v_Mean  || LSU0.STI64.l v0,  i_dst
                                       IAU.ADD i5, i5, i1         || SAU.SUM.u8 i3, v2,  0xF || VAU.ADIFF.u8 v21, v1,  v_Top   || LSU0.LDI64.l v4,  i_src, i_desc
                                       IAU.ADD i6, i6, i2         || SAU.SUM.u8 i0, v20, 0xF || VAU.ADIFF.u8 v22, v1,  v_Temp  || LSU0.LDI64.h v4,  i_adr, i_desc
CMU.VSZMBYTE v_Temp, v_Left, [2222] || IAU.ADD i4, i4, i3         || SAU.SUM.u8 i1, v21, 0xF                                   || LSU1.SWZC4WORD [0000]
                                       IAU.ADD i_best, i_best, i0 || SAU.SUM.u8 i2, v22, 0xF || VAU.ADIFF.u8 v20, v2,  v_Mean  || LSU0.STI64.l v1,  i_dst
                                       IAU.ADD i5, i5, i1         || SAU.SUM.u8 i3, v3,  0xF || VAU.ADIFF.u8 v21, v2,  v_Top   || LSU0.LDI64.l v5,  i_src, i_desc
                                       IAU.ADD i6, i6, i2         || SAU.SUM.u8 i0, v20, 0xF || VAU.ADIFF.u8 v22, v2,  v_Temp  || LSU0.LDI64.h v5,  i_adr, i_desc
CMU.VSZMBYTE v_Temp, v_Left, [3333] || IAU.ADD i4, i4, i3         || SAU.SUM.u8 i1, v21, 0xF                                   || LSU1.SWZC4WORD [0000]
                                       IAU.ADD i_best, i_best, i0 || SAU.SUM.u8 i2, v22, 0xF || VAU.ADIFF.u8 v20, v3,  v_Mean  || LSU0.STI64.l v2,  i_dst
                                       IAU.ADD i5, i5, i1         || SAU.SUM.u8 i3, v4,  0xF || VAU.ADIFF.u8 v21, v3,  v_Top   || LSU0.LDI64.l v6,  i_src, i_desc
                                       IAU.ADD i6, i6, i2         || SAU.SUM.u8 i0, v20, 0xF || VAU.ADIFF.u8 v22, v3,  v_Temp  || LSU0.LDI64.h v6,  i_adr, i_desc
CMU.VSZMBYTE v_Temp, v_Left, [0000] || IAU.ADD i4, i4, i3         || SAU.SUM.u8 i1, v21, 0xF                                   || LSU1.SWZC4WORD [1111]
                                       IAU.ADD i_best, i_best, i0 || SAU.SUM.u8 i2, v22, 0xF || VAU.ADIFF.u8 v20, v4,  v_Mean  || LSU0.STI64.l v3,  i_dst
                                       IAU.ADD i5, i5, i1         || SAU.SUM.u8 i3, v5,  0xF || VAU.ADIFF.u8 v21, v4,  v_Top   || LSU0.LDI64.l v7,  i_src, i_desc
                                       IAU.ADD i6, i6, i2         || SAU.SUM.u8 i0, v20, 0xF || VAU.ADIFF.u8 v22, v4,  v_Temp  || LSU0.LDI64.h v7,  i_adr, i_desc
CMU.VSZMBYTE v_Temp, v_Left, [1111] || IAU.ADD i4, i4, i3         || SAU.SUM.u8 i1, v21, 0xF                                   || LSU1.SWZC4WORD [1111]
                                       IAU.ADD i_best, i_best, i0 || SAU.SUM.u8 i2, v22, 0xF || VAU.ADIFF.u8 v20, v5,  v_Mean  || LSU0.STI64.l v4,  i_dst
                                       IAU.ADD i5, i5, i1         || SAU.SUM.u8 i3, v6,  0xF || VAU.ADIFF.u8 v21, v5,  v_Top   || LSU0.LDI64.l v8,  i_src, i_desc
                                       IAU.ADD i6, i6, i2         || SAU.SUM.u8 i0, v20, 0xF || VAU.ADIFF.u8 v22, v5,  v_Temp  || LSU0.LDI64.h v8,  i_adr, i_desc
CMU.VSZMBYTE v_Temp, v_Left, [2222] || IAU.ADD i4, i4, i3         || SAU.SUM.u8 i1, v21, 0xF                                   || LSU1.SWZC4WORD [1111]
                                       IAU.ADD i_best, i_best, i0 || SAU.SUM.u8 i2, v22, 0xF || VAU.ADIFF.u8 v20, v6,  v_Mean  || LSU0.STI64.l v5,  i_dst
                                       IAU.ADD i5, i5, i1         || SAU.SUM.u8 i3, v7,  0xF || VAU.ADIFF.u8 v21, v6,  v_Top   || LSU0.LDI64.l v9,  i_src, i_desc
                                       IAU.ADD i6, i6, i2         || SAU.SUM.u8 i0, v20, 0xF || VAU.ADIFF.u8 v22, v6,  v_Temp  || LSU0.LDI64.h v9,  i_adr, i_desc
CMU.VSZMBYTE v_Temp, v_Left, [3333] || IAU.ADD i4, i4, i3         || SAU.SUM.u8 i1, v21, 0xF                                   || LSU1.SWZC4WORD [1111]
                                       IAU.ADD i_best, i_best, i0 || SAU.SUM.u8 i2, v22, 0xF || VAU.ADIFF.u8 v20, v7,  v_Mean  || LSU0.STI64.l v6,  i_dst
                                       IAU.ADD i5, i5, i1         || SAU.SUM.u8 i3, v8,  0xF || VAU.ADIFF.u8 v21, v7,  v_Top   || LSU0.LDI64.l v10, i_src, i_desc
                                       IAU.ADD i6, i6, i2         || SAU.SUM.u8 i0, v20, 0xF || VAU.ADIFF.u8 v22, v7,  v_Temp  || LSU0.LDI64.h v10, i_adr, i_desc
CMU.VSZMBYTE v_Temp, v_Left, [0000] || IAU.ADD i4, i4, i3         || SAU.SUM.u8 i1, v21, 0xF                                   || LSU1.SWZC4WORD [2222]
                                       IAU.ADD i_best, i_best, i0 || SAU.SUM.u8 i2, v22, 0xF || VAU.ADIFF.u8 v20, v8,  v_Mean  || LSU0.STI64.l v7,  i_dst
                                       IAU.ADD i5, i5, i1         || SAU.SUM.u8 i3, v9,  0xF || VAU.ADIFF.u8 v21, v8,  v_Top   || LSU0.LDI64.l v11, i_src, i_desc
                                       IAU.ADD i6, i6, i2         || SAU.SUM.u8 i0, v20, 0xF || VAU.ADIFF.u8 v22, v8,  v_Temp  || LSU0.LDI64.h v11, i_adr, i_desc
CMU.VSZMBYTE v_Temp, v_Left, [1111] || IAU.ADD i4, i4, i3         || SAU.SUM.u8 i1, v21, 0xF                                   || LSU1.SWZC4WORD [2222]
                                       IAU.ADD i_best, i_best, i0 || SAU.SUM.u8 i2, v22, 0xF || VAU.ADIFF.u8 v20, v9,  v_Mean  || LSU0.STI64.h v0,  i_dst
                                       IAU.ADD i5, i5, i1         || SAU.SUM.u8 i3, v10, 0xF || VAU.ADIFF.u8 v21, v9,  v_Top   || LSU0.LDI64.l v12, i_src, i_desc
                                       IAU.ADD i6, i6, i2         || SAU.SUM.u8 i0, v20, 0xF || VAU.ADIFF.u8 v22, v9,  v_Temp  || LSU0.LDI64.h v12, i_adr, i_desc
CMU.VSZMBYTE v_Temp, v_Left, [2222] || IAU.ADD i4, i4, i3         || SAU.SUM.u8 i1, v21, 0xF                                   || LSU1.SWZC4WORD [2222]
                                       IAU.ADD i_best, i_best, i0 || SAU.SUM.u8 i2, v22, 0xF || VAU.ADIFF.u8 v20, v10, v_Mean  || LSU0.STI64.h v1,  i_dst
                                       IAU.ADD i5, i5, i1         || SAU.SUM.u8 i3, v11, 0xF || VAU.ADIFF.u8 v21, v10, v_Top   || LSU0.LDI64.l v13, i_src, i_desc
                                       IAU.ADD i6, i6, i2         || SAU.SUM.u8 i0, v20, 0xF || VAU.ADIFF.u8 v22, v10, v_Temp  || LSU0.LDI64.h v13, i_adr, i_desc
CMU.VSZMBYTE v_Temp, v_Left, [3333] || IAU.ADD i4, i4, i3         || SAU.SUM.u8 i1, v21, 0xF                                   || LSU1.SWZC4WORD [2222]
                                       IAU.ADD i_best, i_best, i0 || SAU.SUM.u8 i2, v22, 0xF || VAU.ADIFF.u8 v20, v11, v_Mean  || LSU0.STI64.h v2,  i_dst
                                       IAU.ADD i5, i5, i1         || SAU.SUM.u8 i3, v12, 0xF || VAU.ADIFF.u8 v21, v11, v_Top   || LSU0.LDI64.l v14, i_src, i_desc
                                       IAU.ADD i6, i6, i2         || SAU.SUM.u8 i0, v20, 0xF || VAU.ADIFF.u8 v22, v11, v_Temp  || LSU0.LDI64.h v14, i_adr, i_desc
CMU.VSZMBYTE v_Temp, v_Left, [0000] || IAU.ADD i4, i4, i3         || SAU.SUM.u8 i1, v21, 0xF                                   || LSU1.SWZC4WORD [3333]
                                       IAU.ADD i_best, i_best, i0 || SAU.SUM.u8 i2, v22, 0xF || VAU.ADIFF.u8 v20, v12, v_Mean  || LSU0.STI64.h v3,  i_dst
                                       IAU.ADD i5, i5, i1         || SAU.SUM.u8 i3, v13, 0xF || VAU.ADIFF.u8 v21, v12, v_Top   || LSU0.LDI64.l v15, i_src, i_desc
                                       IAU.ADD i6, i6, i2         || SAU.SUM.u8 i0, v20, 0xF || VAU.ADIFF.u8 v22, v12, v_Temp  || LSU0.LDI64.h v15, i_adr, i_desc
CMU.VSZMBYTE v_Temp, v_Left, [1111] || IAU.ADD i4, i4, i3         || SAU.SUM.u8 i1, v21, 0xF                                   || LSU1.SWZC4WORD [3333]
                                       IAU.ADD i_best, i_best, i0 || SAU.SUM.u8 i2, v22, 0xF || VAU.ADIFF.u8 v20, v13, v_Mean  || LSU0.STI64.h v4,  i_dst
                                       IAU.ADD i5, i5, i1         || SAU.SUM.u8 i3, v14, 0xF || VAU.ADIFF.u8 v21, v13, v_Top
                                       IAU.ADD i6, i6, i2         || SAU.SUM.u8 i0, v20, 0xF || VAU.ADIFF.u8 v22, v13, v_Temp
CMU.VSZMBYTE v_Temp, v_Left, [2222] || IAU.ADD i4, i4, i3         || SAU.SUM.u8 i1, v21, 0xF                                   || LSU1.SWZC4WORD [3333]
                                       IAU.ADD i_best, i_best, i0 || SAU.SUM.u8 i2, v22, 0xF || VAU.ADIFF.u8 v20, v14, v_Mean  || LSU0.STI64.h v5,  i_dst
                                       IAU.ADD i5, i5, i1         || SAU.SUM.u8 i3, v15, 0xF || VAU.ADIFF.u8 v21, v14, v_Top
                                       IAU.ADD i6, i6, i2         || SAU.SUM.u8 i0, v20, 0xF || VAU.ADIFF.u8 v22, v14, v_Temp
CMU.VSZMBYTE v_Temp, v_Left, [3333] || IAU.ADD i4, i4, i3         || SAU.SUM.u8 i1, v21, 0xF                                   || LSU1.SWZC4WORD [3333]
                                       IAU.ADD i_best, i_best, i0 || SAU.SUM.u8 i2, v22, 0xF || VAU.ADIFF.u8 v20, v15, v_Mean  || LSU0.STI64.h v6,  i_dst
CMU.VSZMBYTE i4, i4, [ZZZ1]         || IAU.ADD i5, i5, i1                                    || VAU.ADIFF.u8 v21, v15, v_Top
CMU.CPIVR.x8 v_Temp, i4             || IAU.ADD i6, i6, i2         || SAU.SUM.u8 i0, v20, 0xF || VAU.ADIFF.u8 v22, v15, v_Temp
; calculate standard deviation for  AQ and store original
                                                                     SAU.SUM.u8 i1, v21, 0xF || VAU.ADIFF.u8 v20, v0,  v_Temp  || LSU0.LDO32 i18, enc, qpy
                                       IAU.ADD i_best, i_best, i0 || SAU.SUM.u8 i2, v22, 0xF || VAU.ADIFF.u8 v20, v1,  v_Temp  || LSU0.STI64.h v7,  i_dst
                                       IAU.ADD i5, i5, i1         || SAU.SUM.u8 i1, v20, 0xF || VAU.ADIFF.u8 v20, v2,  v_Temp  || LSU0.STI64.l v8,  i_dst
                                       IAU.ADD i6, i6, i2         || SAU.SUM.u8 i2, v20, 0xF || VAU.ADIFF.u8 v20, v3,  v_Temp  || LSU0.STI64.l v9,  i_dst
                                       IAU.ADD i4, i1, 0          || SAU.SUM.u8 i1, v20, 0xF || VAU.ADIFF.u8 v20, v4,  v_Temp  || LSU0.STI64.l v10, i_dst
                                       IAU.ADD i4, i4, i2         || SAU.SUM.u8 i2, v20, 0xF || VAU.ADIFF.u8 v20, v5,  v_Temp  || LSU0.STI64.l v11, i_dst
                                       IAU.ADD i4, i4, i1         || SAU.SUM.u8 i1, v20, 0xF || VAU.ADIFF.u8 v20, v6,  v_Temp  || LSU0.STI64.l v12, i_dst
                                       IAU.ADD i4, i4, i2         || SAU.SUM.u8 i2, v20, 0xF || VAU.ADIFF.u8 v20, v7,  v_Temp  || LSU0.STI64.l v13, i_dst
                                       IAU.ADD i4, i4, i1         || SAU.SUM.u8 i1, v20, 0xF || VAU.ADIFF.u8 v20, v8,  v_Temp  || LSU0.STI64.l v14, i_dst
                                       IAU.ADD i4, i4, i2         || SAU.SUM.u8 i2, v20, 0xF || VAU.ADIFF.u8 v20, v9,  v_Temp  || LSU0.STI64.l v15, i_dst
                                       IAU.ADD i4, i4, i1         || SAU.SUM.u8 i1, v20, 0xF || VAU.ADIFF.u8 v20, v10, v_Temp  || LSU0.STI64.h v8,  i_dst
                                       IAU.ADD i4, i4, i2         || SAU.SUM.u8 i2, v20, 0xF || VAU.ADIFF.u8 v20, v11, v_Temp  || LSU0.STI64.h v9,  i_dst
                                       IAU.ADD i4, i4, i1         || SAU.SUM.u8 i1, v20, 0xF || VAU.ADIFF.u8 v20, v12, v_Temp  || LSU0.STI64.h v10, i_dst
                                       IAU.ADD i4, i4, i2         || SAU.SUM.u8 i2, v20, 0xF || VAU.ADIFF.u8 v20, v13, v_Temp  || LSU0.STI64.h v11, i_dst
                                       IAU.ADD i4, i4, i1         || SAU.SUM.u8 i1, v20, 0xF || VAU.ADIFF.u8 v20, v14, v_Temp  || LSU0.STI64.h v12, i_dst
                                       IAU.ADD i4, i4, i2         || SAU.SUM.u8 i2, v20, 0xF || VAU.ADIFF.u8 v20, v15, v_Temp  || LSU0.STI64.h v13, i_dst
CMU.CPVV v24, v_Mean                || IAU.ADD i4, i4, i1         || SAU.SUM.u8 i1, v20, 0xF                                   || LSU0.STI64.h v14, i_dst
CMU.CMII.i32 i5, i_best             || IAU.ADD i4, i4, i2         || SAU.SUM.u8 i2, v20, 0xF                                   || LSU0.STI64.h v15, i_dst
PEU.PC1C 0x4 || CMU.CPVV v24, v_Top  || IAU.ADD i_best, i5, 0     || LSU1.LDIL i_mode, I_16x16_0_0_0
CMU.CMII.i32 i6, i_best              || IAU.ADD i4, i4, i1        || SAU.SUM.i32 i7, v_SAD8x8, 0xF
PEU.PC1C 0x4 || CMU.CPVV v24, v_Left || IAU.ADD i_best, i6, 0     || LSU1.LDIL i_mode, I_16x16_1_0_0
CMU.CMII.i32 i7, i_best              || IAU.ADD i4, i4, i2        || SAU.SUB.i32 i18, i18, 0x04
PEU.PC1C 0x4 || BRU.BRA MD_InterRefine
IAU.SHR.u32 i4, i4, 0x0A  || LSU1.LDIL i0, 0x08
; // AQ enable
;IAU.CLAMP0.i32 i4,  i4,  i0
LSU1.LDIL i4, 0x04
IAU.ADD i18, i18, i4      || LSU1.LDIL i0, 0x1F
IAU.CLAMP0.i32 i18, i18, i0
CMU.CPIS s_QPy, i18 || LSU0.STO16 i18, i_MBInfo, mb_qpy

MD_IntraRefine:
; store predicted samples and do a max range calculation
IAU.ADD i_adr, i_dst, 0x40 || LSU1.LDIL i0, I_16x16_1_0_0
CMU.CMII.i32 i_mode, i0    || LSU1.LDIL i1, 0x48
PEU.PC1C 0x1 || CMU.VSZMBYTE v24, v_Left, [0000] || LSU1.SWZC4WORD [0000]
                CMU.VSZMBYTE v28, v28, [ZZZZ]    || LSU0.STI64.l v24, i_dst || LSU1.LDIL i8, quant_table
                VAU.ADIFF.u8 v27, v0,  v24       || LSU0.STI64.h v24, i_adr || LSU1.LDIH i8, quant_table
PEU.PC1C 0x1 || CMU.VSZMBYTE v24, v_Left, [1111] || LSU1.SWZC4WORD [0000]
                CMU.MAX.u8   v28, v28, v27       || LSU0.STI64.l v24, i_dst || LSU1.LDIL i9, qpc_table
                VAU.ADIFF.u8 v27, v1,  v24       || LSU0.STI64.h v24, i_adr || LSU1.LDIH i9, qpc_table
PEU.PC1C 0x1 || CMU.VSZMBYTE v24, v_Left, [2222] || LSU1.SWZC4WORD [0000]
                CMU.MAX.u8   v28, v28, v27       || LSU0.STI64.l v24, i_dst || IAU.ADD i9, i9, i18
                VAU.ADIFF.u8 v27, v2,  v24       || LSU0.STI64.h v24, i_adr || LSU1.LD32.u8.u32 i9, i9
PEU.PC1C 0x1 || CMU.VSZMBYTE v24, v_Left, [3333] || LSU1.SWZC4WORD [0000]
                CMU.MAX.u8   v28, v28, v27       || LSU0.STI64.l v24, i_dst
                VAU.ADIFF.u8 v27, v3,  v24       || LSU0.STI64.h v24, i_adr
PEU.PC1C 0x1 || CMU.VSZMBYTE v24, v_Left, [0000] || LSU1.SWZC4WORD [1111]
                CMU.MAX.u8   v28, v28, v27       || LSU0.STI64.l v24, i_dst
                VAU.ADIFF.u8 v27, v4,  v24       || LSU0.STI64.h v24, i_adr
PEU.PC1C 0x1 || CMU.VSZMBYTE v24, v_Left, [1111] || LSU1.SWZC4WORD [1111]
                CMU.MAX.u8   v28, v28, v27       || LSU0.STI64.l v24, i_dst
                VAU.ADIFF.u8 v27, v5,  v24       || LSU0.STI64.h v24, i_adr
PEU.PC1C 0x1 || CMU.VSZMBYTE v24, v_Left, [2222] || LSU1.SWZC4WORD [1111]
                CMU.MAX.u8   v28, v28, v27       || LSU0.STI64.l v24, i_dst
                VAU.ADIFF.u8 v27, v6,  v24       || LSU0.STI64.h v24, i_adr
PEU.PC1C 0x1 || CMU.VSZMBYTE v24, v_Left, [3333] || LSU1.SWZC4WORD [1111]
                CMU.MAX.u8   v28, v28, v27       || LSU0.STI64.l v24, i_dst, i1
                VAU.ADIFF.u8 v27, v7,  v24       || LSU0.STI64.h v24, i_adr, i1
PEU.PC1C 0x1 || CMU.VSZMBYTE v24, v_Left, [0000] || LSU1.SWZC4WORD [2222]
                CMU.MAX.u8   v28, v28, v27       || LSU0.STI64.l v24, i_dst
                VAU.ADIFF.u8 v27, v8,  v24       || LSU0.STI64.h v24, i_adr
PEU.PC1C 0x1 || CMU.VSZMBYTE v24, v_Left, [1111] || LSU1.SWZC4WORD [2222]
                CMU.MAX.u8   v28, v28, v27       || LSU0.STI64.l v24, i_dst
                VAU.ADIFF.u8 v27, v9,  v24       || LSU0.STI64.h v24, i_adr
PEU.PC1C 0x1 || CMU.VSZMBYTE v24, v_Left, [2222] || LSU1.SWZC4WORD [2222]
                CMU.MAX.u8   v28, v28, v27       || LSU0.STI64.l v24, i_dst
                VAU.ADIFF.u8 v27, v10, v24       || LSU0.STI64.h v24, i_adr
PEU.PC1C 0x1 || CMU.VSZMBYTE v24, v_Left, [3333] || LSU1.SWZC4WORD [2222]
                CMU.MAX.u8   v28, v28, v27       || LSU0.STI64.l v24, i_dst
                VAU.ADIFF.u8 v27, v11, v24       || LSU0.STI64.h v24, i_adr
PEU.PC1C 0x1 || CMU.VSZMBYTE v24, v_Left, [0000] || LSU1.SWZC4WORD [3333]
                CMU.MAX.u8   v28, v28, v27       || LSU0.STI64.l v24, i_dst
                VAU.ADIFF.u8 v27, v12, v24       || LSU0.STI64.h v24, i_adr
PEU.PC1C 0x1 || CMU.VSZMBYTE v24, v_Left, [1111] || LSU1.SWZC4WORD [3333]
                CMU.MAX.u8   v28, v28, v27       || LSU0.STI64.l v24, i_dst
                VAU.ADIFF.u8 v27, v13, v24       || LSU0.STI64.h v24, i_adr
PEU.PC1C 0x1 || CMU.VSZMBYTE v24, v_Left, [2222] || LSU1.SWZC4WORD [3333]
                CMU.MAX.u8   v28, v28, v27       || LSU0.STI64.l v24, i_dst
                VAU.ADIFF.u8 v27, v14, v24       || LSU0.STI64.h v24, i_adr
PEU.PC1C 0x1 || CMU.VSZMBYTE v24, v_Left, [3333] || LSU1.SWZC4WORD [3333]
                CMU.MAX.u8   v28, v28, v27       || LSU0.STI64.l v24, i_dst, i1  || IAU.SHL i10, i9,  0x10
                VAU.ADIFF.u8 v27, v15, v24       || LSU0.STI64.h v24, i_adr, i1  || IAU.OR  i10, i10, i18
                CMU.CPVV.u8.u16 v24, v24                                 || IAU.SHL i18, i18, 0x04 || SAU.ADD.i16 i10, i10, 0x0C
                CMU.MAX.u8   v28, v28, v27       || VAU.OR v25, v24, v24 || IAU.SHL i9,  i9,  0x04
CMU.VSZMBYTE i10, i10, [220Z]                    || VAU.OR v26, v24, v24 || IAU.ADD i9,  i9,  i8
; get rid of spikes maybe too aggressive at 16 samples level
                SAU.SUM.u8   i18, v28, 0xF       || VAU.OR v27, v24, v24 || IAU.ADD i8,  i8,  i18
PEU.PC1C 0x1 || CMU.CPVV.u8.u16 v24, v_Left
; force i16x16 for now
CMU.CPIS s_YQAddr,  i8 || LSU1.LDIL i0, I_4x4
CMU.CPIS s_UVQAddr, i9 || IAU.SHR.u32 i18, i18, 0x10 || LSU0.STO32 i10, i_MBInfo, mb_icPred
;PEU.PC1I 0x6 || BRU.BRA Intra4x4Luma   || LSU0.STO8 i0, i_MBInfo, mb_type
PEU.PC1I 0x1 || BRU.BRA Intra16x16Luma || LSU0.STO8 i_mode, i_MBInfo, mb_type
PEU.PCCX.EQ 0x28 || CMU.CPVV.u8.u16 v0, v0  || VAU.SWZBYTE v27, v24, [3232] || LSU1.SWZ4V [1111] [1111]
PEU.PCCX.EQ 0x28 || CMU.CPVV.u8.u16 v1, v1  || VAU.SWZBYTE v26, v24, [1010] || LSU1.SWZ4V [1111] [1111]
PEU.PCCX.EQ 0x28 || CMU.CPVV.u8.u16 v2, v2  || VAU.SWZBYTE v25, v24, [3232] || LSU1.SWZ4V [0000] [0000]
PEU.PCCX.EQ 0x28 || CMU.CPVV.u8.u16 v3, v3  || VAU.SWZBYTE v24, v24, [1010] || LSU1.SWZ4V [0000] [0000]
nop

.lalign
MD_InterRefine:
LSU0.LDO64.l v16, i_MBInfo,  mb_mvdL0       || LSU1.LDIL i9, qpc_table
LSU0.LDO64.h v16, i_MBInfo, (mb_mvdL0+0x08) || LSU1.LDIH i9, qpc_table
IAU.ADD i9, i9, i18       || LSU1.LDIL i_src, sampleStore
LSU0.LD32.u8.u32 i9, i9   || LSU1.LDIH i_src, sampleStore
IAU.INCS i_src, 0x100
LSU0.LDI128.u8.u16 v24, i_src || LSU1.LDIL i8, quant_table
LSU0.LDI128.u8.u16 v25, i_src || LSU1.LDIH i8, quant_table
LSU0.LDI128.u8.u16 v26, i_src
LSU0.LDI128.u8.u16 v27, i_src
CMU.CPVV.u8.u16 v0, v0 || IAU.SHL i10, i9,  0x10
CMU.CPVV.u8.u16 v1, v1 || IAU.OR  i10, i10, i18      || SAU.SHL.i32 i9, i9, 0x04
CMU.CPVV.u8.u16 v2, v2 || IAU.SHR.u32 i6, i18, 0x02  || SAU.ADD.i16 i10, i10, 0x0C
CMU.CPVV.u8.u16 v3, v3 || IAU.SHL i6, i6,  0x08
                          IAU.INCS i6, 0x100         || VAU.SWZWORD v17, v16, [0123]
                          CMU.CMVV.u32 v16, v16      || IAU.XOR  i0, i0, i0          || LSU1.SWZC4WORD [1331]
PEU.ANDACC             || CMU.CMVV.u32 v17, v17                                      || LSU1.SWZC4WORD [1331]
PEU.PC2C.AND  0x1      || IAU.INCS i0, 0x01
CMU.CMII.i32 i6, i7    || SAU.SHL.i32 i18, i18, 0x04
PEU.PCC0I.OR  0x5, 0x1 || BRU.BRA InterLuma
PEU.PCC0I.AND 0x2, 0x6 || BRU.BRA InterSkip
IAU.ADD i9,  i9,  i8   || LSU1.LDIL i_mode, P_8x8
IAU.ADD i8,  i8,  i18  || SAU.SWZBYTE i10, i10, [2201]
CMU.CPIS s_UVQAddr, i9 || LSU0.STO32 i10, i_MBInfo, mb_icPred
CMU.CPIS s_YQAddr,  i8 || LSU0.STO8 i_mode, i_MBInfo, mb_type
nop

