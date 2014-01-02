; Entropy coding
; This module controls all other modules
.version 0.50
.include config.h
.include ec.h

.data EC_DATA
   encoder:
   .fill 0x40
   DMA_ctrl:
   .fill 0x120
   RC_trail_buffer:
   .int 0x00, 0x00, 0x00, 0x00
   RC_update_table:
   .short 11, 10, 15, 12, 11, 15, 11, 10
   i4x4PredModeV:
   .fill 0x40
   mvV:
   .fill 0x40
   golomb_length_table:
   .byte  0, 1, 3, 3, 5, 5, 5, 5, 7, 7, 7, 7, 7, 7, 7, 7
   .byte  9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9
   .byte 11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11
   .byte 11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11
   .byte 13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13
   .byte 13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13
   .byte 13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13
   .byte 13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13
   .byte 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15
   .byte 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15
   .byte 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15
   .byte 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15
   .byte 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15
   .byte 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15
   .byte 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15
   .byte 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15
   reserved:
   .fill 0x100
   slice_header:
   .fill sizeof_sh
   i4x4PredModeH:
   .fill 0x400
   mvH:
   .fill 0x400


.code .text.EC_CODE
; for linker entry point spec
EC_Main:
EC_Init:
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
CMU.VSZMBYTE s13, s13, [ZZZZ]
CMU.VSZMBYTE s14, s14, [ZZZZ]
.endif

CMU.VSZMBYTE s_frameNum, s_frameNum, [ZZZZ] || LSU0.LDIL enc, encoder || LSU1.LDIH enc, encoder
CMU.VSZMBYTE v24, v24, [ZZZZ] || LSU1.LDIL i0, 0x03
CMU.CPIV.x32 v24.1, i0        || IAU.SUB i0, i0, 0x01  || LSU0.LDO32 i_ctrl, enc, cmdCfg
CMU.CPIV.x32 v24.2, i0        || IAU.SUB i0, i0, 0x01  || LSU0.LDO32 i_width, enc, width
CMU.CPIV.x32 v24.3, i0        || IAU.SUB i0, i0, 0x01  || LSU0.LDO32 i_height, enc, height
CMU.CPIV.x32 v25.0, i0        || IAU.SUB i0, i0, 0x01  || LSU0.LDO32 i_qp, enc, frameQp
CMU.CPIV.x32 v25.1, i0        || IAU.SUB i0, i0, 0x01  || LSU0.LDO32 i_gop, enc, frameGOP
CMU.CPIV.x32 v25.2, i0        || IAU.SUB i0, i0, 0x01  || LSU0.LDO32 i_frame, enc, frameNum
CMU.CPIV.x32 v25.3, i0
CMU.VSZMBYTE i3, i_ctrl, [ZZZ0] || SAU.SHR.u32 i2, i_width, 0x04       || LSU1.LDIL i0, MAX_CODEC
IAU.ONES i3, i3                 || LSU0.LDO32 i4, enc, origFrame       || LSU1.LDIL i1, RC_WINDOW
IAU.CLAMP0.i32 i3, i3, i0       || LSU0.LDO32 i5, enc, nextFrame
SAU.DIV.i32 i2, i2, i3          || LSU0.LDO32 i6, enc, currFrame       || LSU1.LDIL i30, EC_Startup
SAU.MOD.i32 i3, i2, i3          || LSU0.LDO32 i7, enc, refFrame        || LSU1.LDIH i30, EC_Startup
CMU.CPIS s_nrCodecs, i3         || IAU.FEXT.u32 i0, i_ctrl, 0x10, 0x03 || LSU1.LDIL i_row, 0xF000
CMU.VSZMBYTE i10, i10, [ZZZZ]   || IAU.SHL i0, i0, 0x08                || LSU1.LDIH i_row, 0x0FF0
                                   IAU.ADD i_row, i_row, i0            || LSU1.LDIL i_Coeff, 0x8000
CMU.CPIS.i32.f32 s_updateRC, i1 || IAU.SHL i0, i0, 0x09                || LSU1.LDIH i_Coeff, 0x1000
                                   IAU.ADD i_Coeff, i_Coeff, i0
CMU.VSZMBYTE s15, s15, [ZZZZ]   || IAU.SUB.u32s i_gop, i_gop, 0x01
BRU.RPIM 0x0A

BRU.RPS i30, 0x04           || IAU.BSF i1, i_ctrl            || SAU.SHL.u32 i11, i10, 0x04
                               IAU.SHL i0, i1, 0x08          || SAU.ADD.i32 i10, i10, i2   || LSU1.LDIH i1, 0x01
