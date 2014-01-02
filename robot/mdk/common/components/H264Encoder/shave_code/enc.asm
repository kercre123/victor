.version 0.50
.include config.h
.include enc.h

.data ENCODE_DATA enc_data
; do not move struct label
    enc_ctrl:
    .fill 0x80
    dma_ctrl:
    .fill 0x40
    initLumaV_lo:
    .fill 0x08, 0x01, 0x80
    initLumaV_hi:
    .fill 0x08, 0x01, 0x80
    initChromaV_lo:
    .fill 0x08, 0x01, 0x80
    initChromaV_hi:
    .fill 0x08, 0x01, 0x80
    qpc_table:
    .byte  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15
    .byte 16, 17, 17, 18, 19, 20, 20, 21, 22, 22, 23, 23, 24, 24, 25, 25
    quant_table:
	; intra qp [12:43]
	.short 0x3333, 0x1F82, 0x147B, 0x0011, 0x00AA,   40,   52,   64
	.short 0x2E8C, 0x1D42, 0x1234, 0x0011, 0x00AA,   44,   56,   72
	.short 0x2762, 0x199A, 0x1062, 0x0011, 0x00AA,   52,   64,   80
	.short 0x2492, 0x16C1, 0x0E3F, 0x0011, 0x00AA,   56,   72,   92
	.short 0x2000, 0x147B, 0x0D1B, 0x0011, 0x00AA,   64,   80,  100
	.short 0x1C72, 0x11CF, 0x0B4D, 0x0011, 0x00AA,   72,   92,  116
	.short 0x3333, 0x1F82, 0x147B, 0x0012, 0x0155,   80,  104,  128
	.short 0x2E8C, 0x1D42, 0x1234, 0x0012, 0x0155,   88,  112,  144
	.short 0x2762, 0x199A, 0x1062, 0x0012, 0x0155,  104,  128,  160
	.short 0x2492, 0x16C1, 0x0E3F, 0x0012, 0x0155,  112,  144,  184
	.short 0x2000, 0x147B, 0x0D1B, 0x0012, 0x0155,  128,  160,  200
	.short 0x1C72, 0x11CF, 0x0B4D, 0x0012, 0x0155,  144,  184,  232
	.short 0x3333, 0x1F82, 0x147B, 0x0013, 0x02AA,  160,  208,  256
	.short 0x2E8C, 0x1D42, 0x1234, 0x0013, 0x02AA,  176,  224,  288
	.short 0x2762, 0x199A, 0x1062, 0x0013, 0x02AA,  208,  256,  320
	.short 0x2492, 0x16C1, 0x0E3F, 0x0013, 0x02AA,  224,  288,  368
	.short 0x2000, 0x147B, 0x0D1B, 0x0013, 0x02AA,  256,  320,  400
	.short 0x1C72, 0x11CF, 0x0B4D, 0x0013, 0x02AA,  288,  368,  464
	.short 0x3333, 0x1F82, 0x147B, 0x0014, 0x0555,  320,  416,  512
	.short 0x2E8C, 0x1D42, 0x1234, 0x0014, 0x0555,  352,  448,  576
	.short 0x2762, 0x199A, 0x1062, 0x0014, 0x0555,  416,  512,  640
	.short 0x2492, 0x16C1, 0x0E3F, 0x0014, 0x0555,  448,  576,  736
	.short 0x2000, 0x147B, 0x0D1B, 0x0014, 0x0555,  512,  640,  800
	.short 0x1C72, 0x11CF, 0x0B4D, 0x0014, 0x0555,  576,  736,  928
	.short 0x3333, 0x1F82, 0x147B, 0x0015, 0x0AAA,  640,  832, 1024
	.short 0x2E8C, 0x1D42, 0x1234, 0x0015, 0x0AAA,  704,  896, 1152
	.short 0x2762, 0x199A, 0x1062, 0x0015, 0x0AAA,  832, 1024, 1280
	.short 0x2492, 0x16C1, 0x0E3F, 0x0015, 0x0AAA,  896, 1152, 1472
	.short 0x2000, 0x147B, 0x0D1B, 0x0015, 0x0AAA, 1024, 1280, 1600
	.short 0x1C72, 0x11CF, 0x0B4D, 0x0015, 0x0AAA, 1152, 1472, 1856
	.short 0x3333, 0x1F82, 0x147B, 0x0016, 0x1555, 1280, 1664, 2048
	.short 0x2E8C, 0x1D42, 0x1234, 0x0016, 0x1555, 1408, 1792, 2304
	; inter qp [12:43]
	.short 0x3333, 0x1F82, 0x147B, 0x0011, 0x0055,   40,   52,   64
	.short 0x2E8C, 0x1D42, 0x1234, 0x0011, 0x0055,   44,   56,   72
	.short 0x2762, 0x199A, 0x1062, 0x0011, 0x0055,   52,   64,   80
	.short 0x2492, 0x16C1, 0x0E3F, 0x0011, 0x0055,   56,   72,   92
	.short 0x2000, 0x147B, 0x0D1B, 0x0011, 0x0055,   64,   80,  100
	.short 0x1C72, 0x11CF, 0x0B4D, 0x0011, 0x0055,   72,   92,  116
	.short 0x3333, 0x1F82, 0x147B, 0x0012, 0x00AA,   80,  104,  128
	.short 0x2E8C, 0x1D42, 0x1234, 0x0012, 0x00AA,   88,  112,  144
	.short 0x2762, 0x199A, 0x1062, 0x0012, 0x00AA,  104,  128,  160
	.short 0x2492, 0x16C1, 0x0E3F, 0x0012, 0x00AA,  112,  144,  184
	.short 0x2000, 0x147B, 0x0D1B, 0x0012, 0x00AA,  128,  160,  200
	.short 0x1C72, 0x11CF, 0x0B4D, 0x0012, 0x00AA,  144,  184,  232
	.short 0x3333, 0x1F82, 0x147B, 0x0013, 0x0155,  160,  208,  256
	.short 0x2E8C, 0x1D42, 0x1234, 0x0013, 0x0155,  176,  224,  288
	.short 0x2762, 0x199A, 0x1062, 0x0013, 0x0155,  208,  256,  320
	.short 0x2492, 0x16C1, 0x0E3F, 0x0013, 0x0155,  224,  288,  368
	.short 0x2000, 0x147B, 0x0D1B, 0x0013, 0x0155,  256,  320,  400
	.short 0x1C72, 0x11CF, 0x0B4D, 0x0013, 0x0155,  288,  368,  464
	.short 0x3333, 0x1F82, 0x147B, 0x0014, 0x02AA,  320,  416,  512
	.short 0x2E8C, 0x1D42, 0x1234, 0x0014, 0x02AA,  352,  448,  576
	.short 0x2762, 0x199A, 0x1062, 0x0014, 0x02AA,  416,  512,  640
	.short 0x2492, 0x16C1, 0x0E3F, 0x0014, 0x02AA,  448,  576,  736
	.short 0x2000, 0x147B, 0x0D1B, 0x0014, 0x02AA,  512,  640,  800
	.short 0x1C72, 0x11CF, 0x0B4D, 0x0014, 0x02AA,  576,  736,  928
	.short 0x3333, 0x1F82, 0x147B, 0x0015, 0x0555,  640,  832, 1024
	.short 0x2E8C, 0x1D42, 0x1234, 0x0015, 0x0555,  704,  896, 1152
	.short 0x2762, 0x199A, 0x1062, 0x0015, 0x0555,  832, 1024, 1280
	.short 0x2492, 0x16C1, 0x0E3F, 0x0015, 0x0555,  896, 1152, 1472
	.short 0x2000, 0x147B, 0x0D1B, 0x0015, 0x0555, 1024, 1280, 1600
	.short 0x1C72, 0x11CF, 0x0B4D, 0x0015, 0x0555, 1152, 1472, 1856
	.short 0x3333, 0x1F82, 0x147B, 0x0016, 0x0AAA, 1280, 1664, 2048
	.short 0x2E8C, 0x1D42, 0x1234, 0x0016, 0x0AAA, 1408, 1792, 2304
    MVC_lo:
        .short 0x00F8, 0x0008, 0xF800, 0x0800
    MVC_hi:
        .short 0xF8F8, 0xF808, 0x08F8, 0x0808
    codec_offset:
        .fill 0xB0
    predLumaV_lo:
        .fill 0x08
    predLumaV_hi:
        .fill 0x18
    predChromaV_lo:
        .fill 0x08
    predChromaV_hi:
        .fill 0x18
    predMVH:
        .fill 0x080
    predMVV:
        .fill 0x180
    predLumaH:
        .fill 0x200
    predChromaH:
        .fill 0x200
    sampleStore:
        .fill 0x200
    coeffStore:
        .fill 0x200


