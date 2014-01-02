///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief
///

#ifndef __SVU_COMMON_SHAVE_DEFINES__
#define __SVU_COMMON_SHAVE_DEFINES__


//Some DMA defines stay in this file
#include <swcDmaTypes.h>


//channel configuration
#define START_DMA0                  0
#define START_DMA01                 1
#define START_DMA012                2
#define START_DMA0123               3
//randevu
#define RANDEVU_MUTEX               0
#define DDR_ACCESS_MUTEX            4
// we asume that all the time, when we set a shave, will be the chace set at windows 0x1F
#define CMX_CACHE_PTR 0x1F000000
#define MASK_ACTIVATE_L2_CACHE 0xF0FFFFFF

#define DMA_TASK_REQ      0x0FF04004
#define DMA_TASK_STAT     0x0FF04008
#define DMA0_CFG          0x0FF04100
#define DMA0_SRC_ADDR     0x0FF04104
#define DMA0_DST_ADDR     0x0FF04108
#define DMA0_SIZE         0x0FF0410C
#define DMA0_LINE_WIDTH   0x0FF04110
#define DMA0_LINE_STRIDE  0x0FF04114
#define DMA1_CFG          0x0FF04200
#define DMA1_SRC_ADDR     0x0FF04204
#define DMA1_DST_ADDR     0x0FF04208
#define DMA1_SIZE         0x0FF0420C
#define DMA1_LINE_WIDTH   0x0FF04210
#define DMA1_LINE_STRIDE  0x0FF04214
#define DMA2_CFG          0x0FF04300
#define DMA2_SRC_ADDR     0x0FF04304
#define DMA2_DST_ADDR     0x0FF04308
#define DMA2_SIZE         0x0FF0430C
#define DMA2_LINE_WIDTH   0x0FF04310
#define DMA2_LINE_STRIDE  0x0FF04314
#define DMA3_CFG          0x0FF04400
#define DMA3_SRC_ADDR     0x0FF04404
#define DMA3_DST_ADDR     0x0FF04408
#define DMA3_SIZE         0x0FF0440C
#define DMA3_LINE_WIDTH   0x0FF04410
#define DMA3_LINE_STRIDE  0x0FF04414
#define FIFO_RD_BASE      0x0FF05050
#define FIFO_WR_BASE      0x0FF0F000

;/* TRF regs */
#define P_GPR             0x1F
#define P_GPI             0x1E
#define P_SVID            0x1C
#define P_CFG             0x1B
#define G_GALOIS          0x15
#define B_SREPS           0x14
#define C_CMU1            0x13
#define C_CMU0            0x12
#define C_CSI             0x11
#define F_AE              0x10
#define I_AE              0x0F
#define L_ENDIAN          0x0E
#define V_ACC3            0x0D
#define V_ACC2            0x0C
#define V_ACC1            0x0B
#define V_ACC0            0x0A
#define S_ACC             0x09
#define V_STATE           0x08
#define S_STATE           0x07
#define I_STATE           0x06
#define B_RFB             0x05
#define B_STATE           0x04
#define B_MREPS           0x03
#define B_LEND            0x02
#define B_LBEG            0x01
#define B_IP              0x00

#endif
