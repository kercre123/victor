; nalu wrapper
; This module feeds MBInfo into nalu hw
.lalign
nalu_enc_current:
nalu_enc1:
; decode current row for decoder1
CMU.CPVI.x32 i4, v22.0                                           || LSU1.LDIL i0, sizeof_MBRow
CMU.CPVI.x32 i2, v26.1       || IAU.MUL i0, i0, i4               || LSU1.LDIL i1, (sizeof_CoeffBuffer>>2)
CMU.CPVI.x32 i3, v27.1       || IAU.MUL i1, i1, i4               || LSU1.LDIL i4, MBBuffer
CMU.CPVI.x32 i_MBInfo, v28.1 || SAU.SUB.i32 i_ctrl, i3, i2       || LSU1.LDIH i4, MBBuffer
CMU.VSZMBYTE i4, i4, [Z210]  || IAU.ADD i_MBInfo, i_MBInfo, i0   || LSU1.LDIL i_Coeff, (baseCoeffBuffer+sizeof_CoeffBuffer)
CMU.CPSI i0, s_nrCodecs      || IAU.ADD i_MBInfo, i_MBInfo, i4   || LSU1.LDIH i_Coeff, (baseCoeffBuffer+sizeof_CoeffBuffer)>>16
CMU.CPIV.x32 v31.2, i30      || IAU.CMPI i0, 0x01                || LSU0.LDO32 i18, enc, cmdCfg
PEU.PC1I 0x2                 || BRU.BRA nalu_enc_mb
CMU.CPTI i0, P_SVID          || IAU.SHR.u32 i_ctrl, i_ctrl, 0x04 || LSU1.LDIL i14, NAL_BASE_ADR
CMU.CPIS s_ctrlPtr, i_ctrl   || IAU.SHL  i0, i0, 0x11            || LSU1.LDIH i14, NAL_BASE_ADR
IAU.ADD i_Coeff, i_Coeff, i1 || LSU1.LDIL i30, nalu_enc2         
IAU.ADD i_Coeff, i_Coeff, i0 || LSU1.LDIH i30, nalu_enc2
CMU.CPIV.x32 v31.3, i30      
nalu_enc2:
; decode current row for decoder2
CMU.CPVI.x32 i4, v22.0                                           || LSU1.LDIL i0, sizeof_MBRow
CMU.CPVI.x32 i2, v26.2       || IAU.MUL i0, i0, i4               || LSU1.LDIL i1, (sizeof_CoeffBuffer>>2)
CMU.CPVI.x32 i3, v27.2       || IAU.MUL i1, i1, i4               || LSU1.LDIL i4, MBBuffer
CMU.CPVI.x32 i_MBInfo, v28.2 || SAU.SUB.i32 i_ctrl, i3, i2       || LSU1.LDIH i4, MBBuffer
CMU.VSZMBYTE i4, i4, [Z210]  || IAU.ADD i_MBInfo, i_MBInfo, i0   || LSU1.LDIL i_Coeff, (baseCoeffBuffer+2*sizeof_CoeffBuffer)
CMU.CPSI i0, s_nrCodecs      || IAU.ADD i_MBInfo, i_MBInfo, i4   || LSU1.LDIH i_Coeff, (baseCoeffBuffer+2*sizeof_CoeffBuffer)>>16
                                IAU.CMPI i0, 0x02                || LSU0.LDO32 i18, enc, cmdCfg
PEU.PC1I 0x2                 || BRU.BRA nalu_enc_mb
CMU.CPTI i0, P_SVID          || IAU.SHR.u32 i_ctrl, i_ctrl, 0x04 || LSU1.LDIL i14, NAL_BASE_ADR
CMU.CPIS s_ctrlPtr, i_ctrl   || IAU.SHL  i0, i0, 0x11            || LSU1.LDIH i14, NAL_BASE_ADR
IAU.ADD i_Coeff, i_Coeff, i1 || LSU1.LDIL i30, nalu_enc3         
IAU.ADD i_Coeff, i_Coeff, i0 || LSU1.LDIH i30, nalu_enc3
CMU.CPIV.x32 v31.3, i30      
nalu_enc3:
; decode current row for decoder3
CMU.CPVI.x32 i4, v22.0                                           || LSU1.LDIL i0, sizeof_MBRow
CMU.CPVI.x32 i2, v26.3       || IAU.MUL i0, i0, i4               || LSU1.LDIL i1, (sizeof_CoeffBuffer>>2)
CMU.CPVI.x32 i3, v27.3       || IAU.MUL i1, i1, i4               || LSU1.LDIL i4, MBBuffer
CMU.CPVI.x32 i_MBInfo, v28.3 || SAU.SUB.i32 i_ctrl, i3, i2       || LSU1.LDIH i4, MBBuffer
CMU.VSZMBYTE i4, i4, [Z210]  || IAU.ADD i_MBInfo, i_MBInfo, i0   || LSU1.LDIL i_Coeff, (baseCoeffBuffer+3*sizeof_CoeffBuffer)
CMU.CPSI i0, s_nrCodecs      || IAU.ADD i_MBInfo, i_MBInfo, i4   || LSU1.LDIH i_Coeff, (baseCoeffBuffer+3*sizeof_CoeffBuffer)>>16
                                IAU.CMPI i0, 0x03                || LSU0.LDO32 i18, enc, cmdCfg