.code .text.ENCODE_CODE
ENC_Main:
ENC_Init:
.if BASARABIA_PAMANT_ROMANESC==1
CMU.CPTI i2, P_SVID  || LSU1.LDIL i8, SHAVE_0_BASE_ADR
IAU.SHL i2, i2, 0x10 || LSU1.LDIH i8, SHAVE_0_BASE_ADR
IAU.ADD i8, i8, i2   || LSU1.LDIL i0, 0x01
LSU0.STO32 i0, i8, SVU_DCR_ADDR || LSU1.LDIL i1, 0
nop 5
LSU0.STO32 i1, i8, SVU_PC0_ADDR
LSU0.STO32 i1, i8, SVU_PC1_ADDR
nop 5
LSU1.LDIL i0, 0x02
LSU1.LDIL i1, 0x01
LSU0.STO32 i0, i8, SVU_PCC0_ADDR
LSU0.STO32 i1, i8, SVU_PCC1_ADDR
CMU.VSZMBYTE s14, s14, [ZZZZ]
.endif
; entry point at startup, prepare
IAU.XOR i_rowEnd, i_rowEnd, i_rowEnd || LSU0.LDIL enc, enc_ctrl || LSU1.LDIH enc, enc_ctrl
ENC_WaitCtrl:
CMU.CMZ.i32 i_rowEnd || LSU0.LDO32 i_rowEnd, enc, rowEnd
PEU.PC1C 0x1 || BRU.BRA ENC_WaitCtrl
PEU.PC1C 0x1 || BRU.RPIM 0xA
PEU.PC1C 0x6 || LSU0.LDO32 i_rowStart, enc, rowStart
PEU.PC1C 0x6 || LSU0.LDO32 s_width,    enc, picWidth    || LSU1.LDIL i_posy, 0
PEU.PC1C 0x6 || LSU0.LDO32 s_height,   enc, picHeight   || LSU1.LDIH i_posy, 0xFFFF
PEU.PC1C 0x6 || LSU0.LDO32 i_Coeff,    enc, coeffBuffer

