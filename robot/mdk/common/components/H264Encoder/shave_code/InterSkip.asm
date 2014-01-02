; bulky memcopy basically transfer from reference buffers to reconstructed buffer

.lalign
InterSkip:
CMU.CPVI.x32 i0, v_cmd.2 || IAU.SHL i1, i_desc, 0x04                                       || LSU1.LDIL i_dst, origLuma
                            IAU.CMPI i0, 0           || SAU.SUB.i32 i0, i_posx, i_rowStart || LSU1.LDIH i_dst, origLuma
PEU.PCIX.NEQ 0x01        || IAU.ADD i_dst, i_dst, i1                                       || LSU1.LDIL i_src, sampleStore
                            IAU.ADD i_dst, i_dst, i0                                       || LSU1.LDIH i_src, sampleStore
                            IAU.INCS i_src, 0x100                                          || LSU1.LDIL i_smb, 0

; copy luma to output buffers
.lalign
InterLumaSkip:
                     IAU.SUB i0, i_posx, i_rowStart     || LSU0.LDI128.u8.u16 v0, i_src      || LSU1.LDIL i4, predLumaH
                     IAU.SHL i1, i_desc, 0x03           || LSU0.LDI128.u8.u16 v1, i_src      || LSU1.LDIH i4, predLumaH
                     IAU.ADD i4, i4, i0                 || LSU0.LDI128.u8.u16 v2, i_src      || LSU1.LDIL i5, predLumaV_lo
                     IAU.ADD i2, i_dst, 0               || LSU0.LDI128.u8.u16 v3, i_src      || LSU1.LDIH i5, predLumaV_lo
                     IAU.FEXT.u32 i3, i_smb, 0x00, 0x01 || LSU0.LDI128.u8.u16 v4, i_src
PEU.PCIX.NEQ 0x03 || IAU.ADD.u32s i2, i2, 0x08          || LSU0.LDI128.u8.u16 v5, i_src      || SAU.ADD.u32s i4, i4, 0x08
                     IAU.FEXT.u32 i3, i_smb, 0x01, 0x01 || LSU0.LDI128.u8.u16 v6, i_src
PEU.PCIX.NEQ 0x03 || IAU.ADD.u32s i2, i2, i1            || LSU0.LDI128.u8.u16 v7, i_src      || SAU.ADD.u32s i5, i5, 0x08
                                                           LSU0.STI128.u16.u8 v0, i2, i_desc || VAU.ALIGNVEC v9, v0, v9, 0xE
                                                           LSU0.STI128.u16.u8 v1, i2, i_desc || VAU.ALIGNVEC v9, v1, v9, 0xE
                     IAU.INCS i_smb, 0x01               || LSU0.STI128.u16.u8 v2, i2, i_desc || VAU.ALIGNVEC v9, v2, v9, 0xE
                     IAU.SUB.u32s i3, i_smb, 0x03       || LSU0.STI128.u16.u8 v3, i2, i_desc || VAU.ALIGNVEC v9, v3, v9, 0xE
PEU.PCIX.EQ 0     || BRU.BRA InterLumaSkip              || LSU0.STI128.u16.u8 v4, i2, i_desc || VAU.ALIGNVEC v9, v4, v9, 0xE
                                                           LSU0.STI128.u16.u8 v5, i2, i_desc || VAU.ALIGNVEC v9, v5, v9, 0xE
                                                           LSU0.STI128.u16.u8 v6, i2, i_desc || VAU.ALIGNVEC v9, v6, v9, 0xE
CMU.VSZMWORD v8, v8, [ZZZZ]                             || LSU0.STI128.u16.u8 v7, i2, i_desc || VAU.ALIGNVEC v9, v7, v9, 0xE
VAU.OR  v8, v8, v9                                      || LSU0.ST128.u16.u8  v7, i4         || LSU1.SWZV8 [01234567]
                                                           LSU0.ST128.u16.u8  v8, i5

InterChromaSkip:
                            IAU.SHR.u32 i_desc, i_desc, 0x01  || SAU.SUB.i32 i3, i_posx, i_rowStart || LSU0.LDO32 i8,  i_MBInfo,  mb_mvdL0
