.version 00.40.00

.code .text.Coeff2x2
.lalign
Coeff2x2:
                     CMU.VNZ.x32 i2, v0, 0x0
                     CMU.VNZ.x32 i2, v1, 0x1
                     CMU.CPVI i1, v0.0 || IAU.FEXT.u32 i0, i2, 0x00, 0x01
PEU.PCIX.NEQ 0x10 || CMU.CPVI i1, v0.1 || IAU.FEXT.u32 i0, i2, 0x01, 0x01 || LSU0.STI16 i1, i_Coeff
PEU.PCIX.NEQ 0x10 || CMU.CPVI i1, v0.2 || IAU.FEXT.u32 i0, i2, 0x02, 0x01 || LSU0.STI16 i1, i_Coeff
PEU.PCIX.NEQ 0x10 || CMU.CPVI i1, v0.3 || IAU.FEXT.u32 i0, i2, 0x03, 0x01 || LSU0.STI16 i1, i_Coeff
BRU.JMP i30       || LSU0.ST16 i2, i18
PEU.PCIX.NEQ 0x10 || CMU.CPVI i1, v1.0 || IAU.FEXT.u32 i0, i2, 0x08, 0x01 || LSU0.STI16 i1, i_Coeff
PEU.PCIX.NEQ 0x10 || CMU.CPVI i1, v1.1 || IAU.FEXT.u32 i0, i2, 0x09, 0x01 || LSU0.STI16 i1, i_Coeff
PEU.PCIX.NEQ 0x10 || CMU.CPVI i1, v1.2 || IAU.FEXT.u32 i0, i2, 0x0A, 0x01 || LSU0.STI16 i1, i_Coeff
PEU.PCIX.NEQ 0x10 || CMU.CPVI i1, v1.3 || IAU.FEXT.u32 i0, i2, 0x0B, 0x01 || LSU0.STI16 i1, i_Coeff
PEU.PCIX.NEQ 0x10 || CMU.CMZ.i32 i_smb                                    || LSU0.STI16 i1, i_Coeff


.lalign
Coeff4x4:
                     CMU.VNZD.x32 i2, v0, 0x0
                     CMU.VNZD.x32 i2, v2, 0x1
                     CMU.CPVI i1, v0.0 || IAU.FEXT.u32 i0, i2, 0x00, 0x01
PEU.PCIX.NEQ 0x10 || CMU.CPVI i1, v0.1 || IAU.FEXT.u32 i0, i2, 0x01, 0x01 || LSU0.STI16 i1, i_Coeff
PEU.PCIX.NEQ 0x10 || CMU.CPVI i1, v0.2 || IAU.FEXT.u32 i0, i2, 0x02, 0x01 || LSU0.STI16 i1, i_Coeff
PEU.PCIX.NEQ 0x10 || CMU.CPVI i1, v0.3 || IAU.FEXT.u32 i0, i2, 0x03, 0x01 || LSU0.STI16 i1, i_Coeff
PEU.PCIX.NEQ 0x10 || CMU.CPVI i1, v1.0 || IAU.FEXT.u32 i0, i2, 0x04, 0x01 || LSU0.STI16 i1, i_Coeff
PEU.PCIX.NEQ 0x10 || CMU.CPVI i1, v1.1 || IAU.FEXT.u32 i0, i2, 0x05, 0x01 || LSU0.STI16 i1, i_Coeff
PEU.PCIX.NEQ 0x10 || CMU.CPVI i1, v1.2 || IAU.FEXT.u32 i0, i2, 0x06, 0x01 || LSU0.STI16 i1, i_Coeff
PEU.PCIX.NEQ 0x10 || CMU.CPVI i1, v1.3 || IAU.FEXT.u32 i0, i2, 0x07, 0x01 || LSU0.STI16 i1, i_Coeff
PEU.PCIX.NEQ 0x10 || CMU.CPVI i1, v2.0 || IAU.FEXT.u32 i0, i2, 0x08, 0x01 || LSU0.STI16 i1, i_Coeff
PEU.PCIX.NEQ 0x10 || CMU.CPVI i1, v2.1 || IAU.FEXT.u32 i0, i2, 0x09, 0x01 || LSU0.STI16 i1, i_Coeff
PEU.PCIX.NEQ 0x10 || CMU.CPVI i1, v2.2 || IAU.FEXT.u32 i0, i2, 0x0A, 0x01 || LSU0.STI16 i1, i_Coeff
PEU.PCIX.NEQ 0x10 || CMU.CPVI i1, v2.3 || IAU.FEXT.u32 i0, i2, 0x0B, 0x01 || LSU0.STI16 i1, i_Coeff
BRU.JMP i30       || LSU0.ST16 i2, i18
PEU.PCIX.NEQ 0x10 || CMU.CPVI i1, v3.0 || IAU.FEXT.u32 i0, i2, 0x0C, 0x01 || LSU0.STI16 i1, i_Coeff
PEU.PCIX.NEQ 0x10 || CMU.CPVI i1, v3.1 || IAU.FEXT.u32 i0, i2, 0x0D, 0x01 || LSU0.STI16 i1, i_Coeff
PEU.PCIX.NEQ 0x10 || CMU.CPVI i1, v3.2 || IAU.FEXT.u32 i0, i2, 0x0E, 0x01 || LSU0.STI16 i1, i_Coeff
PEU.PCIX.NEQ 0x10 || CMU.CPVI i1, v3.3 || IAU.FEXT.u32 i0, i2, 0x0F, 0x01 || LSU0.STI16 i1, i_Coeff
PEU.PCIX.NEQ 0x10 || CMU.CMZ.i32 i_smb                                    || LSU0.STI16 i1, i_Coeff