VAU.XOR v_cmd, v_cmd, v_cmd  || LSU1.LDIL i0, 0x1000
LSU1.LDIL i_dst, MBBuffer
LSU1.LDIH i_dst, MBBuffer
BRU.RPI i0 || LSU0.STI64.l v_cmd, i_dst

LSU0.LDA64.l v_cmd, 0x4, FIFO_RD_FRONT
BRU.RPIM 0x05

; !!!!!!!!!!!! for each row
ENC_Row:
; pointerization
CMU.CPVI i0, v_cmd.2  || IAU.SUB i2, i_rowEnd, i_rowStart  || LSU0.LDIL i4, origLuma   || LSU1.LDIH i4, origLuma
CMU.CMZ.i32 i0        || IAU.SHL i3, i2, 0x04              || SAU.SHL.i32 i3, i2, 0x02
PEU.PCCX.NEQ 0x01     || IAU.ADD i4, i4, i3                                            || LSU0.LDO32 i_mode, enc, frameType      || LSU1.LDIL i_dst, codec_offset
                         IAU.ADD i5, i4, i3                || SAU.SHL.i32 i8, i2, 0x01 || LSU0.LDO32 i_Coeff, enc, coeffBuffer   || LSU1.LDIH i_dst, codec_offset
                         IAU.ADD i4, i4, 0x08              || LSU0.STO64 i4, i_dst, 0x00
                         IAU.ADD i5, i5, 0x08              || LSU0.LDA64.l v_Left, 0, initLumaV_lo
                         IAU.ADD i4, i5, i3                || LSU0.STO64 i4, i_dst, 0x08
                         IAU.ADD i5, i4, i3                || LSU0.LDA64.h v_Left, 0, initLumaV_hi   || LSU1.LDIL i6, origChromaU
                         IAU.SUB i4, i4, 0x08              || LSU0.STO64 i4, i_dst, 0x18
                         IAU.SUB i5, i5, 0x08              || LSU0.LDA64.l v_Top,  0, initChromaV_lo || LSU1.LDIH i6, origChromaU
