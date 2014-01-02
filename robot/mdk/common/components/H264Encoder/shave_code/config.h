;/* mechanism to protect against multiple inclusions */
.ifndef CONFIG_H
.set CONFIG_H 1

;/* flags to control build variants */
.set STREAM_TEST        0
.set ASIC_VERSION       1
.set BASARABIA_PAMANT_ROMANESC 0

;/* project specific defines */
.set MAX_WIDTH          512
.set MAX_CODEC          4
.set RC_WINDOW          4

;/* default memory layout for shaves */
.set CODEC0_EN          1
.set CODEC1_EN          1
.set CODEC2_EN          0
.set CODEC3_EN          1
.set CODEC4_EN          1
.set CODEC5_EN          0
.set CODEC6_EN          0
.set CODEC7_EN          0
.set ENTROPY_ID         2

.set CODEC0_CODE        0x10000000
.set CODEC0_DATA        0x10004000
.set CODEC1_CODE        0x10020000
.set CODEC1_DATA        0x10024000
.set CODEC2_CODE        0x10040000
.set CODEC2_DATA        0x10044000
.set CODEC3_CODE        0x10060000
.set CODEC3_DATA        0x10064000
.set CODEC4_CODE        0x10080000
.set CODEC4_DATA        0x10084000
.set CODEC5_CODE        0x100A0000
.set CODEC5_DATA        0x100A4000
.set CODEC6_CODE        0x100C0000
.set CODEC6_DATA        0x100C4000
.set CODEC7_CODE        0x100E0000
.set CODEC7_DATA        0x100E4000
.set ENTROPY_CODE       (0x10000000+ENTROPY_ID*0x20000)
.set ENTROPY_DATA       (0x10004000+ENTROPY_ID*0x20000)

;/* global data structure, contains encoder config data structures located on entropy codec */
.set cmdId               0x00
.set cmdCfg              0x04
.set width               0x08
.set height              0x0C
.set frameNum            0x10
.set frameQp             0x14
.set fps                 0x18
.set tgtRate             0x1C
.set avgRate             0x20
.set frameGOP            0x24
.set naluBuffer          0x28
.set naluNext            0x2C
.set origFrame           0x30
.set nextFrame           0x34
.set currFrame           0x38
.set refFrame            0x3C

;/* cmdCfg bitfield details */
.set cfg_enc_pos         0x00
.set cfg_enc_len         0x08
.set cfg_filter_pos      0x08
.set cfg_filter_len      0x08
.set cfg_ec_pos          0x10
.set cfg_ec_len          0x08
.set cfg_prof_pos        0x18
.set cfg_prof_len        0x02
.set cfg_RC_pos          0x1C
.set cfg_RC_len          0x01
.set cfg_AQ_pos          0x1D
.set cfg_AQ_len          0x01
.set cfg_deblock_pos     0x1E
.set cfg_deblock_len     0x01
.set cfg_3D_pos          0x1F
.set cfg_3D_len          0x01

;/* local data structure to control each encoder */
.set rowStart            0x00
.set rowEnd              0x04
.set qpy                 0x08
.set frameType           0x0C
.set picWidth            0x10
.set picHeight           0x14
.set coeffBuffer         0x18
.set entropyFIFO         0x1C

;/* MBInfo data structure, contains all data required for generic entropy encoder */
.set  mb_cfg             0x00
.set  mb_cfg2            0x04
.set  mb_type            0x08
.set  mb_cbp             0x09
.set  mb_subType         0x0A
.set  mb_posx            0x0C
.set  mb_posy            0x0D
.set  mb_idx             0x0E
.set  mb_icPred          0x10
.set  mb_qpy             0x11
.set  mb_qpu             0x12
.set  mb_qpv             0x13
.set  mb_iPred           0x14
.set  mb_refIdxL0        0x1C
.set  mb_refIdxL1        0x20
.set  mb_coeffAddr       0x24
.set  mb_coeffCount      0x28
.set  mb_lumaDCMap       0x2A
.set  mb_lumaACMap       0x2C
.set  mb_chromaDCMap     0x4C
.set  mb_chromaCount     0x4E
.set  mb_chromaACMap     0x50
.set  mb_BS              0x60
.set  mb_mvdL0           0x70
.set  mb_mvdL1           0xB0
.set  sizeof_MBInfo      0xF0
.set  sizeof_MBRow       (sizeof_MBInfo*(MAX_WIDTH>>4))
.set  baseCoeffBuffer    0x10008000
.set  sizeof_CoeffBuffer 0x6000