.lalign
RLE_Luma:
RLE_LumaBlock0:
CMU.CMZ.i32 i_smb || IAU.FEXT.u32 i0, i_cbp, 0x00, 0x01 || LSU1.LDIL i_src, coeffStore
CMU.VSZMBYTE i_smb, i_smb, [ZZZZ] || VAU.XOR v4, v4, v4 || LSU1.LDIH i_src, coeffStore
; skip block
PEU.PC1I 0x1      || CMU.CMII.i32 i_smb, i_smb  || IAU.INCS i_src, 0x80 || LSU1.LDIL i_smb, 0x03
RLE_LumaLoop0:
PEU.PC1C 0x6      || IAU.SHL  i0, i_smb, 0x01   || LSU0.LDI128.i16.i32 v0, i_src
PEU.PC1C 0x6      || IAU.ADD  i18, i_MBInfo, i0 || LSU0.LDI128.i16.i32 v1, i_src
PEU.PC1C 0x6      || BRU.BRA Coeff4x4           || LSU0.LDI128.i16.i32 v2, i_src || LSU1.LDIL i30, RLE_LumaLoop0
PEU.PC1C 0x6      || IAU.INCS i18, mb_lumaACMap || LSU0.LDI128.i16.i32 v3, i_src || LSU1.LDIH i30, RLE_LumaLoop0
PEU.PCCX.EQ 0x10  || IAU.INCS i_smb, 0x01       || LSU0.STO64.l v4, i_MBInfo, mb_lumaACMap
                     IAU.CMPI i_smb, 0x04
PEU.PC1I 0x3      || LSU1.LDIL i30, RLE_LumaBlock1
PEU.PC1I 0x3      || LSU1.LDIH i30, RLE_LumaBlock1
RLE_LumaBlock1:
CMU.CMZ.i32 i_smb || IAU.FEXT.u32 i0, i_cbp, 0x01, 0x01 
; skip block
PEU.PC1I 0x1      || CMU.CMII.i32 i_smb, i_smb  || IAU.INCS i_src, 0x80 || LSU1.LDIL i_smb, 0x07
RLE_LumaLoop1:
PEU.PC1C 0x6      || IAU.SHL  i0, i_smb, 0x01   || LSU0.LDI128.i16.i32 v0, i_src
PEU.PC1C 0x6      || IAU.ADD  i18, i_MBInfo, i0 || LSU0.LDI128.i16.i32 v1, i_src
PEU.PC1C 0x6      || BRU.BRA Coeff4x4           || LSU0.LDI128.i16.i32 v2, i_src || LSU1.LDIL i30, RLE_LumaLoop1
PEU.PC1C 0x6      || IAU.INCS i18, mb_lumaACMap || LSU0.LDI128.i16.i32 v3, i_src || LSU1.LDIH i30, RLE_LumaLoop1
PEU.PCCX.EQ 0x10  || IAU.INCS i_smb, 0x01       || LSU0.STO64.l v4, i_MBInfo, (mb_lumaACMap+0x08)
                     IAU.CMPI i_smb, 0x08
