.version 00.40.00

.code .text.ME_2LT
; local functions
.lalign
ME_2LT:
BRU.BRA ME_Write
CMU.VROT v4,  v4,  0xE  || VAU.ALIGNVEC v5,  v5,  v5,  0xE
CMU.VROT v6,  v6,  0xE  || VAU.ALIGNVEC v7,  v7,  v7,  0xE
CMU.VROT v8,  v8,  0xE  || VAU.ALIGNVEC v9,  v9,  v9,  0xE
CMU.VROT v10, v10, 0xE  || VAU.ALIGNVEC v11, v11, v11, 0xE
nop

.lalign
ME_2RT:
BRU.BRA ME_Write
CMU.VROT v4,  v4,  0x2  || VAU.ALIGNVEC v5,  v5,  v5,  0x2
CMU.VROT v6,  v6,  0x2  || VAU.ALIGNVEC v7,  v7,  v7,  0x2
CMU.VROT v8,  v8,  0x2  || VAU.ALIGNVEC v9,  v9,  v9,  0x2
CMU.VROT v10, v10, 0x2  || VAU.ALIGNVEC v11, v11, v11, 0x2
nop


; global functions
.lalign
ME:
CMU.CPVI.x32 i6, v_cmd.2 || IAU.SUB i4, i_posx, i_rowStart                                 || LSU1.LDIL i_dst, origLuma
CMU.CPVI.x32 i7, v_cmd.3 || IAU.SHL i5, i_desc, 0x04                                       || LSU1.LDIH i_dst, origLuma
                            IAU.MUL i6, i6, i5                                             || LSU1.LDIL i_src, refLuma
                            IAU.MUL i7, i7, i5             || SAU.ADD.i32 i_dst, i_dst, i4 || LSU1.LDIH i_src, refLuma
CMU.CPIS s20, i_posx                                       || SAU.ADD.i32 i_src, i_src, i4
CMU.CPIS s21, i_posy     || IAU.ADD i_dst, i_dst, i6
CMU.CPIS s22, i_dst      || IAU.ADD i_src, i_src, i7
CMU.CPIS s23, i_src      || LSU1.LDIL i_smb, 0

.lalign
ME_8Pac:
                               CMU.VSZMBYTE i8,  i8,  [ZZZZ]   || IAU.SUB i0, i_posx, i_rowStart || LSU0.LDI64.l v12, i_dst, i_desc || LSU1.LDIL i4, predMVH
                               CMU.VSZMBYTE i9,  i9,  [ZZZZ]   || IAU.SHR.u32 i0, i0, 0x02       || LSU0.LDI64.h v12, i_dst, i_desc || LSU1.LDIH i4, predMVH
                               CMU.VSZMBYTE i10, i10, [ZZZZ]   || IAU.ADD i4, i4, i0             || LSU0.LDI64.l v13, i_dst, i_desc || LSU1.LDIL i5, predMVV
                               CMU.VSZMBYTE i11, i11, [ZZZZ]   || IAU.SHR.u32 i1, i_posy, 0x02   || LSU0.LDI64.h v13, i_dst, i_desc || LSU1.LDIH i5, predMVV
                               CMU.VSZMBYTE i12, i12, [ZZZZ]   || IAU.ADD i5, i5, i1             || LSU0.LDI64.l v14, i_dst, i_desc || LSU1.LDIL i2, refLuma
                               CMU.VSZMBYTE i13, i13, [ZZZZ]                                     || LSU0.LDI64.h v14, i_dst, i_desc || LSU1.LDIH i2, refLuma
                               CMU.VSZMBYTE i14, i14, [ZZZZ]   || IAU.SHL i3, i_desc, 0x06       || LSU0.LDI64.l v15, i_dst, i_desc
                               CMU.VSZMBYTE i15, i15, [ZZZZ]   || IAU.CMPI i0, 0                 || LSU0.LDI64.h v15, i_dst, i_desc
PEU.PCIX.NEQ 0x10           || CMU.CMZ.i32 i1                  || IAU.SHL i1, i_desc, 0x03       || LSU0.LD16 i6, i4
PEU.PCCX.NEQ 0x10           || SAU.ADD.u32s i5, i_src, i1                                        || LSU0.LD16 i7, i5
                               SAU.ADD.u32s i2, i2, i3         || IAU.SUB.u32s i4, i_src, i1     || LSU0.LDA64.l v_mvc, 0, MVC_lo   || LSU1.LDIL i6, 0
                               CMU.CMII.i32 i_posx, i_rowStart || IAU.SUB.u32s i0, i2, i4        || LSU0.LDA64.h v_mvc, 0, MVC_hi   || LSU1.LDIL i7, 0
PEU.PCIX.NEQ 0x02           || SAU.ADD.u32s i4, i4, i3         || IAU.SUB.u32s i0, i2, i5        || LSU1.LDIL i1, 0
PEU.PCIX.EQ  0x02           || SAU.SUB.u32s i5, i5, i3         || IAU.SUB i0,  i_rowEnd, 0x08    || LSU1.LDIH i1, 0x0001
PEU.PCCX.EQ  0x05           || CMU.CPII i12, i1                || IAU.ADD i14, i1, 0
PEU.PCCX.EQ  0x01           || SAU.ADD.i8 i18, i6, i7          || IAU.ADD i8,  i1, 0
                               CMU.CMII.i32 i_posx, i0         || IAU.ADD i2,  i_src, i_desc     || LSU0.LD64.l  v0,  i_src
SAU.SHR.i8 i18, i18, 0x01   || CMU.CPSI i0, s_height           || IAU.SUB i3,  i_src, 0x08       || LSU0.LDI64.h v0,  i2, i_desc
PEU.PCCX.EQ  0x05           || CMU.CPII i13, i1                || IAU.ADD i15, i1, 0             || LSU0.LDI64.l v1,  i2, i_desc
PEU.PCCX.EQ  0x01           || CMU.CPIVR.x16 v_mv, i18         || IAU.ADD i9,  i1, 0             || LSU0.LDI64.h v1,  i2, i_desc
VAU.SUB.i8 v27, v_mv, v_mvc || CMU.CMZ.i32 i_posy              || IAU.SUB i0, i0, 0x08           || LSU0.LDI64.l v2,  i2, i_desc
PEU.PCCX.EQ  0x05           || CMU.CPII i12, i1                || IAU.ADD i13, i1, 0             || LSU0.LDI64.h v2,  i2, i_desc
PEU.PCCX.EQ  0x04           || CMU.CPII i10, i1                || VAU.ABS.i8 v27, v27            || LSU0.LDI64.l v3,  i2, i_desc
SAU.ABS.i8 i_best, i18      || CMU.CMII.i32 i_posy, i0         || VAU.ADIFF.u8 v24, v12, v0      || LSU0.LDI64.h v3,  i2, i_desc
PEU.PCCX.EQ  0x05           || CMU.CPII i14, i1                || IAU.ADD i15, i1, 0             || LSU0.LDI64.l v4,  i3, i_desc || SAU.SUM.u8 i_best, i_best
PEU.PCCX.EQ  0x04           || CMU.CPII i11, i1                || VAU.ADIFF.u8 v24, v13, v1      || LSU0.LDI64.h v4,  i3, i_desc
                               SAU.SUM.u8 i0, v24, 0xF         || IAU.ADD i2, i_src, 0x08        || LSU0.LDI64.l v5,  i3, i_desc