PEU.PC1I 0x2                 || BRU.BRA nalu_enc_mb
CMU.CPTI i0, P_SVID          || IAU.SHR.u32 i_ctrl, i_ctrl, 0x04 || LSU1.LDIL i14, NAL_BASE_ADR
CMU.CPIS s_ctrlPtr, i_ctrl   || IAU.SHL  i0, i0, 0x11            || LSU1.LDIH i14, NAL_BASE_ADR
IAU.ADD i_Coeff, i_Coeff, i1 || LSU1.LDIL i30, nalu_enc_hw       
IAU.ADD i_Coeff, i_Coeff, i0 || LSU1.LDIH i30, nalu_enc_hw
CMU.CPIV.x32 v31.3, i30      

.lalign
nalu_enc_hw:
CMU.CPVI.x32 i30, v31.2 || IAU.SUB i0, i_height, 0x01 || LSU1.LDIL i14, NAL_BASE_ADR
CMU.CMII.i32 i0, i_row || LSU1.LDIH i14, NAL_BASE_ADR
.if ASIC_VERSION==1
PEU.PC1C 0x6 || BRU.JMP i30
PEU.PC1C 0x1 || LSU0.LDO32 i2, i14, NAL_INT_STATUS_ADR
LSU1.LDIL i1, NAL_ENC_SLICE_FLAG
LSU1.LDIH i1, NAL_ENC_SLICE_FLAG
IAU.XOR i5, i5, i5
IAU.ADD i5, i5, 0x01
IAU.SHL i5, i5, 0x12
IAU.AND i0, i1, i2
PEU.PC1I 0x1 || BRU.BRA nalu_enc_hw 
PEU.PC1I 0x6 || LSU0.LDO32 i10, enc, naluBuffer
PEU.PC1I 0x6 || LSU0.LDO32 i4,  i14, NAL_ENC_DMA_TX_STATUS_ADR    || LSU1.LDIL i2, (0x977053<<5)
PEU.PC1I 0x6 || LSU0.LDO32 i3,  i14, NAL_ENC_DMA_TX_FIFO_LVL_ADR  || LSU1.LDIH i2, (0x977053<<5)>>16
PEU.PC1I 0x6 || LSU0.STO32 i2,  i14, NAL_ENC_DMA_TX_CFG_ADR       || LSU1.LDIL i1,  0x00C0
PEU.PC1I 0x6 || LSU0.STO32 i1,  i14, NAL_SYNTH_CFG0_ADR
BRU.RPIM 0xA
CMU.CPII i18, i10 || IAU.SUB i5,  i5,  i4   || LSU1.LDIL  i1,  0x01
                     IAU.ADD i10, i10, i5   || LSU0.LDO32 i9, enc, naluNext
                     IAU.ADD i10, i10, i3   || LSU0.STO32 i10, i14, NAL_ENC_DMA_TX_STARTADR_ADR
                     IAU.OR  i2,  i2,  i1   || LSU0.STO32 i3,  i14, NAL_ENC_DMA_TX_LEN_ADR
                     IAU.ADD i2,  i2,  0x02 || LSU0.STO32 i2,  i14, NAL_ENC_DMA_TX_CFG_ADR
