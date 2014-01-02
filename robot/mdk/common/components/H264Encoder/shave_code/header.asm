; Stream headers
; This module builds stream headers PPS, SPS, SEI

.lalign
nalu_enc_param:
; encode SPS
CMU.CPIS s30, i30                                || LSU1.LDIL i30, nalu_enc_ue
IAU.FEXT.u32 i1, i10, cfg_prof_pos, cfg_prof_len || LSU1.LDIH i30, nalu_enc_ue
                    IAU.CMPI i1, 0x00  || LSU1.LDIL i3, PROFILE_MAIN
PEU.PCIX.EQ 0x20 || IAU.CMPI i1, 0x03  || LSU1.LDIL i3, PROFILE_BASE
PEU.PCIX.EQ 0x20 || IAU.XOR i0, i0, i0 || LSU1.LDIL i3, PROFILE_HIGH
BRU.SWP i30      || IAU.INCS i0, 0x01  || LSU1.LDIL i2, 0x0067
IAU.SHL i0, i0, 0x18                   || LSU1.LDIH i2, 0x1FC0
; write marker
IAU.FINS i2, i3, 0x08, 0x08 || LSU0.STI32 i0, i9
; write fixed part of SPS, nal_ref_idc=3, nal_unit_type=7, profile_idc, constraint_set0..3=0, level_idc=40
LSU0.STI32 i2, i9           || LSU1.LDIL i3, 19
; prepare second fixed part of SPS, seq_parameter_sert_id=0, log2_max_frame_num_m4=4, pic_order_cnt_type=2, num_ref_frames=1
IAU.XOR i2, i2, i2
IAU.SUB i18, i_width, 0x01  || LSU1.LDIH i2, 0x95A0
; return from ue, pic_width_in_mbs_m1 encoded
LSU0.LDIL i30, nalu_enc_ue || LSU1.LDIH i30, nalu_enc_ue
BRU.SWP i30 || IAU.SUB i18, i_height, 0x01
nop 5
; return from ue, pic_height_in_mbs_m1 encoded
LSU0.LDIL i30, nalu_enc_be || LSU1.LDIH i30, nalu_enc_be
BRU.SWP i30 || LSU1.LDIL i4, 0x09
; prepare next fixed part of SPS, frame_mbs_only_flag=1, direct_8x8_inference_flag=0, frame_cropping_flag=0
; vui_parameters_present_flag=1, vui disabled except timing
LSU1.LDIL i18, 0x121
nop 4
; encode num_units_in_tick
LSU0.LDIL i30, nalu_enc_be || LSU1.LDIH i30, nalu_enc_be
BRU.SWP i30 || LSU1.LDIL i4, 0x20
LSU1.LDIL i18, 0x3E80
LSU1.LDIH i18, 0x0000
nop 3
; encode time_scale
LSU0.LDIL i30, nalu_enc_be || LSU1.LDIH i30, nalu_enc_be
LSU0.LDO32 i18, enc, fps
BRU.SWP i30 || LSU1.LDIL i4, 0x20
nop 4
IAU.SHL i18, i18, 0x05
; encode fixed_frame_rate_flag & rest of vui bitstream restrictions
LSU0.LDIL i30, nalu_enc_be || LSU1.LDIH i30, nalu_enc_be
BRU.SWP i30 || LSU1.LDIL i4, 0x1B
LSU1.LDIL i18, 0x9135
LSU1.LDIH i18, 0x0458
nop 4
IAU.SUB i3, i3, 0x20 || SAU.SWZBYTE i2, i2, [0123]
; write last piece of stream
PEU.PC1I 0x6 || LSU0.STI32 i2, i9
; encode PPS
CMU.CPSI i30, s30 || LSU1.LDIL i0, 0x01
IAU.FEXT.u32 i1, i10, cfg_prof_pos, cfg_prof_len || LSU1.LDIL i2, 0xCE68
IAU.CMPI i1, 0x01  || LSU1.LDIH i2, 0x8038
PEU.PC1I 0x2 || IAU.FINS i2, i0, 0x0D, 0x01
IAU.FEXT.u32 i1, i10, cfg_deblock_pos, cfg_deblock_len || BRU.BRA DMA_Init
PEU.PC1I 0x1 || IAU.FINS i2, i0, 0x12, 0x01
IAU.SHL i0, i0, 0x18
IAU.INCS i9, 0x04    || LSU0.ST32 i0, i9
; write constant pps, nal_ref_idc=3, nal_unit_type=8, pic_param_set_id=0, seq_param_set_id=0,
; etc etc
IAU.INCS i9, 0x04    || LSU0.ST32 i2, i9
LSU0.STO32 i9, enc, naluBuffer



; unsigned golomb encode
.lalign
nalu_enc_ue:
IAU.INCS i18, 0x01 || LSU0.LDIL i4, golomb_length_table || LSU1.LDIH i4, golomb_length_table
IAU.ADD i4, i4, i18
BRU.BRA nalu_enc_be || LSU0.LD32.u8.u32 i4, i4
nop 5

; signed golomb encode
.lalign
nalu_enc_se:
CMU.CMZ.i32 i18 || IAU.ABS i18, i18 || LSU1.LDIL i4, golomb_length_table
IAU.SHL i18, i18, 0x01 || LSU1.LDIH i4, golomb_length_table
PEU.PC1C 0x5 || IAU.INCS i18, 0x01
IAU.ADD i4, i4, i18
BRU.BRA nalu_enc_be || LSU0.LD32.u8.u32 i4, i4
nop 5

; insert bits into stream
.lalign
nalu_enc_be:
CMU.CMII.i32 i3, i4 || IAU.SUB.u32s i5, i4, i3 || SAU.AND i1, i3, i3 || LSU1.LDIL i0, 0
PEU.PC1C 0x2 || CMU.CPII i1, i4 || IAU.SUB i0, i3, i4
CMU.VSZMBYTE i1, i0, [10DD] || IAU.SHR.u32 i6, i18, i5
CMU.CPII i3, i0 || BRU.JMP i30 || IAU.FINS i2, i6, i1
PEU.PC1C 0x5 || CMU.VSZMBYTE i2, i2, [0123] || LSU1.LDIL i3, 0x20
PEU.PC1C 0x5 || CMU.VSZMBYTE i2, i2, [ZZZZ] || LSU0.STI32 i2, i9
IAU.SUB i6, i3, i5
CMU.VSZMBYTE i5, i6, [10DD] || IAU.SUB i3, i3, i5
IAU.FINS i2, i18, i5