CMU.CPVI.x32 i4, v_cmd.3 || IAU.SHL i6, i_desc, 0x03          || SAU.SHL.i32 i1, i_desc, 0x05       || LSU1.LDIL i_dst, origChromaU
CMU.CPVI.x32 i5, v_cmd.2 || IAU.MUL i4, i4, i6                || SAU.SHR.u32 i3, i3, 0x01           || LSU1.LDIH i_dst, origChromaU
                            IAU.MUL i5, i5, i6                || SAU.SHL.i32 i6, i_desc, 0x02       || LSU1.LDIL i2, refChromaU
                            VAU.XOR v8, v8, v8                || SAU.ADD.i32 i_dst, i_dst, i3       || LSU1.LDIH i2, refChromaU
                            IAU.ADD i4, i4, i3                || SAU.ADD.i32 i3, i2, i1             || LSU0.STO64.l v8, i_MBInfo,  mb_coeffCount
                            IAU.ADD i4, i4, i2                || SAU.ADD.i32 i_dst, i_dst, i5       || LSU0.STO64.l v8, i_MBInfo, (mb_coeffCount+0x08)
                            IAU.FEXT.i32 i9,  i8, 0x03, 0xD                                         || LSU0.STO64.l v8, i_MBInfo, (mb_coeffCount+0x10)
                            IAU.FEXT.i32 i10, i8, 0x13, 0xD                                         || LSU0.STO64.l v8, i_MBInfo, (mb_coeffCount+0x18)
                            IAU.MUL i10, i10, i_desc          || SAU.ADD.i32 i8, i4, i9             || LSU0.STO64.l v8, i_MBInfo, (mb_coeffCount+0x20)
                                                                 SAU.SUB.i32 i_cbp, i_cbp, i_cbp    || LSU0.STO64.l v8, i_MBInfo, (mb_coeffCount+0x28)
                            IAU.ADD i8, i8, i10               || SAU.SUB.i32 i6, i_posx, i_rowStart || LSU0.STO64.l v8, i_MBInfo, (mb_coeffCount+0x30)
CMU.CMII.i32 i3, i8      || IAU.SUB.u32s i0,  i2,  i8         || SAU.SHL.i32 i0, i_desc, 0x02
PEU.PC1I 0x2             || IAU.ADD.u32s i8,  i8,  i1
PEU.PC1C 0x5             || IAU.SUB.u32s i8,  i8,  i1
                            IAU.ADD.u32s i9,  i8,  i_desc     || SAU.ADD.u32s i12, i8,  i0          || LSU1.LDIL i4, predChromaH
                            IAU.ADD.u32s i10, i9,  i_desc     || SAU.ADD.u32s i13, i9,  i0          || LSU1.LDIH i4, predChromaH
                            IAU.ADD.u32s i11, i10, i_desc     || SAU.ADD.u32s i14, i10, i0          || LSU1.LDIL i5, predChromaV_lo
                            IAU.SUB.u32s i0, i3, i9           || SAU.ADD.u32s i15, i11, i0          || LSU1.LDIH i5, predChromaV_lo