PEU.PC1I 0x3      || LSU1.LDIL i30, RLE_LumaBlock2
PEU.PC1I 0x3      || LSU1.LDIH i30, RLE_LumaBlock2
RLE_LumaBlock2:
CMU.CMZ.i32 i_smb || IAU.FEXT.u32 i0, i_cbp, 0x02, 0x01
; skip block
PEU.PC1I 0x1      || CMU.CMII.i32 i_smb, i_smb  || IAU.INCS i_src, 0x80 || LSU1.LDIL i_smb, 0x0B
RLE_LumaLoop2:
PEU.PC1C 0x6      || IAU.SHL  i0, i_smb, 0x01   || LSU0.LDI128.i16.i32 v0, i_src
PEU.PC1C 0x6      || IAU.ADD  i18, i_MBInfo, i0 || LSU0.LDI128.i16.i32 v1, i_src
PEU.PC1C 0x6      || BRU.BRA Coeff4x4           || LSU0.LDI128.i16.i32 v2, i_src || LSU1.LDIL i30, RLE_LumaLoop2
PEU.PC1C 0x6      || IAU.INCS i18, mb_lumaACMap || LSU0.LDI128.i16.i32 v3, i_src || LSU1.LDIH i30, RLE_LumaLoop2
PEU.PCCX.EQ 0x10  || IAU.INCS i_smb, 0x01       || LSU0.STO64.l v4, i_MBInfo, (mb_lumaACMap+0x10)
                     IAU.CMPI i_smb, 0x0C
PEU.PC1I 0x3      || LSU1.LDIL i30, RLE_LumaBlock3
PEU.PC1I 0x3      || LSU1.LDIH i30, RLE_LumaBlock3
RLE_LumaBlock3:
CMU.CPSI i30, s30
CMU.CMZ.i32 i_smb || IAU.FEXT.u32 i0, i_cbp, 0x03, 0x01
; skip block
PEU.PC1I 0x1      || CMU.CMII.i32 i_smb, i_smb  || BRU.JMP i30 || LSU1.LDIL i_smb, 0x0F
RLE_LumaLoop3:
PEU.PC1C 0x6      || IAU.SHL  i0, i_smb, 0x01   || LSU0.LDI128.i16.i32 v0, i_src
PEU.PC1C 0x6      || IAU.ADD  i18, i_MBInfo, i0 || LSU0.LDI128.i16.i32 v1, i_src
PEU.PC1C 0x6      || BRU.BRA Coeff4x4           || LSU0.LDI128.i16.i32 v2, i_src || LSU1.LDIL i30, RLE_LumaLoop3
PEU.PC1C 0x6      || IAU.INCS i18, mb_lumaACMap || LSU0.LDI128.i16.i32 v3, i_src || LSU1.LDIH i30, RLE_LumaLoop3
PEU.PCCX.EQ 0x10  || IAU.INCS i_smb, 0x01       || LSU0.STO64.l v4, i_MBInfo, (mb_lumaACMap+0x18)
                     IAU.CMPI i_smb, 0x10
PEU.PC1I 0x3      || CMU.CPSI i30, s30
nop

