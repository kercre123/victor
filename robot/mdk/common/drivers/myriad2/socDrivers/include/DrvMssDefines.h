#ifndef _DRV_MSS_DEF_H_
#define _DRV_MSS_DEF_H_
               
// MSS_MIPI_CFG_ADR             ================================================================
#define D_DRV_MSS_MIPI_CFG_EN_0  (1<<0)
#define D_DRV_MSS_MIPI_CFG_EN_1  (1<<1)
#define D_DRV_MSS_MIPI_CFG_EN_2  (1<<2)
#define D_DRV_MSS_MIPI_CFG_EN_3  (1<<3)
#define D_DRV_MSS_MIPI_CFG_EN_4  (1<<4)
#define D_DRV_MSS_MIPI_CFG_EN_5  (1<<5)
// bit 6-7 reserved
#define D_DRV_MSS_MIPI_CFG_C0_RX  (0<<8)    // always is rx
#define D_DRV_MSS_MIPI_CFG_C1_RX  (0<<9)    // always is rx
#define D_DRV_MSS_MIPI_CFG_C2_RX  (0<<10)   // always is rx

#define D_DRV_MSS_MIPI_CFG_C3_RX  (0<<11)
#define D_DRV_MSS_MIPI_CFG_C3_TX  (1<<11)

#define D_DRV_MSS_MIPI_CFG_C4_RX  (0<<12)   // always is rx

#define D_DRV_MSS_MIPI_CFG_C5_RX  (0<<13)
#define D_DRV_MSS_MIPI_CFG_C5_TX  (1<<13)




// MSS_MIPI_CIF_CFG_ADR                 ================================================================
#define D_DRV_MSS_MIPI_RX0_TO_CIF0    (1<<0)
#define D_DRV_MSS_MIPI_RX1_TO_CIF0    (1<<1)
// bits 8:2 reserved 
#define D_DRV_MSS_MIPI_RX3_TO_CIF1    (1<<9)
// bit 10 reserved
#define D_DRV_MSS_MIPI_RX5_TO_CIF1    (1<<11)


// MSS_LCD_MIPI_CFG_ADR                ================================================================
// bits 2:0 res
#define D_DRV_MSS_LCD_TO_MIPI_TX3    (1<<3)
// bit 4 res
#define D_DRV_MSS_LCD_TO_MIPI_TX5    (1<<5)


// MSS_MIPI_SIPP_CFG_ADR               ================================================================
#define D_DRV_MSS_MIPI_RX0_TO_SIPP0   (1<<0)
#define D_DRV_MSS_MIPI_RX1_TO_SIPP0   (1<<1)
// bits 7:2 reserved
#define D_DRV_MSS_MIPI_RX2_TO_SIPP1   (1<<8)
// bits 14:9 reserved
#define D_DRV_MSS_MIPI_RX3_TO_SIPP2   (1<<15)
// bit 16 reserved
#define D_DRV_MSS_MIPI_RX5_TO_SIPP2   (1<<17)
// bits 21:18 reserved
#define D_DRV_MSS_MIPI_RX4_TO_SIPP3   (1<<22)
// bit 23 reserved

// MSS_SIPP_MIPI_CFG_ADR               ================================================================
//bits 2:0 reserved
#define D_DRV_MSS_SIPP0_TO_MIPI_TX3   (1<<3)
// bits 10:4 reserved
#define D_DRV_MSS_SIPP1_TO_MIPI_TX5   (1<<11)


// MSS_GPIO_CFG_ADR                        ================================================================
#define D_DRV_MSS_CIF_TO_GPIO   (1<<0)
#define D_DRV_MSS_CIF_TO_MIPI   (0<<0)

#define D_DRV_MSS_LCD_TO_GPIO   (1<<1)
#define D_DRV_MSS_LCD_TO_MIPI   (0<<1)