CMU.VSZMBYTE i1, i1, [1032]                                                                || LSU1.LDIL i14, 0xF000
CMU.CMZ.i32 i3              || IAU.FINS i_ctrl, i0, i1                                     || LSU1.LDIH i14, 0x0FF0
PEU.PCCX.NEQ 0x02           || IAU.ADD i14, i14, i0          || SAU.ADD.i32 i10, i10, 0x01 || LSU1.LDIL i13, 0x4000
nop
PEU.PCCX.NEQ 0x02           || IAU.SHL i0, i0, 0x09          || SAU.SUB.i32 i3,  i3,  0x01 || LSU1.LDIH i13, 0x1000
                               IAU.ADD i13, i13, i0          || SAU.SHL.i32 i12, i10, 0x04 || LSU1.LDIL i1, enc_offset
                               IAU.SUB.u32s i0, i_width, i11                               || LSU1.LDIH i1, enc_offset
CMU.CMZ.i32 i0              || IAU.ADD i_dst, i13, i1                                      || LSU1.LDIL i15, SLICE_TYPE_I
PEU.PCCX.NEQ 0x10           || CMU.SHLIV.x32 v26, i11        || LSU0.STO32 i_width, i_dst, picWidth
PEU.PCCX.NEQ 0x10           || CMU.SHLIV.x32 v27, i12        || LSU0.STO32 i_height, i_dst, picHeight
EC_Startup:
PEU.PCCX.NEQ 0x10           || CMU.SHLIV.x32 v28, i13        || LSU0.STO32 i_qp, i_dst, qpy
PEU.PCCX.NEQ 0x10           || CMU.SHLIV.x32 v29, i14        || LSU0.STO32 i15, i_dst, frameType
PEU.PCCX.NEQ 0x10                                            || LSU0.STO32 i_Coeff, i_dst, coeffBuffer || LSU1.LDIL i0, sizeof_CoeffBuffer
PEU.PCCX.NEQ 0x10           || IAU.ADD i_Coeff, i_Coeff, i0  || LSU0.STO32 i_row, i_dst, entropyFIFO
PEU.PCCX.NEQ 0x10                                            || LSU0.STO32 i11, i_dst, rowStart
PEU.PCCX.NEQ 0x10                                            || LSU0.STO32 i12, i_dst, rowEnd

EC_Params:
CMU.VSZMWORD v26, v26, [0123] || VAU.SWZWORD v27, v27, [0123]
CMU.VSZMWORD v28, v28, [0123] || VAU.SWZWORD v29, v29, [0123]
; encode pps, sps
CMU.CPSI i_row, s_nrCodecs || BRU.BRA nalu_enc_param
IAU.XOR i0, i0, i0         || LSU0.LDO32 i10, enc, cmdCfg
IAU.SUB i_row, i0, i_row   || LSU0.LDO32 i9,  enc, naluBuffer
IAU.SHR.u32 i_width,  i_width,  0x04 || LSU1.LDIL i30, EC_Encode
IAU.SHR.u32 i_height, i_height, 0x04 || LSU1.LDIH i30, EC_Encode
IAU.SUB.u32s i_frame, i_frame,  0x01 || LSU1.LDIL i_gop, 0

.lalign
EC_Encode:
CMU.CPSI i4, s_nrCodecs  || SAU.SUM.i32 i1, v29, 0x01
CMU.CPVCR.x32 v31, v22.0 || IAU.SUB.u32s i0, i4, 0x00
PEU.PCIX.NEQ 0x10        || SAU.SUM.i32 i1, v29, 0x02 || LSU0.ST64.l v31, i1
CMU.CPVCR.x32 v31, v22.1 || IAU.SUB.u32s i0, i4, 0x01 || BRU.BRA DMA_Prep
PEU.PCIX.NEQ 0x10        || SAU.SUM.i32 i1, v29, 0x04 || LSU0.ST64.l v31, i1
CMU.CPVCR.x32 v31, v22.2 || IAU.SUB.u32s i0, i4, 0x02 || LSU1.LDIL i30, EC_CurrRow
PEU.PCIX.NEQ 0x10        || SAU.SUM.i32 i1, v29, 0x08 || LSU0.ST64.l v31, i1
CMU.CPVCR.x32 v31, v22.3 || IAU.SUB.u32s i0, i4, 0x03 || LSU1.LDIH i30, EC_CurrRow
PEU.PCIX.NEQ 0x10        || CMU.CPIVR.x32 v22, i_row  || LSU0.ST64.l v31, i1

