; mechanism to protect against multiple inclusions
.ifndef EC_H
.set EC_H 1

;// nalu slice header
;// slice header syntax
.set sh_first_mb_slice         0x00
.set sh_slice_info             0x02
.set sh_pps_id                 0x04
.set sh_redundant_pic_cnt      0x05
.set sh_num_ref_idx_l0_m1      0x06
.set sh_num_ref_idx_l1_m1      0x07
.set sh_frame_num              0x08
.set sh_idr_pic_id             0x0C
.set sh_pic_order_cnt_lsb      0x0E
.set sh_delta_pic_order_bottom 0x10
.set sh_delta_pic_order_cnt0   0x14
.set sh_delta_pic_order_cnt1   0x18
.set sh_slice_qp_delta         0x1C
.set sh_slice_qs_delta         0x1D
.set sh_slice_filtering_offset 0x1E
;// reference picture list reordering syntax
.set sh_reordering_of_pic_num_idc_l0   0x0020
.set sh_pic_num_val_l0                 0x0028
.set sh_reordering_of_pic_num_idc_l1   0x00A8
.set sh_pic_num_val_l1                 0x00B0
;// prediction weight table syntax
.set sh_default_log2_weight_denom      0x0130
.set sh_luma_weight_l0_flag            0x0134
.set sh_chroma_weight_l0_flag          0x0138
.set sh_luma_weight_offset_l0          0x013C
.set sh_chroma_weight_offset_l0        0x017C
.set sh_luma_weight_l1_flag            0x01FC
.set sh_chroma_weight_l1_flag          0x0200
.set sh_luma_weight_offset_l1          0x0204
.set sh_chroma_weight_offset_l1        0x0244
;// decoded reference picture marking syntax
.set sh_no_output_of_prior_pics_flag   0x02C4
.set sh_mmco                           0x02C8
.set sh_difference_of_pic_nums_m1      0x02D8
.set sh_long_term_frame_idx            0x0358
;// MVC modification
.set sh_mod_pic_num_idc_l0             0x03D8
.set sh_mod_pic_num_idc_l1             0x03E8
.set sizeof_sh                         0x0400

;// NAL configuration words for mb types
.set NAL_CFG_P16x16                       0x01005500
.set NAL_CFG_P16x8                        0x01015500
.set NAL_CFG_P8x16                        0x01025500
.set NAL_CFG_P8x8                         0x01035500
.set NAL_CFG_I4x4                         0x00080000
.set NAL_CFG_I16x16                       0x00000000
;// NAL int flags
.set NAL_ENC_STEP_FLAG                    0x40000000
.set NAL_ENC_SLICE_FLAG                   0x08000000