IAU.SHL i_best, i_best, 0x4 || CMU.VSZMBYTE v_amv, v27, [Z3Z1] || VAU.ADIFF.u8 v24, v14, v2      || LSU0.LDI64.h v5,  i3, i_desc
IAU.ADD i_best, i_best, i0  || SAU.SUM.u8 i0, v24, 0xF                                           || LSU0.LDI64.l v6,  i3, i_desc
                               CMU.VSZMBYTE v27, v27, [Z2Z0]   || VAU.ADIFF.u8 v24, v15, v3      || LSU0.LDI64.h v6,  i3, i_desc
IAU.ADD i_best, i_best, i0  || SAU.SUM.u8 i0, v24, 0xF         || VAU.ADD.i16 v_amv, v27, v_amv  || LSU0.LDI64.l v7,  i3, i_desc
IAU.ADD i2, i_src, 0x08     || CMU.CPIS s18, i_posx            || VAU.ADIFF.u8 v25, v12, v4      || LSU0.LDI64.h v7,  i3, i_desc
IAU.ADD i_best, i_best, i0  || SAU.SUM.u8 i0, v24, 0xF         || VAU.SHL.u16 v_amv, v_amv, 0x04 || LSU0.LDI64.l v8,  i2, i_desc
IAU.ADD i6, i_src, 0        || CMU.CPIS s19, i_posy            || VAU.ADIFF.u8 v25, v13, v5      || LSU0.LDI64.h v8,  i2, i_desc
IAU.ADD i_best, i_best, i0  || SAU.SUM.u8 i0, v25, 0xF                                           || LSU0.LDI64.l v9,  i2, i_desc
                               CMU.CPVI.x16 i8.l,  v_amv.0     || VAU.ADIFF.u8 v25, v14, v6      || LSU0.LDI64.h v9,  i2, i_desc
IAU.ADD i8,  i8,  i0        || SAU.SUM.u8 i0, v25, 0xF                                           || LSU0.LDI64.l v10, i2, i_desc
                               CMU.CPVI.x16 i9.l,  v_amv.1     || VAU.ADIFF.u8 v25, v15, v7      || LSU0.LDI64.h v10, i2, i_desc
IAU.ADD i8,  i8,  i0        || SAU.SUM.u8 i0, v25, 0xF                                           || LSU0.LDI64.l v11, i2, i_desc
IAU.ADD i3,  i4,  0         || CMU.CPVI.x16 i10.l, v_amv.2     || VAU.ADIFF.u8 v24, v12, v8      || LSU0.LDI64.h v11, i2, i_desc
IAU.ADD i8,  i8,  i0        || SAU.SUM.u8 i0, v25, 0xF                                           || LSU0.LDI64.l v0,  i3, i_desc
                               CMU.CPVI.x16 i11.l, v_amv.3     || VAU.ADIFF.u8 v24, v13, v9      || LSU0.LDI64.h v0,  i3, i_desc
IAU.ADD i8,  i8,  i0        || SAU.SUM.u8 i0, v24, 0xF                                           || LSU0.LDI64.l v1,  i3, i_desc
                               CMU.CPVI.x16 i12.l, v_amv.4     || VAU.ADIFF.u8 v24, v14, v10     || LSU0.LDI64.h v1,  i3, i_desc
IAU.ADD i9,  i9,  i0        || SAU.SUM.u8   i0,  v24, 0xF                                        || LSU0.LDI64.l v2,  i3, i_desc
                               CMU.CPVI.x16 i13.l, v_amv.5     || VAU.ADIFF.u8 v24, v15, v11     || LSU0.LDI64.h v2,  i3, i_desc
IAU.ADD i9,  i9,  i0        || SAU.SUM.u8   i0,  v24, 0xF                                        || LSU0.LDI64.l v3,  i3, i_desc
IAU.SUB i2,  i4,  0x08      || CMU.CPVI.x16 i14.l, v_amv.6     || VAU.ADIFF.u8 v25, v12, v0      || LSU0.LDI64.h v3,  i3, i_desc
IAU.ADD i9,  i9,  i0        || SAU.SUM.u8   i0,  v24, 0xF                                        || LSU0.LDI64.l v4,  i2, i_desc
                               CMU.CPVI.x16 i15.l, v_amv.7     || VAU.ADIFF.u8 v25, v13, v1      || LSU0.LDI64.h v4,  i2, i_desc
IAU.ADD i9,  i9,  i0        || SAU.SUM.u8   i0,  v25, 0xF                                        || LSU0.LDI64.l v5,  i2, i_desc
                               CMU.VSZMWORD v16, v12, [1010]   || VAU.ADIFF.u8 v25, v14, v2      || LSU0.LDI64.h v5,  i2, i_desc
IAU.ADD i10, i10, i0        || SAU.SUM.u8   i0,  v25, 0xF                                        || LSU0.LDI64.l v6,  i2, i_desc
                               CMU.VSZMWORD v17, v12, [3232]   || VAU.ADIFF.u8 v25, v15, v3      || LSU0.LDI64.h v6,  i2, i_desc
IAU.ADD i10, i10, i0        || SAU.SUM.u8   i0,  v25, 0xF                                        || LSU0.LDI64.l v7,  i2, i_desc
IAU.ADD i3,  i4,  0x08      || CMU.VSZMWORD v18, v13, [1010]   || VAU.ADIFF.u8 v24, v12, v4      || LSU0.LDI64.h v7,  i2, i_desc
IAU.ADD i10, i10, i0        || SAU.SUM.u8   i0,  v25, 0xF                                        || LSU0.LDI64.l v8,  i3, i_desc
                               CMU.VSZMWORD v19, v13, [3232]   || VAU.ADIFF.u8 v24, v13, v5      || LSU0.LDI64.h v8,  i3, i_desc
IAU.ADD i10, i10, i0        || SAU.SUM.u8   i0,  v24, 0xF                                        || LSU0.LDI64.l v9,  i3, i_desc
                               CMU.VSZMWORD v20, v14, [1010]   || VAU.ADIFF.u8 v24, v14, v6      || LSU0.LDI64.h v9,  i3, i_desc
IAU.ADD i12, i12, i0        || SAU.SUM.u8   i0,  v24, 0xF                                        || LSU0.LDI64.l v10, i3, i_desc
                               CMU.VSZMWORD v21, v14, [3232]   || VAU.ADIFF.u8 v24, v15, v7      || LSU0.LDI64.h v10, i3, i_desc
IAU.ADD i12, i12, i0        || SAU.SUM.u8   i0,  v24, 0xF                                        || LSU0.LDI64.l v11, i3, i_desc
IAU.ADD i2,  i5,  0         || CMU.VSZMWORD v22, v15, [1010]   || VAU.ADIFF.u8 v25, v12, v8      || LSU0.LDI64.h v11, i3, i_desc
IAU.ADD i12, i12, i0        || SAU.SUM.u8   i0,  v24, 0xF                                        || LSU0.LDI64.l v0,  i2, i_desc
                               CMU.VSZMWORD v23, v15, [3232]   || VAU.ADIFF.u8 v25, v13, v9      || LSU0.LDI64.h v0,  i2, i_desc