.lalign
EC_CurrRow:
CMU.CMZ.i32 i_row || VAU.OR v22, v24, v24
PEU.PC1C 0x3 || BRU.BRA nalu_enc_current
CMU.CPSI i0, s_nrCodecs                                               || LSU1.LDIL i30, EC_NextRow
CMU.VSZMBYTE s_DMAStat, s_DMAStat, [ZZZZ]         || IAU.CMPI i0, 0x1 || LSU1.LDIH i30, EC_NextRow
PEU.PCIX.EQ 0x04 || CMU.VSZMWORD v22, v22, [1032] || IAU.CMPI i0, 0x2
PEU.PCIX.EQ 0x04 || CMU.VSZMWORD v22, v22, [1032] || IAU.CMPI i0, 0x3
PEU.PCIX.EQ 0x04 || CMU.VSZMWORD v22, v22, [2103]

.lalign
EC_NextRow:
IAU.ADD i_row, i_row, 0x01
PEU.PC1I 0x4 || BRU.BRA EC_EncodeLoop
nop 5
CMU.CMII.i32 i_row, i_height || IAU.CMPI i_frame, 0
PEU.PC1C 0x4 || BRU.BRA nalu_enc_next
PEU.PCC0I.AND 0x3, 0x1 || BRU.BRA EC_EncodeLoop
LSU1.LDIL i30, EC_EncodeLoop
LSU1.LDIH i30, EC_EncodeLoop
nop 3

EC_NextFrame:
IAU.MUL i2, i_width, i_height     || LSU0.LDO32 i4, enc, origFrame
LSU0.LDO32 i5, enc, nextFrame     || LSU1.LDIL i_row, 0
CMU.CMZ.i32 i_gop || IAU.SHL i2, i2, 0x08                || LSU0.LDO32 i6, enc, currFrame     || LSU1.LDIL i3, SLICE_TYPE_P
PEU.PCCX.EQ 0x20  || IAU.SHR.u32 i1, i2, 0x01            || LSU0.LDO32 i7, enc, refFrame      || LSU1.LDIL i3, SLICE_TYPE_I
PEU.PCCX.EQ 0x10  || IAU.SUB.u32s i_frame, i_frame, 0x01 || LSU0.LDO32 i_gop, enc, frameGOP
BRU.BRA nalu_enc_next || IAU.ADD i2, i2, i1              || LSU0.STO32 i_frame, enc, frameNum
.if STREAM_TEST==1
CMU.CMZ.i32 i_frame
PEU.PCCX.NEQ 0x01 || IAU.ADD i5, i5, i2 || LSU0.STO32 i5, enc, origFrame
LSU0.STO32 i5, enc, nextFrame
IAU.ADD i6, i6, i2 || LSU0.STO32 i6, enc, refFrame
IAU.SUB.u32s i_gop, i_gop, 0x01 || LSU0.STO32 i6, enc, currFrame
.else
CMU.CMZ.i32 i_frame || IAU.XOR i4, i4, i4
PEU.PCCX.EQ  0x01 || IAU.ADD i4, i5, 0 || LSU0.STO32 i5, enc, origFrame
LSU0.STO32 i4, enc, nextFrame
LSU0.STO32 i6, enc, refFrame
IAU.SUB.u32s i_gop, i_gop, 0x01 || LSU0.STO32 i7, enc, currFrame
.endif

.lalign
EC_EncodeLoop:
.if BASARABIA_PAMANT_ROMANESC==1
CMU.CPTI i2, P_SVID  || LSU1.LDIL i14, SHAVE_0_BASE_ADR
IAU.SHL i2, i2, 0x10 || LSU1.LDIH i14, SHAVE_0_BASE_ADR
IAU.ADD i14, i14, i2
LSU0.LDO32 i1, i14, SVU_PC0_ADDR
.endif
CMU.CPSI i1, s_DMAStat || LSU1.LDIL i0, DMA_BUSY_MASK
CMU.CMTI.BITP i0, P_GPI || IAU.CMPI i1, 0xC
; wait for DMA task to finish
PEU.PC1C 0x6 || BRU.RPIM 0 || CMU.CMTI.BITP i0, P_GPI
PEU.PC1I 0x4 || BRU.BRA DMA_Schedule
PEU.PC1I 0x4 || LSU1.LDIL i30, EC_EncodeLoop
PEU.PC1I 0x4 || LSU1.LDIH i30, EC_EncodeLoop
PEU.PC1I 0x3 || LSU0.LDO32 i8, enc, frameGOP
PEU.PC1I 0x3 || LSU0.LDO32 i9, enc, frameQp
PEU.PC1I 0x3 || CMU.CPIVR.x32 v22, i_height