;// NAL hw
.set NAL_BASE_ADR                         0xC1600000
.set NAL_ENC_DMA_TX_CFG_ADR               0x000
.set NAL_ENC_DMA_TX_STARTADR_ADR          0x004
.set NAL_ENC_DMA_TX_LEN_ADR               0x008
.set NAL_ENC_DMA_TX_STATUS_ADR            0x00c
.set NAL_ENC_DMA_TX_FIFO_LVL_ADR          0x010
.set NAL_ENC_TX_FIFO_RD_ADR               0x014
.set NAL_ENC_DMA_RX_CFG_ADR               0x018
.set NAL_ENC_DMA_RX_STATUS_ADR            0x01c
.set NAL_DEC_DMA_TX_CFG_ADR               0x020
.set NAL_DEC_DMA_TX_STATUS_ADR            0x024
.set NAL_DEC_DMA_RX_CFG_ADR               0x028
.set NAL_DEC_DMA_RX_STARTADR_ADR          0x02c
.set NAL_DEC_DMA_RX_LEN_ADR               0x030
.set NAL_DEC_DMA_RX_STATUS_ADR            0x034
.set NAL_DEC_DMA_RX_FIFO_LVL_ADR          0x038
.set NAL_DEC_RX_FIFO_WR_ADR               0x03c
.set NAL_INT_ENABLE_ADR                   0x040
.set NAL_INT_STATUS_ADR                   0x044
.set NAL_INT_CLEAR_ADR                    0x048
.set NAL_AHB_MAX_BURST_LEN_ADR            0x04c
.set NAL_SYNTH_CFG0_ADR                   0x050
.set NAL_SYNTH_CFG1_ADR                   0x054
.set NAL_SYNTH_CFG2_ADR                   0x058
.set NAL_SYNTH_CFG3_ADR                   0x05c
.set NAL_SYNTH_STEP_CTRL                  0x060
.set NAL_SYNTH_FIXED_ADDR_ADR             0x064
.set NAL_SYNTH_VARIABLE_ADDR_ADR          0x068
.set NAL_SYNTH_HEADER_ADDR_ADR            0x06c
.set NAL_SYNTH_STATUS_ADR                 0x070
.set NAL_PARSE_CFG0_ADR                   0x074
.set NAL_PARSE_CFG1_ADR                   0x078
.set NAL_PARSE_CFG2_ADR                   0x07c
.set NAL_PARSE_CFG3_ADR                   0x080
.set NAL_PARSE_CFG4_ADR                   0x084
.set NAL_PARSE_STEP_CTRL                  0x088
.set NAL_PARSE_FIXED_ADDR_ADR             0x08c
.set NAL_PARSE_VARIABLE_ADDR_ADR          0x090
.set NAL_PARSE_HEADER_ADDR_ADR            0x094
.set NAL_PARSE_STATUS_ADR                 0x098
.set NAL_VLB_WORD0_ADR                    0x09c
.set NAL_VLB_WORD1_ADR                    0x0a0
.set NAL_BIN_COUNT_ADR                    0x0a4
.set NAL_PARSE_PPS_ID                     0x0a8
.set NAL_PARSE_TS_MODE_FLUSH_ADDR         0x0ac
.set NAL_METRIC0_ADR                      0x0b0
.set NAL_METRIC1_ADR                      0x0b4
.set NAL_METRIC2_ADR                      0x0b8
.set NAL_METRIC3_ADR                      0x0bc
.set NAL_PARSE_LWR_BOUND_ADR              0x0C0
.set NAL_PARSE_UPR_BOUND_ADR              0x0C4
.set NAL_SYNTH_LWR_BOUND_ADR              0x0C8
.set NAL_SYNTH_UPR_BOUND_ADR              0x0CC


;// vss slice header
.set vss_StreamFormat     0x00
.set vss_VersionID        0x01
.set vss_sliceType        0x02
.set vss_sliceQP          0x03
.set vss_modelNumber      0x04
.set vss_flgFieldPic      0x05
.set vss_numRefFwd        0x06
.set vss_numRefBwd        0x08
.set vss_currMB           0x0A
.set vss_picWidthMBS      0x0C
.set vss_firstMB          0x0E
.set vss_lastMB           0x10
.set vss_entropy          0x12
.set vss_bitPos           0x13
.set vss_chromaFormat     0x14
.set vss_MBAFF            0x15
.set vss_transform8x8     0x16
.set sizeof_vss_header    0x40

.set CBC_ENC_TXDONE_FLAG  0x0001
.set CBC_ENC_RXDONE_FLAG  0x0040
.set CBC_ENC_RXIDLE_FLAG  0x0080
.set CBC_ENC_SLICE_FLAG   0x1000

.set CPR_BLK_RST0_ADR                     0x80030008