.lalign
RLE_Chroma:
CMU.CMZ.i32 i_smb || IAU.FEXT.u32 i0, i_cbp, 0x05, 0x01 || VAU.XOR v4, v4, v4
CMU.VSZMBYTE i_smb, i_smb, [ZZZZ]  || LSU1.LDIL i_src, coeffStore
CMU.CPSI i30, s30                  || LSU1.LDIH i_src, coeffStore
; skip block
PEU.PC1I 0x1      || CMU.CMII.i32 i_smb, i_smb    || BRU.JMP i30 || LSU1.LDIL i_smb, 0x07
RLE_ChromaLoop:
PEU.PC1C 0x6      || IAU.SHL  i0, i_smb, 0x01     || LSU0.LDI128.i16.i32 v0, i_src
PEU.PC1C 0x6      || IAU.ADD  i18, i_MBInfo, i0   || LSU0.LDI128.i16.i32 v1, i_src
PEU.PC1C 0x6      || BRU.BRA Coeff4x4             || LSU0.LDI128.i16.i32 v2, i_src || LSU1.LDIL i30, RLE_ChromaLoop
PEU.PC1C 0x6      || IAU.INCS i18, mb_chromaACMap || LSU0.LDI128.i16.i32 v3, i_src || LSU1.LDIH i30, RLE_ChromaLoop
PEU.PCCX.EQ 0x10  || IAU.INCS i_smb, 0x01         || LSU0.STO64.l v4, i_MBInfo,  mb_chromaACMap
PEU.PCCX.EQ 0x10  || IAU.CMPI i_smb, 0x08         || LSU0.STO64.l v4, i_MBInfo, (mb_chromaACMap+0x08)
PEU.PC1I 0x3      || CMU.CPSI i30, s30
nop 2

; forward transform
; input:  i30, return address
;         v_QParam, quantise params
;         v_Residual0-3, residuals
; output: v_DC0-1, DC coeffs
;         v_Coeff0-3, coeffs
.lalign
forward_4x4:
;trans
CMU.ROT4L v0, v_Residual0  || IAU.ADD i0, i_smb, 0x02
;t0 = p0+p3
VAU.ADD.i16 v0, v3, v0     || IAU.SHR.u32 i0, i0, 0x01     || LSU1.LDIL i1, sampleStore
;t3 = p0-p3
VAU.SUB.i16 v3, v3, v0     || IAU.SHL i0, i0, 0x05         || LSU1.LDIH i1, sampleStore
;t1 = p1+p2
VAU.ADD.i16 v1, v2, v1     || IAU.ADD i1, i1, i0           || LSU1.LDIL i0, 0x100
;t2 = p1-p2
VAU.SUB.i16 v2, v2, v1     || IAU.ADD i0, i0, i1
;t0+t1
VAU.ADD.i16 v_Coeff0, v0, v1  || LSU0.LDI128.u8.u16 v_Residual0, i1
;t0-t1
VAU.SUB.i16 v_Coeff2, v0, v1  || LSU0.LDI128.u8.u16 v_Residual1, i1
;t3+t2
VAU.ADD.i16 v_Coeff1, v3, v2  || LSU0.LDI128.u8.u16 v_Residual2, i1
;t3-t2
VAU.SUB.i16 v_Coeff3, v3, v2  || LSU0.LDI128.u8.u16 v_Residual3, i1
; t3<<1 + t2
VAU.ADD.i16 v_Coeff1, v_Coeff1, v3
; t3 - t2<<1
VAU.SUB.i16 v_Coeff3, v_Coeff3, v2
nop
;trans
CMU.ROT4L v0, v_Coeff0
;t0 = p0+p3
VAU.ADD.i16 v0, v3, v0        || LSU0.LDI128.u8.u16 v_Pred0, i0
;t3 = p0-p3
VAU.SUB.i16 v3, v3, v0        || LSU0.LDI128.u8.u16 v_Pred1, i0
;t1 = p1+p2
VAU.ADD.i16 v1, v2, v1        || LSU0.LDI128.u8.u16 v_Pred2, i0
;t2 = p1-p2
VAU.SUB.i16 v2, v2, v1        || LSU0.LDI128.u8.u16 v_Pred3, i0
;t0+t1
VAU.ADD.i16 v_Coeff0, v0, v1
;t0-t1
VAU.SUB.i16 v_Coeff2, v0, v1
;t3+t2
VAU.ADD.i16 v_Coeff1, v3, v2      || CMU.CPVI.x32 i1, v_Coeff0.0
;t3-t2
VAU.SUB.i16 v_Coeff3, v3, v2      || CMU.CPVV.i16.i32 v9, v_QParam
; t3<<1 + t2
VAU.ADD.i16 v_Coeff1, v_Coeff1, v3  || IAU.SHR.u32 i0, i_smb, 0x01
; t3 - t2<<1
VAU.SUB.i16 v_Coeff3, v_Coeff3, v2  || IAU.INCS i0, 0x10
; accelerated quant
CMU.LUTW32 i1, i0, 0x3
                                     VAU.ABS.i16 v0, v_Coeff0
