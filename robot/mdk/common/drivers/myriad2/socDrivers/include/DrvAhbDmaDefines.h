///
/// @file
/// @copyright All code copyright Movidius Ltd 2013, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @brief     Definitions and types needed by AHB DMA
///
#ifndef DRV_AHB_DMA_DEFINES_H
#define DRV_AHB_DMA_DEFINES_H

#include "registersMyriad2.h"  // not present in 1

// start of each CH
#define DRV_AHB_DMA_CH0_START_ADDR              (AHB_DMA_BASE_ADR + 0x000)
#define DRV_AHB_DMA_CH1_START_ADDR              (AHB_DMA_BASE_ADR + 0x058)
#define DRV_AHB_DMA_CH2_START_ADDR              (AHB_DMA_BASE_ADR + 0x0b0)
#define DRV_AHB_DMA_CH3_START_ADDR              (AHB_DMA_BASE_ADR + 0x108)
#define DRV_AHB_DMA_CH4_START_ADDR              (AHB_DMA_BASE_ADR + 0x160)
#define DRV_AHB_DMA_CH5_START_ADDR              (AHB_DMA_BASE_ADR + 0x1b8)
#define DRV_AHB_DMA_CH6_START_ADDR              (AHB_DMA_BASE_ADR + 0x210)
#define DRV_AHB_DMA_CH7_START_ADDR              (AHB_DMA_BASE_ADR + 0x268)
#define DRV_AHB_DMA_COMMON_START_ADDR           (AHB_DMA_BASE_ADR + 0x2c0)

// CH register offsets
#define DRV_AHB_DMA_SAR_OFFSET          0x000
#define DRV_AHB_DMA_DAR_OFFSET          0x008
#define DRV_AHB_DMA_LLP_OFFSET          0x010
#define DRV_AHB_DMA_CTL_OFFSET          0x018
#define DRV_AHB_DMA_SSTAT_OFFSET        0x020 // not implemented
#define DRV_AHB_DMA_DSTAT_OFFSET        0x028 // not implemented
#define DRV_AHB_DMA_SSTATAR_OFFSET      0x030 // not implemented
#define DRV_AHB_DMA_DSTATAR_OFFSET      0x038 // not implemented
#define DRV_AHB_DMA_CFG_OFFSET          0x040
#define DRV_AHB_DMA_SGR_OFFSET          0x048
#define DRV_AHB_DMA_DSR_OFFSET          0x050

// CH0
#define DRV_AHB_DMA_SAR0      (DRV_AHB_DMA_CH0_START_ADDR + DRV_AHB_DMA_SAR_OFFSET)
#define DRV_AHB_DMA_DAR0      (DRV_AHB_DMA_CH0_START_ADDR + DRV_AHB_DMA_DAR_OFFSET)
#define DRV_AHB_DMA_LLP0      (DRV_AHB_DMA_CH0_START_ADDR + DRV_AHB_DMA_LLP_OFFSET)
#define DRV_AHB_DMA_CTL0      (DRV_AHB_DMA_CH0_START_ADDR + DRV_AHB_DMA_CTL_OFFSET)
#define DRV_AHB_DMA_SSTAT0    (DRV_AHB_DMA_CH0_START_ADDR + DRV_AHB_DMA_SSTAT_OFFSET) // not implemented
#define DRV_AHB_DMA_DSTAT0    (DRV_AHB_DMA_CH0_START_ADDR + DRV_AHB_DMA_DSTAT_OFFSET) // not implemented
#define DRV_AHB_DMA_SSTATAR0  (DRV_AHB_DMA_CH0_START_ADDR + DRV_AHB_DMA_SSTATAR_OFFSET) // not implemented
#define DRV_AHB_DMA_DSTATAR0  (DRV_AHB_DMA_CH0_START_ADDR + DRV_AHB_DMA_DSTATAR_OFFSET) // not implemented
#define DRV_AHB_DMA_CFG0      (DRV_AHB_DMA_CH0_START_ADDR + DRV_AHB_DMA_CFG_OFFSET)
#define DRV_AHB_DMA_SGR0      (DRV_AHB_DMA_CH0_START_ADDR + DRV_AHB_DMA_SGR_OFFSET)
#define DRV_AHB_DMA_DSR0      (DRV_AHB_DMA_CH0_START_ADDR + DRV_AHB_DMA_DSR_OFFSET)

// CH1
#define DRV_AHB_DMA_SAR1      (DRV_AHB_DMA_CH1_START_ADDR + DRV_AHB_DMA_SAR_OFFSET)
#define DRV_AHB_DMA_DAR1      (DRV_AHB_DMA_CH1_START_ADDR + DRV_AHB_DMA_DAR_OFFSET)
#define DRV_AHB_DMA_LLP1      (DRV_AHB_DMA_CH1_START_ADDR + DRV_AHB_DMA_LLP_OFFSET)
#define DRV_AHB_DMA_CTL1      (DRV_AHB_DMA_CH1_START_ADDR + DRV_AHB_DMA_CTL_OFFSET)
#define DRV_AHB_DMA_SSTAT1    (DRV_AHB_DMA_CH1_START_ADDR + DRV_AHB_DMA_SSTAT_OFFSET) // not implemented
#define DRV_AHB_DMA_DSTAT1    (DRV_AHB_DMA_CH1_START_ADDR + DRV_AHB_DMA_DSTAT_OFFSET) // not implemented
#define DRV_AHB_DMA_SSTATAR1  (DRV_AHB_DMA_CH1_START_ADDR + DRV_AHB_DMA_SSTATAR_OFFSET) // not implemented
#define DRV_AHB_DMA_DSTATAR1  (DRV_AHB_DMA_CH1_START_ADDR + DRV_AHB_DMA_DSTATAR_OFFSET) // not implemented
#define DRV_AHB_DMA_CFG1      (DRV_AHB_DMA_CH1_START_ADDR + DRV_AHB_DMA_CFG_OFFSET)
#define DRV_AHB_DMA_SGR1      (DRV_AHB_DMA_CH1_START_ADDR + DRV_AHB_DMA_SGR_OFFSET)
#define DRV_AHB_DMA_DSR1      (DRV_AHB_DMA_CH1_START_ADDR + DRV_AHB_DMA_DSR_OFFSET)

// CH2
#define DRV_AHB_DMA_SAR2      (DRV_AHB_DMA_CH2_START_ADDR + DRV_AHB_DMA_SAR_OFFSET)
#define DRV_AHB_DMA_DAR2      (DRV_AHB_DMA_CH2_START_ADDR + DRV_AHB_DMA_DAR_OFFSET)
#define DRV_AHB_DMA_LLP2      (DRV_AHB_DMA_CH2_START_ADDR + DRV_AHB_DMA_LLP_OFFSET)
#define DRV_AHB_DMA_CTL2      (DRV_AHB_DMA_CH2_START_ADDR + DRV_AHB_DMA_CTL_OFFSET)
#define DRV_AHB_DMA_SSTAT2    (DRV_AHB_DMA_CH2_START_ADDR + DRV_AHB_DMA_SSTAT_OFFSET) // not implemented
#define DRV_AHB_DMA_DSTAT2    (DRV_AHB_DMA_CH2_START_ADDR + DRV_AHB_DMA_DSTAT_OFFSET) // not implemented
#define DRV_AHB_DMA_SSTATAR2  (DRV_AHB_DMA_CH2_START_ADDR + DRV_AHB_DMA_SSTATAR_OFFSET) // not implemented
#define DRV_AHB_DMA_DSTATAR2  (DRV_AHB_DMA_CH2_START_ADDR + DRV_AHB_DMA_DSTATAR_OFFSET) // not implemented
#define DRV_AHB_DMA_CFG2      (DRV_AHB_DMA_CH2_START_ADDR + DRV_AHB_DMA_CFG_OFFSET)
#define DRV_AHB_DMA_SGR2      (DRV_AHB_DMA_CH2_START_ADDR + DRV_AHB_DMA_SGR_OFFSET)
#define DRV_AHB_DMA_DSR2      (DRV_AHB_DMA_CH2_START_ADDR + DRV_AHB_DMA_DSR_OFFSET)