; update buffer indexes
CMU.CPSI i4, s_nrCodecs  || VAU.SWZWORD v24, v24, [2103] || LSU1.LDIL i5, 0x10
CMU.CPII i3, i4          || VAU.SWZWORD v23, v25, [3210] || LSU1.LDIL i6, 0x04
; read reply from shaves to complete cycle
BRU.RPI i4 || LSU0.LDA64.l v_cmd, 0x4, FIFO_RD_FRONT
BRU.RPIM 0x05


; gop to slice type translation
CMU.CMZ.i32 i_gop || IAU.SUB.u32s i0, i8, 0x01    || LSU0.LDO32 i2, enc, nextFrame
PEU.PCCX.NEQ 0x01 || IAU.SUB.u32s i0, i_gop, 0x01 || LSU1.LDIL i8, SLICE_TYPE_P
PEU.PCIX.EQ  0x20 || VAU.SUB.i32 v22, v22, 0x01   || LSU1.LDIL i8, SLICE_TYPE_I
EC_EncodeUpdate:
CMU.LUT32.u32 i1, i5, 0x3  || IAU.ADD i0, i_row, i3 || SAU.ADD.i32 i5, i5, 0x01 || LSU1.LDIL i11, enc_offset
CMU.CMII.i32 i0, i_height  || IAU.SUB.u32s i3, i3, 0x01 || LSU1.LDIH i11, enc_offset
PEU.PCIX.NEQ 0 || BRU.BRA EC_EncodeUpdate  || IAU.ADD i1, i1, i11
PEU.PC1C 0x3   || IAU.SUB i0, i0, i_height
PEU.PC1C 0x1                                       || LSU0.STO32 i8, i1, frameType
PEU.PCCX.EQ 0x10       || IAU.CMPI i_frame, 0      || LSU0.STO32 i9, i1, qpy
PEU.PCC0I.AND 0x3, 0x1 || IAU.SUB i0, i0, i4
CMU.LUTW32 i0, i6, 0x3 || SAU.ADD.i32 i6, i6, 0x01

EC_WaitNext:
CMU.CMVV.i32 v25, v22  || IAU.CMPI i2, 0 
PEU.PCC0I.AND 0x1, 0x1 || BRU.BRA EC_WaitNext || LSU0.LDO32 i2, enc, nextFrame
nop 5
CMU.CMII.i32 i_row, i_height
PEU.PC1C 0x4 || BRU.BRA EC_Encode
PEU.PC1C 0x3 || BRU.BRA EC_ShutDown
nop 5

.lalign
EC_ShutDown:
; encoding complete, send HALT
CMU.CPSI i18, s_nrCodecs || LSU0.LDIL i0, 0x0000 || LSU1.LDIH i0, 0xFFFF
CMU.CPIV.x32 v_cmd.0, i0 || LSU1.LDIL i_src, 0x14
EC_Destroy:
CMU.LUT32.u32 i_dst, i_src, 0x3 || IAU.SUB.u32s i18, i18, 0x01
PEU.PC1I 0x2 || BRU.BRA EC_Destroy
IAU.INCS i_src, 0x01
LSU0.ST64.l v_cmd, i_dst
nop 3

 ; return point from hw process
.if BASARABIA_PAMANT_ROMANESC==1
CMU.CPTI i2, P_SVID  || LSU1.LDIL i8, SHAVE_0_BASE_ADR
IAU.SHL i2, i2, 0x10 || LSU1.LDIH i8, SHAVE_0_BASE_ADR
IAU.ADD i8, i8, i2
LSU0.LDO32 i0, i8, SVU_PC0_ADDR
LSU0.LDO32 i1, i8, SVU_PC1_ADDR
BRU.RPIM 0x05
.else
BRU.RPIM 0x05
.endif
BRU.SWIH 0x1F

;EC_debug:
nop 5


; v20 : idxOrig
; v21 : idxRef