CMU.VSZMWORD v1, v0, [1032]       || VAU.ABS.i16 v2, v_Coeff1
CMU.CPVV.i16.i32 v0, v0           || VAU.ABS.i16 v4, v_Coeff2
CMU.CPVV.i16.i32 v1, v1           || VAU.ABS.i16 v6, v_Coeff3
CMU.VSZMWORD v3, v2, [1032]
CMU.CPVV.i16.i32 v2, v2           || VAU.SWZWORD v8, v_QParam, [2222]
CMU.CPVV.i16.i32 v3, v3           || VAU.MUL.i32 v0, v0, v9   || LSU1.SWZ4V [3210] [1100]
CMU.VSZMWORD v5, v4, [1032]       || VAU.MUL.i32 v1, v1, v9   || LSU1.SWZ4V [3210] [1100]
CMU.CPVV.i16.i32 v4, v4           || VAU.MUL.i32 v2, v2, v9   || LSU1.SWZ4V [3210] [2211]
CMU.CPVV.i16.i32 v5, v5           || VAU.MUL.i32 v3, v3, v9   || LSU1.SWZ4V [3210] [2211]
CMU.VSZMWORD v7, v6, [1032]       || VAU.MUL.i32 v4, v4, v9   || LSU1.SWZ4V [3210] [1100]
CMU.CPVV.i16.i32 v6, v6           || VAU.MUL.i32 v5, v5, v9   || LSU1.SWZ4V [3210] [1100]
CMU.CPVV.i16.i32 v7, v7           || VAU.MUL.i32 v6, v6, v9   || LSU1.SWZ4V [3210] [2211]
CMU.VSZMBYTE v8, v8, [Z100]       || VAU.MUL.i32 v7, v7, v9   || LSU1.SWZ4V [3210] [2211]
; check for double swizzle if it works on board and file bug for ssim
;                                     VAU.ABS.i16 v0, v_Coeff0
;                                     VAU.ABS.i16 v2, v_Coeff1
;CMU.CPVV.i16.i32 v1, v0           || VAU.ABS.i16 v4, v_Coeff2                             || LSU0.SWZC4WORD [1032]
;CMU.CPVV.i16.i32 v0, v0           || VAU.ABS.i16 v6, v_Coeff3
;CMU.CPVV.i16.i32 v3, v2           || VAU.MUL.i32 v0, v0, v9   || LSU1.SWZ4V [3210] [1100] || LSU0.SWZC4WORD [1032]
;CMU.CPVV.i16.i32 v2, v2           || VAU.MUL.i32 v1, v1, v9   || LSU1.SWZ4V [3210] [1100]
;CMU.CPVV.i16.i32 v5, v4           || VAU.MUL.i32 v2, v2, v9   || LSU1.SWZ4V [3210] [2211] || LSU0.SWZC4WORD [1032]
;CMU.CPVV.i16.i32 v4, v4           || VAU.MUL.i32 v3, v3, v9   || LSU1.SWZ4V [3210] [2211]
;CMU.CPVV.i16.i32 v7, v6           || VAU.MUL.i32 v4, v4, v9   || LSU1.SWZ4V [3210] [1100] || LSU0.SWZC4WORD [1032]
;CMU.CPVV.i16.i32 v6, v6           || VAU.MUL.i32 v5, v5, v9   || LSU1.SWZ4V [3210] [1100]
;CMU.VSZMWORD v8, v_QParam, [2222] || VAU.MUL.i32 v6, v6, v9   || LSU1.SWZ4V [3210] [2211]
;CMU.VSZMBYTE v8, v8, [Z100]       || VAU.MUL.i32 v7, v7, v9   || LSU1.SWZ4V [3210] [2211]
CMU.VSZMWORD v9, v9, [3333]
CMU.CPVV v12, v_Coeff0            || VAU.ADD.i32 v0, v0, v8
CMU.CPVV v13, v_Coeff1            || VAU.ADD.i32 v1, v1, v8
CMU.CPVV v14, v_Coeff2            || VAU.ADD.i32 v2, v2, v8
CMU.CPVV v15, v_Coeff3            || VAU.ADD.i32 v3, v3, v8
                                     VAU.ADD.i32 v4, v4, v8
                                     VAU.ADD.i32 v5, v5, v8
                                     VAU.ADD.i32 v6, v6, v8
                                     VAU.ADD.i32 v7, v7, v8
                                     VAU.SHR.i32 v0, v0, v9