nalu_enc_flush:
BRU.RPIM 0x0A
LSU0.LDO32 i2, i14, NAL_ENC_DMA_TX_CFG_ADR || LSU1.LDIL i1, 0x02
IAU.AND i0, i1, i2
PEU.PC1I 0x6 || BRU.BRA nalu_enc_flush
PEU.PC1I 0x6 || BRU.RPIM 0x0A
PEU.PC1I 0x1 || LSU1.LDIL i1,  0x0100
PEU.PC1I 0x1 || LSU0.STO32 i1,  i14, NAL_SYNTH_CFG0_ADR
PEU.PC1I 0x1 || BRU.RPIM 0x0A
PEU.PC1I 0x1 || BRU.RPIM 0x0A
.if STREAM_TEST==1
LSU0.STO32 i10, enc, naluBuffer
BRU.BRA RC_Frame
nop 5
.else
; preemptive int trigger mechanism to permit chaining
BRU.SWIC 0 || IAU.SUB.u32s i18, i10, i18 || LSU0.STO32 i10, enc, cmdId
; swih will exit for last frame
;PEU.PC1C 0x6 || BRU.SWIC 0 
nalu_enc_waitNalu:
CMU.CMZ.i32 i9 || IAU.XOR i10, i10, i10 || LSU0.LDO32 i9, enc, naluNext
; stall if nalu not ready
PEU.PC1C 0x1 || BRU.BRA nalu_enc_waitNalu
PEU.PC1C 0x6 || BRU.BRA RC_Frame
PEU.PC1C 0x1 || BRU.RPIM 0x14
PEU.PC1C 0x1 || BRU.RPIM 0x14
PEU.PC1C 0x6 || LSU0.STO32 i9,  enc, naluBuffer
PEU.PC1C 0x6 || LSU0.STO32 i10, enc, naluNext
nop
.endif
.else
BRU.JMP i30
nop 5
.endif

.lalign
nalu_enc_next:
; end of row mv candidate selection trick
IAU.SHL i1, i_width, 0x03 || LSU0.LDIL i2, mvH || LSU1.LDIH i2, mvH
IAU.ADD i2, i2, i1
LSU0.LDO64.l v16, i2, -12
BRU.RPIM 0x05
VAU.SWZWORD v16, v16, [0101]
LSU0.ST64.l v16, i2

; decode next row for decoder0, decoder0 is always present :-)
CMU.CPVI.x32 i4, v22.3                                           || LSU1.LDIL i0, sizeof_MBRow
CMU.CPVI.x32 i2, v26.0       || IAU.MUL i0, i0, i4               || LSU1.LDIL i1, (sizeof_CoeffBuffer>>2)
CMU.CPVI.x32 i3, v27.0       || IAU.MUL i1, i1, i4               || LSU1.LDIL i4, MBBuffer
CMU.CPVI.x32 i_MBInfo, v28.0 || SAU.SUB.i32 i_ctrl, i3, i2       || LSU1.LDIH i4, MBBuffer
CMU.VSZMBYTE i4, i4, [Z210]  || IAU.ADD i_MBInfo, i_MBInfo, i0   || LSU1.LDIL i_Coeff, baseCoeffBuffer
                                IAU.ADD i_MBInfo, i_MBInfo, i4   || LSU1.LDIH i_Coeff, baseCoeffBuffer
CMU.CMZ.i32 i_row            || LSU0.LDO32 i18, enc, cmdCfg
PEU.PC1C 0x2                 || BRU.BRA nalu_enc_mb
CMU.CPTI i0, P_SVID          || IAU.SHR.u32 i_ctrl, i_ctrl, 0x04 || LSU1.LDIL i14, NAL_BASE_ADR
CMU.CPIS s_ctrlPtr, i_ctrl   || IAU.SHL  i0, i0, 0x11            || LSU1.LDIH i14, NAL_BASE_ADR
IAU.ADD i_Coeff, i_Coeff, i1
IAU.ADD i_Coeff, i_Coeff, i0 
CMU.CPIV.x32 v31.3, i30      

; reprogram NAL synth at start of frame
CMU.CPSI i9, s_frameNum || IAU.FEXT.u32 i13, i18, cfg_prof_pos, cfg_prof_len  || LSU0.LDO32 i10, enc, naluBuffer
CMU.CMZ.i32 i_gop || IAU.CMPI i13, 0x01   || SAU.SUB.i32 i3, i_qp, 0x0E || LSU1.LDIL i0, 0x0100
PEU.PC1I 0x2 || LSU1.LDIL i0, 0x8100
PEU.PCCX.EQ  0x02           || IAU.ADD i1, i0, SLICE_TYPE_I   || SAU.SUB.i32 i9, i9, i9                  || LSU1.LDIL i_dst, slice_header
PEU.PCCX.NEQ 0x01           || IAU.ADD i1, i0, SLICE_TYPE_P                                              || LSU1.LDIH i_dst, slice_header
CMU.VSZMBYTE i2, i9, [ZZZ0] || IAU.ADD i9, i9, 0x01           || LSU0.STO16 i1, i_dst, sh_slice_info     || LSU1.LDIL i14, NAL_BASE_ADR
CMU.CPIS s_frameNum, i9     || IAU.MUL i11, i_width, i_height || LSU0.STO32 i2, i_dst, sh_frame_num      || LSU1.LDIH i14, NAL_BASE_ADR
CMU.VSZMBYTE s_initFlag, s_initFlag, [ZZZZ]                   || LSU0.STO8  i3, i_dst, sh_slice_qp_delta || LSU1.LDIL i0,  0x00C0
.if ASIC_VERSION==1
                               IAU.SUB.u32s i3, i13, 0x01     || LSU0.LDIL i13, 0x01                              || LSU1.LDIL i1,  0x0002