IAU.ADD i12, i12, i0        || SAU.SUM.u8   i0,  v25, 0xF                                        || LSU0.LDI64.l v1,  i2, i_desc
                                                                  VAU.ADIFF.u8 v25, v14, v10     || LSU0.LDI64.h v1,  i2, i_desc
IAU.ADD i13, i13, i0        || SAU.SUM.u8   i0,  v25, 0xF                                        || LSU0.LDI64.l v2,  i2, i_desc
                                                                  VAU.ADIFF.u8 v25, v15, v11     || LSU0.LDI64.h v2,  i2, i_desc || LSU1.LDIL i18, 0
IAU.ADD i13, i13, i0        || SAU.SUM.u8   i0,  v25, 0xF                                        || LSU0.LDI64.l v3,  i2, i_desc
IAU.SUB i3,  i5,  0x08                                                                           || LSU0.LDI64.h v3,  i2, i_desc
IAU.ADD i13, i13, i0        || SAU.SUM.u8   i0,  v25, 0xF      || VAU.ADIFF.u8 v24, v12, v0      || LSU0.LDI64.l v4,  i3, i_desc
IAU.SUB.u32s i1, i_best, i8                                                                      || LSU0.LDI64.h v4,  i3, i_desc
IAU.ADD i13, i13, i0        || SAU.SUM.u8   i0,  v24, 0xF      || VAU.ADIFF.u8 v24, v13, v1      || LSU0.LDI64.l v5,  i3, i_desc
                               CMU.CMZ.i32 i1                                                    || LSU0.LDI64.h v5,  i3, i_desc
IAU.ADD i11, i11, i0        || SAU.SUM.u8   i0,  v24, 0xF      || VAU.ADIFF.u8 v24, v14, v2      || LSU0.LDI64.l v6,  i3, i_desc
PEU.PCCX.NEQ 0x25           || CMU.CPII i_best, i8             || IAU.SUB i_src, i6, 0x08        || LSU0.LDI64.h v6,  i3, i_desc || LSU1.LDIL i18, 0x00F8
IAU.ADD i11, i11, i0        || SAU.SUM.u8   i0,  v24, 0xF      || VAU.ADIFF.u8 v24, v15, v3      || LSU0.LDI64.l v7,  i3, i_desc
IAU.ADD i2,  i5,  0x08                                                                           || LSU0.LDI64.h v7,  i3, i_desc
IAU.ADD i11, i11, i0        || SAU.SUM.u8   i0,  v24, 0xF      || VAU.ADIFF.u8 v25, v12, v4      || LSU0.LDI64.l v8,  i2, i_desc
IAU.SUB.u32s i1, i_best, i9                                                                      || LSU0.LDI64.h v8,  i2, i_desc
IAU.ADD i11, i11, i0        || SAU.SUM.u8   i0,  v25, 0xF      || VAU.ADIFF.u8 v25, v13, v5      || LSU0.LDI64.l v9,  i2, i_desc
                               CMU.CMZ.i32 i1                                                    || LSU0.LDI64.h v9,  i2, i_desc
IAU.ADD i14, i14, i0        || SAU.SUM.u8   i0,  v25, 0xF      || VAU.ADIFF.u8 v25, v14, v6      || LSU0.LDI64.l v10, i2, i_desc
PEU.PCCX.NEQ 0x25           || CMU.CPII i_best, i9             || IAU.ADD i_src, i6, 0x08        || LSU0.LDI64.h v10, i2, i_desc || LSU1.LDIL i18, 0x0008
IAU.ADD i14, i14, i0        || SAU.SUM.u8   i0,  v25, 0xF      || VAU.ADIFF.u8 v25, v15, v7      || LSU0.LDI64.l v11, i2, i_desc
CMU.CMII.i32 i_best, i10                                                                         || LSU0.LDI64.h v11, i2, i_desc
IAU.ADD i14, i14, i0        || SAU.SUM.u8   i0,  v25, 0xF      || VAU.ADIFF.u8 v24, v12, v8      || CMU.CMII.i32 i_best, i10
PEU.PC1C 0x2                || CMU.CPII i_best, i10            || IAU.ADD i_src, i4, 0           || LSU1.LDIL i18, 0xF800
IAU.ADD i14, i14, i0        || SAU.SUM.u8   i0,  v24, 0xF      || VAU.ADIFF.u8 v24, v13, v9      || CMU.CMII.i32 i_best, i11
PEU.PC1C 0x2                || CMU.CPII i_best, i11            || IAU.ADD i_src, i5, 0           || LSU1.LDIL i18, 0x0800
IAU.ADD i15, i15, i0        || SAU.SUM.u8   i0,  v24, 0xF      || VAU.ADIFF.u8 v24, v14, v10     || LSU1.LDIL i8, refLuma
IAU.SHL i6,  i_desc, 0x02   || SAU.SHL.u32  i7,  i_desc, 0x06                                    || LSU1.LDIH i8, refLuma
IAU.ADD i15, i15, i0        || SAU.SUM.u8   i0,  v24, 0xF      || VAU.ADIFF.u8 v24, v15, v11     || CMU.CMII.i32 i_best, i12
PEU.PC1C 0x2                || CMU.CPII i_best, i12            || IAU.SUB i_src, i4, 0x08        || LSU1.LDIL i18, 0xF8F8
IAU.ADD i15, i15, i0        || SAU.SUM.u8   i0,  v24, 0xF      || CMU.CMII.i32 i_best, i13       || BRU.BRA ME_Load
PEU.PC1C 0x2                || CMU.CPII i_best, i13            || IAU.ADD i_src, i4, 0x08        || LSU1.LDIL i18, 0xF808
IAU.ADD i15, i15, i0        || CMU.CMII.i32 i_best, i14                                          || LSU1.LDIL i30, ME_4Pac
PEU.PC1C 0x2                || CMU.CPII i_best, i14            || IAU.SUB i_src, i5, 0x08        || LSU1.LDIL i18, 0x08F8
IAU.ADD i9,  i8,  i7        || CMU.CMII.i32 i_best, i15                                          || LSU1.LDIH i30, ME_4Pac
PEU.PC1C 0x2                || CMU.CPII i_best, i15            || IAU.ADD i_src, i5, 0x08        || LSU1.LDIL i18, 0x0808

.lalign 
ME_Load:
CMU.VSZMBYTE i10, i10, [ZZZZ]   || IAU.FEXT.i32 i0, i18, 0x00, 0x08    || SAU.SUB.u32s i12, i12, i12        || LSU1.LDIL i13, 0
CMU.VSZMBYTE i11, i11, [ZZZZ]   || IAU.FEXT.i32 i1, i18, 0x08, 0x08    || SAU.ADD.i32 i_posx, i_posx, i0    || LSU1.LDIL i14, 0
CMU.CPIVR.x16 v27, i18          || IAU.FEXT.u32 i0, i_src, 0x00, 0x03  || SAU.ADD.i32 i_posy, i_posy, i1    || LSU1.LDIL i15, 0
PEU.PCIX.NEQ 0x01               || IAU.SUB.u32s i_src, i_src, 0x04     || VAU.SHR.i8 v_mvc, v_mvc, 0x01     || LSU1.LDIL i1, 0x0000
CMU.CMZ.i32 i0                  || IAU.ADD.u32s i2, i_src, 0           || VAU.SUB.i8 v_mv,  v_mv,  v27      || LSU1.LDIH i1, 0x0001
                                   IAU.ADD.u32s i5, i_src, i6          || SAU.ADD.u32s i2, i2, i_desc       || LSU0.LDO64.h v4,  i2, 0x08