CMU.VSZMBYTE v8, v8, [ZZZZ]       || VAU.SHR.i32 v1, v1, v9
CMU.CPVV.i32.i16s v_Coeff0, v0    || VAU.SHR.i32 v2, v2, v9
CMU.CPVV.i32.i16s v1, v1          || VAU.SHR.i32 v3, v3, v9
CMU.CPVV.i32.i16s v_Coeff1, v2    || VAU.SHR.i32 v4, v4, v9
CMU.CPVV.i32.i16s v3, v3          || VAU.SHR.i32 v5, v5, v9
CMU.CPVV.i32.i16s v_Coeff2, v4    || VAU.SHR.i32 v6, v6, v9
CMU.CPVV.i32.i16s v5, v5          || VAU.SHR.i32 v7, v7, v9
CMU.CPVV.i32.i16s v_Coeff3, v6    || VAU.ADD.i16 v_Coeff0, v1, 0 || LSU1.SWZ4V [1032] [3210] || PEU.PVEN4WORD 0xC
CMU.CPVV.i32.i16s v7, v7          || VAU.ADD.i16 v_Coeff1, v3, 0 || LSU1.SWZ4V [1032] [3210] || PEU.PVEN4WORD 0xC
                 BRU.JMP i30      || VAU.ADD.i16 v_Coeff2, v5, 0 || LSU1.SWZ4V [1032] [3210] || PEU.PVEN4WORD 0xC
                 CMU.CMZ.i16 v12  || VAU.ADD.i16 v_Coeff3, v7, 0 || LSU1.SWZ4V [1032] [3210] || PEU.PVEN4WORD 0xC
PEU.PVV16 0x4 || CMU.CMZ.i16 v13  || VAU.SUB.i16 v_Coeff0, v8, v_Coeff0
PEU.PVV16 0x4 || CMU.CMZ.i16 v14  || VAU.SUB.i16 v_Coeff1, v8, v_Coeff1
PEU.PVV16 0x4 || CMU.CMZ.i16 v15  || VAU.SUB.i16 v_Coeff2, v8, v_Coeff2
PEU.PVV16 0x4                     || VAU.SUB.i16 v_Coeff3, v8, v_Coeff3