// CH3
#define DRV_AHB_DMA_SAR3      (DRV_AHB_DMA_CH3_START_ADDR + DRV_AHB_DMA_SAR_OFFSET)
#define DRV_AHB_DMA_DAR3      (DRV_AHB_DMA_CH3_START_ADDR + DRV_AHB_DMA_DAR_OFFSET)
#define DRV_AHB_DMA_LLP3      (DRV_AHB_DMA_CH3_START_ADDR + DRV_AHB_DMA_LLP_OFFSET)
#define DRV_AHB_DMA_CTL3      (DRV_AHB_DMA_CH3_START_ADDR + DRV_AHB_DMA_CTL_OFFSET)
#define DRV_AHB_DMA_SSTAT3    (DRV_AHB_DMA_CH3_START_ADDR + DRV_AHB_DMA_SSTAT_OFFSET) // not implemented
#define DRV_AHB_DMA_DSTAT3    (DRV_AHB_DMA_CH3_START_ADDR + DRV_AHB_DMA_DSTAT_OFFSET) // not implemented
#define DRV_AHB_DMA_SSTATAR3  (DRV_AHB_DMA_CH3_START_ADDR + DRV_AHB_DMA_SSTATAR_OFFSET) // not implemented
#define DRV_AHB_DMA_DSTATAR3  (DRV_AHB_DMA_CH3_START_ADDR + DRV_AHB_DMA_DSTATAR_OFFSET) // not implemented
#define DRV_AHB_DMA_CFG3      (DRV_AHB_DMA_CH3_START_ADDR + DRV_AHB_DMA_CFG_OFFSET)
#define DRV_AHB_DMA_SGR3      (DRV_AHB_DMA_CH3_START_ADDR + DRV_AHB_DMA_SGR_OFFSET)
#define DRV_AHB_DMA_DSR3      (DRV_AHB_DMA_CH3_START_ADDR + DRV_AHB_DMA_DSR_OFFSET)

// CH4
#define DRV_AHB_DMA_SAR4      (DRV_AHB_DMA_CH4_START_ADDR + DRV_AHB_DMA_SAR_OFFSET)
#define DRV_AHB_DMA_DAR4      (DRV_AHB_DMA_CH4_START_ADDR + DRV_AHB_DMA_DAR_OFFSET)
#define DRV_AHB_DMA_LLP4      (DRV_AHB_DMA_CH4_START_ADDR + DRV_AHB_DMA_LLP_OFFSET)
#define DRV_AHB_DMA_CTL4      (DRV_AHB_DMA_CH4_START_ADDR + DRV_AHB_DMA_CTL_OFFSET)
#define DRV_AHB_DMA_SSTAT4    (DRV_AHB_DMA_CH4_START_ADDR + DRV_AHB_DMA_SSTAT_OFFSET) // not implemented
#define DRV_AHB_DMA_DSTAT4    (DRV_AHB_DMA_CH4_START_ADDR + DRV_AHB_DMA_DSTAT_OFFSET) // not implemented
#define DRV_AHB_DMA_SSTATAR4  (DRV_AHB_DMA_CH4_START_ADDR + DRV_AHB_DMA_SSTATAR_OFFSET) // not implemented
#define DRV_AHB_DMA_DSTATAR4  (DRV_AHB_DMA_CH4_START_ADDR + DRV_AHB_DMA_DSTATAR_OFFSET) // not implemented
#define DRV_AHB_DMA_CFG4      (DRV_AHB_DMA_CH4_START_ADDR + DRV_AHB_DMA_CFG_OFFSET)
#define DRV_AHB_DMA_SGR4      (DRV_AHB_DMA_CH4_START_ADDR + DRV_AHB_DMA_SGR_OFFSET)
#define DRV_AHB_DMA_DSR4      (DRV_AHB_DMA_CH4_START_ADDR + DRV_AHB_DMA_DSR_OFFSET)

// CH5
#define DRV_AHB_DMA_SAR5      (DRV_AHB_DMA_CH5_START_ADDR + DRV_AHB_DMA_SAR_OFFSET)
#define DRV_AHB_DMA_DAR5      (DRV_AHB_DMA_CH5_START_ADDR + DRV_AHB_DMA_DAR_OFFSET)
#define DRV_AHB_DMA_LLP5      (DRV_AHB_DMA_CH5_START_ADDR + DRV_AHB_DMA_LLP_OFFSET)
#define DRV_AHB_DMA_CTL5      (DRV_AHB_DMA_CH5_START_ADDR + DRV_AHB_DMA_CTL_OFFSET)
#define DRV_AHB_DMA_SSTAT5    (DRV_AHB_DMA_CH5_START_ADDR + DRV_AHB_DMA_SSTAT_OFFSET) // not implemented
#define DRV_AHB_DMA_DSTAT5    (DRV_AHB_DMA_CH5_START_ADDR + DRV_AHB_DMA_DSTAT_OFFSET) // not implemented
#define DRV_AHB_DMA_SSTATAR5  (DRV_AHB_DMA_CH5_START_ADDR + DRV_AHB_DMA_SSTATAR_OFFSET) // not implemented
#define DRV_AHB_DMA_DSTATAR5  (DRV_AHB_DMA_CH5_START_ADDR + DRV_AHB_DMA_DSTATAR_OFFSET) // not implemented
#define DRV_AHB_DMA_CFG5      (DRV_AHB_DMA_CH5_START_ADDR + DRV_AHB_DMA_CFG_OFFSET)
#define DRV_AHB_DMA_SGR5      (DRV_AHB_DMA_CH5_START_ADDR + DRV_AHB_DMA_SGR_OFFSET)
#define DRV_AHB_DMA_DSR5      (DRV_AHB_DMA_CH5_START_ADDR + DRV_AHB_DMA_DSR_OFFSET)

// CH6
#define DRV_AHB_DMA_SAR6      (DRV_AHB_DMA_CH6_START_ADDR + DRV_AHB_DMA_SAR_OFFSET)
#define DRV_AHB_DMA_DAR6      (DRV_AHB_DMA_CH6_START_ADDR + DRV_AHB_DMA_DAR_OFFSET)
#define DRV_AHB_DMA_LLP6      (DRV_AHB_DMA_CH6_START_ADDR + DRV_AHB_DMA_LLP_OFFSET)
#define DRV_AHB_DMA_CTL6      (DRV_AHB_DMA_CH6_START_ADDR + DRV_AHB_DMA_CTL_OFFSET)
#define DRV_AHB_DMA_SSTAT6    (DRV_AHB_DMA_CH6_START_ADDR + DRV_AHB_DMA_SSTAT_OFFSET) // not implemented
#define DRV_AHB_DMA_DSTAT6    (DRV_AHB_DMA_CH6_START_ADDR + DRV_AHB_DMA_DSTAT_OFFSET) // not implemented
#define DRV_AHB_DMA_SSTATAR6  (DRV_AHB_DMA_CH6_START_ADDR + DRV_AHB_DMA_SSTATAR_OFFSET) // not implemented
#define DRV_AHB_DMA_DSTATAR6  (DRV_AHB_DMA_CH6_START_ADDR + DRV_AHB_DMA_DSTATAR_OFFSET) // not implemented
#define DRV_AHB_DMA_CFG6      (DRV_AHB_DMA_CH6_START_ADDR + DRV_AHB_DMA_CFG_OFFSET)
#define DRV_AHB_DMA_SGR6      (DRV_AHB_DMA_CH6_START_ADDR + DRV_AHB_DMA_SGR_OFFSET)
#define DRV_AHB_DMA_DSR6      (DRV_AHB_DMA_CH6_START_ADDR + DRV_AHB_DMA_DSR_OFFSET)

