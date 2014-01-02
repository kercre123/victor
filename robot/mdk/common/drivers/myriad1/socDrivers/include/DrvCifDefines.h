#ifndef _BRINGUP_SABRE_CIF_DEF_H_
#define _BRINGUP_SABRE_CIF_DEF_H_

typedef enum
{
    CAMERA_1,
    CAMERA_2,
} tyCIFDevice;

/* ******************************************************************************************
                                   CIF control register defines
******************************************************************************************  */
// bit 0
#define D_CIF_CTRL_ENABLE        (0x0001)
// bit 1
#define D_CIF_CTRL_RGB_BAYER_EN  (0x0002)
// bit 2
#define D_CIF_CTRL_FRAME_RATE_EN (0x0004)
// bit 3
#define D_CIF_CTRL_WINDOW_EN     (0x0008)
//bit 4:5
#define D_CIF_CTRL_STATISTICS_DISABLED    (0x0000)
#define D_CIF_CTRL_STATISTICS_FULL        (0x0010)
#define D_CIF_CTRL_STATISTICS_WINDOW      (0x0020)
// bit 6:7
#define D_CIF_CTRL_SUBSAMPLE_DISABLED     (0x0000)
#define D_CIF_CTRL_SUBSAMPLE_H            (0x0040)   // subsample horizontal
#define D_CIF_CTRL_SUBSAMPLE_V            (0x0080)   // subsample vertical
#define D_CIF_CTRL_SUBSAMPLE_HV           (0x00C0)   // subsample horizontal and vertical
// bit 8
#define D_CIF_CTRL_COLOR_CORECTION        (0x00100)           // color corection enable
// bit 9
#define D_CIF_CTRL_CSC_ENABLE             (0x00200)           // color space conversion
// bit 10
// bit 11:12
#define D_CIF_CTRL_PREV_SUBSAMPLE_DISABLED     (0x0000)
#define D_CIF_CTRL_PREV_SUBSAMPLE_H            (0x0800)   // subsample horizontal preview image
#define D_CIF_CTRL_PREV_SUBSAMPLE_V            (0x1000)   // subsample vertical preview image
#define D_CIF_CTRL_PREV_SUBSAMPLE_HV           (0x1800)   // subsample horizontal and vertical preview image
// bit 13
#define D_CIF_CTRL_TIM_GEN_EN  (0x2000)   // enable timing generators
// bit 14
#define D_CIF_CTRL_PWM0 (0x4000)
// bit 15
#define D_CIF_CTRL_PWM1 (0x8000)
// bit 16
#define D_CIF_CTRL_PWM2 (0x10000)

/* ******************************************************************************************
                                   CIF input interface register defines (table 4)
******************************************************************************************  */
//bit 0
#define D_CIF_IN_PIX_CLK_SENSOR     (0x0000)
#define D_CIF_IN_PIX_CLK_CPR        (0x0001)
// bit 1
#define D_CIF_IN_HSINK_ON_BLANK_YES (0x0002)  // default is NO HSINC on blank period
// bit 2
#define D_CIF_IN_SINC_ITUR_BT656   (0x0004)  // use embeded sincronization, default is sinc using external vsinc and hsinc signals
// bit 3
#define D_CIF_IN_SINC_DRIVED_BY_SABRE (0x0008) // sabre drives the hsinc and vsinc signals

/* ******************************************************************************************
                                CIF input format register defines (table 5)
******************************************************************************************  */
// bit 0:2
#define D_CIF_INFORM_FORMAT_RGB_BAYER (0x0000)
#define D_CIF_INFORM_FORMAT_YUV444    (0x0001)
#define D_CIF_INFORM_FORMAT_YUV422    (0x0002)
#define D_CIF_INFORM_FORMAT_RGB888    (0x0004)
#define D_CIF_INFORM_FORMAT_RGB656    (0x0005)
#define D_CIF_INFORM_FORMAT_RGB666    (0x0006)
#define D_CIF_INFORM_FORMAT_RGB444    (0x0007)
// bit 3:4
#define D_CIF_INFORM_BAYER_GRGR_BGBG   (0x0000) // naming rule : first line GRGR, second line BGBG
#define D_CIF_INFORM_BAYER_RGRG_GBGB   (0x0008)
#define D_CIF_INFORM_BAYER_GBGB_RGRG   (0x0010)
#define D_CIF_INFORM_BAYER_BGBG_GRGR   (0x0018)
// bit 5:7
#define D_CIF_INFORM_DEMOSAIC_7_0      (0x0000)
#define D_CIF_INFORM_DEMOSAIC_8_1      (0x0020)
#define D_CIF_INFORM_DEMOSAIC_9_2      (0x0040)
#define D_CIF_INFORM_DEMOSAIC_10_3     (0x0060)
#define D_CIF_INFORM_DEMOSAIC_11_4     (0x0080)
// bit 8:9
#define D_CIF_INFORM_DAT_SIZE_8        (0x0000) // data bus size
#define D_CIF_INFORM_DAT_SIZE_16       (0x0100)
#define D_CIF_INFORM_DAT_SIZE_24       (0x0200)
//bit 10
#define D_CIF_INFORM_CR_BEFORE_CB      (0x0400) // cr before cb, default is cb before cr
//bit 11
#define D_CIF_INFORM_Y_AFTER_CBCR      (0x0800) // y after cbcr, default is y before cbcr
//bit 12
#define D_CIF_INFORM_BGR  (0x1000)  // sets BGR order, default is RGB
//bit 13
#define D_CIF_INFORM_UV_OFFSET (0x2000)  //add 128 to U and V