// MSS_LOOPBACK_CFG_ADR                ================================================================
#define D_DRV_MSS_LOOPBACK_SIPPTX0_SIPPRX1   (1<<0)
#define D_DRV_MSS_LOOPBACK_SIPPTX1_SIPPRX3   (1<<1)
#define D_DRV_MSS_LOOPBACK_LCD_CIF0          (1<<2)


// MSS_AMC_RD_PRI_ADR                    ================================================================
// MSS_AMC_WR_PRI_ADR                   ================================================================
// MSS_AMC_DBG_TPRV_ADR               ================================================================
// MSS_AMC_DBG_TPWV_ADR              ================================================================
// MSS_AMC_DBG_SPAV_ADR               ================================================================


// MSS_CLK_CTRL_ADR                         ================================================================
// &
// MSS_RSTN_CTRL_ADR                      ================================================================
#define D_DRV_MSS_CLK_APB_SALVE              (1<<0)
#define D_DRV_MSS_CLK_APB_CTRL               (1<<1)
#define D_DRV_MSS_CLK_RTL_AHB_CTRL           (1<<2)
#define D_DRV_MSS_CLK_RTL_AHB2AHB            (1<<3)
#define D_DRV_MSS_CLK_RTL                    (1<<4)  // real time leon
#define D_DRV_MSS_CLK_RTL_DSU                (1<<5)
#define D_DRV_MSS_CLK_RTL_L2                 (1<<6)
#define D_DRV_MSS_CLK_ICB                    (1<<7)
#define D_DRV_MSS_CLK_MXI_CTRL               (1<<8)
#define D_DRV_MSS_CLK_MXI_DEFAULT_SLAVE      (1<<9)
#define D_DRV_MSS_CLK_AXI_MON                (1<<10)
#define D_DRV_MSS_CLK_H264_NAL               (1<<11)
#define D_DRV_MSS_CLK_SEBI                   (1<<12)
#define D_DRV_MSS_CLK_MIPI                   (1<<13)
#define D_DRV_MSS_CLK_CIF0                   (1<<14)
#define D_DRV_MSS_CLK_CIF1                   (1<<15)
#define D_DRV_MSS_CLK_LCD                    (1<<16)
#define D_DRV_MSS_CLK_AMC                    (1<<17)
#define D_DRV_MSS_CLK_SIPP                   (1<<18)


// MSS_SIPP_CLK_CTRL_ADR                ================================================================
#define D_DRV_MSS_SC_RAW                 (1<<0)   
#define D_DRV_MSS_SC_LSC                 (1<<1)
#define D_DRV_MSS_SC_DEBAYER             (1<<2)
#define D_DRV_MSS_SC_CHROMA_DEN          (1<<3)
#define D_DRV_MSS_SC_LUMA_DEN            (1<<4)  // real time leon
#define D_DRV_MSS_SC_SHARP               (1<<5)
#define D_DRV_MSS_SC_POLYPH_SCALE        (1<<6)
#define D_DRV_MSS_SC_MEDIAN              (1<<7)
#define D_DRV_MSS_SC_LOOKUP_TBL          (1<<8)
#define D_DRV_MSS_SC_EDGE_OPERATOR       (1<<9)
#define D_DRV_MSS_SC_CONVOLUTION_KERN    (1<<10)
#define D_DRV_MSS_SC_HARRIS              (1<<11)
#define D_DRV_MSS_SC_COLOR_COMBI         (1<<12)
#define D_DRV_MSS_SC_DYNAMIC_SHIFT       (1<<13)
#define D_DRV_MSS_SC_MIPI_TX0            (1<<14)
#define D_DRV_MSS_SC_MIPI_TX1            (1<<15)
#define D_DRV_MSS_SC_MIPI_RX0            (1<<16)
#define D_DRV_MSS_SC_MIPI_RX1            (1<<17)
#define D_DRV_MSS_SC_MIPI_RX2            (1<<18)
#define D_DRV_MSS_SC_MIPI_RX3            (1<<19)






#endif