PEU.PCIX.NEQ 0x20           || IAU.SHL  i13, i13, 0x18                                                            || LSU1.LDIL i1,  0x000A
                               IAU.SHL  i11, i11, 0x13                                                            || LSU1.LDIH i1,  0x1A00
                               IAU.OR   i0,  i0,  i11         || LSU0.STI32 i13, i10
                               IAU.SHL  i12, i_width, 0x04                                                        || LSU1.LDIL i2,  0x0900
                               IAU.OR   i1,  i1,  i12         || LSU0.STO32 i10, enc, naluBuffer                  || LSU1.LDIH i2,  0x0018
                               IAU.XOR  i11, i11, i11         || LSU0.STO32 i1,  i14, NAL_SYNTH_CFG1_ADR          || LSU1.LDIL i3,  0x0007
CMU.CMZ.i32 i_gop           || IAU.ADD  i11, i11, 0x01        || LSU0.STO32 i2,  i14, NAL_SYNTH_CFG2_ADR          || LSU1.LDIH i3,  0x1000
PEU.PCCX.EQ  0x20           || IAU.SHL  i11, i11, 0x12                                                            || LSU1.LDIL i3,  0x0017
                                                                 LSU0.STO32 i3,  i14, NAL_SYNTH_CFG3_ADR          || LSU1.LDIL i12, ((0x977053<<5)|0x01)
CMU.VSZMBYTE i3, i_dst, [Z210]                                || LSU0.STO32 i10, i14, NAL_ENC_DMA_TX_STARTADR_ADR || LSU1.LDIH i12, ((0x977053<<5)|0x01)>>16
                                                                 LSU0.STO32 i11, i14, NAL_ENC_DMA_TX_LEN_ADR      || LSU1.LDIL i13, ((0xFAC688<<5)|0x01)
                                                                 LSU0.STO32 i12, i14, NAL_ENC_DMA_TX_CFG_ADR      || LSU1.LDIH i13, ((0xFAC688<<5)|0x01)>>16
CMU.CPTI i1, P_SVID                                           || LSU0.STO32 i13, i14, NAL_ENC_DMA_RX_CFG_ADR      || LSU1.LDIL i2,  0x4000
                               IAU.SHL  i1, i1, 0x11          || LSU0.STO32 i0,  i14, NAL_SYNTH_CFG0_ADR          || LSU1.LDIH i2,  0x1000
                               IAU.ADD  i2, i2, i1                                                                || LSU1.LDIL i12, 0x04
                               IAU.ADD  i2, i2, i3            || LSU0.STO32 i12, i14, NAL_AHB_MAX_BURST_LEN_ADR   || LSU1.LDIL i1, (NAL_ENC_STEP_FLAG|NAL_ENC_SLICE_FLAG)
                               IAU.XOR  i0, i0, i0            || LSU0.STO32 i2,  i14, NAL_SYNTH_HEADER_ADDR_ADR   || LSU1.LDIH i1, (NAL_ENC_STEP_FLAG|NAL_ENC_SLICE_FLAG)>>16
                               IAU.NOT  i0, i0                || LSU0.STO32 i0,  i14, NAL_SYNTH_LWR_BOUND_ADR     ;|| LSU0.STO32 i1,  i14  NAL_INT_ENABLE_ADR
							                                     LSU0.STO32 i0,  i14, NAL_SYNTH_UPR_BOUND_ADR  
                                                                 LSU0.STO32 i0,  i14, NAL_INT_CLEAR_ADR
.endif

