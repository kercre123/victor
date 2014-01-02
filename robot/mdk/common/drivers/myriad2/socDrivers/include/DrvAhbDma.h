///  
/// @file
/// @copyright All code copyright Movidius Ltd 2013, all rights reserved 
///            For License Warranty see: common/license.txt   
///
/// @brief     AHB DMA Driver API
/// 
/// This module contains the helper functions necessary to control the 
/// AHB DMA block in the Myriad Soc
/// 
/// 

#ifndef DRV_AHB_DMA_H
#define DRV_AHB_DMA_H 

#include "DrvAhbDmaDefines.h"

                                          

/* ***********************************************************************//**
   *************************************************************************
   DrvAhbDmaByteCopy - copy bytes from a memory location to another memory location
@{
----------------------------------------------------------------------------
@}
@param
      ch - dma channel number 
@param
     dst - destiantion address 
@param
     src - source address 
@param
     len - length in bytes 
@return
    void 
@{
info:
@}
 ************************************************************************* */                                               
void DrvAhbDmaByteCopy(u32 ch, u32 dst, u32 src, u32 len);

/* ***********************************************************************//**
   *************************************************************************
   DrvAhbDmaTransfer - copy words from a memory location/peripheral to another memory location/peripheral using software arbitration
@{
----------------------------------------------------------------------------
@}
@param
      ch - dma channel number 
@param
     dst - destiantion address 
@param
     src - source address 
@param
     len - length in words, the word size depends of DMA config
@param
     cfg  - already defined options can be found in DrvAhbDmaDefines.h, value to be written in DRV_AHB_DMA_CTL[X]
@param
     enAndWait - one of the 3 possible values :  DRV_AHB_DMA_START_WAIT, DRV_AHB_DMA_START_NO_WAIT, DRV_AHB_DMA_NO_START
                          DRV_AHB_DMA_START_WAIT : the function is blocking, waits for the DMA to finish transfering
                          DRV_AHB_DMA_START_NO_WAIT: the function starts the transfer but doesn't wait 
                          DRV_AHB_DMA_NO_START : dma is configured but is not started, start is done later in the code on some event
@return
    void 
@{
info:
@}
 ************************************************************************* */                                               
void DrvAhbDmaTransfer(u32 ch, u32 dst, u32 src, u32 len, u32 cfg, u32 enAndWait);


/* ***********************************************************************//**
   *************************************************************************
   DrvAhbDmaTransferHw - copy words from a memory location/peripheral to another memory location/peripheral but lets user set hardware arbitration
@{
----------------------------------------------------------------------------
@}
@param
      ch - dma channel number 
@param
     dst - destiantion address 
@param
     src - source address 
@param
     len - length in words, the word size depends of DMA config
@param
     ctl0  - already defined options can be found in DrvAhbDmaDefines.h, value to be written in DRV_AHB_DMA_CTL[X]
@param
     cfg0 - value to be written in the DRV_AHB_DMA_CFG[0:31][X]  - already defines values for some peripherals can be found in DrvAhbDmaDefines.h
@param
     cfg1 - value to be written in the DRV_AHB_DMA_CFG[32:63][X]  - already defines values for some peripherals can be found in DrvAhbDmaDefines.h
     
@param
     enAndWait - one of the 3 possible values :  DRV_AHB_DMA_START_WAIT, DRV_AHB_DMA_START_NO_WAIT, DRV_AHB_DMA_NO_START
                          DRV_AHB_DMA_START_WAIT : the function is blocking, waits for the DMA to finish transfering
                          DRV_AHB_DMA_START_NO_WAIT: the function starts the transfer but doesn't wait 
                          DRV_AHB_DMA_NO_START : dma is configured but is not started, start is done later in the code on some event
@return
    void 
@{
info:
@}
 ************************************************************************* */      
void DrvAhbDmaTransferHw(u32 ch, u32 dst, u32 src, u32 len, u32 ctl0, u32 cfg0, u32 cfg1, u32 enAndWait);

#endif // DRV_AHB_DMA_H 