.set CBC_BASE_ADR                         0xC1600000
.set CBC_MODE_CFG_ADR                     0x000
.set CBC_ENC_DMA_TX_CFG_ADR               0x004
.set CBC_ENC_DMA_TX_VSTRIDE_ADR           0x008
.set CBC_ENC_DMA_TX_LINE_WIDTH_ADR        0x00c
.set CBC_ENC_DMA_TX_STARTADR_ADR          0x010
.set CBC_ENC_DMA_TX_LEN_ADR               0x014
.set CBC_ENC_DMA_TX_STARTADR_SHAD_ADR     0x018
.set CBC_ENC_DMA_TX_LEN_SHADOW_ADR        0x01c
.set CBC_ENC_DMA_TX_STATUS_ADR            0x020
.set CBC_ENC_DMA_RX_CFG_ADR               0x024
.set CBC_ENC_DMA_RX_VSTRIDE_ADR           0x028
.set CBC_ENC_DMA_RX_LINE_WIDTH_ADR        0x02c
.set CBC_ENC_DMA_RX_STARTADR_ADR          0x030
.set CBC_ENC_DMA_RX_LEN_ADR               0x034
.set CBC_ENC_DMA_RX_STARTADR_SHAD_ADR     0x038
.set CBC_ENC_DMA_RX_LEN_SHADOW_ADR        0x03c
.set CBC_ENC_DMA_RX_STATUS_ADR            0x040
.set CBC_ENC_TX_DATA_FIFO_STATUS_ADR      0x044
.set CBC_ENC_RX_DATA_FIFO_STATUS_ADR      0x048
.set CBC_ENC_TX_DATA_INT_LEVEL_ADR        0x04c
.set CBC_ENC_RX_DATA_INT_LEVEL_ADR        0x050
.set CBC_DEC_DMA_TX_CFG_ADR               0x054
.set CBC_DEC_DMA_TX_VSTRIDE_ADR           0x058
.set CBC_DEC_DMA_TX_LINE_WIDTH_ADR        0x05c
.set CBC_DEC_DMA_TX_STARTADR_ADR          0x060
.set CBC_DEC_DMA_TX_LEN_ADR               0x064
.set CBC_DEC_DMA_TX_STARTADR_SHAD_ADR     0x068
.set CBC_DEC_DMA_TX_LEN_SHADOW_ADR        0x06c
.set CBC_DEC_DMA_TX_STATUS_ADR            0x070
.set CBC_DEC_DMA_RX_CFG_ADR               0x074
.set CBC_DEC_DMA_RX_VSTRIDE_ADR           0x078
.set CBC_DEC_DMA_RX_LINE_WIDTH_ADR        0x07c
.set CBC_DEC_DMA_RX_STARTADR_ADR          0x080
.set CBC_DEC_DMA_RX_LEN_ADR               0x084
.set CBC_DEC_DMA_RX_STARTADR_SHAD_ADR     0x088
.set CBC_DEC_DMA_RX_LEN_SHADOW_ADR        0x08c
.set CBC_DEC_DMA_RX_STATUS_ADR            0x090
.set CBC_DEC_TX_DATA_FIFO_STATUS_ADR      0x094
.set CBC_DEC_RX_DATA_FIFO_STATUS_ADR      0x098
.set CBC_DEC_TX_DATA_INT_LEVEL_ADR        0x09c
.set CBC_DEC_RX_DATA_INT_LEVEL_ADR        0x0a0
.set CBC_INT_ENABLE_ADR                   0x0a4
.set CBC_INT_STATUS_ADR                   0x0a8
.set CBC_INT_CLEAR_ADR                    0x0ac
.set CBC_LOOPBACK_ADR                     0x0b0
.set CBC_AHB_MAX_BURST_LEN_ADR            0x0b4
.set CBC_ENC_TX_DATA_FIFO_ADR             0x100
.set CBC_ENC_RX_DATA_FIFO_ADR             0x140
.set CBC_DEC_TX_DATA_FIFO_ADR             0x180
.set CBC_DEC_RX_DATA_FIFO_ADR             0x1c0

;// local register defs
.set s_initFlag      s24
.set s_ctrlPtr       s25
.set s_frameNum      s26
.set s_updateRC      s27
.set i_width         i16
.set i_height        i17
.set i_qp            i19
.set i_gop           i20
.set i_frame         i21
.set i_row           i22
.set i_ctrl          i23

.set v_mv            v30

.endif