; prepare data for a single MB
.lalign
nalu_enc_mb:
IAU.XOR i18, i18, i18          || LSU0.LDO8     i18, i_MBInfo,  mb_type        || LSU1.LDIL i0, nalu_enc_i16x16
IAU.XOR i15, i15, i15          || LSU0.LDO8     i15, i_MBInfo,  mb_cbp         || LSU1.LDIH i0, nalu_enc_i16x16
IAU.XOR i_posx, i_posx, i_posx || LSU0.LDO8  i_posx, i_MBInfo,  mb_posx        || LSU1.LDIL i1, nalu_enc_i4x4
IAU.XOR i_posy, i_posy, i_posy || LSU0.LDO8  i_posy, i_MBInfo,  mb_posy        || LSU1.LDIH i1, nalu_enc_i4x4
                                  LSU0.LDO64.l v_mv, i_MBInfo,  mb_mvdL0       || LSU1.LDIL i2, nalu_enc_inter
                                  LSU0.LDO64.h v_mv, i_MBInfo, (mb_mvdL0+0x08) || LSU1.LDIH i2, nalu_enc_inter
; mb type logic
                         CMU.CPII i30, i0            || IAU.CMPI i18, I_4x4            || LSU1.LDIL i_src, mvH
PEU.PCIX.EQ  0x04     || CMU.CPII i30, i1            || IAU.CMPI i18, P_8x8            || LSU1.LDIH i_src, mvH
PEU.PCIX.EQ  0x04     || CMU.CPII i30, i2            || IAU.SHL  i3, i_posx, 0x03      
                         BRU.JMP i30                 || IAU.XOR i4, i4, i4             || LSU1.LDIL i30, nalu_enc_write                     
                         CMU.CMZ.i32 i_posx          || IAU.XOR i5, i5, i5             || LSU1.LDIH i30, nalu_enc_write
PEU.PCCX.NEQ 0x10     || CMU.CMZ.i32 i_posy          || IAU.ADD i_src, i_src, i3       || LSU0.LDA64 i4, 0, mvV
PEU.PCCX.NEQ 0x10     || CMU.VSZMBYTE i8, i8, [ZZZZ] || IAU.XOR i6, i6, i6             || LSU0.LD64  i6, i_src
PEU.PCCX.NEQ 0x10     || CMU.VSZMBYTE i9, i9, [ZZZZ] || IAU.XOR i7, i7, i7             || LSU0.LDO64 i8, i_src, 0x08
nop 5

.lalign
nalu_enc_write:
.if ASIC_VERSION==1
LSU0.LDIL i14, NAL_BASE_ADR || LSU1.LDIH i14, NAL_BASE_ADR
LSU0.LDO32 i2, i14, NAL_INT_STATUS_ADR
LSU1.LDIL i1, NAL_ENC_STEP_FLAG 
LSU1.LDIH i1, NAL_ENC_STEP_FLAG
BRU.RPIM 0x14
CMU.CMZ.f32 s_initFlag || IAU.AND i0, i1, i2
PEU.PCC0I.AND 0x6, 0x1 || BRU.BRA nalu_enc_write 
PEU.PCC0I.OR  0x1, 0x6 || LSU0.LDO32 i_Coeff, i_MBInfo, mb_coeffAddr
PEU.PC1C 0x1 || LSU0.LDO32 i2,  i14, NAL_SYNTH_CFG0_ADR
CMU.CPVI.x32 i30, v31.3         || LSU1.LDIL i1, 0xFFFF
CMU.CPII i_src, i_MBInfo        || LSU1.LDIH i1, 0xFFFF
nop
; setup encoding
                                    LSU0.STO32 i_MBInfo, i14, NAL_SYNTH_FIXED_ADDR_ADR
BRU.BRA DMA_Schedule             || LSU0.STO32 i_Coeff,  i14, NAL_SYNTH_VARIABLE_ADDR_ADR || LSU1.LDIL i0, 0x07
IAU.SUB i_ctrl, i_ctrl, 0x01     || LSU0.STO32 i0,    i14, NAL_SYNTH_STEP_CTRL         || LSU1.LDIL i3, 0x07
PEU.PCIX.NEQ 0x20                || LSU0.STO32 i1,    i14, NAL_INT_CLEAR_ADR           || LSU1.LDIL i30, nalu_enc_mb
PEU.PCIX.NEQ 0x20                || IAU.OR i2, i2, i3                                  || LSU1.LDIH i30, nalu_enc_mb
IAU.INCS i_MBInfo, sizeof_MBInfo || SAU.ADD.f32 s_initFlag, s_initFlag, 1.0
PEU.PC1C 0x1                     || LSU0.STO32 i2,    i14, NAL_SYNTH_CFG0_ADR

.else
CMU.CPVI.x32 i30, v31.3
BRU.BRA DMA_Schedule             || LSU1.LDIL i0, 0x07
IAU.SUB i_ctrl, i_ctrl, 0x01     || LSU1.LDIL i3, 0x03
PEU.PCIX.NEQ 0x20                || LSU1.LDIL i30, nalu_enc_mb
PEU.PCIX.NEQ 0x20                || LSU1.LDIH i30, nalu_enc_mb
SAU.ADD.f32 s_initFlag, s_initFlag, 1.0
IAU.INCS i_MBInfo, sizeof_MBInfo
.endif