// CH7
#define DRV_AHB_DMA_SAR7      (DRV_AHB_DMA_CH7_START_ADDR + DRV_AHB_DMA_SAR_OFFSET)
#define DRV_AHB_DMA_DAR7      (DRV_AHB_DMA_CH7_START_ADDR + DRV_AHB_DMA_DAR_OFFSET)
#define DRV_AHB_DMA_LLP7      (DRV_AHB_DMA_CH7_START_ADDR + DRV_AHB_DMA_LLP_OFFSET)
#define DRV_AHB_DMA_CTL7      (DRV_AHB_DMA_CH7_START_ADDR + DRV_AHB_DMA_CTL_OFFSET)
#define DRV_AHB_DMA_SSTAT7    (DRV_AHB_DMA_CH7_START_ADDR + DRV_AHB_DMA_SSTAT_OFFSET) // not implemented
#define DRV_AHB_DMA_DSTAT7    (DRV_AHB_DMA_CH7_START_ADDR + DRV_AHB_DMA_DSTAT_OFFSET) // not implemented
#define DRV_AHB_DMA_SSTATAR7  (DRV_AHB_DMA_CH7_START_ADDR + DRV_AHB_DMA_SSTATAR_OFFSET) // not implemented
#define DRV_AHB_DMA_DSTATAR7  (DRV_AHB_DMA_CH7_START_ADDR + DRV_AHB_DMA_DSTATAR_OFFSET) // not implemented
#define DRV_AHB_DMA_CFG7      (DRV_AHB_DMA_CH7_START_ADDR + DRV_AHB_DMA_CFG_OFFSET)
#define DRV_AHB_DMA_SGR7      (DRV_AHB_DMA_CH7_START_ADDR + DRV_AHB_DMA_SGR_OFFSET)
#define DRV_AHB_DMA_DSR7      (DRV_AHB_DMA_CH7_START_ADDR + DRV_AHB_DMA_DSR_OFFSET)

// common control
#define DRV_AHB_DMA_RAWTFR          (DRV_AHB_DMA_COMMON_START_ADDR + 0x000)
#define DRV_AHB_DMA_RAWBLOCK        (DRV_AHB_DMA_COMMON_START_ADDR + 0x008)
#define DRV_AHB_DMA_RAWSRCTRAN      (DRV_AHB_DMA_COMMON_START_ADDR + 0x010)
#define DRV_AHB_DMA_RAWDSTTRAN      (DRV_AHB_DMA_COMMON_START_ADDR + 0x018)
#define DRV_AHB_DMA_RAWERR          (DRV_AHB_DMA_COMMON_START_ADDR + 0x020)
#define DRV_AHB_DMA_STATUSTFR       (DRV_AHB_DMA_COMMON_START_ADDR + 0x028)
#define DRV_AHB_DMA_STATUSBLOCK     (DRV_AHB_DMA_COMMON_START_ADDR + 0x030)
#define DRV_AHB_DMA_STATUSSRCTRAN   (DRV_AHB_DMA_COMMON_START_ADDR + 0x038)
#define DRV_AHB_DMA_STATUSDSTTRAN   (DRV_AHB_DMA_COMMON_START_ADDR + 0x040)
#define DRV_AHB_DMA_STATUSERR       (DRV_AHB_DMA_COMMON_START_ADDR + 0x048)
#define DRV_AHB_DMA_MASKTFR         (DRV_AHB_DMA_COMMON_START_ADDR + 0x050)
#define DRV_AHB_DMA_MASKBLOCK       (DRV_AHB_DMA_COMMON_START_ADDR + 0x058)
#define DRV_AHB_DMA_MASKSRCTRAN     (DRV_AHB_DMA_COMMON_START_ADDR + 0x060)
#define DRV_AHB_DMA_MASKDSTTRAN     (DRV_AHB_DMA_COMMON_START_ADDR + 0x068)
#define DRV_AHB_DMA_MASKERR         (DRV_AHB_DMA_COMMON_START_ADDR + 0x070)
#define DRV_AHB_DMA_CLEARTFR        (DRV_AHB_DMA_COMMON_START_ADDR + 0x078)
#define DRV_AHB_DMA_CLEARBLOCK      (DRV_AHB_DMA_COMMON_START_ADDR + 0x080)
#define DRV_AHB_DMA_CLEARSRCTRAN    (DRV_AHB_DMA_COMMON_START_ADDR + 0x088)
#define DRV_AHB_DMA_CLEARDSTTRAN    (DRV_AHB_DMA_COMMON_START_ADDR + 0x090)
#define DRV_AHB_DMA_CLEARERR        (DRV_AHB_DMA_COMMON_START_ADDR + 0x098)
#define DRV_AHB_DMA_STATUSINT       (DRV_AHB_DMA_COMMON_START_ADDR + 0x0a0)
#define DRV_AHB_DMA_REQSRCREG       (DRV_AHB_DMA_COMMON_START_ADDR + 0x0a8)
#define DRV_AHB_DMA_REQDSTREG       (DRV_AHB_DMA_COMMON_START_ADDR + 0x0b0)
#define DRV_AHB_DMA_SGLRQSRCREG     (DRV_AHB_DMA_COMMON_START_ADDR + 0x0b8)
#define DRV_AHB_DMA_SGLRQDSTREG     (DRV_AHB_DMA_COMMON_START_ADDR + 0x0c0)
#define DRV_AHB_DMA_LSTSRCREG       (DRV_AHB_DMA_COMMON_START_ADDR + 0x0c8)
#define DRV_AHB_DMA_LSTDSTREG       (DRV_AHB_DMA_COMMON_START_ADDR + 0x0d0)
#define DRV_AHB_DMA_DMACFGREG       (DRV_AHB_DMA_COMMON_START_ADDR + 0x0d8)
#define DRV_AHB_DMA_CHENREG         (DRV_AHB_DMA_COMMON_START_ADDR + 0x0e0)
#define DRV_AHB_DMA_DMAIDREG        (DRV_AHB_DMA_COMMON_START_ADDR + 0x0e8)
#define DRV_AHB_DMA_DMATESTREG      (DRV_AHB_DMA_COMMON_START_ADDR + 0x0f0)
#define DRV_AHB_DMA_COMP_PARAM_6    (DRV_AHB_DMA_COMMON_START_ADDR + 0x108)
#define DRV_AHB_DMA_COMP_PARAM_5    (DRV_AHB_DMA_COMMON_START_ADDR + 0x110)
#define DRV_AHB_DMA_COMP_PARAM_4    (DRV_AHB_DMA_COMMON_START_ADDR + 0x118)
#define DRV_AHB_DMA_COMP_PARAM_3    (DRV_AHB_DMA_COMMON_START_ADDR + 0x120)
#define DRV_AHB_DMA_COMP_PARAM_2    (DRV_AHB_DMA_COMMON_START_ADDR + 0x128)
#define DRV_AHB_DMA_COMP_PARAM_1    (DRV_AHB_DMA_COMMON_START_ADDR + 0x130)
#define DRV_AHB_DMA_COMP_ID         (DRV_AHB_DMA_COMMON_START_ADDR + 0x138)

// bit fileds for registers