PEU.PCCX.EQ  0x20               || IAU.SUB.u32s i0, i9, i5             || LSU1.LDO32  i_adr, i2, -4         || LSU0.LD64.l  v4,  i2
PEU.PCIX.EQ  0x01               || IAU.SUB.u32s i5, i5, i7             || SAU.ADD.u32s i2, i2, i_desc       || LSU0.LDO64.h v5,  i2, 0x08
PEU.PCCX.EQ  0x20               || IAU.ADD.u32s i3, i5, 0              || LSU1.LDO32  i_adr, i2, -4         || LSU0.LD64.l  v5,  i2
                                   IAU.ADD.u32s i4, i5, i6             || SAU.ADD.u32s i2, i2, i_desc       || LSU0.LDO64.h v6,  i2, 0x08
PEU.PCCX.EQ  0x20               || IAU.SUB.u32s i0, i9, i4             || LSU1.LDO32  i_adr, i2, -4         || LSU0.LD64.l  v6,  i2
PEU.PCIX.EQ  0x01               || IAU.SUB.u32s i4, i4, i7             || SAU.SUB.u32s i9, i9, i9           || LSU0.LDO64.h v7,  i2, 0x08
PEU.PCCX.EQ  0x24               || CMU.SHLIV.x32 v4,  i_adr            || LSU1.LDO32  i_adr, i2, -4         || LSU0.LD64.l  v7,  i2
VAU.SUB.i8 v27, v_mv, v_mvc     || IAU.ADD.u32s i2, i4, 0              || SAU.ADD.u32s i3, i3, i_desc       || LSU0.LDO64.h v8,  i3, 0x08
PEU.PCCX.EQ  0x24               || CMU.SHLIV.x32 v5,  i_adr            || LSU1.LDO32  i_adr, i3, -4         || LSU0.LD64.l  v8,  i3
VAU.ABS.i8 v27, v27             || IAU.SUB.u32s i4, i_src, i6          || SAU.ADD.u32s i3, i3, i_desc       || LSU0.LDO64.h v9,  i3, 0x08
PEU.PCCX.EQ  0x24               || CMU.SHLIV.x32 v6,  i_adr            || LSU1.LDO32  i_cbp, i3, -4         || LSU0.LD64.l  v9,  i3
                                   IAU.SUB.u32s i0, i8, i4             || SAU.ADD.u32s i3, i3, i_desc       || LSU0.LDO64.h v10, i3, 0x08
PEU.PCCX.EQ  0x24               || CMU.SHLIV.x32 v7,  i_adr            || LSU1.LDO32  i_adr, i3, -4         || LSU0.LD64.l  v10, i3
PEU.PCIX.NEQ 0x01               || IAU.ADD.u32s i4, i4, i7             || SAU.SUB.u32s i8, i8, i8           || LSU0.LDO64.h v11, i3, 0x08
PEU.PCCX.EQ  0x24               || CMU.SHLIV.x32 v8,  i_adr            || LSU1.LDO32  i_adr, i3, -4         || LSU0.LD64.l  v11, i3
CMU.VSZMBYTE v_amv, v27, [Z3Z1] || IAU.ADD.u32s i3, i4, 0              || SAU.ADD.u32s i2, i2, i_desc       || LSU0.LDO64.h v12, i2, 0x08
PEU.PCCX.EQ  0x24               || CMU.SHLIV.x32 v9,  i_adr            || LSU1.LDO32  i_adr, i2, -4         || LSU0.LD64.l  v12, i2
CMU.VSZMBYTE v27, v27, [Z2Z0]   || IAU.SUB.u32s i0, i_posx, i_rowStart || SAU.ADD.u32s i2, i2, i_desc       || LSU0.LDO64.h v13, i2, 0x08
PEU.PCCX.EQ  0x24               || CMU.SHLIV.x32 v10, i_adr            || LSU1.LDO32  i_adr, i2, -4         || LSU0.LD64.l  v13, i2
PEU.PCIX.EQ  0x04               || CMU.CPII i8, i1                     || SAU.ADD.u32s i2, i2, i_desc       || LSU0.LDO64.h v14, i2, 0x08
PEU.PCCX.EQ  0x24               || CMU.SHLIV.x32 v11, i_adr            || LSU1.LDO32  i_adr, i2, -4         || LSU0.LD64.l  v14, i2
PEU.PCIX.EQ  0x06               || CMU.CPII i12, i1                    || SAU.ADD.u32s i14, i1, 0           || LSU0.LDO64.h v15, i2, 0x08
PEU.PCCX.EQ  0x24               || CMU.SHLIV.x32 v12, i_adr            || LSU1.LDO32  i_adr, i2, -4         || LSU0.LD64.l  v15, i2
VAU.ADD.i16  v_amv, v27, v_amv  || IAU.SUB.u32s i0, i_rowEnd, 0x08     || SAU.ADD.u32s i3, i3, i_desc       || LSU0.LDO64.h v0,  i3, 0x08
PEU.PCCX.EQ  0x24               || CMU.SHLIV.x32 v13, i_adr            || LSU1.LDO32  i_adr, i3, -4         || LSU0.LD64.l  v0,  i3
VAU.SHL.i16  v_amv, v_amv, 0x04 || IAU.SUB.u32s i0, i0, i_posx         || SAU.ADD.u32s i3, i3, i_desc       || LSU0.LDO64.h v1,  i3, 0x08
PEU.PCCX.EQ  0x24               || CMU.SHLIV.x32 v14, i_adr            || LSU1.LDO32  i_adr, i3, -4         || LSU0.LD64.l  v1,  i3
PEU.PCIX.EQ  0x04               || CMU.CPII i9, i1                     || SAU.ADD.u32s i3, i3, i_desc       || LSU0.LDO64.h v2,  i3, 0x08
PEU.PCCX.EQ  0x24               || CMU.SHLIV.x32 v15, i_adr            || LSU1.LDO32  i_adr, i3, -4         || LSU0.LD64.l  v2,  i3
PEU.PCIX.EQ  0x06               || CMU.CPII i13, i1                    || SAU.ADD.u32s i15, i1, 0           || LSU0.LDO64.h v3,  i3, 0x08
PEU.PCCX.EQ  0x24               || CMU.SHLIV.x32 v0,  i_adr            || LSU1.LDO32  i_adr, i3, -4         || LSU0.LD64.l  v3,  i3
BRU.JMP i30                     || CMU.CPSI i2, s_height               || SAU.ADD.u32s i3, i_posy, 0x08
PEU.PCCX.EQ  0x04               || CMU.SHLIV.x32 v1,  i_adr  || IAU.SUB.u32s i0, i_posy, 0
PEU.PC1I 0x1                    || CMU.CPII i10, i1          || IAU.ADD i12, i1, 0  || SAU.ADD.i32 i13, i1, 0
PEU.PCCX.EQ  0x04               || CMU.SHLIV.x32 v2,  i_adr  || IAU.SUB.u32s i0, i2, i3
PEU.PC1I 0x1                    || CMU.CPII i11, i1          || IAU.ADD i14, i1, 0  || SAU.ADD.i32 i15, i1, 0
PEU.PCCX.EQ  0x04               || CMU.SHLIV.x32 v3,  i_adr