; encode as i4x4
.lalign
nalu_enc_i4x4:
; not yet implemented
BRU.SWIH 0x1F

; encode as i16x16
.lalign
nalu_enc_i16x16:
CMU.CPII i1, i18        || IAU.FEXT.u32 i0, i15, 0, 0x04
PEU.PC1I 0x2            || IAU.INCS i1, 0x0C
BRU.JMP i30             || CMU.CMZ.i32 i_gop || VAU.XOR v16, v16, v16
PEU.PC1C 0x1            || IAU.SUB  i1, i1, I_4x4
                           IAU.FEXT.u32 i0, i15, 0x04, 0x02 || LSU0.ST64.l  v16, i_src  || LSU1.LDIL i10, NAL_CFG_I16x16
                           IAU.SHL i0, i0, 0x02             || LSU0.STA64.l v16, 0, mvV || LSU1.LDIH i10, NAL_CFG_I16x16
                           IAU.ADD i1, i1, i0               || LSU0.STO32 i10, i_MBInfo, mb_cfg
                                                               LSU0.STO8  i1,  i_MBInfo, mb_type

.lalign
nalu_enc_inter:
              CMU.CMVV.u32 v_mv, v_mv || VAU.SWZWORD v16, v_mv, [0123] || LSU1.SWZC4WORD [1331]
PEU.ANDACC || CMU.CMVV.u32 v16, v16   || IAU.XOR i0, i0, i0            || LSU1.SWZC4WORD [1331]
PEU.PC2C.AND 0x1  || CMU.CPIT C_CMU0, i0 || BRU.BRA nalu_enc_p16x16
PEU.PCXC 0x1, 0x2 || BRU.BRA nalu_enc_p16x8
PEU.PCXC 0x1, 0x3 || BRU.BRA nalu_enc_p8x16
nop
CMU.CPZV v16, 0x1          || LSU1.LDIL i1, 0x01
CMU.CPVV.i16.i8s v16, v_mv || IAU.SHL i1, i1, 0x10
CMU.CPIVR.x32 v18, i1      || LSU1.LDIL i10, 0x01
VAU.ADD.i16 v16, v18, v16  || LSU1.SWZV8 [73727170]
; otherwise encode as P8x8
nalu_enc_p8x8:
; DeriveMV_8x8
CMU.MAX.i8 i1, i6, i7 || IAU.ADD i0, i6, i7
CMU.MIN.i8 i2, i6, i7 || IAU.ADD i0, i0, i4
CMU.MIN.i8 i1, i1, i4 || IAU.FEXT.u32 i3, i0, 0x10, 0x08
CMU.CMII.i32 i3, i10  || SAU.SUM.i32 i6, v16, 0x1
PEU.PC1C 0x2 || CMU.MAX.i8 i2, i2, i1
nop
PEU.PC1C 0x5 || CMU.CPII i2, i0
                         SAU.SUB.i8 i3, i6, i2
nop
                         IAU.FEXT.i32 i4, i3, 0x00, 0x08
                         IAU.FEXT.i32 i4, i3, 0x08, 0x08  || LSU0.STO16 i4, i_MBInfo, (mb_mvdL0+0x00)
CMU.MAX.i8 i1, i7, i8 || IAU.ADD i0, i7, i8               || LSU0.STO16 i4, i_MBInfo, (mb_mvdL0+0x02)
CMU.MIN.i8 i2, i7, i8 || IAU.ADD i0, i0, i6
CMU.MIN.i8 i1, i1, i6 || IAU.FEXT.u32 i3, i0, 0x10, 0x08
CMU.CMII.i32 i3, i10  || SAU.SUM.i32 i7, v16, 0x2
PEU.PC1C 0x2 || CMU.MAX.i8 i2, i2, i1
nop
PEU.PC1C 0x5 || CMU.CPII i2, i0
CMU.MAX.i8 i1, i6, i7 || SAU.SUB.i8 i3, i7, i2
CMU.MIN.i8 i2, i6, i7
CMU.MIN.i8 i1, i1, i5 || IAU.FEXT.i32 i4, i3, 0x00, 0x08
                         IAU.FEXT.i32 i4, i3, 0x08, 0x08  || LSU0.STO16 i4, i_MBInfo, (mb_mvdL0+0x08)