//DRV_AHB_DMA_LLPx
// bits 0:1
#define DRV_AHB_DMA_LLP_LMS0     (0x00 <<0)
#define DRV_AHB_DMA_LLP_LMS1     (0x01 <<0)
#define DRV_AHB_DMA_LLP_LMS2     (0x02 <<0)
#define DRV_AHB_DMA_LLP_LMS3     (0x03 <<0)
// bits 2:31
#define DRV_AHB_DMA_LLP_LOC_MASK (~(0x03))

//DRV_AHB_DMA_CTLx
//bit 0
#define DRV_AHB_DMA_CTL_INT_EN             (1<<0)
// bit 1:3
#define DRV_AHB_DMA_CTL_DST_TR_WIDTH_8      (0x0<<1)
#define DRV_AHB_DMA_CTL_DST_TR_WIDTH_16     (0x1<<1)
#define DRV_AHB_DMA_CTL_DST_TR_WIDTH_32     (0x2<<1)
#define DRV_AHB_DMA_CTL_DST_TR_WIDTH_64     (0x3<<1)
#define DRV_AHB_DMA_CTL_DST_TR_WIDTH_128    (0x4<<1)
#define DRV_AHB_DMA_CTL_DST_TR_WIDTH_256    (0x5<<1)
//bit 4:6
#define DRV_AHB_DMA_CTL_SRC_TR_WIDTH_8      (0x0<<4)
#define DRV_AHB_DMA_CTL_SRC_TR_WIDTH_16     (0x1<<4)
#define DRV_AHB_DMA_CTL_SRC_TR_WIDTH_32     (0x2<<4)
#define DRV_AHB_DMA_CTL_SRC_TR_WIDTH_64     (0x3<<4)
#define DRV_AHB_DMA_CTL_SRC_TR_WIDTH_128    (0x4<<4)
#define DRV_AHB_DMA_CTL_SRC_TR_WIDTH_256    (0x5<<4)
// bit 7:8
#define DRV_AHB_DMA_CTL_DINC_INC           (0x0<<7)
#define DRV_AHB_DMA_CTL_DINC_DEC           (0x1<<7)
#define DRV_AHB_DMA_CTL_DINC_NO            (0x2<<7)
// bit 9:10
#define DRV_AHB_DMA_CTL_SINC_INC           (0x0<<9)
#define DRV_AHB_DMA_CTL_SINC_DEC           (0x1<<9)
#define DRV_AHB_DMA_CTL_SINC_NO            (0x2<<9)
// bit 11:13
#define DRV_AHB_DMA_CTL_DEST_MSIZE_1       (0<<11)
#define DRV_AHB_DMA_CTL_DEST_MSIZE_4       (1<<11)
#define DRV_AHB_DMA_CTL_DEST_MSIZE_8       (2<<11)
#define DRV_AHB_DMA_CTL_DEST_MSIZE_16      (3<<11)
#define DRV_AHB_DMA_CTL_DEST_MSIZE_32      (4<<11)
#define DRV_AHB_DMA_CTL_DEST_MSIZE_64      (5<<11)
#define DRV_AHB_DMA_CTL_DEST_MSIZE_128     (6<<11)
#define DRV_AHB_DMA_CTL_DEST_MSIZE_256     (7<<11)
// bit 14:16
#define DRV_AHB_DMA_CTL_SRC_MSIZE_1        (0<<14)
#define DRV_AHB_DMA_CTL_SRC_MSIZE_4        (1<<14)
#define DRV_AHB_DMA_CTL_SRC_MSIZE_8        (2<<14)
#define DRV_AHB_DMA_CTL_SRC_MSIZE_16       (3<<14)
#define DRV_AHB_DMA_CTL_SRC_MSIZE_32       (4<<14)
#define DRV_AHB_DMA_CTL_SRC_MSIZE_64       (5<<14)
#define DRV_AHB_DMA_CTL_SRC_MSIZE_128      (6<<14)
#define DRV_AHB_DMA_CTL_SRC_MSIZE_256      (7<<14)
// bit 17
#define DRV_AHB_DMA_CTL_SRC_GATHER_EN      (1<<17)
// bit 18
#define DRV_AHB_DMA_CTL_DST_SCATTER_EN     (1<<18)
// 19 undefined
// 20:22
#define DRV_AHB_DMA_CTL_TT_FC_D_M2M        (0<<20)
#define DRV_AHB_DMA_CTL_TT_FC_D_M2P        (1<<20)
#define DRV_AHB_DMA_CTL_TT_FC_D_P2M        (2<<20)
#define DRV_AHB_DMA_CTL_TT_FC_D_P2P        (3<<20)
#define DRV_AHB_DMA_CTL_TT_FC_P_P2M        (4<<20)
#define DRV_AHB_DMA_CTL_TT_FC_SP_P2P       (5<<20)
#define DRV_AHB_DMA_CTL_TT_FC_P_M2P        (6<<20)
#define DRV_AHB_DMA_CTL_TT_FC_DP_P2P       (7<<20)
// 23:24
#define DRV_AHB_DMA_CTL_DMS_1              (0<<23)
#define DRV_AHB_DMA_CTL_DMS_2              (1<<23)
#define DRV_AHB_DMA_CTL_DMS_3              (2<<23)
#define DRV_AHB_DMA_CTL_DMS_4              (3<<23)
//25:26
#define DRV_AHB_DMA_CTL_SMS_1              (0<<25)
#define DRV_AHB_DMA_CTL_SMS_2              (1<<25)
#define DRV_AHB_DMA_CTL_SMS_3              (2<<25)
#define DRV_AHB_DMA_CTL_SMS_4              (3<<25)
// 27
#define DRV_AHB_DMA_CTL_LLP_DST_EN         (1<<27)
// 28
#define DRV_AHB_DMA_CTL_LLP_SRC_EN         (1<<28)
// bit 29:31 unused
// bit 0 // second 32 bit word
#define DRV_AHB_DMA_CTL_BLOCK_TS           (1<<(32-32))
// bit 44
#define DRV_AHB_DMA_CTL_DONE               (1<<(44-32))
// bit 45:63 undefined