; v23 : prev
; v24 : coeff buffer
; v25 : row
; v26 : start
; v27 : end
; v28 : base addr
; v29 : fifo
; entropy coder calls this function to set up initial fetches
.lalign
DMA_Init:
CMU.CPZV v20, 0x3       || IAU.XOR i0, i0, i0   || LSU0.LDO32 i1, enc, origFrame || LSU1.LDIL i2, origLuma
CMU.CPVI.x32 i7, v28.0  || IAU.SUB i0, i0, 0x01                                  || LSU1.LDIH i2, origLuma
PEU.PVEN4WORD 0x5 || CMU.CPIV.x32 v23.0, i0 || IAU.SUB i0, i0, 0x01 || VAU.ADD.i32 v20, v20, 0x01
PEU.PVEN4WORD 0x1 || CMU.CPIV.x32 v23.1, i0 || IAU.SUB i0, i0, 0x01 || VAU.ADD.i32 v21, v21, 0x02
PEU.PVEN4WORD 0x2 || CMU.CPIV.x32 v23.2, i0 || IAU.SUB i0, i0, 0x01 || VAU.ADD.i32 v21, v21, 0x01
PEU.PVEN4WORD 0x8 || CMU.CPIV.x32 v23.3, i0                         || VAU.ADD.i32 v21, v21, 0x03
CMU.VSZMBYTE i2, i2, [Z210] || SAU.MUL.i32 i6, i_width, i_height || LSU1.LDIL i0, (DMA_ENABLE | DMA_DST_USE_STRIDE)
CMU.CPVI.x32 i3, v26.0      || IAU.ADD i2, i2, i7
CMU.CPVI.x32 i4, v27.0      || IAU.SHL i5, i_width, 0x04 || LSU0.STA32 i0, 0x4, DMA0_CFG
                               IAU.SUB i4, i4, i3        || LSU0.STA32 i1, 0x4, DMA0_SRC_ADDR
                               IAU.SHL i3, i4, 0x04      || LSU0.STA32 i2, 0x4, DMA0_DST_ADDR
                               IAU.SUB i5, i5, i4        || LSU0.STA32 i3, 0x4, DMA0_SIZE         || LSU1.LDIL i2, origChromaU
                               IAU.SHL i6, i6, 0x08      || LSU0.STA32 i4, 0x4, DMA0_LINE_WIDTH   || LSU1.LDIH i2, origChromaU
CMU.VSZMBYTE i2, i2, [Z210] || IAU.ADD i1, i1, i6        || LSU0.STA32 i5, 0x4, DMA0_LINE_STRIDE
                               IAU.ADD i2, i2, i7        || LSU0.STA32 i0, 0x4, DMA1_CFG
                               IAU.SHR.u32 i3, i3, 0x02  || LSU0.STA32 i1, 0x4, DMA1_SRC_ADDR
                               IAU.SHR.u32 i4, i4, 0x01  || LSU0.STA32 i2, 0x4, DMA1_DST_ADDR
                               IAU.SHR.u32 i5, i5, 0x01  || LSU0.STA32 i3, 0x4, DMA1_SIZE         || LSU1.LDIL i2, origChromaV
                               IAU.SHR.u32 i6, i6, 0x02  || LSU0.STA32 i4, 0x4, DMA1_LINE_WIDTH   || LSU1.LDIH i2, origChromaV
CMU.VSZMBYTE i2, i2, [Z210] || IAU.ADD i1, i1, i6        || LSU0.STA32 i5, 0x4, DMA1_LINE_STRIDE
                               IAU.ADD i2, i2, i7        || LSU0.STA32 i0, 0x4, DMA2_CFG
                                                            LSU0.STA32 i1, 0x4, DMA2_SRC_ADDR
                                                            LSU0.STA32 i2, 0x4, DMA2_DST_ADDR
                                                            LSU0.STA32 i3, 0x4, DMA2_SIZE
BRU.JMP i30                                              || LSU0.STA32 i4, 0x4, DMA2_LINE_WIDTH
CMU.CPVV v22, v23                                        || LSU0.STA32 i5, 0x4, DMA2_LINE_STRIDE || LSU1.LDIL i0, 0x02
; start DMA task
CMU.VSZMBYTE s_DMAStat, s_DMAStat, [ZZZZ] || LSU0.STA32 i0, 0x4, DMA_TASK_REQ || LSU1.LDIL i0, DMA_BUSY_MASK
BRU.RPIM 0x14
CMU.CMTI.BITP i0, P_GPI
; wait for DMA task to finish
PEU.PC1C 0x6 || BRU.RPIM 0 || CMU.CMTI.BITP i0, P_GPI