PEU.PCCX.NEQ 0x01     || IAU.ADD i6, i6, i3                || LSU0.STO64 i4, i_dst, 0x10
                         IAU.ADD i7, i6, i8                || LSU0.LDA64.h v_Top,  0, initChromaV_hi
CMU.CPVI i18, v_cmd.0 || IAU.INCS i6, (MAX_WIDTH<<3)       || LSU0.STO64 i6, i_dst, 0x20
CMU.CPVI i0,  v_cmd.1 || IAU.INCS i7, (MAX_WIDTH<<3)       || LSU0.STO64 i6, i_dst, 0x30            || LSU1.LDIL i1, (sizeof_CoeffBuffer>>2)
CMU.CMZ.i32 i18       || IAU.MUL i1, i1, i0                || LSU0.STO64 i6, i_dst, 0x28            || LSU1.LDIL i2, sizeof_MBRow
PEU.PC1C 0x4          || BRU.BRA ENC_Stall
                         IAU.MUL i2, i2, i0                || LSU0.STA64.l v_Left, 0, predLumaV_lo  || LSU1.LDIL i_MBInfo, MBBuffer
                         SAU.SHL.i32 i18, i18, 0x04        || LSU0.STA64.h v_Left, 0, predLumaV_hi  || LSU1.LDIH i_MBInfo, MBBuffer
                         IAU.ADD i_Coeff, i_Coeff, i1      || LSU0.STA64.l v_Top,  0, predChromaV_lo
CMU.CPII i_posx, i_rowStart                                || LSU0.STA64.h v_Top,  0, predChromaV_hi
CMU.CPII i_posy, i18  || IAU.ADD i_MBInfo, i_MBInfo, i2


; !!!!!!!!! for each mb
.lalign
ENC_Col:
CMU.CPSI i2, s_width       || IAU.CMPI i_mode, SLICE_TYPE_I || LSU0.LDIL i30, ME       || LSU1.LDIH i30, ME
PEU.PCIX.EQ  0x30          || IAU.SHR.u32 i2, i2, 0x04      || LSU0.LDIL i30, MD_Intra || LSU1.LDIH i30, MD_Intra
BRU.JMP i30                || IAU.SHR.u32 i0, i_posx, 0x04  || LSU1.LDIL i3,  0xFFFF
CMU.CPIVR.x32 v_SAD8x8, i3 || IAU.SHR.u32 i1, i_posy, 0x04
                              IAU.MUL i2, i2, i1
                              CMU.CPIS s_CoeffStart, i_Coeff       || LSU0.STO8  i0, i_MBInfo, mb_posx
                              IAU.ADD i2, i2, i0                   || LSU0.STO8  i1, i_MBInfo, mb_posy
                              IAU.SUB i_desc, i_rowEnd, i_rowStart || LSU0.STO16 i2, i_MBInfo, mb_idx

.lalign
ENC_ColCycle:
CMU.CPSI i4, s_CoeffStart         || IAU.INCS i_posx, 0x10              || LSU0.LDO32 i_mode, enc, frameType
CMU.CPII i5, i4                   || IAU.SUB.u32s i0, i_rowEnd, i_posx  || LSU1.LDIL i6, 0x02
PEU.PCIX.NEQ 0 || BRU.BRA ENC_Col || IAU.FEXT.u32 i0, i_cbp, 0x04, 0x02
CMU.CMII.i32 i0, i6
PEU.PC1C 0x2                      || IAU.FINS i_cbp, i6, 0x04, 0x02
                                     IAU.SUB.u32s i4, i_Coeff, i4       || LSU0.STO8  i_cbp, i_MBInfo, mb_cbp
                                     IAU.SHR.u32  i4, i4, 0x01          || LSU0.STO32 i5, i_MBInfo, mb_coeffAddr
                                     IAU.INCS i_MBInfo, sizeof_MBInfo   || LSU0.STO16 i4, i_MBInfo, mb_coeffCount

