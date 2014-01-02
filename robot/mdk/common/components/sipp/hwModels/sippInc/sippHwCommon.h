// -----------------------------------------------------------------------------
// Copyright (C) 2013 Movidius Ltd. All rights reserved
//
// Company          : Movidius
// Author           : Razvan Delibasa (razvan.delibasa@movidius.com)
// Description      : SIPP Accelerator HW model common definitions
//
// Mainly APB register map related
// -----------------------------------------------------------------------------

#ifndef __SIPP_HWCOMMON_H__
#define __SIPP_HWCOMMON_H__

// SIPP Filter/buffer IDs
// Following are filter and input/output buffer IDs
#define SIPP_RAW_ID        0    /* RAW filter */
#define SIPP_LSC_ID        1    /* Lens shading filter */
#define SIPP_DBYR_ID       2    /* Debayer */
#define SIPP_CHROMA_ID     3    /* Chroma denoise */
#define SIPP_LUMA_ID       4    /* Luma denoise in/out */
#define SIPP_SHARPEN_ID    5    /* Sharpening */
#define SIPP_UPFIRDN_ID    6    /* Polyphase FIR */
#define SIPP_MED_ID        7    /* Median */
#define SIPP_LUT_ID        8    /* Look-up table */
#define SIPP_EDGE_OP_ID    9    /* Edge operator */
#define SIPP_CONV_ID       10   /* Programmable convolution */
#define SIPP_HARRIS_ID     11   /* Harris corners */
#define SIPP_CC_ID         12   /* Colour combination */
                                /* 13 - Reserved */
#define SIPP_DBYR_PPM_ID   20   /* Debayer post-processing median */

// Following are filter and input buffer IDs
#define SIPP_MIPI_TX0_ID   14   /* MIPI Tx[0] filter (input buffer only) */
#define SIPP_MIPI_TX1_ID   15   /* MIPI Tx[1] filter (input buffer only) */

// Following are input buffer IDs
#define SIPP_LSC_GM_ID     16   /* Lens shading correction gain mesh buffer */
#define SIPP_CHROMA_REF_ID 17   /* Chroma denoise reference in */
#define SIPP_LUMA_REF_ID   18   /* Luma denoise reference in */
#define SIPP_LUT_LOAD_ID   19   /* LUT loader in */
#define SIPP_CC_CHROMA_ID  21   /* Colour combination - chroma in */

// Following are output buffer IDs
                                /* 14 - Reserved */
#define SIPP_STATS_ID      15   /* RAW stats out */

// Following are filter and output buffer IDs
#define SIPP_MIPI_RX0_ID   16   /* MIPI Rx[0] filter (output buffer only) */
#define SIPP_MIPI_RX1_ID   17   /* MIPI Rx[1] filter (output buffer only) */
#define SIPP_MIPI_RX2_ID   18   /* MIPI Rx[2] filter (output buffer only) */
#define SIPP_MIPI_RX3_ID   19   /* MIPI Rx[3] filter (output buffer only) */

#define SIPP_MAX_ID        21

#define SIPP_RAW_ID_MASK        (1 << SIPP_RAW_ID)
#define SIPP_STATS_MASK         (1 << SIPP_STATS_ID)
#define SIPP_LSC_ID_MASK        (1 << SIPP_LSC_ID)
#define SIPP_LSC_GM_ID_MASK     (1 << SIPP_LSC_GM_ID)
#define SIPP_DBYR_ID_MASK       (1 << SIPP_DBYR_ID)
#define SIPP_DBYR_PPM_ID_MASK   (1 << SIPP_DBYR_PPM_ID)
#define SIPP_CHROMA_ID_MASK     (1 << SIPP_CHROMA_ID)
#define SIPP_CHROMA_REF_ID_MASK (1 << SIPP_CHROMA_REF_ID)
#define SIPP_LUMA_ID_MASK       (1 << SIPP_LUMA_ID)
#define SIPP_LUMA_REF_ID_MASK   (1 << SIPP_LUMA_REF_ID)
#define SIPP_SHARPEN_ID_MASK    (1 << SIPP_SHARPEN_ID)
#define SIPP_UPFIRDN_ID_MASK    (1 << SIPP_UPFIRDN_ID)
#define SIPP_MED_ID_MASK        (1 << SIPP_MED_ID)
#define SIPP_LUT_ID_MASK        (1 << SIPP_LUT_ID)
#define SIPP_LUT_LOAD_MASK      (1 << SIPP_LUT_LOAD_ID)
#define SIPP_EDGE_OP_ID_MASK    (1 << SIPP_EDGE_OP_ID)
#define SIPP_CONV_ID_MASK       (1 << SIPP_CONV_ID)
#define SIPP_HARRIS_ID_MASK     (1 << SIPP_HARRIS_ID)
#define SIPP_CC_ID_MASK         (1 << SIPP_CC_ID)
#define SIPP_CC_CHROMA_ID_MASK  (1 << SIPP_CC_CHROMA_ID)
#define SIPP_MIPI_TX0_ID_MASK   (1 << SIPP_MIPI_TX0_ID)
#define SIPP_MIPI_TX1_ID_MASK   (1 << SIPP_MIPI_TX1_ID)
#define SIPP_MIPI_RX0_ID_MASK   (1 << SIPP_MIPI_RX0_ID)
#define SIPP_MIPI_RX1_ID_MASK   (1 << SIPP_MIPI_RX1_ID)
#define SIPP_MIPI_RX2_ID_MASK   (1 << SIPP_MIPI_RX2_ID)
#define SIPP_MIPI_RX3_ID_MASK   (1 << SIPP_MIPI_RX3_ID)