PEU.PCIX.EQ  0x02        || IAU.SUB.u32s i0, i3, i10          || SAU.SUB.u32s i9,  i9,  i1
PEU.PCIX.EQ  0x02        || IAU.SUB.u32s i0, i3, i11          || SAU.SUB.u32s i10, i10, i1          || LSU0.LD128.u8.u16  v0, i8
PEU.PCIX.EQ  0x02        || IAU.SUB.u32s i0, i3, i12          || SAU.SUB.u32s i11, i11, i1          || LSU0.LD128.u8.u16  v1, i9
PEU.PCIX.EQ  0x02        || IAU.SUB.u32s i0, i3, i13          || SAU.SUB.u32s i12, i12, i1          || LSU0.LD128.u8.u16  v2, i10
PEU.PCIX.EQ  0x02        || IAU.SUB.u32s i0, i3, i14          || SAU.SUB.u32s i13, i13, i1          || LSU0.LD128.u8.u16  v3, i11
PEU.PCIX.EQ  0x02        || IAU.SUB.u32s i0, i3, i15          || SAU.SUB.u32s i14, i14, i1          || LSU0.LD128.u8.u16  v4, i12
PEU.PCIX.EQ  0x02        || IAU.ADD i4, i4, i6                || SAU.SUB.u32s i15, i15, i1          || LSU0.LD128.u8.u16  v5, i13
                            CMU.VSZMWORD v8, v8, [ZZZZ]                                             || LSU0.LD128.u8.u16  v6, i14 || LSU1.LDIL i1, (refChromaV-refChromaU)
                            IAU.ADD i3, i_dst, 0                                                    || LSU0.LD128.u8.u16  v7, i15 || LSU1.LDIL i2, (origChromaV-origChromaU)
                            VAU.ALIGNVEC v9, v0, v9, 0XE                                            || LSU0.STI128.u16.u8 v0, i3, i_desc
                            VAU.ALIGNVEC v9, v1, v9, 0XE                                            || LSU0.STI128.u16.u8 v1, i3, i_desc
                            VAU.ALIGNVEC v9, v2, v9, 0XE                                            || LSU0.STI128.u16.u8 v2, i3, i_desc
                            VAU.ALIGNVEC v9, v3, v9, 0XE                                            || LSU0.STI128.u16.u8 v3, i3, i_desc
                            VAU.ALIGNVEC v9, v4, v9, 0XE                                            || LSU0.STI128.u16.u8 v4, i3, i_desc
                            VAU.ALIGNVEC v9, v5, v9, 0XE                                            || LSU0.STI128.u16.u8 v5, i3, i_desc
                            VAU.ALIGNVEC v9, v6, v9, 0XE                                            || LSU0.STI128.u16.u8 v6, i3, i_desc
                            VAU.ALIGNVEC v9, v7, v9, 0XE                                            || LSU0.STI128.u16.u8 v7, i3, i_desc
                            VAU.OR  v8, v8, v9                || SAU.ADD.u32s i8,  i8,  i1          || LSU0.STI128.u16.u8 v7, i4  || LSU1.SWZV8 [01234567]
                                                                 SAU.ADD.u32s i9,  i9,  i1          || LSU0.STI128.u16.u8 v8, i5
                                                                 SAU.ADD.u32s i10, i10, i1          || LSU0.LD128.u8.u16  v0, i8
                                                                 SAU.ADD.u32s i11, i11, i1          || LSU0.LD128.u8.u16  v1, i9
                                                                 SAU.ADD.u32s i12, i12, i1          || LSU0.LD128.u8.u16  v2, i10
                                                                 SAU.ADD.u32s i13, i13, i1          || LSU0.LD128.u8.u16  v3, i11
                                                                 SAU.ADD.u32s i14, i14, i1          || LSU0.LD128.u8.u16  v4, i12
                                                                 SAU.ADD.u32s i15, i15, i1          || LSU0.LD128.u8.u16  v5, i13
                            CMU.VSZMWORD v8, v8, [ZZZZ]                                             || LSU0.LD128.u8.u16  v6, i14 || LSU1.LDIL i_src, codec_offset
                            IAU.ADD i3, i_dst, i2                                                   || LSU0.LD128.u8.u16  v7, i15 || LSU1.LDIH i_src, codec_offset
                            VAU.ALIGNVEC v9, v0, v9, 0XE                                            || LSU0.STI128.u16.u8 v0, i3, i_desc
                            VAU.ALIGNVEC v9, v1, v9, 0XE                                            || LSU0.STI128.u16.u8 v1, i3, i_desc
                            VAU.ALIGNVEC v9, v2, v9, 0XE                                            || LSU0.STI128.u16.u8 v2, i3, i_desc
                            VAU.ALIGNVEC v9, v3, v9, 0XE                                            || LSU0.STI128.u16.u8 v3, i3, i_desc
                            VAU.ALIGNVEC v9, v4, v9, 0XE                                            || LSU0.STI128.u16.u8 v4, i3, i_desc
                            VAU.ALIGNVEC v9, v5, v9, 0XE                                            || LSU0.STI128.u16.u8 v5, i3, i_desc
                            VAU.ALIGNVEC v9, v6, v9, 0XE                                            || LSU0.STI128.u16.u8 v6, i3, i_desc
                            VAU.ALIGNVEC v9, v7, v9, 0XE                                            || LSU0.STI128.u16.u8 v7, i3, i_desc
                            VAU.OR  v8, v8, v9                                                      || LSU0.STI128.u16.u8 v7, i4  || LSU1.SWZV8 [01234567]
                                                                                                       LSU0.STI128.u16.u8 v8, i5
; update codec offset
LSU0.LDI64.l v0, i_src     || LSU1.LDIL i30, ENC_ColCycle
LSU0.LDI64.h v0, i_src     || LSU1.LDIH i30, ENC_ColCycle
LSU0.LDI64.l v1, i_src
LSU0.LDI64.h v1, i_src
LSU0.LDI64.l v2, i_src
LSU0.LDI64.h v2, i_src
nop
IAU.SUB i_src, i_src, 0x30 || VAU.ADD.u32s v0, v0, 0x10
nop
LSU0.STI64.l v0, i_src     || VAU.ADD.u32s v1, v1, 0x10  || BRU.JMP i30
LSU0.STI64.h v0, i_src
LSU0.STI64.l v1, i_src     || VAU.ADD.u32s v2, v2, 0x08
LSU0.STI64.h v1, i_src
LSU0.STI64.l v2, i_src
LSU0.STI64.h v2, i_src