; 2nd stage 4 pixel search range
.lalign
ME_4Pac:
CMU.CPVI.x16 i8.l,  v_amv.0   || IAU.ADD i6, i_src, 0                            || VAU.ADIFF.u8 v27, v16, v8  || LSU1.SWZV8 [54325432]
CMU.CPVI.x16 i9.l,  v_amv.1                                                      || VAU.ADIFF.u8 v27, v17, v9  || LSU1.SWZV8 [54325432]
CMU.CPVI.x16 i10.l, v_amv.2                           || SAU.SUM.u8 i2, v27, 0x3 || VAU.ADIFF.u8 v27, v18, v10 || LSU1.SWZV8 [54325432]
CMU.CPVI.x16 i11.l, v_amv.3                           || SAU.SUM.u8 i2, v27, 0x3 || VAU.ADIFF.u8 v27, v19, v11 || LSU1.SWZV8 [54325432]
CMU.CPVI.x16 i12.l, v_amv.4   || IAU.ADD i11, i11, i2 || SAU.SUM.u8 i2, v27, 0x3 || VAU.ADIFF.u8 v27, v20, v12 || LSU1.SWZV8 [54325432]
CMU.CPVI.x16 i13.l, v_amv.5   || IAU.ADD i11, i11, i2 || SAU.SUM.u8 i2, v27, 0x3 || VAU.ADIFF.u8 v27, v21, v13 || LSU1.SWZV8 [54325432]
CMU.CPVI.x16 i14.l, v_amv.6   || IAU.ADD i11, i11, i2 || SAU.SUM.u8 i2, v27, 0x3 || VAU.ADIFF.u8 v27, v22, v14 || LSU1.SWZV8 [54325432]
CMU.CPVI.x16 i15.l, v_amv.7   || IAU.ADD i11, i11, i2 || SAU.SUM.u8 i2, v27, 0x3 || VAU.ADIFF.u8 v27, v23, v15 || LSU1.SWZV8 [54325432]
                                 IAU.ADD i11, i11, i2 || SAU.SUM.u8 i2, v27, 0x3 || VAU.ADIFF.u8 v24, v16, v4
                                 IAU.ADD i11, i11, i2 || SAU.SUM.u8 i2, v27, 0x3 || VAU.ADIFF.u8 v25, v17, v5
