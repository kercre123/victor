.version 0.50

.code .text.Convolution7x7Fp16ToFp16
; CoreConvolution7x7:
;void Convolution7x7Fp16ToFp16_asm(u8** in(i18), u8** out(i17), half conv[49](i16), u32 inWidth(i15))
;internal computation are made on fp16, output result is fp16
.lalign
Convolution7x7Fp16ToFp16_asm:
CMU.CPII     i0,  i16
LSU0.LD32    i8,  i17
LSU0.LD64    i1,  i18
LSU0.LDO64   i3,  i18, 0x08
LSU0.LDO64   i5,  i18, 0x10
LSU0.LDO32   i7,  i18, 0x18
CMU.CPII    i9, i15
nop
LSU0.LD64.l  v10, i0        || LSU1.LDO64.h v10, i0,  0x08
LSU0.LDO64.l v12, i0,  0x10 || LSU1.LDO64.h v12, i0,  0x18
LSU0.LDO64.l v14, i0,  0x20 || LSU1.LDO64.h v14, i0,  0x28
LSU0.LDO64.l v16, i0,  0x30 || LSU1.LDO64.h v16, i0,  0x38
LSU0.LDO64.l v18, i0,  0x40 || LSU1.LDO64.h v18, i0,  0x48
LSU0.LDO64.l v20, i0,  0x50 || LSU1.LDO64.h v20, i0,  0x58
LSU0.LDO64.l v22, i0,  0x60
; initial data set
CMU.VSZMBYTE v11, v10, [3232] || LSU0.LDI64.l v1,  i1
CMU.VSZMBYTE v10, v10, [1010] || LSU0.LDI64.h v1,  i1
CMU.VSZMBYTE v13, v12, [3232] || LSU0.LDI64.l v2,  i2
CMU.VSZMBYTE v12, v12, [1010] || LSU0.LDI64.h v2,  i2
CMU.VSZMBYTE v15, v14, [3232] || LSU0.LDI64.l v3,  i3
CMU.VSZMBYTE v14, v14, [1010] || LSU0.LDI64.h v3,  i3
CMU.VSZMBYTE v17, v16, [3232] || LSU0.LDI64.l v1,  i1
; take off
CMU.VSZMBYTE v16, v16, [1010] || LSU0.LDI64.h v1,  i1   || SAU.SUM.u32 i10, v1,  0x8
CMU.VSZMBYTE v19, v18, [3232] || LSU0.LDI64.l v4,  i4   || SAU.SUM.u32 i11, v1,  0x4
CMU.VSZMBYTE v18, v18, [1010] || LSU0.LDI64.h v4,  i4   || SAU.SUM.u32 i12, v1,  0x2
CMU.VSZMBYTE v21, v20, [3232] || LSU0.LDI64.l v5,  i5
CMU.VSZMBYTE v20, v20, [1010] || LSU0.LDI64.h v5,  i5   || BRU.BRA ___CoreConvF16_Loop
CMU.VSZMBYTE v22, v22, [1010] || LSU0.LDI64.l v6,  i6
IAU.SHR.u32 i0,  i10, 0x10    || LSU0.LDI64.h v6,  i6
                                 LSU0.LDI64.l v7,  i7
                                 LSU0.LDI64.h v7,  i7
CMU.CPVV      v0,  v1         || LSU0.LDI64.l v2,  i2