; dma states
;[0000] rec Y
;[0010] rec U
;[0011] rec V
;[0100] org Y
;[0110] org U
;[0111] org V
;[1000] ref Y
;[1010] ref U
;[1011] ref V
; entropy coder calls this function to process DMA request based on pointers
.lalign
DMA_Schedule:
CMU.CPSI i18, s_DMAStat  || LSU1.LDIL i0, DMA_BUSY_MASK
CMU.CMTI.BITP i0, P_GPI  || IAU.CMPI i18, 0xC
PEU.PCC0I.OR  0x6, 0x3   || BRU.JMP i30
PEU.PCC0I.AND 0x1, 0x4   || CMU.CPIVR.x32 v19, i_width   || SAU.SHR.u32 i0, i18, 0x01
PEU.PCC0I.AND 0x1, 0x4   || VAU.SHL.i32 v19, v19, 0x04   || LSU1.LDIL i1, DMA_ctrl
PEU.PCC0I.AND 0x1, 0x4   || VAU.SUB.i32 v18, v27, v26    || SAU.SHL.u32 i0, i0, 0x05
PEU.PCC0I.AND 0x1, 0x4                                   || LSU1.LDIH i1, DMA_ctrl
PEU.PCC0I.AND 0x1, 0x4   || IAU.ADD i1, i1, i0           || LSU1.LDIL i0, (DMA_ENABLE | DMA_DST_USE_STRIDE)
IAU.MUL i6, i_width, i_height    || LSU0.LDI64.l v16, i1
VAU.SUB.i32 v19, v19, v18        || LSU0.LDI64.h v16, i1
IAU.SHL i6, i6, 0x06             || LSU0.LDI64.l v17, i1      || LSU1.LDIL i_src, 0x4100
IAU.FEXT.u32 i9, i18, 0x01, 0x01 || LSU0.LDI64.h v17, i1      || LSU1.LDIH i_src, 0x0FF0
PEU.PC1I 0x6 || VAU.SHR.i32 v18, v18, 0x01
PEU.PC1I 0x6 || VAU.SHR.i32 v19, v19, 0x01
CMU.CPSI i8, s_nrCodecs || IAU.FEXT.u32 i9, i18, 0x03, 0x01 || LSU1.LDIL i7, (origChromaV-origChromaU)
PEU.PC1I 0x6 || LSU1.LDIL i7, (refChromaV-refChromaU)

DMA_ScheduleLoop:
CMU.CPVI.x32 i1, v16.0  || IAU.FEXT.u32 i9, i18, 0x00, 0x01 || SAU.SUM.i32 i4, v18, 0x01
PEU.PCIX.NEQ 0x01       || CMU.CMZ.i32 i1 || IAU.ADD i1, i1, i6
CMU.CPVI.x32 i2, v17.0  || IAU.FEXT.u32 i9, i18, 0x00, 0x01
PEU.PCIX.NEQ 0x01       || IAU.ADD i2, i2, i7  || SAU.SUM.i32 i5, v19, 0x01
IAU.FEXT.u32 i9, i18, 0x01, 0x01 || SAU.SHL.i32 i3, i4, 0x04 || LSU1.LDIL i10, 0
PEU.PCIX.NEQ 0x02 ||IAU.FEXT.u32 i9, i18, 0x02, 0x02  || SAU.SHL.i32 i3, i4, 0x03
; address inversion
PEU.PC1I 0x1 || CMU.CPII i2, i1 || IAU.ADD i1, i2, 0 || LSU1.LDIL i0, (DMA_ENABLE|DMA_SRC_USE_STRIDE)
PEU.PCCX.NEQ 0x10 || IAU.SUB.u32s i8, i8, 0x01     || LSU0.ST32  i0, i_src
PEU.PC1I 0x6      || BRU.BRA DMA_ScheduleLoop
PEU.PCCX.NEQ 0x10 || CMU.VSZMWORD v16, v16, [0321] || LSU0.STO32 i1, i_src, 0x04
PEU.PCCX.NEQ 0x10 || CMU.VSZMWORD v17, v17, [0321] || LSU0.STO32 i2, i_src, 0x08
PEU.PCCX.NEQ 0x10 || CMU.VSZMWORD v18, v18, [0321] || LSU0.STO32 i3, i_src, 0x0C
PEU.PCCX.NEQ 0x10 || CMU.VSZMWORD v19, v19, [0321] || LSU0.STO32 i4, i_src, 0x10
PEU.PCCX.NEQ 0x11 || IAU.INCS i_src, 0x100         || LSU0.STO32 i5, i_src, 0x14
; return point
IAU.FEXT.u32 i9, i18, 0x00, 0x02 || SAU.ADD.i32 i18, i18, 0x01
PEU.PCIX.EQ 0x02 || SAU.ADD.i32 i18, i18, 0x02
IAU.FEXT.u32 i1, i_src, 0x08, 0x04
IAU.SUB i1, i1, 0x02 || BRU.JMP i30
PEU.PC1I 0x3 || LSU0.STA32 i1, 0x4, DMA_TASK_REQ
PEU.PC1I 0x3 || BRU.RPIM 0x14
CMU.CPIS s_DMAStat, i18 || IAU.CMPI i18, 0x0C
; handle buffer indexes
PEU.PC1I 0x3 || CMU.VSZMWORD v20, v20, [2103]
PEU.PC1I 0x3 || CMU.VSZMWORD v21, v21, [2103]