// DRV_AHB_DMA_CFG0
// bit 0:4 unused
// bit 5:7
#define DRV_AHB_DMA_CFG_CH_PRIOR_0         (0<<5)
#define DRV_AHB_DMA_CFG_CH_PRIOR_1         (1<<5)
#define DRV_AHB_DMA_CFG_CH_PRIOR_2         (2<<5)
#define DRV_AHB_DMA_CFG_CH_PRIOR_3         (3<<5)
#define DRV_AHB_DMA_CFG_CH_PRIOR_4         (4<<5)
#define DRV_AHB_DMA_CFG_CH_PRIOR_5         (5<<5)
#define DRV_AHB_DMA_CFG_CH_PRIOR_6         (6<<5)
#define DRV_AHB_DMA_CFG_CH_PRIOR_7         (7<<5)
// bit 8
#define DRV_AHB_DMA_CFG_CH_SUSP            (1<<8)
// bit 9
#define DRV_AHB_DMA_CFG_FIFO_EMPTY         (1<<9)
// bit 10
#define DRV_AHB_DMA_CFG_HS_SEL_DST_HW      (0<<10)
#define DRV_AHB_DMA_CFG_HS_SEL_DST_SW      (1<<10)
// bit 11
#define DRV_AHB_DMA_CFG_HS_SEL_SRC_HW      (0<<11)
#define DRV_AHB_DMA_CFG_HS_SEL_SRC_SW      (1<<11)
// bit 12:13
#define DRV_AHB_DMA_CFG_LOCK_CH_L_TRANSFER        (0<<12)
#define DRV_AHB_DMA_CFG_LOCK_CH_L_B_TRANSFER      (1<<12)
#define DRV_AHB_DMA_CFG_LOCK_CH_L_TRANSACTION     (2<<12)
// bit 14:15
#define DRV_AHB_DMA_CFG_LOCK_B_L_TRANSFER         (0<<14)
#define DRV_AHB_DMA_CFG_LOCK_B_L_B_TRANSFER       (1<<14)
#define DRV_AHB_DMA_CFG_LOCK_B_L_TRANSACTION      (2<<14)
// bit 16
#define DRV_AHB_DMA_CFG_LOCK_CH                   (1<<16)
// bit 17
#define DRV_AHB_DMA_CFG_LOCK_B                    (1<<17)
// bit 18
#define DRV_AHB_DMA_CFG_DST_HS_POL_HI             (0<<18)
#define DRV_AHB_DMA_CFG_DST_HS_POL_LO             (1<<18)
// bit 19
#define DRV_AHB_DMA_CFG_SRC_HS_POL_HI             (0<<19)
#define DRV_AHB_DMA_CFG_SRC_HS_POL_LO             (1<<19)
// bit 20:29
#define DRV_AHB_DMA_CFG_MAX_ABRST_MASK            (0x3FF<<20)
#define DRV_AHB_DMA_CFG_MAX_ABRST_POS             (20)
// bit 30
#define DRV_AHB_DMA_CFG_RELOAD_SRC                (1<<30)
// bit 31
#define DRV_AHB_DMA_CFG_RELOAD_DST                (1<<31)
// bit 32
#define DRV_AHB_DMA_CFG_FCMODE                    (1<<(32-32))
// bit 33
#define DRV_AHB_DMA_CFG_FIFO_MODE                 (1<<(33-32))
// bit 34:36
#define DRV_AHB_DMA_CFG_PROTCTL_MASK              (7<<(34-32))  // HPROT filed in the AHB bus
// bit 37
#define DRV_AHB_DMA_CFG_DS_UPD_EN              (1<<(37-32))  // feature not implemented
// bit 38
#define DRV_AHB_DMA_CFG_SS_UPD_EN              (1<<(38-32))  // feature not implemented
// bit 39
#define DRV_AHB_DMA_CFG_SRC_PER_POS            (39-32)  //   start bit of this filed
// bit 43
#define DRV_AHB_DMA_CFG_DEST_PER_POS           (43-32)  //   start bit of this filed


//DRV_AHB_DMA_DMACFGREG
#define DRV_AHB_DMA_DMACFGREG_EN (1<<0)

// DRV_AHB_DMA_CHENREG
#define DRV_AHB_DMA_CHENREG_EN0  (1<<0)
#define DRV_AHB_DMA_CHENREG_EN1  (1<<1)
#define DRV_AHB_DMA_CHENREG_EN2  (1<<2)
#define DRV_AHB_DMA_CHENREG_EN3  (1<<3)
#define DRV_AHB_DMA_CHENREG_EN4  (1<<4)
#define DRV_AHB_DMA_CHENREG_EN5  (1<<5)
#define DRV_AHB_DMA_CHENREG_EN6  (1<<6)
#define DRV_AHB_DMA_CHENREG_EN7  (1<<7)

#define DRV_AHB_DMA_CHENREG_WEN0  (1<<8)
#define DRV_AHB_DMA_CHENREG_WEN1  (1<<9)
#define DRV_AHB_DMA_CHENREG_WEN2  (1<<10)
#define DRV_AHB_DMA_CHENREG_WEN3  (1<<11)
#define DRV_AHB_DMA_CHENREG_WEN4  (1<<12)
#define DRV_AHB_DMA_CHENREG_WEN5  (1<<13)
#define DRV_AHB_DMA_CHENREG_WEN6  (1<<14)
#define DRV_AHB_DMA_CHENREG_WEN7  (1<<15)


#define DRV_AHB_DMA_START_WAIT      (1)
#define DRV_AHB_DMA_START_NO_WAIT   (2)
#define DRV_AHB_DMA_NO_START        (3)


//==================================================================================================================================================================================
//==================================================================================================================================================================================
//        C O N F I G                                     DRV_AHB_DMA_CFG
//==================================================================================================================================================================================
//==================================================================================================================================================================================

#define D_AHB_DMA_CFG_ROIC_DATA_TO_MEM_0   (DRV_AHB_DMA_CFG_CH_PRIOR_1 \
                                           | DRV_AHB_DMA_CFG_HS_SEL_DST_SW \
                                           | DRV_AHB_DMA_CFG_HS_SEL_SRC_HW \
                                           | (0 << DRV_AHB_DMA_CFG_MAX_ABRST_POS) \
                                           | DRV_AHB_DMA_CFG_RELOAD_SRC)

#define D_AHB_DMA_CFG_ROIC_DATA_TO_MEM_1   ((8 << DRV_AHB_DMA_CFG_SRC_PER_POS) \
                                           |(0 << DRV_AHB_DMA_CFG_DEST_PER_POS))



#define D_AHB_DMA_CFG_MEM_TO_ROIC_CAL_0    (DRV_AHB_DMA_CFG_CH_PRIOR_3 \
                                           | DRV_AHB_DMA_CFG_HS_SEL_DST_HW \
                                           | DRV_AHB_DMA_CFG_HS_SEL_SRC_SW \
                                           | (0 << DRV_AHB_DMA_CFG_MAX_ABRST_POS) \
                                           | DRV_AHB_DMA_CFG_RELOAD_DST)

#define D_AHB_DMA_CFG_MEM_TO_ROIC_CAL_1    ((0 << DRV_AHB_DMA_CFG_SRC_PER_POS) \
                                           |(9 << DRV_AHB_DMA_CFG_DEST_PER_POS))





//==================================================================================================================================================================================
//==================================================================================================================================================================================
//        C O N F I G                                                                            DRV_AHB_DMA_CTL
//==================================================================================================================================================================================
//==================================================================================================================================================================================

#define D_AHB_DMA_CTL_ROIC_DATA_TO_MEM         ( DRV_AHB_DMA_CTL_INT_EN          \
                                               | DRV_AHB_DMA_CTL_DST_TR_WIDTH_32 \
                                               | DRV_AHB_DMA_CTL_SRC_TR_WIDTH_32 \
                                               | DRV_AHB_DMA_CTL_DINC_INC        \
                                               | DRV_AHB_DMA_CTL_SINC_NO         \
                                               | DRV_AHB_DMA_CTL_DEST_MSIZE_8    \
                                               | DRV_AHB_DMA_CTL_SRC_MSIZE_8     \
                                               | DRV_AHB_DMA_CTL_TT_FC_D_P2M )


#define D_AHB_DMA_CTL_MEM_TO_ROIC_CAL          ( DRV_AHB_DMA_CTL_INT_EN          \
                                               | DRV_AHB_DMA_CTL_DST_TR_WIDTH_32 \
                                               | DRV_AHB_DMA_CTL_SRC_TR_WIDTH_32 \
                                               | DRV_AHB_DMA_CTL_DINC_NO         \
                                               | DRV_AHB_DMA_CTL_SINC_INC        \
                                               | DRV_AHB_DMA_CTL_DEST_MSIZE_8    \
                                               | DRV_AHB_DMA_CTL_SRC_MSIZE_8     \
                                               | DRV_AHB_DMA_CTL_TT_FC_D_M2P )


//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//        C O N F I G                         F O R                                   R O I C      T O        M E M O R Y           and              M E M O R Y     T O    R O I C
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------



//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//        C O N F I G                         F O R                                      M E M O R Y    T O     P E R I P H E R A L    -  SW    - GENERIC
//       - to be used for peripherals that don't suport HW handshake
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