.lalign
___CoreConvF16_Loop:
; 1. row
CMU.SHLIV.x16 v0,  i0  || SAU.SUM.u32 i10, v2,  0x08 || VAU.MACPZ.f16 v0,  v10      || LSU1.SWZ4V [3210] [3333] || LSU0.LDI64.h v2,  i2
CMU.SHLIV.x16 v0,  i10 || IAU.SHR.u32 i0,  i11, 0x10 || VAU.MACP.f16  v0,  v11      || LSU1.SWZ4V [3210] [2222]
CMU.SHLIV.x16 v0,  i0  || SAU.SUM.u32 i11, v2,  0x04 || VAU.MACP.f16  v0,  v10      || LSU1.SWZ4V [3210] [2222]
CMU.SHLIV.x16 v0,  i11 || IAU.SHR.u32 i0,  i12, 0x10 || VAU.MACP.f16  v0,  v11      || LSU1.SWZ4V [3210] [1111]
CMU.SHLIV.x16 v0,  i0  || SAU.SUM.u32 i12, v2,  0x02 || VAU.MACP.f16  v0,  v10      || LSU1.SWZ4V [3210] [1111]
CMU.SHLIV.x16 v0,  i12 || IAU.SHR.u32 i0,  i10, 0x10 || VAU.MACP.f16  v0,  v11      || LSU1.SWZ4V [3210] [0000]
CMU.CPVV      v0,  v2                                || VAU.MACP.f16  v0,  v10      || LSU1.SWZ4V [3210] [0000] || LSU0.LDI64.l v3,  i3
; 2. row
CMU.SHLIV.x16 v0,  i0  || SAU.SUM.u32 i10, v3,  0x08 || VAU.MACP.f16  v0,  v13      || LSU1.SWZ4V [3210] [2222] || LSU0.LDI64.h v3,  i3
CMU.SHLIV.x16 v0,  i10 || IAU.SHR.u32 i0,  i11, 0x10 || VAU.MACP.f16  v0,  v12      || LSU1.SWZ4V [3210] [2222]
CMU.SHLIV.x16 v0,  i0  || SAU.SUM.u32 i11, v3,  0x04 || VAU.MACP.f16  v0,  v13      || LSU1.SWZ4V [3210] [1111]
CMU.SHLIV.x16 v0,  i11 || IAU.SHR.u32 i0,  i12, 0x10 || VAU.MACP.f16  v0,  v12      || LSU1.SWZ4V [3210] [1111]
CMU.SHLIV.x16 v0,  i0  || SAU.SUM.u32 i12, v3,  0x02 || VAU.MACP.f16  v0,  v13      || LSU1.SWZ4V [3210] [0000]
CMU.SHLIV.x16 v0,  i12 || IAU.SHR.u32 i0,  i10, 0x10 || VAU.MACP.f16  v0,  v12      || LSU1.SWZ4V [3210] [0000]
CMU.CPVV      v0,  v3                                || VAU.MACP.f16  v0,  v11      || LSU1.SWZ4V [3210] [3333] || LSU0.LDI64.l v4,  i4
; 3. row
CMU.SHLIV.x16 v0,  i0  || SAU.SUM.u32 i10, v4,  0x08 || VAU.MACP.f16  v0,  v14      || LSU1.SWZ4V [3210] [2222] || LSU0.LDI64.h v4,  i4
CMU.SHLIV.x16 v0,  i10 || IAU.SHR.u32 i0,  i11, 0x10 || VAU.MACP.f16  v0,  v15      || LSU1.SWZ4V [3210] [1111]
CMU.SHLIV.x16 v0,  i0  || SAU.SUM.u32 i11, v4,  0x04 || VAU.MACP.f16  v0,  v14      || LSU1.SWZ4V [3210] [1111]
CMU.SHLIV.x16 v0,  i11 || IAU.SHR.u32 i0,  i12, 0x10 || VAU.MACP.f16  v0,  v15      || LSU1.SWZ4V [3210] [0000]
CMU.SHLIV.x16 v0,  i0  || SAU.SUM.u32 i12, v4,  0x02 || VAU.MACP.f16  v0,  v14      || LSU1.SWZ4V [3210] [0000]
CMU.SHLIV.x16 v0,  i12 || IAU.SHR.u32 i0,  i10, 0x10 || VAU.MACP.f16  v0,  v13      || LSU1.SWZ4V [3210] [3333]
CMU.CPVV      v0,  v4                                || VAU.MACP.f16  v0,  v12      || LSU1.SWZ4V [3210] [3333] || LSU0.LDI64.l v5,  i5
; 4. row
CMU.SHLIV.x16 v0,  i0  || SAU.SUM.u32 i10, v5,  0x08 || VAU.MACP.f16  v0,  v17      || LSU1.SWZ4V [3210] [1111] || LSU0.LDI64.h v5,  i5
CMU.SHLIV.x16 v0,  i10 || IAU.SHR.u32 i0,  i11, 0x10 || VAU.MACP.f16  v0,  v16      || LSU1.SWZ4V [3210] [1111]
CMU.SHLIV.x16 v0,  i0  || SAU.SUM.u32 i11, v5,  0x04 || VAU.MACP.f16  v0,  v17      || LSU1.SWZ4V [3210] [0000]
CMU.SHLIV.x16 v0,  i11 || IAU.SHR.u32 i0,  i12, 0x10 || VAU.MACP.f16  v0,  v16      || LSU1.SWZ4V [3210] [0000]
CMU.SHLIV.x16 v0,  i0  || SAU.SUM.u32 i12, v5,  0x02 || VAU.MACP.f16  v0,  v15      || LSU1.SWZ4V [3210] [3333]
CMU.SHLIV.x16 v0,  i12 || IAU.SHR.u32 i0,  i10, 0x10 || VAU.MACP.f16  v0,  v14      || LSU1.SWZ4V [3210] [3333]
CMU.CPVV      v0,  v5                                || VAU.MACP.f16  v0,  v15      || LSU1.SWZ4V [3210] [2222] || LSU0.LDI64.l v6,  i6
; 5. row
CMU.SHLIV.x16 v0,  i0  || SAU.SUM.u32 i10, v6,  0x08 || VAU.MACP.f16  v0,  v18      || LSU1.SWZ4V [3210] [1111] || LSU0.LDI64.h v6,  i6
CMU.SHLIV.x16 v0,  i10 || IAU.SHR.u32 i0,  i11, 0x10 || VAU.MACP.f16  v0,  v19      || LSU1.SWZ4V [3210] [0000]
CMU.SHLIV.x16 v0,  i0  || SAU.SUM.u32 i11, v6,  0x04 || VAU.MACP.f16  v0,  v18      || LSU1.SWZ4V [3210] [0000]
CMU.SHLIV.x16 v0,  i11 || IAU.SHR.u32 i0,  i12, 0x10 || VAU.MACP.f16  v0,  v17      || LSU1.SWZ4V [3210] [3333]
CMU.SHLIV.x16 v0,  i0  || SAU.SUM.u32 i12, v6,  0x02 || VAU.MACP.f16  v0,  v16      || LSU1.SWZ4V [3210] [3333]
CMU.SHLIV.x16 v0,  i12 || IAU.SHR.u32 i0,  i10, 0x10 || VAU.MACP.f16  v0,  v17      || LSU1.SWZ4V [3210] [2222]
CMU.CPVV      v0,  v6                                || VAU.MACP.f16  v0,  v16      || LSU1.SWZ4V [3210] [2222] || LSU0.LDI64.l v7,  i7
; 6. row
CMU.SHLIV.x16 v0,  i0  || SAU.SUM.u32 i10, v7,  0x08 || VAU.MACP.f16  v0,  v21      || LSU1.SWZ4V [3210] [0000] || LSU0.LDI64.h v7,  i7
CMU.SHLIV.x16 v0,  i10 || IAU.SHR.u32 i0,  i11, 0x10 || VAU.MACP.f16  v0,  v20      || LSU1.SWZ4V [3210] [0000]
CMU.SHLIV.x16 v0,  i0  || SAU.SUM.u32 i11, v7,  0x04 || VAU.MACP.f16  v0,  v19      || LSU1.SWZ4V [3210] [3333]
CMU.SHLIV.x16 v0,  i11 || IAU.SHR.u32 i0,  i12, 0x10 || VAU.MACP.f16  v0,  v18      || LSU1.SWZ4V [3210] [3333]
CMU.SHLIV.x16 v0,  i0  || SAU.SUM.u32 i12, v7,  0x02 || VAU.MACP.f16  v0,  v19      || LSU1.SWZ4V [3210] [2222]
CMU.SHLIV.x16 v0,  i12 || IAU.SHR.u32 i0,  i10, 0x10 || VAU.MACP.f16  v0,  v18      || LSU1.SWZ4V [3210] [2222]
CMU.CPVV      v0,  v7                                || VAU.MACP.f16  v0,  v19      || LSU1.SWZ4V [3210] [1111] || LSU0.LDI64.l v1,  i1
; 7. row
CMU.SHLIV.x16 v0,  i0  || SAU.SUM.u32 i10, v1,  0x08 || VAU.MACP.f16  v0,  v22      || LSU1.SWZ4V [3210] [0000] || LSU0.LDI64.h v1,  i1
CMU.SHLIV.x16 v0,  i10 || IAU.SHR.u32 i0,  i11, 0x10 || VAU.MACP.f16  v0,  v21      || LSU1.SWZ4V [3210] [3333]
CMU.SHLIV.x16 v0,  i0  || SAU.SUM.u32 i11, v1,  0x04 || VAU.MACP.f16  v0,  v20      || LSU1.SWZ4V [3210] [3333]
CMU.SHLIV.x16 v0,  i11 || IAU.SHR.u32 i0,  i12, 0x10 || VAU.MACP.f16  v0,  v21      || LSU1.SWZ4V [3210] [2222]
CMU.SHLIV.x16 v0,  i0  || SAU.SUM.u32 i12, v1,  0x02 || VAU.MACP.f16  v0,  v20      || LSU1.SWZ4V [3210] [2222]
CMU.SHLIV.x16 v0,  i12 || IAU.SHR.u32 i0,  i10, 0x10 || VAU.MACP.f16  v0,  v21      || LSU1.SWZ4V [3210] [1111]
CMU.CPVV      v0,  v1                                || VAU.MACPW.f16 v8,  v0, v20  || LSU1.SWZ4V [3210] [1111]
; death row, the long wait for MAC writeback
BRU.RPIM 0x05
IAU.SUB.u32s i9,  i9,  0x10
PEU.PC1I 0x1 || BRU.JMP i30
PEU.PC1I 0x6 || BRU.BRA ___CoreConvF16_Loop
nop
nop
IAU.ADD i8, i8, 0x08 || LSU0.ST64.l v8, i8
IAU.ADD i8, i8, 0x08 || LSU0.ST64.h v8, i8
LSU0.LDI64.l v2,  i2