ENC_RowCycle:
; transfer neighbours to next shave
LSU0.LDA64.l v_Left, 0, predLumaV_lo
LSU0.LDA64.h v_Left, 0, predLumaV_hi
LSU0.LDA64.l v_Top,  0, predChromaV_lo
LSU0.LDA64.h v_Top,  0, predChromaV_hi
CMU.CPSI i9, s_width      || LSU0.LDO32 i0, enc, entropyFIFO ; || LSU1.LDIL i0, 0x06
CMU.CMII.i32 i_rowEnd, i9 ; || LSU0.STA32 i0, 0x4, SVU_CACHE_CTRL_TXN_TYPE
; change window like a sneaky bastard
PEU.PC1C 0x4 || LSU0.STA64.l v_Left, 0x3, initLumaV_lo
PEU.PC1C 0x4 || LSU0.STA64.h v_Left, 0x3, initLumaV_hi
PEU.PC1C 0x4 || LSU0.STA64.l v_Top,  0x3, initChromaV_lo
PEU.PC1C 0x4 || LSU0.STA64.h v_Top,  0x3, initChromaV_hi
LSU0.ST64.l v_cmd, i0
BRU.RPIM 0x14
; update circular buffers
PEU.PVEN4WORD 0xC || VAU.ADD.i32 v_cmd, v_cmd, 0x01 || LSU1.LDIL i0, 0x01
CMU.CPIVR.x32 v20, i0 || LSU1.LDIL i0, 0x03
CMU.CPIVR.x32 v21, i0
PEU.PVEN4WORD 0x4 || VAU.AND v_cmd, v_cmd, v20
PEU.PVEN4WORD 0x8 || VAU.AND v_cmd, v_cmd, v21
.if BASARABIA_PAMANT_ROMANESC==1
CMU.CPTI i2, P_SVID  || LSU1.LDIL i14, SHAVE_0_BASE_ADR
IAU.SHL i2, i2, 0x10 || LSU1.LDIH i14, SHAVE_0_BASE_ADR
IAU.ADD i14, i14, i2
LSU0.LDO32 i1, i14, SVU_PC1_ADDR
nop 5
.endif
LSU0.LDA64.l v_cmd, 0x4, FIFO_RD_FRONT
nop 5
.if BASARABIA_PAMANT_ROMANESC==1
CMU.CPSI i14, s14 || LSU0.LDO32 i2, i14, SVU_PC1_ADDR
nop 5
IAU.SUB i2, i2, i1
IAU.ADD i14, i14, i2
CMU.CPIS s14, i14
.endif
BRU.BRA ENC_Row  
nop 5
ENC_debug:
BRU.SWIH 0x1F

ENC_Stall:
; skip this cycle
LSU0.LDO32 i0, enc, entropyFIFO
IAU.XOR i0, i0, i0 || LSU1.LDIL i1, MAX_CODEC
IAU.SUB i1, i0, i1
IAU.SHL i1, i1, 0x04 
CMU.CMII.i32 i1, i18
 ; return point from hw process
.if BASARABIA_PAMANT_ROMANESC==1
PEU.PC1C 0x2 || CMU.CPTI i2, P_SVID  || LSU1.LDIL i8, SHAVE_0_BASE_ADR
PEU.PC1C 0x2 || IAU.SHL i2, i2, 0x10 || LSU1.LDIH i8, SHAVE_0_BASE_ADR
PEU.PC1C 0x2 || IAU.ADD i8, i8, i2
PEU.PC1C 0x2 || LSU0.LDO32 i0, i8, SVU_PC0_ADDR
PEU.PC1C 0x2 || LSU0.LDO32 i1, i8, SVU_PC1_ADDR
PEU.PC1C 0x2 || BRU.RPIM 0xA
.endif
PEU.PC1C 0x2 || BRU.SWIH 0x1F
; check for halt command received
PEU.PC1C 0x5 || LSU0.ST64.l v_cmd, i0
nop 5
BRU.BRA ENC_Row || LSU0.LDA64.l v_cmd, 0x4, FIFO_RD_FRONT
nop 5

.include MotionEstimation.asm
.include ModeDecision.asm
.include Intra16x16Luma.asm
.include InterLuma.asm
.include InterSkip.asm
.include IntraChroma.asm
.include InterChroma.asm
.include tq.asm