CMU.CPVV v26, v24             || IAU.ADD i11, i11, i2                            || VAU.ADIFF.u8 v27, v16, v0  || LSU1.SWZV8 [54325432]
CMU.VSZMWORD v24, v25, [10DD] || IAU.ADD i11, i11, i2                            || VAU.ADIFF.u8 v24, v18, v6
CMU.VSZMWORD v25, v26, [DD32]                         || SAU.SUM.u8 i0, v24, 0xF || VAU.ADIFF.u8 v25, v19, v7
CMU.CPVV v26, v24                                     || SAU.SUM.u8 i1, v25, 0xF || VAU.ADIFF.u8 v27, v17, v1  || LSU1.SWZV8 [54325432]
CMU.VSZMWORD v24, v25, [10DD] || IAU.ADD i8,  i8,  i0 || SAU.SUM.u8 i2, v27, 0x3 || VAU.ADIFF.u8 v24, v20, v8
CMU.VSZMWORD v25, v26, [DD32] || IAU.ADD i9,  i9,  i1 || SAU.SUM.u8 i0, v24, 0xF || VAU.ADIFF.u8 v25, v21, v9
CMU.CPVV v26, v24             || IAU.ADD i10, i10, i2 || SAU.SUM.u8 i1, v25, 0xF || VAU.ADIFF.u8 v27, v18, v2  || LSU1.SWZV8 [54325432]
CMU.VSZMWORD v24, v25, [10DD] || IAU.ADD i8,  i8,  i0 || SAU.SUM.u8 i2, v27, 0x3 || VAU.ADIFF.u8 v24, v22, v10
CMU.VSZMWORD v25, v26, [DD32] || IAU.ADD i9,  i9,  i1 || SAU.SUM.u8 i0, v24, 0xF || VAU.ADIFF.u8 v25, v23, v11
CMU.CPVV v26, v24             || IAU.ADD i10, i10, i2 || SAU.SUM.u8 i1, v25, 0xF || VAU.ADIFF.u8 v27, v19, v3  || LSU1.SWZV8 [54325432]
CMU.VSZMWORD v24, v25, [10DD] || IAU.ADD i8,  i8,  i0 || SAU.SUM.u8 i2, v27, 0x3 || VAU.ADIFF.u8 v24, v16, v0
CMU.VSZMWORD v25, v26, [DD32] || IAU.ADD i9,  i9,  i1 || SAU.SUM.u8 i0, v24, 0xF || VAU.ADIFF.u8 v25, v17, v1
CMU.CPVV v26, v24             || IAU.ADD i10, i10, i2 || SAU.SUM.u8 i1, v25, 0xF || VAU.ADIFF.u8 v27, v20, v4  || LSU1.SWZV8 [54325432]
CMU.VSZMWORD v24, v25, [10DD] || IAU.ADD i8,  i8,  i0 || SAU.SUM.u8 i2, v27, 0x3 || VAU.ADIFF.u8 v24, v18, v2
CMU.VSZMWORD v25, v26, [DD32] || IAU.ADD i9,  i9,  i1 || SAU.SUM.u8 i0, v24, 0xF || VAU.ADIFF.u8 v25, v19, v3
CMU.CPVV v26, v24             || IAU.ADD i10, i10, i2 || SAU.SUM.u8 i1, v25, 0xF || VAU.ADIFF.u8 v27, v21, v5  || LSU1.SWZV8 [54325432]
CMU.VSZMWORD v24, v25, [10DD] || IAU.ADD i12, i12, i0 || SAU.SUM.u8 i2, v27, 0x3 || VAU.ADIFF.u8 v24, v20, v4
CMU.VSZMWORD v25, v26, [DD32] || IAU.ADD i13, i13, i1 || SAU.SUM.u8 i0, v24, 0xF || VAU.ADIFF.u8 v25, v21, v5
CMU.CPVV v26, v24             || IAU.ADD i10, i10, i2 || SAU.SUM.u8 i1, v25, 0xF || VAU.ADIFF.u8 v27, v22, v6  || LSU1.SWZV8 [54325432]
CMU.VSZMWORD v24, v25, [10DD] || IAU.ADD i12, i12, i0 || SAU.SUM.u8 i2, v27, 0x3 || VAU.ADIFF.u8 v24, v22, v6
CMU.VSZMWORD v25, v26, [DD32] || IAU.ADD i13, i13, i1 || SAU.SUM.u8 i0, v24, 0xF || VAU.ADIFF.u8 v25, v23, v7
CMU.CPVV v26, v24             || IAU.ADD i10, i10, i2 || SAU.SUM.u8 i1, v25, 0xF || VAU.ADIFF.u8 v27, v23, v7  || LSU1.SWZV8 [54325432]
CMU.VSZMWORD v24, v25, [10DD] || IAU.ADD i12, i12, i0 || SAU.SUM.u8 i2, v27, 0x3 || VAU.ADIFF.u8 v24, v16, v8
CMU.VSZMWORD v25, v26, [DD32] || IAU.ADD i13, i13, i1 || SAU.SUM.u8 i0, v24, 0xF || VAU.ADIFF.u8 v25, v17, v9
CMU.CPVV v26, v24             || IAU.ADD i10, i10, i2 || SAU.SUM.u8 i1, v25, 0xF
CMU.VSZMWORD v24, v25, [10DD] || IAU.ADD i12, i12, i0 || SAU.SUM.u8 i2, v27, 0x3 || VAU.ADIFF.u8 v24, v18, v10
CMU.VSZMWORD v25, v26, [DD32] || IAU.ADD i13, i13, i1 || SAU.SUM.u8 i0, v24, 0xF || VAU.ADIFF.u8 v25, v19, v11
CMU.CPVV v26, v24             || IAU.ADD i10, i10, i2 || SAU.SUM.u8 i1, v25, 0xF
CMU.VSZMWORD v24, v25, [10DD] || IAU.ADD i14, i14, i0                            || VAU.ADIFF.u8 v24, v20, v12
CMU.VSZMWORD v25, v26, [DD32] || IAU.ADD i15, i15, i1 || SAU.SUM.u8 i0, v24, 0xF || VAU.ADIFF.u8 v25, v21, v13
CMU.CPVV v26, v24                                     || SAU.SUM.u8 i1, v25, 0xF
CMU.VSZMWORD v24, v25, [10DD] || IAU.ADD i14, i14, i0                            || VAU.ADIFF.u8 v24, v22, v14
CMU.VSZMWORD v25, v26, [DD32] || IAU.ADD i15, i15, i1 || SAU.SUM.u8 i0, v24, 0xF || VAU.ADIFF.u8 v25, v23, v15
CMU.CPVV v26, v24                                     || SAU.SUM.u8 i1, v25, 0xF
CMU.VSZMWORD v24, v25, [10DD] || IAU.ADD i14, i14, i0                            || LSU1.LDIL i18, 0
CMU.VSZMWORD v25, v26, [DD32] || IAU.ADD i15, i15, i1 || SAU.SUM.u8 i0, v24, 0xF
CMU.CMII.i32 i_best, i8                               || SAU.SUM.u8 i1, v25, 0xF
PEU.PC1C 0x2 || CMU.CPII i_best, i8  || IAU.SUB i_src, i6, 0x04 || LSU1.LDIL i18, 0x00FC
CMU.CMII.i32 i_best, i9       || IAU.ADD i14, i14, i0  || LSU1.LDIL i8, refLuma
PEU.PC1C 0x2 || CMU.CPII i_best, i9  || IAU.ADD i_src, i6, 0x04 || LSU1.LDIL i18, 0x0004
CMU.CMII.i32 i_best, i10      || IAU.ADD i15, i15, i1  || LSU1.LDIH i8, refLuma
PEU.PC1C 0x2 || CMU.CPII i_best, i10 || IAU.ADD i_src, i4, 0    || LSU1.LDIL i18, 0xFC00
CMU.CMII.i32 i_best, i11      || SAU.SHL.i32 i7, i_desc, 0x06
PEU.PC1C 0x2 || CMU.CPII i_best, i11 || IAU.ADD i_src, i5, 0    || LSU1.LDIL i18, 0x0400
CMU.CMII.i32 i_best, i12      || SAU.SHL.i32 i6, i_desc, 0x02
PEU.PC1C 0x2 || CMU.CPII i_best, i12 || IAU.SUB i_src, i4, 0x04 || LSU1.LDIL i18, 0xFCFC
CMU.CMII.i32 i_best, i13      || SAU.ADD.u32s i9, i8, i7  || BRU.BRA ME_Load
PEU.PC1C 0x2 || CMU.CPII i_best, i13 || IAU.ADD i_src, i4, 0x04 || LSU1.LDIL i18, 0xFC04
CMU.CMII.i32 i_best, i14      || LSU1.LDIL i30, ME_2Pac
PEU.PC1C 0x2 || CMU.CPII i_best, i14 || IAU.SUB i_src, i5, 0x04 || LSU1.LDIL i18, 0x04FC
CMU.CMII.i32 i_best, i15      || LSU1.LDIH i30, ME_2Pac
PEU.PC1C 0x2 || CMU.CPII i_best, i15 || IAU.ADD i_src, i5, 0x04 || LSU1.LDIL i18, 0x0404


; 3rd stage 2 pixel search range
.lalign
ME_2Pac:
CMU.CPVI.x16 i8.l,  v_amv.0                                                      || VAU.ADIFF.u8 v27, v16, v6  || LSU1.SWZV8 [54325432]
CMU.CPVI.x16 i9.l,  v_amv.1                                                      || VAU.ADIFF.u8 v27, v17, v7  || LSU1.SWZV8 [54325432]
CMU.CPVI.x16 i10.l, v_amv.2                           || SAU.SUM.u8 i2, v27, 0x3 || VAU.ADIFF.u8 v27, v18, v8  || LSU1.SWZV8 [54325432]
CMU.CPVI.x16 i11.l, v_amv.3                           || SAU.SUM.u8 i2, v27, 0x3 || VAU.ADIFF.u8 v27, v19, v9  || LSU1.SWZV8 [54325432]
CMU.CPVI.x16 i12.l, v_amv.4   || IAU.ADD i11, i11, i2 || SAU.SUM.u8 i2, v27, 0x3 || VAU.ADIFF.u8 v27, v20, v10 || LSU1.SWZV8 [54325432]
CMU.CPVI.x16 i13.l, v_amv.5   || IAU.ADD i11, i11, i2 || SAU.SUM.u8 i2, v27, 0x3 || VAU.ADIFF.u8 v27, v21, v11 || LSU1.SWZV8 [54325432]
CMU.CPVI.x16 i14.l, v_amv.6   || IAU.ADD i11, i11, i2 || SAU.SUM.u8 i2, v27, 0x3 || VAU.ADIFF.u8 v27, v22, v12 || LSU1.SWZV8 [54325432]
CMU.CPVI.x16 i15.l, v_amv.7   || IAU.ADD i11, i11, i2 || SAU.SUM.u8 i2, v27, 0x3 || VAU.ADIFF.u8 v27, v23, v13 || LSU1.SWZV8 [54325432]
                                 IAU.ADD i11, i11, i2 || SAU.SUM.u8 i2, v27, 0x3 || VAU.ADIFF.u8 v24, v16, v4  || LSU1.SWZV8 [65434321]
                                 IAU.ADD i11, i11, i2 || SAU.SUM.u8 i2, v27, 0x3 || VAU.ADIFF.u8 v25, v17, v5  || LSU1.SWZV8 [65434321]