.lalign
DMA_Prep:
; reconstructed calculation
                               IAU.SHL i0, i_width, 0x08 || VAU.SHR.i32 v1, v26, 0x01 || LSU0.LDO32 i2, enc, currFrame
.if STREAM_TEST==1
CMU.CPIVR.x32 v0, i0        || IAU.MUL i4, i0, i_height  || VAU.SUB.i32 v6, v27, v26
CMU.CMVV.i32 v23, v22       || VAU.MUL.i32 v16, v0, v23
IAU.SHR.u32 i3, i4, 0x01
IAU.ADD i3, i3, i4          || VAU.SHL.i32 v6,  v6,  0x04
CMU.CPIVR.x32 v4, i4        || VAU.SHR.i32 v17, v16, 0x02
CMU.CPIVR.x32 v2, i2        || VAU.ADD.i32 v16, v16, v26 || IAU.ADD i3, i3, i2
.else
CMU.CPIVR.x32 v0, i0        || IAU.MUL i4, i0, i_height  || VAU.SUB.i32 v6, v27, v26  || LSU0.LDO32 i3, enc, refFrame
CMU.CMVV.i32 v23, v22       || VAU.MUL.i32 v16, v0, v23
nop
                               VAU.SHL.i32 v6,  v6,  0x04
CMU.CPIVR.x32 v4, i4        || VAU.SHR.i32 v17, v16, 0x02
CMU.CPIVR.x32 v2, i2        || VAU.ADD.i32 v16, v16, v26
.endif
CMU.CPIVR.x32 v3, i3        || VAU.ADD.i32 v17, v17, v1
PEU.PVV32 0x4               || VAU.ADD.i32 v2,  v3,  0
IAU.SHR.u32 i5, i4, 0x02    || VAU.ADD.i32 v17, v17, v4   || LSU1.LDIL i_dst, DMA_ctrl
CMU.CPIVR.x32 v5, i5        || VAU.ADD.i32 v16, v16, v2   || LSU1.LDIH i_dst, DMA_ctrl
CMU.CMZ.i32 v23             || VAU.ADD.i32 v17, v17, v2   || LSU1.LDIL i2, origLuma
PEU.PVV32 0x4               || VAU.SUB.i32 v16, v16, v16  || LSU1.LDIH i2, origLuma
PEU.PVV32 0x4               || VAU.SUB.i32 v17, v17, v17  || LSU1.LDIL i3, origChromaU
                               VAU.MUL.i32 v18, v20, v6   || LSU1.LDIH i3, origChromaU
CMU.CPIVR.x32 v2, i2        || IAU.SUB i0, i_height, 0x01
CMU.VSZMBYTE v2, v2, [Z210]                               || LSU0.LDO32 i2, enc, origFrame
CMU.CPIVR.x32 v3, i3        || VAU.SHR.i32 v19, v18, 0x02 || LSU0.LDO32 i3, enc, nextFrame
CMU.VSZMBYTE v3, v3, [Z210] || VAU.ADD.i32 v18, v18, v2   || LSU0.STO64.l v16, i_dst, 0x00
CMU.CPIVR.x32 v2, i0        || VAU.ADD.i32 v19, v19, v3   || LSU0.STO64.h v16, i_dst, 0x08
                               VAU.ADD.i32 v18, v18, v28  || LSU0.STO64.l v17, i_dst, 0x20
                               VAU.ADD.i32 v19, v19, v28  || LSU0.STO64.h v17, i_dst, 0x28
; original calculation
CMU.CMVV.i32 v25, v2        || VAU.ADD.i32 v7,  v25, 0x01 || LSU0.STO64.l v18, i_dst, 0x10
PEU.PVV32 0x3               || VAU.SUB.i32 v7,  v25, v2   || LSU0.STO64.h v18, i_dst, 0x18
                                                             LSU0.STO64.l v19, i_dst, 0x30
                               VAU.MUL.i32 v16, v0,  v7   || LSU0.STO64.h v19, i_dst, 0x38