// Buffer fill level inc/dec and context update control bits of
// SIPP_I/OBUF_FC and SIPP_ICTX registers
#define SIPP_INCDEC_BIT 30
#define SIPP_START_BIT  30
#define SIPP_CTXUP_BIT  31
#define SIPP_INCDEC_BIT_MASK (1 << SIPP_INCDEC_BIT)
#define SIPP_START_BIT_MASK  (1 << SIPP_START_BIT)
#define SIPP_CTXUP_BIT_MASK  (1 << SIPP_CTXUP_BIT)

// Mask and offset for working with bfl and cbl fields of SIPP_I/OBUF_FC
// registers
#define SIPP_NL_MASK    0x3ff
#define SIPP_CBL_OFFSET 16

// Mask and offset for working with SIPP_[FILT]_FRM_DIM registers
#define SIPP_IMGDIM_SIZE 16
#define SIPP_IMGDIM_MASK 0xffff

// Mask for working with kl field of filter configuration registers (offset generally 0)
#define SIPP_KL_MASK 0xf

// General enable/disable macros
#define ENABLED   1
#define DISABLED  0

// Default/shadow registers indices
#define DEFAULT   0
#define SHADOW    1

// Filter constant kernel sizes
#define RAW_KERNEL_SIZE         5
#define HIST_KERNEL_SIZE        3
#define LSC_KERNEL_SIZE         1
#define DBYR_AHD_KERNEL_SIZE    11
#define DBYR_BIL_KERNEL_SIZE    3
#define DBYR_PPM_KERNEL_SIZE    3
#define CHROMA_V_KERNEL_SIZE    21
#define CHROMA_H0_KERNEL_SIZE   23
#define CHROMA_H1_KERNEL_SIZE   17
#define CHROMA_H2_KERNEL_SIZE   13
#define CHROMA_REF_KERNEL_SIZE  21
#define LUMA_KERNEL_SIZE        7
#define LUMA_REF_KERNEL_SIZE    11
#define LUT_KERNEL_SIZE         1
#define EDGE_OP_KERNEL_SIZE     3
#define CC_LUMA_KERNEL_SIZE     1
#define CC_CHROMA_KERNEL_SIZE   5

// CMXDMA transaction direction
#define CMXCMX         0
#define CMXDDR         1
#define DDRCMX         2
#define DDRDDR         3

// Format encoding
#define PLANAR         0
#define BAYER          1

// Bayer pattern encoding
#define GRBG           0
#define RGGB           1
#define GBRG           2
#define BGGR           3

// Image out order encoding
#define P_RGB          0
#define P_BGR          1
#define P_RBG          2
#define P_BRG          3
#define P_GRB          4
#define P_GBR          5

// Edge Operator General Use Defines
//#######################################################################################
// Input modes
#define NORMAL_MODE                  0
#define PRE_FP16_GRAD                1
#define PRE_U8_GRAD                  2
// Output modes
#define SCALED_MAGN_16BIT            0
#define SCALED_MAGN_8BIT             1
#define MAGN_ORIENT_16BIT            2
#define ORIENT_8BIT                  3
#define SCALED_GRADIENTS_16BIT       4
#define SCALED_GRADIENTS_32BIT       5
// Theta modes
#define NORMAL_THETA                 0
#define X_AXIS_REFL                  1
#define XY_AXIS_REFL                 2

#endif // __SIPP_HWCOMMON_H__