;//MB_CFG
;//[31:30] mb struct coding ("00" frame, "10" top, "11" bottom )
;//[29:26] reserved
;//[25:24] mb_type ("00" I, "01" P, "10" B, "11" skipped )
;//[22]    IntraPCM
;//[21]    B_direct
;//[18]    transform8x8 flag
;//[17:16] partition size ("00" 16x16, "01" 16x8, "10" 8x16, "11" 8x8 )
;//[15:14] prediction 3
;//[13:12] prediction 2
;//[11:10] prediction 1
;//[09:08] prediction 0 ( prediction mode "00" intra, "01" List0, "10" List1, "11" Bi-Pred )
;//[07:06] sub-partition 3
;//[05:04] sub-partition 2
;//[03:02] sub-partition 1
;//[01:00] sub-partition 0 (sub part size "00" 8x8, "01" 8x4, "10" 4x8, "11" 4x4 )
.set MB_CFG_SUBMB        0x00000000  /* 8x8 sub mb size supported only */
.set MB_CFG_PPRED        0x00005500  /* P slices support only List0 pred */
.set MB_CFG_IPRED        0x00000000
.set MB_CFG_INTER_16x16  0x00000000
.set MB_CFG_INTER_16x8   0x00010000
.set MB_CFG_INTER_8x16   0x00020000
.set MB_CFG_INTER_8x8    0x00030000
.set MB_CFG_I            0x00000000
.set MB_CFG_P            0x01000000
.set MB_CFG_B            0x02000000
.set MB_CFG_SKIP         0x03000000

;/* profile_idc */
.set PROFILE_BASE       66
.set PROFILE_MAIN       77
.set PROFILE_HIGH       100

;/* intra 4x4 prediction modes */
.set I_PRED_4x4_V       0
.set I_PRED_4x4_H       1
.set I_PRED_4x4_DC      2
.set I_PRED_4x4_DDL     3
.set I_PRED_4x4_DDR     4
.set I_PRED_4x4_VR      5
.set I_PRED_4x4_HDN     6
.set I_PRED_4x4_VL      7
.set I_PRED_4x4_HUP     8

;/* slice types */
.set SLICE_TYPE_P       0
.set SLICE_TYPE_B       1
.set SLICE_TYPE_I       2
.set SLICE_TYPE_SP      3
.set SLICE_TYPE_SI      4

;/* prediction modes */
.set P_L0_16x16         0
.set P_L0_L0_16x8       1
.set P_L0_L0_8x16       2
.set P_8x8              3
.set P_8x8ref0          4
.set I_4x4              5
.set I_16x16_0_0_0      6
.set I_16x16_1_0_0      7
.set I_16x16_2_0_0      8
.set I_16x16_3_0_0      9
.set I_16x16_0_1_0     10
.set I_16x16_1_1_0     11
.set I_16x16_2_1_0     12
.set I_16x16_3_1_0     13
.set I_16x16_0_2_0     14
.set I_16x16_1_2_0     15
.set I_16x16_2_2_0     16
.set I_16x16_3_2_0     17
.set I_16x16_0_0_1     18
.set I_16x16_1_0_1     19
.set I_16x16_2_0_1     20
.set I_16x16_3_0_1     21
.set I_16x16_0_1_1     22
.set I_16x16_1_1_1     23
.set I_16x16_2_1_1     24
.set I_16x16_3_1_1     25
.set I_16x16_0_2_1     26
.set I_16x16_1_2_1     27
.set I_16x16_2_2_1     28
.set I_16x16_3_2_1     29
.set I_PCM             30

;//DMA channel configuration
.set DMA_ENABLE		0x01
.set DMA_SRC_USE_STRIDE	0x02
.set DMA_DST_USE_STRIDE	0x04
.set DMA_TX_FIFO_ADDR	0x08
.set DMA_RX_FIFO_ADDR	0x10
.set DMA_FORCE_SRC_AXI	0x20

;/* DMA reg addresses  */
.set DMA_BUSY_MASK     0x0008
.set DMA_TASK_REQ      0x4004
.set DMA_TASK_STAT     0x4008
.set DMA0_CFG          0x4100
.set DMA0_SRC_ADDR     0x4104
.set DMA0_DST_ADDR     0x4108
.set DMA0_SIZE         0x410C
.set DMA0_LINE_WIDTH   0x4110
.set DMA0_LINE_STRIDE  0x4114
.set DMA1_CFG          0x4200
.set DMA1_SRC_ADDR     0x4204
.set DMA1_DST_ADDR     0x4208
.set DMA1_SIZE         0x420C
.set DMA1_LINE_WIDTH   0x4210
.set DMA1_LINE_STRIDE  0x4214
.set DMA2_CFG          0x4300
.set DMA2_SRC_ADDR     0x4304
.set DMA2_DST_ADDR     0x4308
.set DMA2_SIZE         0x430C
.set DMA2_LINE_WIDTH   0x4310
.set DMA2_LINE_STRIDE  0x4314
.set DMA3_CFG          0x4400
.set DMA3_SRC_ADDR     0x4404
.set DMA3_DST_ADDR     0x4408
.set DMA3_SIZE         0x440C
.set DMA3_LINE_WIDTH   0x4410
.set DMA3_LINE_STRIDE  0x4414