; inverse transform
; input:  i30, return address
;         v_QParam, quantise params
;         v_DC0-1, DC coeffs
;         v_Coeff0-3, coeffs
; output: v_Residual0-3, residuals
.lalign
inverse_4x4:
IAU.SHR.u32 i0, i_smb, 0x01 ||  VAU.MUL.i16  v_Residual0, v_Coeff0, v_QParam || LSU0.LDI64.l v12, i_src || LSU1.SWZV8 [66556655]
IAU.INCS i0, 0x10           ||  VAU.MUL.i16  v_Residual1, v_Coeff1, v_QParam || LSU0.LDI64.h v12, i_src || LSU1.SWZV8 [77667766]
CMU.LUT32.u32 i1, i0, 0x3   ||  VAU.MUL.i16  v_Residual2, v_Coeff2, v_QParam || LSU0.LDI64.l v13, i_src || LSU1.SWZV8 [66556655]
IAU.SHR.u32 i0, i_smb, 0x01 ||  VAU.MUL.i16  v_Residual3, v_Coeff3, v_QParam || LSU0.LDI64.h v13, i_src || LSU1.SWZV8 [77667766]
CMU.CMZ.i32 i1  || IAU.SHL i0, i0, 0x05
PEU.PC1C 0x6 || CMU.CPIV.x32 v_Residual0.0, i1
;trans
CMU.TP4R v0, v_Residual0           || IAU.INCS i0, 0x100
; do the shifts p1>>1
VAU.SHR.i16 v_Residual0, v1, 0x01  || LSU0.LDI64.l v14, i_src  || LSU1.LDIL i1, sampleStore
; do the shifts p3>>1
VAU.SHR.i16 v_Residual1, v3, 0x01  || LSU0.LDI64.h v14, i_src  || LSU1.LDIH i1, sampleStore
; s02 = p0+p2
VAU.ADD.i16 v0, v0, v2             || LSU0.LDI64.l v15, i_src  || IAU.ADD i0, i0, i1
; d02 = p0-p2
VAU.SUB.i16 v1, v0, v2             || LSU0.LDI64.h v15, i_src
; s13 = p1 + (p3>>1)
VAU.ADD.i16 v3, v1, v_Residual1    || LSU0.LDI128.u8.u16 v_Pred0, i0
; d13 = (p1>>1) - p3
VAU.SUB.i16 v2, v_Residual0, v3    || LSU0.LDI128.u8.u16 v_Pred1, i0
;s02+s13
VAU.ADD.i16 v_Residual0, v0, v3    || LSU0.LDI128.u8.u16 v_Pred2, i0
;d02+d13
VAU.ADD.i16 v_Residual1, v1, v2    || LSU0.LDI128.u8.u16 v_Pred3, i0
;d02-d13
VAU.SUB.i16 v_Residual2, v1, v2    || CMU.VILV.x16 v_Coeff0, v_Coeff1, v14, v12
;s02-s13
VAU.SUB.i16 v_Residual3, v0, v3    || CMU.VILV.x16 v_Coeff2, v_Coeff3, v15, v13
nop
; trans
CMU.TP4R v0, v_Residual0
; do the shifts p1>>1
VAU.SHR.i16 v_Residual0, v1, 0x01
; do the shifts p3>>1
VAU.SHR.i16 v_Residual1, v3, 0x01
; s02 = p0+p2
VAU.ADD.i16 v0, v0, v2
; d02 = p0-p2
VAU.SUB.i16 v1, v0, v2
; s13 = p1 + (p3>>1)
VAU.ADD.i16 v3, v1, v_Residual1
; d13 = (p1>>1) - p3
VAU.SUB.i16 v2, v_Residual0, v3
;s02+s13
VAU.ADD.i16 v_Residual0, v0, v3
;d02+d13
VAU.ADD.i16 v_Residual1, v1, v2
;d02-d13
VAU.SUB.i16 v_Residual2, v1, v2 || LSU1.LDIL i0, 0x20
;s02-s13
VAU.SUB.i16 v_Residual3, v0, v3 || CMU.CPIVR.x16 v8, i0
; apply rounding
VAU.ADD.i16 v_Residual0, v_Residual0, v8
VAU.ADD.i16 v_Residual1, v_Residual1, v8
VAU.ADD.i16 v_Residual2, v_Residual2, v8   || BRU.JMP i30
VAU.ADD.i16 v_Residual3, v_Residual3, v8
VAU.SHR.i16 v_Residual0, v_Residual0, 0x06
VAU.SHR.i16 v_Residual1, v_Residual1, 0x06
VAU.SHR.i16 v_Residual2, v_Residual2, 0x06
VAU.SHR.i16 v_Residual3, v_Residual3, 0x06