#define D_AHB_DMA_MEM_WORD_TO_PER_BYTE         ( DRV_AHB_DMA_CTL_INT_EN           \
                                               | DRV_AHB_DMA_CTL_DST_TR_WIDTH_8   \
                                               | DRV_AHB_DMA_CTL_SRC_TR_WIDTH_32  \
                                               | DRV_AHB_DMA_CTL_DINC_NO          \
                                               | DRV_AHB_DMA_CTL_SINC_INC         \
                                               | DRV_AHB_DMA_CTL_DEST_MSIZE_1     \
                                               | DRV_AHB_DMA_CTL_SRC_MSIZE_1      \
                                               | DRV_AHB_DMA_CTL_TT_FC_D_M2M      \
                                               | DRV_AHB_DMA_CTL_DMS_1            \
                                               | DRV_AHB_DMA_CTL_SMS_1)



#define D_AHB_DMA_MEM_WORD_TO_PER_HWORD        ( DRV_AHB_DMA_CTL_INT_EN           \
                                               | DRV_AHB_DMA_CTL_DST_TR_WIDTH_16  \
                                               | DRV_AHB_DMA_CTL_SRC_TR_WIDTH_32  \
                                               | DRV_AHB_DMA_CTL_DINC_NO          \
                                               | DRV_AHB_DMA_CTL_SINC_INC         \
                                               | DRV_AHB_DMA_CTL_DEST_MSIZE_1     \
                                               | DRV_AHB_DMA_CTL_SRC_MSIZE_1      \
                                               | DRV_AHB_DMA_CTL_TT_FC_D_M2M      \
                                               | DRV_AHB_DMA_CTL_DMS_1            \
                                               | DRV_AHB_DMA_CTL_SMS_1)


#define D_AHB_DMA_MEM_WORD_TO_PER_WORD         ( DRV_AHB_DMA_CTL_INT_EN           \
                                               | DRV_AHB_DMA_CTL_DST_TR_WIDTH_32  \
                                               | DRV_AHB_DMA_CTL_SRC_TR_WIDTH_32  \
                                               | DRV_AHB_DMA_CTL_DINC_NO          \
                                               | DRV_AHB_DMA_CTL_SINC_INC         \
                                               | DRV_AHB_DMA_CTL_DEST_MSIZE_1     \
                                               | DRV_AHB_DMA_CTL_SRC_MSIZE_1      \
                                               | DRV_AHB_DMA_CTL_TT_FC_D_M2M      \
                                               | DRV_AHB_DMA_CTL_DMS_1            \
                                               | DRV_AHB_DMA_CTL_SMS_1)


#define D_AHB_DMA_MEM_HWORD_TO_PER_BYTE        ( DRV_AHB_DMA_CTL_INT_EN           \
                                               | DRV_AHB_DMA_CTL_DST_TR_WIDTH_8   \
                                               | DRV_AHB_DMA_CTL_SRC_TR_WIDTH_16  \
                                               | DRV_AHB_DMA_CTL_DINC_NO          \
                                               | DRV_AHB_DMA_CTL_SINC_INC         \
                                               | DRV_AHB_DMA_CTL_DEST_MSIZE_1     \
                                               | DRV_AHB_DMA_CTL_SRC_MSIZE_1      \
                                               | DRV_AHB_DMA_CTL_TT_FC_D_M2M      \
                                               | DRV_AHB_DMA_CTL_DMS_1            \
                                               | DRV_AHB_DMA_CTL_SMS_1)


#define D_AHB_DMA_MEM_HWORD_TO_PER_HWORD       ( DRV_AHB_DMA_CTL_INT_EN           \
                                               | DRV_AHB_DMA_CTL_DST_TR_WIDTH_16  \
                                               | DRV_AHB_DMA_CTL_SRC_TR_WIDTH_16  \
                                               | DRV_AHB_DMA_CTL_DINC_NO          \
                                               | DRV_AHB_DMA_CTL_SINC_INC         \
                                               | DRV_AHB_DMA_CTL_DEST_MSIZE_1     \
                                               | DRV_AHB_DMA_CTL_SRC_MSIZE_1      \
                                               | DRV_AHB_DMA_CTL_TT_FC_D_M2M      \
                                               | DRV_AHB_DMA_CTL_DMS_1            \
                                               | DRV_AHB_DMA_CTL_SMS_1)

#define D_AHB_DMA_MEM_HWORD_TO_PER_WORD       ( DRV_AHB_DMA_CTL_INT_EN           \
                                               | DRV_AHB_DMA_CTL_DST_TR_WIDTH_32  \
                                               | DRV_AHB_DMA_CTL_SRC_TR_WIDTH_16  \
                                               | DRV_AHB_DMA_CTL_DINC_NO          \
                                               | DRV_AHB_DMA_CTL_SINC_INC         \
                                               | DRV_AHB_DMA_CTL_DEST_MSIZE_1     \
                                               | DRV_AHB_DMA_CTL_SRC_MSIZE_1      \
                                               | DRV_AHB_DMA_CTL_TT_FC_D_M2M      \
                                               | DRV_AHB_DMA_CTL_DMS_1            \
                                               | DRV_AHB_DMA_CTL_SMS_1)


#define D_AHB_DMA_MEM_BYTE_TO_PER_BYTE         ( DRV_AHB_DMA_CTL_INT_EN           \
                                               | DRV_AHB_DMA_CTL_DST_TR_WIDTH_8   \
                                               | DRV_AHB_DMA_CTL_SRC_TR_WIDTH_8   \
                                               | DRV_AHB_DMA_CTL_DINC_NO          \
                                               | DRV_AHB_DMA_CTL_SINC_INC         \
                                               | DRV_AHB_DMA_CTL_DEST_MSIZE_1     \
                                               | DRV_AHB_DMA_CTL_SRC_MSIZE_1      \
                                               | DRV_AHB_DMA_CTL_TT_FC_D_M2M      \
                                               | DRV_AHB_DMA_CTL_DMS_1            \
                                               | DRV_AHB_DMA_CTL_SMS_1)

#define D_AHB_DMA_MEM_BYTE_TO_PER_HWORD        ( DRV_AHB_DMA_CTL_INT_EN           \
                                               | DRV_AHB_DMA_CTL_DST_TR_WIDTH_16  \
                                               | DRV_AHB_DMA_CTL_SRC_TR_WIDTH_8   \
                                               | DRV_AHB_DMA_CTL_DINC_NO          \
                                               | DRV_AHB_DMA_CTL_SINC_INC         \
                                               | DRV_AHB_DMA_CTL_DEST_MSIZE_1     \
                                               | DRV_AHB_DMA_CTL_SRC_MSIZE_1      \
                                               | DRV_AHB_DMA_CTL_TT_FC_D_M2M      \
                                               | DRV_AHB_DMA_CTL_DMS_1            \
                                               | DRV_AHB_DMA_CTL_SMS_1)

#define D_AHB_DMA_MEM_BYTE_TO_PER_WORD         (DRV_AHB_DMA_CTL_INT_EN            \
                                               | DRV_AHB_DMA_CTL_DST_TR_WIDTH_32  \
                                               | DRV_AHB_DMA_CTL_SRC_TR_WIDTH_8   \
                                               | DRV_AHB_DMA_CTL_DINC_NO          \
                                               | DRV_AHB_DMA_CTL_SINC_INC         \
                                               | DRV_AHB_DMA_CTL_DEST_MSIZE_1     \
                                               | DRV_AHB_DMA_CTL_SRC_MSIZE_1      \
                                               | DRV_AHB_DMA_CTL_TT_FC_D_M2M      \
                                               | DRV_AHB_DMA_CTL_DMS_1            \
                                               | DRV_AHB_DMA_CTL_SMS_1)


//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//        C O N F I G                         F O R                                  P E R I P H E R A L    T O     M E M O R Y
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