CMU.MAX.i8 i2, i2, i1 || SAU.SUM.i32  i5, v16, 0x4        || LSU0.STO16 i4, i_MBInfo, (mb_mvdL0+0x0A)
nop
CMU.MAX.i8 i1, i6, i7 || SAU.SUB.i8 i3, i5, i2            || LSU1.LDIL  i10, NAL_CFG_P8x8
CMU.MIN.i8 i2, i6, i7                                     || LSU1.LDIH  i10, NAL_CFG_P8x8
CMU.MIN.i8 i1, i1, i5 || IAU.FEXT.i32 i4, i3, 0x00, 0x08  || LSU0.STO32 i10, i_MBInfo, mb_cfg
                         IAU.FEXT.i32 i4, i3, 0x08, 0x08  || LSU0.STO16 i4,  i_MBInfo, (mb_mvdL0+0x20)
CMU.MAX.i8 i2, i2, i1 || SAU.SUM.i32  i5, v16, 0x8        || LSU0.STO16 i4,  i_MBInfo, (mb_mvdL0+0x22)
BRU.JMP i30
                         SAU.SUB.i8 i3, i5, i2
CMU.VSZMWORD v17, v16, [3120]                             || LSU0.ST64.h  v16, i_src
                         IAU.FEXT.i32 i4, i3, 0x00, 0x08  || LSU0.STA64.h v17, 0, mvV
                         IAU.FEXT.i32 i4, i3, 0x08, 0x08  || LSU0.STO16 i4, i_MBInfo, (mb_mvdL0+0x28)
                                                             LSU0.STO16 i4, i_MBInfo, (mb_mvdL0+0x2A)

.lalign
nalu_enc_p16x16:
CMU.CPIVR.x32 v18, i1      || IAU.SUB i7, i_width, 0x01 || LSU1.LDIL i10, 0x01
VAU.ADD.i16 v16, v18, v16  || IAU.SUB i7, i7, i_posx    || LSU1.SWZV8 [73727170]
PEU.PC1I 0x1               || IAU.ADD i8, i9, 0
; DeriveMV_16x16
CMU.MAX.i8 i1, i6, i4 || IAU.ADD i0, i6, i4
CMU.MIN.i8 i2, i6, i4 || IAU.ADD i0, i0, i8
CMU.MIN.i8 i1, i1, i8 || IAU.FEXT.u32 i3, i0, 0x10, 0x08 || LSU1.LDIL i7, 0x01
CMU.CMII.i32 i3, i10  || SAU.SUM.i32 i7, v16, 0x1        || LSU1.LDIL i10, P_L0_16x16
PEU.PC1C 0x2 || CMU.MAX.i8 i2, i2, i1
nop
PEU.PC1C 0x5 || CMU.CPII i2, i0
BRU.JMP i30                      || SAU.SUB.i8 i3, i7, i2    || LSU0.STO8  i10, i_MBInfo, mb_type
CMU.VSZMWORD v17, v16, [3120]    || LSU0.ST64.h  v16, i_src  || LSU1.LDIL  i10, NAL_CFG_P16x16
                                    LSU0.STA64.h v17, 0, mvV || LSU1.LDIH  i10, NAL_CFG_P16x16
IAU.FEXT.i32 i4, i3, 0x00, 0x08  || LSU0.STO32 i10, i_MBInfo, mb_cfg
IAU.FEXT.i32 i4, i3, 0x08, 0x08  || LSU0.STO16 i4, i_MBInfo, (mb_mvdL0+0x00)
                                    LSU0.STO16 i4, i_MBInfo, (mb_mvdL0+0x02)
                                   
.lalign
nalu_enc_p16x8:
VAU.ADD.i16 v16, v18, v16  || LSU1.SWZV8 [73727170]
; DeriveMV_16x8
CMU.MAX.i8 i1, i6, i8 || IAU.ADD i0, i6, i8
CMU.MIN.i8 i2, i6, i8 || IAU.ADD i0, i0, i4
CMU.MIN.i8 i1, i1, i4 || IAU.FEXT.u32 i3, i0, 0x10, 0x08
CMU.CMII.i32 i3, i10  || IAU.FEXT.u32 i3, i6, 0x10, 0x08 || SAU.SUM.i32 i7, v16, 0x1
PEU.PC1C 0x2 || CMU.MAX.i8 i2, i2, i1
nop
PEU.PC1C 0x5 || CMU.CPII i2, i0
PEU.PCIX.NEQ 0x04 || CMU.CPII i2, i6
SAU.SUB.i8 i3, i7, i2                        || LSU1.LDIL  i6, P_L0_L0_16x8
                                                LSU0.STO8  i6, i_MBInfo, mb_type