/* ******************************************************************************************
                                CIF output format register defines (table 8)
******************************************************************************************  */
//bit 0
#define D_CIF_OUTF_MAP_LUT       (0x0000)
#define D_CIF_OUTF_BAYER         (0x0001)   

//bit 1:2
#define D_CIF_OUTF_FORMAT_444    (0x0000)
#define D_CIF_OUTF_FORMAT_422    (0x0002)
#define D_CIF_OUTF_FORMAT_420    (0x0004)
#define D_CIF_OUTF_FORMAT_411    (0x0006)
//bit 3:4
#define D_CIF_OUTF_CHROMA_SUB_CO_SITE_EVEN    (0x000)
#define D_CIF_OUTF_CHROMA_SUB_CO_SITE_ODD     (0x008)
#define D_CIF_OUTF_CHROMA_SUB_CO_SITE_CENTER  (0x018)
//bit 5
#define D_CIF_OUTF_STORAGE_PLANAR (0x0020)  // default is linear storage
// bit 6
#define D_CIF_OUTF_Y_AFTER_CBCR   (0x0040) // default is Y before CBCR
// bit 7
#define D_CIF_OUTF_CR_FIRST       (0x0080) // default is  CB first
// bit 8:10
#define D_CIF_OUTF_FLIP_V         (0x0100)
#define D_CIF_OUTF_FLIP_H         (0x0200)
#define D_CIF_OUTF_ROT_90R        (0x0300)
#define D_CIF_OUTF_ROT_90L        (0x0400)
#define D_CIF_OUTF_ROT_180        (0x0500)
// bit 11
#define D_CIF_OUTF_YCBCR_24BIT_PACK  (0x0800) // output will be packed on 24 bits, default is 32 bits (8 bits are not used)


/* ******************************************************************************************
                       CIF preview format register defines
******************************************************************************************  */

//bit 0:2
#define D_CIF_PREV_SEL_COL_MAP_LUT   (0x0000)
#define D_CIF_PREV_SEL_CSC           (0x0001)
#define D_CIF_PREV_SEL_CRC           (0x0002)
#define D_CIF_PREV_SEL_WIN_SUBS      (0x0003)
#define D_CIF_PREV_SEL_RAW_BAYER     (0x0004)
//bit 3:5
#define D_CIF_PREV_OUTF_RGB888     (0x0000)
#define D_CIF_PREV_OUTF_RGB565     (0x0008)
#define D_CIF_PREV_OUTF_RGB555     (0x0010)
#define D_CIF_PREV_OUTF_RGB_BY     (0x0018)
#define D_CIF_PREV_OUTF_RGB332     (0x0020)
#define D_CIF_PREV_OUTF_RGBA8888   (0x0028)
#define D_CIF_PREV_OUTF_RGBA5551   (0x0030)
#define D_CIF_PREV_OUTF_RGBA4444   (0x0038)
//bit 6:13
#define D_CIF_PREV_ALPHA_MASK     (0x3FC0)
//bit 14
#define D_CIF_PREV_RGB888DOWN_FUNC (0x4000)   // for downsampling from RGB 888 to other formats use conversion functions, default is truncation
//bit 15
#define D_CIF_PREV_BGR_ORDER       (0x8000)   // bgr order, default is RGB
// bit 16:18
#define D_CIF_PREV_FLIP_V         (0x10000)
#define D_CIF_PREV_FLIP_H         (0x20000)
#define D_CIF_PREV_ROT_90R        (0x30000)
#define D_CIF_PREV_ROT_90L        (0x40000)
#define D_CIF_PREV_ROT_180        (0x50000)