CMU.CPVV v26, v24             || IAU.ADD i11, i11, i2                            || VAU.ADIFF.u8 v27, v16, v2  || LSU1.SWZV8 [54325432]
CMU.VSZMWORD v24, v25, [10DD] || IAU.ADD i11, i11, i2                            || VAU.ADIFF.u8 v24, v18, v6  || LSU1.SWZV8 [65434321]
CMU.VSZMWORD v25, v26, [DD32]                         || SAU.SUM.u8 i0, v24, 0xF || VAU.ADIFF.u8 v25, v19, v7  || LSU1.SWZV8 [65434321]
CMU.CPVV v26, v24                                     || SAU.SUM.u8 i1, v25, 0xF || VAU.ADIFF.u8 v27, v17, v3  || LSU1.SWZV8 [54325432]
CMU.VSZMWORD v24, v25, [10DD] || IAU.ADD i8,  i8,  i0 || SAU.SUM.u8 i2, v27, 0x3 || VAU.ADIFF.u8 v24, v20, v8  || LSU1.SWZV8 [65434321]
CMU.VSZMWORD v25, v26, [DD32] || IAU.ADD i9,  i9,  i1 || SAU.SUM.u8 i0, v24, 0xF || VAU.ADIFF.u8 v25, v21, v9  || LSU1.SWZV8 [65434321]
CMU.CPVV v26, v24             || IAU.ADD i10, i10, i2 || SAU.SUM.u8 i1, v25, 0xF || VAU.ADIFF.u8 v27, v18, v4  || LSU1.SWZV8 [54325432]
CMU.VSZMWORD v24, v25, [10DD] || IAU.ADD i8,  i8,  i0 || SAU.SUM.u8 i2, v27, 0x3 || VAU.ADIFF.u8 v24, v22, v10 || LSU1.SWZV8 [65434321]
CMU.VSZMWORD v25, v26, [DD32] || IAU.ADD i9,  i9,  i1 || SAU.SUM.u8 i0, v24, 0xF || VAU.ADIFF.u8 v25, v23, v11 || LSU1.SWZV8 [65434321]
CMU.CPVV v26, v24             || IAU.ADD i10, i10, i2 || SAU.SUM.u8 i1, v25, 0xF || VAU.ADIFF.u8 v27, v19, v5  || LSU1.SWZV8 [54325432]
CMU.VSZMWORD v24, v25, [10DD] || IAU.ADD i8,  i8,  i0 || SAU.SUM.u8 i2, v27, 0x3 || VAU.ADIFF.u8 v24, v16, v2  || LSU1.SWZV8 [65434321]
CMU.VSZMWORD v25, v26, [DD32] || IAU.ADD i9,  i9,  i1 || SAU.SUM.u8 i0, v24, 0xF || VAU.ADIFF.u8 v25, v17, v3  || LSU1.SWZV8 [65434321]
CMU.CPVV v26, v24             || IAU.ADD i10, i10, i2 || SAU.SUM.u8 i1, v25, 0xF || VAU.ADIFF.u8 v27, v20, v6  || LSU1.SWZV8 [54325432]
CMU.VSZMWORD v24, v25, [10DD] || IAU.ADD i8,  i8,  i0 || SAU.SUM.u8 i2, v27, 0x3 || VAU.ADIFF.u8 v24, v18, v4  || LSU1.SWZV8 [65434321]
CMU.VSZMWORD v25, v26, [DD32] || IAU.ADD i9,  i9,  i1 || SAU.SUM.u8 i0, v24, 0xF || VAU.ADIFF.u8 v25, v19, v5  || LSU1.SWZV8 [65434321]
CMU.CPVV v26, v24             || IAU.ADD i10, i10, i2 || SAU.SUM.u8 i1, v25, 0xF || VAU.ADIFF.u8 v27, v21, v7  || LSU1.SWZV8 [54325432]
CMU.VSZMWORD v24, v25, [10DD] || IAU.ADD i12, i12, i0 || SAU.SUM.u8 i2, v27, 0x3 || VAU.ADIFF.u8 v24, v20, v6  || LSU1.SWZV8 [65434321]
CMU.VSZMWORD v25, v26, [DD32] || IAU.ADD i13, i13, i1 || SAU.SUM.u8 i0, v24, 0xF || VAU.ADIFF.u8 v25, v21, v7  || LSU1.SWZV8 [65434321]
CMU.CPVV v26, v24             || IAU.ADD i10, i10, i2 || SAU.SUM.u8 i1, v25, 0xF || VAU.ADIFF.u8 v27, v22, v8  || LSU1.SWZV8 [54325432]
CMU.VSZMWORD v24, v25, [10DD] || IAU.ADD i12, i12, i0 || SAU.SUM.u8 i2, v27, 0x3 || VAU.ADIFF.u8 v24, v22, v8  || LSU1.SWZV8 [65434321]
CMU.VSZMWORD v25, v26, [DD32] || IAU.ADD i13, i13, i1 || SAU.SUM.u8 i0, v24, 0xF || VAU.ADIFF.u8 v25, v23, v9  || LSU1.SWZV8 [65434321]
CMU.CPVV v26, v24             || IAU.ADD i10, i10, i2 || SAU.SUM.u8 i1, v25, 0xF || VAU.ADIFF.u8 v27, v23, v9  || LSU1.SWZV8 [54325432]
CMU.VSZMWORD v24, v25, [10DD] || IAU.ADD i12, i12, i0 || SAU.SUM.u8 i2, v27, 0x3 || VAU.ADIFF.u8 v24, v16, v6  || LSU1.SWZV8 [65434321]
CMU.VSZMWORD v25, v26, [DD32] || IAU.ADD i13, i13, i1 || SAU.SUM.u8 i0, v24, 0xF || VAU.ADIFF.u8 v25, v17, v7  || LSU1.SWZV8 [65434321]
CMU.CPVV v26, v24             || IAU.ADD i10, i10, i2 || SAU.SUM.u8 i1, v25, 0xF
CMU.VSZMWORD v24, v25, [10DD] || IAU.ADD i12, i12, i0 || SAU.SUM.u8 i2, v27, 0x3 || VAU.ADIFF.u8 v24, v18, v8  || LSU1.SWZV8 [65434321]
CMU.VSZMWORD v25, v26, [DD32] || IAU.ADD i13, i13, i1 || SAU.SUM.u8 i0, v24, 0xF || VAU.ADIFF.u8 v25, v19, v9  || LSU1.SWZV8 [65434321]
CMU.CPVV v26, v24             || IAU.ADD i10, i10, i2 || SAU.SUM.u8 i1, v25, 0xF
CMU.VSZMWORD v24, v25, [10DD] || IAU.ADD i14, i14, i0                            || VAU.ADIFF.u8 v24, v20, v10 || LSU1.SWZV8 [65434321]
CMU.VSZMWORD v25, v26, [DD32] || IAU.ADD i15, i15, i1 || SAU.SUM.u8 i0, v24, 0xF || VAU.ADIFF.u8 v25, v21, v11 || LSU1.SWZV8 [65434321]
CMU.CPVV v26, v24                                     || SAU.SUM.u8 i1, v25, 0xF
CMU.VSZMWORD v24, v25, [10DD] || IAU.ADD i14, i14, i0                            || VAU.ADIFF.u8 v24, v22, v12 || LSU1.SWZV8 [65434321]
CMU.VSZMWORD v25, v26, [DD32] || IAU.ADD i15, i15, i1 || SAU.SUM.u8 i0, v24, 0xF || VAU.ADIFF.u8 v25, v23, v13 || LSU1.SWZV8 [65434321]
CMU.CPVV v26, v24                                     || SAU.SUM.u8 i1, v25, 0xF
CMU.VSZMWORD v24, v25, [10DD] || IAU.ADD i14, i14, i0                            || LSU1.LDIL i18, 0
CMU.VSZMWORD v25, v26, [DD32] || IAU.ADD i15, i15, i1 || SAU.SUM.u8 i0, v24, 0xF
CMU.CMII.i32 i_best, i8                               || SAU.SUM.u8 i1, v25, 0xF
PEU.PC1C 0x2 || CMU.CPII i_best, i8  || LSU1.LDIL i18, 0x00FE
CMU.CMII.i32 i_best, i9       || IAU.ADD i14, i14, i0
PEU.PC1C 0x2 || CMU.CPII i_best, i9  || LSU1.LDIL i18, 0x0002
CMU.CMII.i32 i_best, i10      || IAU.ADD i15, i15, i1
PEU.PC1C 0x2 || CMU.CPII i_best, i10 || LSU1.LDIL i18, 0xFE00
CMU.CMII.i32 i_best, i11
PEU.PC1C 0x2 || CMU.CPII i_best, i11 || LSU1.LDIL i18, 0x0200
CMU.CMII.i32 i_best, i12
PEU.PC1C 0x2 || CMU.CPII i_best, i12 || LSU1.LDIL i18, 0xFEFE
CMU.CMII.i32 i_best, i13
PEU.PC1C 0x2 || CMU.CPII i_best, i13 || LSU1.LDIL i18, 0xFE02
CMU.CMII.i32 i_best, i14
PEU.PC1C 0x2 || CMU.CPII i_best, i14 || LSU1.LDIL i18, 0x02FE
CMU.CMII.i32 i_best, i15
PEU.PC1C 0x2 || CMU.CPII i_best, i15 || LSU1.LDIL i18, 0x0202
CMU.CMZ.i8 i18          || LSU0.LDIL i_adr, sampleStore || LSU1.LDIH i_adr, sampleStore
PEU.PC1C 0x4            || BRU.BRA ME_2LT  || IAU.SUB i_posx, i_posx, 0x02
PEU.PC1C 0x2            || BRU.BRA ME_2RT  || IAU.ADD i_posx, i_posx, 0x02
PEU.PCXC 0x4, 0x1       || BRU.RFB16I 0xE  || IAU.SUB i_posy, i_posy, 0x02
PEU.PCXC 0x2, 0x1       || BRU.RFB16I 0x2  || IAU.ADD i_posy, i_posy, 0x02
CMU.CPSI i4, s18        || IAU.SHL i0, i_smb, 0x06   || SAU.ADD.i32 i6, i_posx, 0
CMU.CPSI i5, s19        || IAU.ADD i_adr, i_adr, i0  || SAU.ADD.i32 i7, i_posy, 0
nop

