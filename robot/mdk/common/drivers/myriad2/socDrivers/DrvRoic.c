///   
/// @file
/// @copyright All code copyright Movidius Ltd 2013, all rights reserved. 
///            For License Warranty see: common/license.txt   
///
/// @brief     Driver for ROIC module
/// 
/// 
/// 


// 1: Includes
// ----------------------------------------------------------------------------
#include "registersMyriad.h"
#include "stdio.h"
#include "DrvIcb.h"
#include "DrvAhbDma.h"
#include "DrvGpio.h"
#include "assert.h"

#include "DrvRoic.h"

// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------

// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------

// 4: Static Local Data 
// ----------------------------------------------------------------------------

// 5: Static Function Prototypes
// ----------------------------------------------------------------------------

static void setRoicGpio()
{
    DrvGpioMode(GPIO_ROIC_CLK, GPIO_ROIC_CLK_MODE);
    DrvGpioMode(GPIO_ROIC_CMD, GPIO_ROIC_CMD_MODE);
    DrvGpioMode(GPIO_ROIC_CAL, GPIO_ROIC_CAL_MODE);  
    DrvGpioMode(GPIO_ROIC_D0,  GPIO_ROIC_D0_MODE | D_GPIO_DIR_IN);   
    DrvGpioMode(GPIO_ROIC_D1,  GPIO_ROIC_D1_MODE | D_GPIO_DIR_IN);    
}

// 6: Functions Implementation
// ----------------------------------------------------------------------------

void DrvRoicInitCmd(u32 commandAddress, u32 commandLength, u8 dmaCh)
{
           // setup dma to transfer data between memory and ROIC CMD, and start transfer, in this way the fifo is full on the enable 
           DrvAhbDmaTransfer(dmaCh, ROIC_CMD_BASE, commandAddress, commandLength, D_AHB_DMA_MEM_WORD_TO_PER_WORD, DRV_AHB_DMA_START_WAIT);   
}


void DrvRoicInitCalibration(u32 calibrationDataAddress, u32 calibrationDataLength, u8 dmaCh )
{
           // setup dma to transfer data between memory and ROIC Calilbration Data, and start so fifo would be full on enable 
           DrvAhbDmaTransferHw(dmaCh, ROIC_CAL_BASE, calibrationDataAddress, calibrationDataLength, 
                               D_AHB_DMA_CTL_MEM_TO_ROIC_CAL, 
                               D_AHB_DMA_CFG_MEM_TO_ROIC_CAL_0, 
                               D_AHB_DMA_CFG_MEM_TO_ROIC_CAL_1, 
                               DRV_AHB_DMA_START_NO_WAIT);   
}

void DrvRoicInitData(u32 dataAddress, u32 dataLength, u8 dmaCh)
{
           // setup dma to transfer data between ROIC Data buffer and memory, but don't start 
           DrvAhbDmaTransferHw(dmaCh, dataAddress, ROIC_DATA_BASE, dataLength, 
                               D_AHB_DMA_CTL_ROIC_DATA_TO_MEM, 
                               D_AHB_DMA_CFG_ROIC_DATA_TO_MEM_0, 
                               D_AHB_DMA_CFG_ROIC_DATA_TO_MEM_1, 
                               DRV_AHB_DMA_START_NO_WAIT);      
}

void DrvRoicInitTiming(u32 framePeriod, u32 noLines, u32 linePeriod, u32 noWords, u32 wordPeriod)
{
         // setup frame length in CC
         SET_REG_WORD(ROIC_LINECFG_ADDR,     framePeriod & DRV_ROIC_FRAME_PERIOD_MASK );

         // setup no of lines and line period in cc
         SET_REG_WORD(ROIC_LINECFG_ADDR,    ((noLines    << DRV_ROIC_LINECFG_NO_LINES      ) & DRV_ROIC_LINECFG_NO_LINES_MASK      ) | 
                                            ((linePeriod << DRV_ROIC_LINECFG_LINE_PERIOD   ) & DRV_ROIC_LINECFG_LINE_PERIOD_MASK   ));
                                            
         // setup no of words per line and word length in cc                                   
         SET_REG_WORD(ROIC_WORDPERIOD_ADDR, ((noWords    << DRV_ROIC_WORDPERIOD_WORDS      ) & DRV_ROIC_WORDPERIOD_WORDS_MASK      ) | 
                                            ((wordPeriod << DRV_ROIC_WORDPERIOD_WORD_PERIOD) & DRV_ROIC_WORDPERIOD_WORD_PERIOD_MASK) );

}

void DrvRoicInitDelay(u32 calStart, u32 delay)
{
         SET_REG_WORD(ROIC_LINECFG_ADDR,    ((calStart << DRV_ROIC_DELAYCFG_CAL_START ) & DRV_ROIC_DELAYCFG_CAL_START_MASK ) |    // setup calibration start in cc after frame start
                                            ((delay    << DRV_ROIC_DELAYCFG_DELAY_VAL ) & DRV_ROIC_DELAYCFG_DELAY_VAL_MASK ));    // setup data start after line start in cc
}

void DrvRoicStart(u8 noDataLines, u8 dataTsh, u8 calTsh)
{
         setRoicGpio();

         SET_REG_WORD(ROIC_DMATH_ADDR, ((dataTsh << DRV_ROIC_DMATH_DATA_TRESHOLD) & DRV_ROIC_DMATH_DATA_TRESHOLD_MASK) | 
                                       ((calTsh  << DRV_ROIC_DMATH_CAL_TRESHOLD ) & DRV_ROIC_DMATH_CAL_TRESHOLD_MASK ));
                                       
                                       
         // enable irqs                              
         SET_REG_WORD(ROIC_CFGIRQ_ADDR,    DRV_ROIC_IRQ_CAL_FIFO_EMPTY 
                                         | DRV_ROIC_IRQ_DATA_FIFO_FULL 
                                         | DRV_ROIC_IRQ_FRAME_START    
                                         | DRV_ROIC_IRQ_LINE_START      
                                         | DRV_ROIC_IRQ_CAL_FIFO_UNDER 
                                         | DRV_ROIC_IRQ_DATA_FIFO_UNDER
                                         | DRV_ROIC_IRQ_PREAMBLE2_ERR  
                                         | DRV_ROIC_IRQ_PREAMBLE1_ERR  
                                         | DRV_ROIC_IRQ_EOF);
         
         SET_REG_WORD(ROIC_CLRIRQ_ADDR,     DRV_ROIC_IRQ_CAL_FIFO_EMPTY 
                                         | DRV_ROIC_IRQ_DATA_FIFO_FULL 
                                         | DRV_ROIC_IRQ_FRAME_START    
                                         | DRV_ROIC_IRQ_LINE_START      
                                         | DRV_ROIC_IRQ_CAL_FIFO_UNDER 
                                         | DRV_ROIC_IRQ_DATA_FIFO_UNDER
                                         | DRV_ROIC_IRQ_PREAMBLE2_ERR  
                                         | DRV_ROIC_IRQ_PREAMBLE1_ERR  
                                         | DRV_ROIC_IRQ_EOF);
                                    
         // enable controller
         SET_REG_WORD(ROIC_ENABLE_ADDR, ((noDataLines == DRV_ROIC_2_DATA_LINE) ? (DRV_ROIC_ENABLE_EN | DRV_ROIC_ENABLE_DAT2_EN) : DRV_ROIC_ENABLE_EN)); 
}