#define D_AHB_DMA_PER_WORD_TO_MEM_BYTE         ( DRV_AHB_DMA_CTL_INT_EN           \
                                               | DRV_AHB_DMA_CTL_DST_TR_WIDTH_8   \
                                               | DRV_AHB_DMA_CTL_SRC_TR_WIDTH_32  \
                                               | DRV_AHB_DMA_CTL_DINC_INC         \
                                               | DRV_AHB_DMA_CTL_SINC_NO          \
                                               | DRV_AHB_DMA_CTL_DEST_MSIZE_1     \
                                               | DRV_AHB_DMA_CTL_SRC_MSIZE_1      \
                                               | DRV_AHB_DMA_CTL_TT_FC_D_M2M      \
                                               | DRV_AHB_DMA_CTL_DMS_1            \
                                               | DRV_AHB_DMA_CTL_SMS_1)



#define D_AHB_DMA_PER_WORD_TO_MEM_HWORD        ( DRV_AHB_DMA_CTL_INT_EN           \
                                               | DRV_AHB_DMA_CTL_DST_TR_WIDTH_16  \
                                               | DRV_AHB_DMA_CTL_SRC_TR_WIDTH_32  \
                                               | DRV_AHB_DMA_CTL_DINC_INC         \
                                               | DRV_AHB_DMA_CTL_SINC_NO          \
                                               | DRV_AHB_DMA_CTL_DEST_MSIZE_1     \
                                               | DRV_AHB_DMA_CTL_SRC_MSIZE_1      \
                                               | DRV_AHB_DMA_CTL_TT_FC_D_M2M      \
                                               | DRV_AHB_DMA_CTL_DMS_1            \
                                               | DRV_AHB_DMA_CTL_SMS_1)


#define D_AHB_DMA_PER_WORD_TO_MEM_WORD         ( DRV_AHB_DMA_CTL_INT_EN           \
                                               | DRV_AHB_DMA_CTL_DST_TR_WIDTH_32  \
                                               | DRV_AHB_DMA_CTL_SRC_TR_WIDTH_32  \
                                               | DRV_AHB_DMA_CTL_DINC_INC         \
                                               | DRV_AHB_DMA_CTL_SINC_NO          \
                                               | DRV_AHB_DMA_CTL_DEST_MSIZE_1     \
                                               | DRV_AHB_DMA_CTL_SRC_MSIZE_1      \
                                               | DRV_AHB_DMA_CTL_TT_FC_D_M2M      \
                                               | DRV_AHB_DMA_CTL_DMS_1            \
                                               | DRV_AHB_DMA_CTL_SMS_1)


#define D_AHB_DMA_PER_HWORD_TO_MEM_BYTE        ( DRV_AHB_DMA_CTL_INT_EN           \
                                               | DRV_AHB_DMA_CTL_DST_TR_WIDTH_8   \
                                               | DRV_AHB_DMA_CTL_SRC_TR_WIDTH_16  \
                                               | DRV_AHB_DMA_CTL_DINC_INC         \
                                               | DRV_AHB_DMA_CTL_SINC_NO          \
                                               | DRV_AHB_DMA_CTL_DEST_MSIZE_1     \
                                               | DRV_AHB_DMA_CTL_SRC_MSIZE_1      \
                                               | DRV_AHB_DMA_CTL_TT_FC_D_M2M      \
                                               | DRV_AHB_DMA_CTL_DMS_1            \
                                               | DRV_AHB_DMA_CTL_SMS_1)


#define D_AHB_DMA_PER_HWORD_TO_MEM_HWORD       ( DRV_AHB_DMA_CTL_INT_EN           \
                                               | DRV_AHB_DMA_CTL_DST_TR_WIDTH_16  \
                                               | DRV_AHB_DMA_CTL_SRC_TR_WIDTH_16  \
                                               | DRV_AHB_DMA_CTL_DINC_INC          \
                                               | DRV_AHB_DMA_CTL_SINC_NO         \
                                               | DRV_AHB_DMA_CTL_DEST_MSIZE_1     \
                                               | DRV_AHB_DMA_CTL_SRC_MSIZE_1      \
                                               | DRV_AHB_DMA_CTL_TT_FC_D_M2M      \
                                               | DRV_AHB_DMA_CTL_DMS_1            \
                                               | DRV_AHB_DMA_CTL_SMS_1)

#define D_AHB_DMA_PER_HWORD_TO_MEM_WORD       ( DRV_AHB_DMA_CTL_INT_EN           \
                                               | DRV_AHB_DMA_CTL_DST_TR_WIDTH_32  \
                                               | DRV_AHB_DMA_CTL_SRC_TR_WIDTH_16  \
                                               | DRV_AHB_DMA_CTL_DINC_INC          \
                                               | DRV_AHB_DMA_CTL_SINC_NO         \
                                               | DRV_AHB_DMA_CTL_DEST_MSIZE_1     \
                                               | DRV_AHB_DMA_CTL_SRC_MSIZE_1      \
                                               | DRV_AHB_DMA_CTL_TT_FC_D_M2M      \
                                               | DRV_AHB_DMA_CTL_DMS_1            \
                                               | DRV_AHB_DMA_CTL_SMS_1)


#define D_AHB_DMA_PER_BYTE_TO_MEM_BYTE         ( DRV_AHB_DMA_CTL_INT_EN           \
                                               | DRV_AHB_DMA_CTL_DST_TR_WIDTH_8   \
                                               | DRV_AHB_DMA_CTL_SRC_TR_WIDTH_8   \
                                               | DRV_AHB_DMA_CTL_DINC_INC          \
                                               | DRV_AHB_DMA_CTL_SINC_NO         \
                                               | DRV_AHB_DMA_CTL_DEST_MSIZE_1     \
                                               | DRV_AHB_DMA_CTL_SRC_MSIZE_1      \
                                               | DRV_AHB_DMA_CTL_TT_FC_D_M2M      \
                                               | DRV_AHB_DMA_CTL_DMS_1            \
                                               | DRV_AHB_DMA_CTL_SMS_1)

#define D_AHB_DMA_PER_BYTE_TO_MEM_HWORD        ( DRV_AHB_DMA_CTL_INT_EN           \
                                               | DRV_AHB_DMA_CTL_DST_TR_WIDTH_16  \
                                               | DRV_AHB_DMA_CTL_SRC_TR_WIDTH_8   \
                                               | DRV_AHB_DMA_CTL_DINC_INC         \
                                               | DRV_AHB_DMA_CTL_SINC_NO         \
                                               | DRV_AHB_DMA_CTL_DEST_MSIZE_1     \
                                               | DRV_AHB_DMA_CTL_SRC_MSIZE_1      \
                                               | DRV_AHB_DMA_CTL_TT_FC_D_M2M      \
                                               | DRV_AHB_DMA_CTL_DMS_1            \
                                               | DRV_AHB_DMA_CTL_SMS_1)

#define D_AHB_DMA_PER_BYTE_TO_MEM_WORD         (DRV_AHB_DMA_CTL_INT_EN            \
                                               | DRV_AHB_DMA_CTL_DST_TR_WIDTH_32  \
                                               | DRV_AHB_DMA_CTL_SRC_TR_WIDTH_8   \
                                               | DRV_AHB_DMA_CTL_DINC_INC         \
                                               | DRV_AHB_DMA_CTL_SINC_NO          \
                                               | DRV_AHB_DMA_CTL_DEST_MSIZE_1     \
                                               | DRV_AHB_DMA_CTL_SRC_MSIZE_1      \
                                               | DRV_AHB_DMA_CTL_TT_FC_D_M2M      \
                                               | DRV_AHB_DMA_CTL_DMS_1            \
                                               | DRV_AHB_DMA_CTL_SMS_1)

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//        C O N F I G                         F O R                                              M E M O R Y    T O     M E M O R Y
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