/* ******************************************************************************************
                       CIF DMA config register defines
******************************************************************************************  */
// bit 0
#define D_CIF_DMA_ENABLE     (0x0001)
//bit 1
#define D_CIF_DMA_ACTIVITY_MASK  (0x0002)   // mask bit for activity
// bit 2:3
#define D_CIF_DMA_AUTO_RESTART_DISABLED    (0x0000)
#define D_CIF_DMA_AUTO_RESTART_ONCE_SHADOW (0x0004)   // at the end of the transfer start another one with the shadow registers
#define D_CIF_DMA_AUTO_RESTART_CONTINUOUS  (0x0008)   // continuosus transfer mode using main register settings
#define D_CIF_DMA_AUTO_RESTART_PING_PONG   (0x000C)   // continuous ping pong mode
// bit 4
#define D_CIF_DMA_FIFO_ADRESSING (0x0010) // do not increment address after each burst
// bit 5:9
#define D_CIF_DMA_AXI_BURST_MASK (0x03E0) // mask to set the maximum baxi burst
#define D_CIF_DMA_AXI_BURST_1     (0x020)  // default axi burst is 1
#define D_CIF_DMA_AXI_BURST_2     (0x040)
#define D_CIF_DMA_AXI_BURST_3     (0x060)
#define D_CIF_DMA_AXI_BURST_4     (0x080)
#define D_CIF_DMA_AXI_BURST_5     (0x0A0)
#define D_CIF_DMA_AXI_BURST_6     (0x0C0)
#define D_CIF_DMA_AXI_BURST_7     (0x0E0)
#define D_CIF_DMA_AXI_BURST_8     (0x100)
#define D_CIF_DMA_AXI_BURST_9     (0x120)
#define D_CIF_DMA_AXI_BURST_10    (0x140)
#define D_CIF_DMA_AXI_BURST_11    (0x160)
#define D_CIF_DMA_AXI_BURST_12    (0x180)
#define D_CIF_DMA_AXI_BURST_13    (0x1A0)
#define D_CIF_DMA_AXI_BURST_14    (0x1C0)
#define D_CIF_DMA_AXI_BURST_15    (0x1E0)
#define D_CIF_DMA_AXI_BURST_16    (0x200)
//bit 10
#define D_CIF_DMA_V_STRIDE_EN (0x0400) // vertical stride enable

/* ******************************************************************************************
                       CIF PWM ctrl register defines
******************************************************************************************  */
// bit 0:1
#define D_CIF_PWM_TRIGG_HSINC_1 (0x0000)   // trigger on rising edge of hsinc
#define D_CIF_PWM_TRIGG_HSINC_0 (0x0001)   // trigger on falling edge of hsinc
#define D_CIF_PWM_TRIGG_VSINC_1 (0x0002)   // trigger on rising edge of vsinc
#define D_CIF_PWM_TRIGG_VSINC_0 (0x0003)   // trigger on falling edge of vsinc
// bit 2
#define D_CIF_PWM_REPEAT_EN     (0x0004)  // enable repeat
// bit 3
#define D_CIF_PWM_OUT_INVERT    (0x0008)

/* ******************************************************************************************
                       CIF int defines
******************************************************************************************  */
// bit 0
#define D_CIF_INT_EOL             (0x0001)
#define D_CIF_INT_EOF             (0x0002)
#define D_CIF_INT_DMA0_DONE       (0x0004)
#define D_CIF_INT_DMA0_IDLE       (0x0008)
// bit 4
#define D_CIF_INT_DMA0_OVER       (0x0010)
#define D_CIF_INT_DMA0_UNDER      (0x0020)
#define D_CIF_INT_DMA0_F_FULL     (0x0040)
#define D_CIF_INT_DMA0_F_EMPTY    (0x0080)
// bit 8
#define D_CIF_INT_DMA1_DONE       (0x0100)
#define D_CIF_INT_DMA1_IDLE       (0x0200)
#define D_CIF_INT_DMA1_OVER       (0x0400)
#define D_CIF_INT_DMA1_UNDER      (0x0800)
// bit 12
#define D_CIF_INT_DMA1_F_FULL     (0x1000)
#define D_CIF_INT_DMA1_F_EMPTY    (0x2000)
#define D_CIF_INT_DMA2_DONE       (0x4000)
#define D_CIF_INT_DMA2_IDLE       (0x8000)
// bit 16
#define D_CIF_INT_DMA2_OVER       (0x00010000)
#define D_CIF_INT_DMA2_UNDER      (0x00020000)
#define D_CIF_INT_DMA2_F_FULL     (0x00040000)
#define D_CIF_INT_DMA2_F_EMPTY    (0x00080000)
// bit 20
#define D_CIF_INT_DMA3_DONE       (0x00100000)
#define D_CIF_INT_DMA3_IDLE       (0x00200000)
#define D_CIF_INT_DMA3_OVER       (0x00400000)
#define D_CIF_INT_DMA3_UNDER      (0x00800000)
// bit 24
#define D_CIF_INT_DMA3_F_FULL     (0x01000000)
#define D_CIF_INT_DMA3_F_EMPTY    (0x02000000)



#endif // _BRINGUP_SABRE_CIF_DEF_H_