IAU.FEXT.i32 i6, i3, 0x00, 0x08
IAU.FEXT.i32 i6, i3, 0x08, 0x08              || LSU0.STO16 i6, i_MBInfo, (mb_mvdL0+0x00)
CMU.MAX.i8 i1, i4, i5 || IAU.ADD i0, i4, i5  || LSU0.STO16 i6, i_MBInfo, (mb_mvdL0+0x02)
CMU.MIN.i8 i2, i4, i5 || IAU.ADD i0, i0, i7
CMU.MIN.i8 i1, i1, i7 || IAU.FEXT.u32 i3, i0, 0x10, 0x08
CMU.CMII.i32 i3, i10  || IAU.FEXT.u32 i3, i5, 0x10, 0x08 || SAU.SUM.i32 i7, v16, 0x4
PEU.PC1C 0x2 || CMU.MAX.i8 i2, i2, i1
nop
PEU.PC1C 0x5 || CMU.CPII i2, i0
PEU.PCIX.NEQ 0x04 || CMU.CPII i2, i5         || LSU1.LDIL i10, NAL_CFG_P16x8
BRU.JMP i30                                  || LSU1.LDIH i10, NAL_CFG_P16x8
SAU.SUB.i8 i3, i7, i2            || LSU0.STO32 i10, i_MBInfo, mb_cfg
CMU.VSZMWORD v17, v16, [3120]    || LSU0.ST64.h  v16, i_src
IAU.FEXT.i32 i6, i3, 0x00, 0x08  || LSU0.STA64.h v17, 0, mvV
IAU.FEXT.i32 i6, i3, 0x08, 0x08  || LSU0.STO16 i6,  i_MBInfo, (mb_mvdL0+0x20)
                                    LSU0.STO16 i6,  i_MBInfo, (mb_mvdL0+0x22)

.lalign
nalu_enc_p8x16:
; DeriveMV_8x16
CMU.MAX.i8 i1, i6, i7 || IAU.ADD i0, i6, i7 
CMU.MIN.i8 i2, i6, i7 || IAU.ADD i0, i0, i4
CMU.MIN.i8 i1, i1, i4 || IAU.FEXT.u32 i3, i0, 0x10, 0x08
CMU.CMII.i32 i3, i10  || IAU.FEXT.u32 i3, i4, 0x10, 0x08 || SAU.SUM.i32 i6, v16, 0x1
PEU.PC1C 0x2 || CMU.MAX.i8 i2, i2, i1
nop
PEU.PC1C 0x5 || CMU.CPII i2, i0
PEU.PCIX.NEQ 0x04 || CMU.CPII i2, i4
SAU.SUB.i8 i3, i6, i2                       || LSU1.LDIL  i4, P_L0_L0_8x16
                                               LSU0.STO8  i4, i_MBInfo, mb_type
IAU.FEXT.i32 i4, i3, 0x00, 0x08
IAU.FEXT.i32 i4, i3, 0x08, 0x08             || LSU0.STO16 i4, i_MBInfo, (mb_mvdL0+0x00)
CMU.MAX.i8 i1, i7, i8 || IAU.ADD i0, i7, i8 || LSU0.STO16 i4, i_MBInfo, (mb_mvdL0+0x02)
CMU.MIN.i8 i2, i7, i8 || IAU.ADD i0, i0, i6
CMU.MIN.i8 i1, i1, i6 || IAU.FEXT.u32 i3, i0, 0x10, 0x08
CMU.CMII.i32 i3, i10  || IAU.FEXT.u32 i3, i8, 0x10, 0x08 || SAU.SUM.i32 i6, v16, 0x2
PEU.PC1C 0x2 || CMU.MAX.i8 i2, i2, i1
nop
PEU.PC1C 0x5 || CMU.CPII i2, i0
PEU.PCIX.NEQ 0x04 || CMU.CPII i2, i8        || LSU1.LDIL i10, NAL_CFG_P8x16
BRU.JMP i30                                 || LSU1.LDIH i10, NAL_CFG_P8x16
SAU.SUB.i8 i3, i6, i2                       || LSU0.STO32 i10, i_MBInfo, mb_cfg
CMU.VSZMWORD v17, v16, [3120]               || LSU0.ST64.h  v16, i_src
IAU.FEXT.i32 i4, i3, 0x00, 0x08             || LSU0.STA64.h v17, 0, mvV
IAU.FEXT.i32 i4, i3, 0x08, 0x08             || LSU0.STO16 i4, i_MBInfo, (mb_mvdL0+0x08)
                                               LSU0.STO16 i4, i_MBInfo, (mb_mvdL0+0x0A)