;/* TRF regs */
.set P_GPR             0x1F
.set P_GPI             0x1E
.set P_SVID            0x1C
.set P_CFG             0x1B
.set G_GALOIS          0x15
.set B_SREPS           0x14
.set C_CMU1            0x13
.set C_CMU0            0x12
.set C_CSI             0x11
.set F_AE              0x10
.set I_AE              0x0F
.set L_ENDIAN          0x0E
.set V_ACC3            0x0D
.set V_ACC2            0x0C
.set V_ACC1            0x0B
.set V_ACC0            0x0A
.set S_ACC             0x09
.set V_STATE           0x08
.set S_STATE           0x07
.set I_STATE           0x06
.set B_RFB             0x05
.set B_STATE           0x04
.set B_MREPS           0x03
.set B_LEND            0x02
.set B_LBEG            0x01
.set B_IP              0x00

.set SLICE_LOCAL       0x4
.set SHAVE0_FIFO       0xf000
.set SHAVE1_FIFO       0xf100
.set SHAVE2_FIFO       0xf200
.set SHAVE3_FIFO       0xf300
.set SHAVE4_FIFO       0xf400
.set SHAVE5_FIFO       0xf500
.set SHAVE6_FIFO       0xf600
.set SHAVE7_FIFO       0xf700
.set FIFO_RD_FRONT     0x5050

;// svu base addresses for DCU access
.set SHAVE_0_BASE_ADR    0x80140000
.set SHAVE_1_BASE_ADR    0x80150000
.set SHAVE_2_BASE_ADR    0x80160000
.set SHAVE_3_BASE_ADR    0x80170000
.set SHAVE_4_BASE_ADR    0x80180000
.set SHAVE_5_BASE_ADR    0x80190000
.set SHAVE_6_BASE_ADR    0x801A0000
.set SHAVE_7_BASE_ADR    0x801B0000

.set SVU_SLICE_RST           0x0000
.set SVU_SLICE_CTRL          0x0004
.set SVU_SLICE_DBG           0x0008
.set SVU_SLICE_OFS_WIN_A     0x0010
.set SVU_SLICE_OFS_WIN_B     0x0014
.set SVU_SLICE_OFS_WIN_C     0x0018
.set SVU_SLICE_OFS_WIN_D     0x001C
.set SVU_SLICE_WIN_CPC       0x0020
.set SVU_SLICE_NW_CPC        0x0024

.set SVU_CACHE_CTRL          0x7000
.set SVU_CACHE_SVU_CTRL      0x7004
.set SVU_CACHE_CNT_IN        0x7008
.set SVU_CACHE_CNT_OUT       0x700C
.set SVU_CACHE_CTRL_TXN_ADDR 0x7010
.set SVU_CACHE_CTRL_TXN_TYPE 0x7014
.set SVU_CACHE_STATUS        0x7018

.set SVU_DCR_ADDR            0x1800
.set SVU_DSR_ADDR            0x1804
.set SVU_PC0_ADDR            0x1830
.set SVU_PCC0_ADDR           0x1834
.set SVU_PC1_ADDR            0x1838
.set SVU_PCC1_ADDR           0x183C

;// uninitialised data structure start addresses
;// double buffer for original Luma
.set origLuma            0x1C000000
;// double buffer for reference Chroma
.set refChromaU          0x1C004000
.set refChromaV          0x1C006000
;// double buffer for reference Luma
.set refLuma             0x1C008000
;// define the offset from base for control data structures
.set enc_offset          0x00010000
.set enc_data            (0x1C000000+enc_offset)
;// double buffer for original Chroma
.set origChromaU         0x1C012000
.set origChromaV         0x1C013000
;// mbinfo storage
.set MBBuffer            0x1C014000

.set s_DMAStat           s16
.set s_nrCodecs          s17
;// 'static' registers
.set i_MBInfo            i24
.set i_Coeff             i25
; // generic dst address
.set i_dst               i26
;// generic source address
.set i_src               i27
;// MB position
.set i_posx              i28
.set i_posy              i29
.set i_jumpAddr          i30
;// encoder data structure start addr, probably global addr
.set enc                 i31
;// command vector for svu sync
.set v_cmd               v31
.set v_cmd.0             v31.0  // y coordinate
.set v_cmd.1             v31.1  // mbinfo buffer idx
.set v_cmd.2             v31.2  // orig buffer idx on encoder, stream buffer idx on ec
.set v_cmd.3             v31.3  // ref buffer idx on encoder

.endif