CMU.CPIVR.x32 v2, i2
CMU.CPIVR.x32 v3, i3
CMU.CMVV.i32  v7, v22       || VAU.SHR.i32 v17, v16, 0x02
                               VAU.ADD.i32 v16, v16, v26
                               VAU.ADD.i32 v17, v17, v1
PEU.PVV32 0x4               || VAU.ADD.i32 v2,  v3,  0    || LSU0.LDO32 i2, enc, currFrame
                               VAU.ADD.i32 v17, v17, v4   || LSU0.LDO32 i3, enc, refFrame
CMU.CPIVR.x32 v3, i0        || VAU.ADD.i32 v16, v16, v2   || LSU0.STO64.l v18, i_dst, 0x50
CMU.CMZ.i32 v7              || VAU.ADD.i32 v17, v17, v2   || LSU0.STO64.h v18, i_dst, 0x58
PEU.PVV32 0x4               || VAU.SUB.i32 v16, v16, v16  || LSU0.STO64.l v19, i_dst, 0x70
PEU.PVV32 0x4               || VAU.SUB.i32 v17, v17, v17  || LSU0.STO64.h v19, i_dst, 0x78
; reference calculation
CMU.CMVV.i32 v7, v3         || VAU.ADD.i32 v7,  v7,  0x01 || LSU0.STO64.l v16, i_dst, 0x40
PEU.PVV32 0x3               || VAU.SUB.i32 v7,  v7,  v3   || LSU0.STO64.h v16, i_dst, 0x48
CMU.CPIVR.x32 v2, i2                                      || LSU0.STO64.l v17, i_dst, 0x60
CMU.CPIVR.x32 v3, i3        || VAU.MUL.i32 v16, v0,  v7   || LSU0.STO64.h v17, i_dst, 0x68
CMU.CMZ.i32 i_gop           || IAU.ADD i7, i_gop, 0
PEU.PC1C 0x1                                              || LSU0.LDO32 i7, enc, frameGOP
                               VAU.SHR.i32 v17, v16, 0x02
                               VAU.ADD.i32 v16, v16, v26
CMU.CMVV.i32  v7, v22       || VAU.ADD.i32 v17, v17, v1
PEU.PVV32 0x3               || VAU.ADD.i32 v2,  v3,  0
CMU.CPIVR.x32 v4, i_gop     || VAU.ADD.i32 v17, v17, v4
IAU.SUB.u32s i7, i7, 0x01   || VAU.ADD.i32 v16, v16, v2
CMU.CPIVR.x32 v5, i7        || VAU.ADD.i32 v17, v17, v2
PEU.PVV32 0x4               || VAU.ADD.i32 v4, v5, 0      || CMU.CMZ.i32 v25
PEU.PVV32 0x4               || VAU.SUB.i32 v16, v16, v16
PEU.PVV32 0x4               || VAU.SUB.i32 v17, v17, v17  || CMU.CMZ.i32 v4
PEU.PVV32 0x1               || VAU.SUB.i32 v16, v16, v16
PEU.PVV32 0x1               || VAU.SUB.i32 v17, v17, v17  || LSU1.LDIL i2, refLuma
                               VAU.MUL.i32 v18, v21, v6   || LSU1.LDIH i2, refLuma
CMU.CPIVR.x32 v2, i2                                      || LSU1.LDIL i3, refChromaU
CMU.VSZMBYTE v2, v2, [Z210]                               || LSU1.LDIH i3, refChromaU
CMU.CPIVR.x32 v3, i3        || VAU.SHR.i32 v19, v18, 0x02
CMU.VSZMBYTE v3, v3, [Z210] || VAU.ADD.i32 v18, v18, v2   || LSU0.STO64.l v16, i_dst, 0x80
                               VAU.ADD.i32 v19, v19, v3   || LSU0.STO64.h v16, i_dst, 0x88
BRU.JMP i30                 || VAU.ADD.i32 v18, v18, v28  || LSU0.STO64.l v17, i_dst, 0xA0
                               VAU.ADD.i32 v19, v19, v28  || LSU0.STO64.h v17, i_dst, 0xA8
                                                             LSU0.STO64.l v18, i_dst, 0x90
                                                             LSU0.STO64.h v18, i_dst, 0x98
                                                             LSU0.STO64.l v19, i_dst, 0xB0
                                                             LSU0.STO64.h v19, i_dst, 0xB8                                                                                                                     

; add nalu wrapper
.include wrap.asm
.include RateControl.asm
; add headers
.include header.asm


