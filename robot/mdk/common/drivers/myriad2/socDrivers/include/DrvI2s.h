///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt   
///
/// @brief     Low-level driver for I2S interface
/// 
/// 
/// 

#ifndef _DRV_I2S_H_
#define _DRV_I2S_H_

#include "mv_types.h"
#include "DrvI2sDefines.h"

/* ***********************************************************************//**
I2S - configure channel 0 of a i2s module
@{
----------------------------------------------------------------------------
@}
@param
      base  - base address of the i2s module, one of the following could be used: I2S1_BASE_ADR, I2S2_BASE_ADR, I2S3_BASE_ADR
@param
      channels - bit fields that specify which channels will be enabled.
               - bit[0] = 1 enable channel 0, bit[0] = 0 channel 0 disabled 
               - bit[1] = 1 enable channel 1, bit[1] = 0 channel 1 disabled 
               - bit[2] = 1 enable channel 2, bit[2] = 0 channel 2 disabled 
@param
      txNotRx - configure channel(s) as rx or tx
@param
      master  - 1 module is master
              - 0 module is slave
              .
@param
      clkCfg - word to be written in the I2S_TFCR or I2S_RFCR registers
              - defines in the DrvI2sDefines.h can be used for this
              - this sets the number of cc per word and the clock gating
              .
@param
      fifoLvl - fifo level at which the interrupt is triggered
@param
      lenCfg - word length
              - defines in the DrvI2sDefines.h can be used for this
              .
@param
      intMask - interrupt masks
@param
      dmaAddr    - system address where data is stored 
@param      
      dmaCh - AHB DMA channel to be used   
@param      
      dmaL - number of bytes to be transferd by the DMA
             Ex: if fifo trigger level is 8 (7+1) then this should be 
             32 bytes for 1 ch 
             64 for 2 ch
             96 for 3 ch
             
@return
@{
info:

@}
 ************************************************************************* */
void DrvI2sInit(u32 base, u32 channels  ,u32 txNotRx,
                                         u32 master,
                                         u32 clkCfg,
                                         u32 fifoLvl,
                                         u32 lenCfg,
                                         u32 intMask, 
                                         u32 dmaAddr, 
                                         u32 dmaCh, 
                                         u32 dmaL);



#endif // _DRV_I2S_H_