.lalign
ME_Write:
CMU.CPSI i_posx, s20          || IAU.INCS i_adr, 0x100          || SAU.SUB.i32 i4, i6, i4         || LSU1.LDIL i2, predMVH
CMU.CPSI i_posy, s21          || IAU.SUB i0, i4, i_rowStart     || SAU.SUB.i32 i5, i7, i5         || LSU1.LDIH i2, predMVH
CMU.VSZMBYTE i18, i4,  [ZZZ0] || IAU.SHR.u32 i1, i5, 0x02       || SAU.SHL.i32 i4, i4, 0x02       || LSU1.LDIL i3, predMVV
CMU.VSZMBYTE i18, i5,  [ZZ0D] || IAU.SHR.u32 i0, i0, 0x02       || SAU.SHL.i32 i5, i5, 0x02       || LSU1.LDIH i3, predMVV
CMU.VSZMWORD v16, v4,  [ZZ21] || IAU.ADD i2, i2, i0             || SAU.SHL.i32 i1, i_desc, 0x03
CMU.VSZMWORD v17, v5,  [ZZ21] || IAU.ADD i3, i3, i1             || SAU.ADD.i32 i_smb, i_smb, 0x01 || LSU0.ST16 i18, i2
CMU.VSZMWORD v18, v6,  [ZZ21] || IAU.SHL i0, i_smb, 0x02                                          || LSU0.ST16 i18, i3
CMU.VSZMWORD v19, v7,  [ZZ21] || IAU.ADD  i_mode, i_MBInfo, i0  || SAU.DEALWORD i2, i_smb         || LSU0.STI64.l v16, i_adr
CMU.VSZMWORD v20, v8,  [ZZ21] || IAU.INCS i_mode, mb_mvdL0      || SAU.SWZBYTE  i5, i5, [1032]    || LSU0.STI64.l v17, i_adr
CMU.VSZMWORD v21, v9,  [ZZ21] || IAU.SHR.u32  i3, i2, 0x10      || SAU.CMBBYTE  i4, i5, [EEDD]    || LSU0.STI64.l v18, i_adr
CMU.VSZMWORD v22, v10, [ZZ21] || IAU.FEXT.u32 i2, i2, 0x0, 0x1                                    || LSU0.STI64.l v19, i_adr || LSU1.LDIL i30, ME_8Pac
CMU.VSZMWORD v23, v11, [ZZ21] || IAU.CMPI i_smb, 0x04                                             || LSU0.STI64.l v20, i_adr || LSU1.LDIH i30, ME_8Pac
PEU.PCIX.EQ  0x20             || CMU.CPSI i_dst, s22                                              || LSU0.STI64.l v21, i_adr || LSU1.LDIL i30, MD_Intra
PEU.PCIX.EQ  0x20             || CMU.CPSI i_src, s23                                              || LSU0.STI64.l v22, i_adr || LSU1.LDIH i30, MD_Intra
BRU.JMP i30                   || CMU.CMZ.i16 i3                                                   || LSU0.STI64.l v23, i_adr
PEU.PCCX.NEQ 0x03             || IAU.ADD i_dst, i_dst, i1       || SAU.ADD.i32 i_src, i_src, i1
PEU.PCCX.NEQ 0x01             || IAU.INCS i_posy, 0x08          || LSU0.ST32 i4, i_mode
BRU.RFB16Z 0                  || CMU.CMZ.i16 i2
PEU.PCCX.NEQ 0x03             || IAU.ADD i_dst, i_dst, 0x08     || SAU.ADD.i32 i_src, i_src, 0x08
PEU.PCCX.NEQ 0x01             || IAU.INCS i_posx, 0x08          || CMU.SHLIV.x32 v_SAD8x8, i_best