#define D_AHB_DMA_MEM_WORD_TO_MEM_BYTE         ( DRV_AHB_DMA_CTL_INT_EN           \
                                               | DRV_AHB_DMA_CTL_DST_TR_WIDTH_8   \
                                               | DRV_AHB_DMA_CTL_SRC_TR_WIDTH_32  \
                                               | DRV_AHB_DMA_CTL_DINC_INC         \
                                               | DRV_AHB_DMA_CTL_SINC_INC         \
                                               | DRV_AHB_DMA_CTL_DEST_MSIZE_1     \
                                               | DRV_AHB_DMA_CTL_SRC_MSIZE_1      \
                                               | DRV_AHB_DMA_CTL_TT_FC_D_M2M      \
                                               | DRV_AHB_DMA_CTL_DMS_1            \
                                               | DRV_AHB_DMA_CTL_SMS_1)



#define D_AHB_DMA_MEM_WORD_TO_MEM_HWORD        ( DRV_AHB_DMA_CTL_INT_EN           \
                                               | DRV_AHB_DMA_CTL_DST_TR_WIDTH_16  \
                                               | DRV_AHB_DMA_CTL_SRC_TR_WIDTH_32  \
                                               | DRV_AHB_DMA_CTL_DINC_INC         \
                                               | DRV_AHB_DMA_CTL_SINC_INC         \
                                               | DRV_AHB_DMA_CTL_DEST_MSIZE_1     \
                                               | DRV_AHB_DMA_CTL_SRC_MSIZE_1      \
                                               | DRV_AHB_DMA_CTL_TT_FC_D_M2M      \
                                               | DRV_AHB_DMA_CTL_DMS_1            \
                                               | DRV_AHB_DMA_CTL_SMS_1)


#define D_AHB_DMA_MEM_WORD_TO_MEM_WORD         ( DRV_AHB_DMA_CTL_INT_EN           \
                                               | DRV_AHB_DMA_CTL_DST_TR_WIDTH_32  \
                                               | DRV_AHB_DMA_CTL_SRC_TR_WIDTH_32  \
                                               | DRV_AHB_DMA_CTL_DINC_INC         \
                                               | DRV_AHB_DMA_CTL_SINC_INC         \
                                               | DRV_AHB_DMA_CTL_DEST_MSIZE_1     \
                                               | DRV_AHB_DMA_CTL_SRC_MSIZE_1      \
                                               | DRV_AHB_DMA_CTL_TT_FC_D_M2M      \
                                               | DRV_AHB_DMA_CTL_DMS_1            \
                                               | DRV_AHB_DMA_CTL_SMS_1)


#define D_AHB_DMA_MEM_HWORD_TO_MEM_BYTE        ( DRV_AHB_DMA_CTL_INT_EN           \
                                               | DRV_AHB_DMA_CTL_DST_TR_WIDTH_8   \
                                               | DRV_AHB_DMA_CTL_SRC_TR_WIDTH_16  \
                                               | DRV_AHB_DMA_CTL_DINC_INC         \
                                               | DRV_AHB_DMA_CTL_SINC_INC         \
                                               | DRV_AHB_DMA_CTL_DEST_MSIZE_1     \
                                               | DRV_AHB_DMA_CTL_SRC_MSIZE_1      \
                                               | DRV_AHB_DMA_CTL_TT_FC_D_M2M      \
                                               | DRV_AHB_DMA_CTL_DMS_1            \
                                               | DRV_AHB_DMA_CTL_SMS_1)


#define D_AHB_DMA_MEM_HWORD_TO_MEM_HWORD       ( DRV_AHB_DMA_CTL_INT_EN           \
                                               | DRV_AHB_DMA_CTL_DST_TR_WIDTH_16  \
                                               | DRV_AHB_DMA_CTL_SRC_TR_WIDTH_16  \
                                               | DRV_AHB_DMA_CTL_DINC_INC         \
                                               | DRV_AHB_DMA_CTL_SINC_INC         \
                                               | DRV_AHB_DMA_CTL_DEST_MSIZE_1     \
                                               | DRV_AHB_DMA_CTL_SRC_MSIZE_1      \
                                               | DRV_AHB_DMA_CTL_TT_FC_D_M2M      \
                                               | DRV_AHB_DMA_CTL_DMS_1            \
                                               | DRV_AHB_DMA_CTL_SMS_1)

#define D_AHB_DMA_MEM_HWORD_TO_MEM_WORD       ( DRV_AHB_DMA_CTL_INT_EN           \
                                               | DRV_AHB_DMA_CTL_DST_TR_WIDTH_32  \
                                               | DRV_AHB_DMA_CTL_SRC_TR_WIDTH_16  \
                                               | DRV_AHB_DMA_CTL_DINC_INC         \
                                               | DRV_AHB_DMA_CTL_SINC_INC         \
                                               | DRV_AHB_DMA_CTL_DEST_MSIZE_1     \
                                               | DRV_AHB_DMA_CTL_SRC_MSIZE_1      \
                                               | DRV_AHB_DMA_CTL_TT_FC_D_M2M      \
                                               | DRV_AHB_DMA_CTL_DMS_1            \
                                               | DRV_AHB_DMA_CTL_SMS_1)


#define D_AHB_DMA_MEM_BYTE_TO_MEM_BYTE         ( DRV_AHB_DMA_CTL_INT_EN           \
                                               | DRV_AHB_DMA_CTL_DST_TR_WIDTH_8   \
                                               | DRV_AHB_DMA_CTL_SRC_TR_WIDTH_8   \
                                               | DRV_AHB_DMA_CTL_DINC_INC         \
                                               | DRV_AHB_DMA_CTL_SINC_INC         \
                                               | DRV_AHB_DMA_CTL_DEST_MSIZE_1     \
                                               | DRV_AHB_DMA_CTL_SRC_MSIZE_1      \
                                               | DRV_AHB_DMA_CTL_TT_FC_D_M2M      \
                                               | DRV_AHB_DMA_CTL_DMS_1            \
                                               | DRV_AHB_DMA_CTL_SMS_1)

#define D_AHB_DMA_MEM_BYTE_TO_MEM_HWORD        ( DRV_AHB_DMA_CTL_INT_EN           \
                                               | DRV_AHB_DMA_CTL_DST_TR_WIDTH_16  \
                                               | DRV_AHB_DMA_CTL_SRC_TR_WIDTH_8   \
                                               | DRV_AHB_DMA_CTL_DINC_INC         \
                                               | DRV_AHB_DMA_CTL_SINC_INC         \
                                               | DRV_AHB_DMA_CTL_DEST_MSIZE_1     \
                                               | DRV_AHB_DMA_CTL_SRC_MSIZE_1      \
                                               | DRV_AHB_DMA_CTL_TT_FC_D_M2M      \
                                               | DRV_AHB_DMA_CTL_DMS_1            \
                                               | DRV_AHB_DMA_CTL_SMS_1)

#define D_AHB_DMA_MEM_BYTE_TO_MEM_WORD         (DRV_AHB_DMA_CTL_INT_EN            \
                                               | DRV_AHB_DMA_CTL_DST_TR_WIDTH_32  \
                                               | DRV_AHB_DMA_CTL_SRC_TR_WIDTH_8   \
                                               | DRV_AHB_DMA_CTL_DINC_INC         \
                                               | DRV_AHB_DMA_CTL_SINC_INC         \
                                               | DRV_AHB_DMA_CTL_DEST_MSIZE_1     \
                                               | DRV_AHB_DMA_CTL_SRC_MSIZE_1      \
                                               | DRV_AHB_DMA_CTL_TT_FC_D_M2M      \
                                               | DRV_AHB_DMA_CTL_DMS_1            \
                                               | DRV_AHB_DMA_CTL_SMS_1)



#endif // DRV_AHB_DMA_DEFINES_H

