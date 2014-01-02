///   
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved. 
///            For License Warranty see: common/license.txt   
///
/// @brief     Driver for I2S module
/// 
/// 
/// 


// 1: Includes
// ----------------------------------------------------------------------------

#include "registersMyriad.h"
#include "mv_types.h"
#include "stdio.h"
#include "DrvI2s.h"
#include "DrvAhbDma.h"

// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------

// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------

// 4: Static Local Data 
// ----------------------------------------------------------------------------

// 5: Static Function Prototypes
// ----------------------------------------------------------------------------

// 6: Functions Implementation
// ----------------------------------------------------------------------------






/* ***********************************************************************//**
   *************************************************************************
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
                                         u32 dmaL)
{
      u32 i2sModule;
      i2sModule = ((base - LPB1_CONTROL_ADR) >> 16) & 0x0F;
      if (master)
         switch (i2sModule)
         {
           case 0x0B: SET_REG_WORD(CPR_GEN_CTRL_ADR, GET_REG_WORD_VAL(CPR_GEN_CTRL_ADR) | 0x01); break;
           case 0x0C: SET_REG_WORD(CPR_GEN_CTRL_ADR, GET_REG_WORD_VAL(CPR_GEN_CTRL_ADR) | 0x02); break;
           case 0x0D: SET_REG_WORD(CPR_GEN_CTRL_ADR, GET_REG_WORD_VAL(CPR_GEN_CTRL_ADR) | 0x04); break;
           default:
            break;
         }


      SET_REG_WORD(base + I2S_IER   , 1);        // enable I2S module
      SET_REG_WORD(base + I2S_CER   , 0);        // disable clock generation
      SET_REG_WORD(base + I2S_CCR   , clkCfg);  //clock configuration

      SET_REG_WORD(base + I2S_ITER  , 0);  // disable tx block
      
      SET_REG_WORD(base + I2S_TER0  , 0);  // transmit disable, all are enabled after reset 
      SET_REG_WORD(base + I2S_TER1  , 0);  
      SET_REG_WORD(base + I2S_TER2  , 0);  

      SET_REG_WORD(base + I2S_IRER  , 0);  // disable rx block
      SET_REG_WORD(base + I2S_RER0  , 0);  // receiver disable
      SET_REG_WORD(base + I2S_RER1  , 0);  // receiver disable
      SET_REG_WORD(base + I2S_RER2  , 0);  // receiver disable      

      // mask interrupts, only for the first enabled channel, 
      // the rest channles will get data on the irq of the first enabled channel, 
      // since the same clock is driving all channels 
      if ((channels & D_I2S_CH0) == D_I2S_CH0)
             SET_REG_WORD(base + I2S_IMR0  , intMask); 
      else    
             if ((channels & D_I2S_CH1) == D_I2S_CH1)  
                    SET_REG_WORD(base + I2S_IMR1  , intMask); 
             else
                    if ((channels & D_I2S_CH2) == D_I2S_CH2)           
                           SET_REG_WORD(base + I2S_IMR2  , intMask);              

      if (txNotRx)  // set channels for tx
      {
           if ((channels & D_I2S_CH0) == D_I2S_CH0) 
           {
                SET_REG_WORD(base + I2S_TFCR0 , fifoLvl); // transmit fifo lvl
                SET_REG_WORD(base + I2S_TCR0  , lenCfg);  // data length
           }
           if ((channels & D_I2S_CH1) == D_I2S_CH1) 
           {
                SET_REG_WORD(base + I2S_TFCR1 , fifoLvl); // transmit fifo lvl
                SET_REG_WORD(base + I2S_TCR1  , lenCfg);  // data length
           }
           if ((channels & D_I2S_CH2) == D_I2S_CH2) 
           {
                SET_REG_WORD(base + I2S_TFCR2 , fifoLvl); // transmit fifo lvl
                SET_REG_WORD(base + I2S_TCR2  , lenCfg);  // data length
           }
           
           SET_REG_WORD(base + I2S_TXFFR , 1);       // flush the tx fifo
           SET_REG_WORD(base + I2S_ITER  , 1);       // enable tx block
           SET_REG_WORD(base + I2S_IRER  , 1);       // enable rx block
           
                   
           if ((channels & D_I2S_CH0) == D_I2S_CH0)            
               SET_REG_WORD(base + I2S_TER0  , 1);       // enable transmition for ch 0
           if ((channels & D_I2S_CH1) == D_I2S_CH1)            
               SET_REG_WORD(base + I2S_TER1  , 1);       // enable transmition
           if ((channels & D_I2S_CH2) == D_I2S_CH2)            
               SET_REG_WORD(base + I2S_TER2  , 1);       // enable transmition
               
           // fill 1/2 of the fifo by starting the DMA
           DrvAhbDmaTransfer(dmaCh, base + I2S_TXDMA, (u32)dmaAddr, dmaL, D_AHB_DMA_MEM_HWORD_TO_PER_HWORD, DRV_AHB_DMA_START_WAIT);     // configure the DMA
           // start a second transfer to fill the buffer 
           SET_REG_WORD(DRV_AHB_DMA_CHENREG,  (1<<dmaCh) | (1<<(dmaCh+8))); // restart the DMA
            
           //while((GET_REG_WORD_VAL(DRV_AHB_DMA_RAWTFR) & (1<<dmaCh)) == 0); // wait for the transfer before enabling clock generation                  
               
      }
      else            // set channel for rx
      {
           if ((channels & D_I2S_CH0) == D_I2S_CH0)                  
           {
                 SET_REG_WORD(base + I2S_RFCR0 , fifoLvl);  // fifo length triger (triggered on value +1)
                 SET_REG_WORD(base + I2S_RCR0  , lenCfg);   // data length
           }
           if ((channels & D_I2S_CH1) == D_I2S_CH1)                  
           {
                 SET_REG_WORD(base + I2S_RFCR1 , fifoLvl);  // fifo length triger (triggered on value +1)
                 SET_REG_WORD(base + I2S_RCR1  , lenCfg);   // data length
           }
           if ((channels & D_I2S_CH2) == D_I2S_CH2)                  
           {
                 SET_REG_WORD(base + I2S_RFCR2 , fifoLvl);  // fifo length triger (triggered on value +1)
                 SET_REG_WORD(base + I2S_RCR2  , lenCfg);   // data length
           }
           
           SET_REG_WORD(base + I2S_RXFFR , 1);        // flush the tx fifo
           SET_REG_WORD(base + I2S_IRER  , 1);        // enable rx block
           SET_REG_WORD(base + I2S_ITER  , 1);        // enable tx block
           if ((channels & D_I2S_CH0) == D_I2S_CH0)                             
                 SET_REG_WORD(base + I2S_RER0  , 1);      // enable receiver
           if ((channels & D_I2S_CH1) == D_I2S_CH1)                             
                 SET_REG_WORD(base + I2S_RER1  , 1);      // enable receiver
           if ((channels & D_I2S_CH2) == D_I2S_CH2)                             
                 SET_REG_WORD(base + I2S_RER2  , 1);      // enable receiver

           // configure the DMA, but don't start it                                 
           // dma will be started in an RX interrupt later
           DrvAhbDmaTransfer(dmaCh, (u32)dmaAddr, base + I2S_RXDMA, dmaL, D_AHB_DMA_PER_HWORD_TO_MEM_HWORD, DRV_AHB_DMA_NO_START);     
      }

      if (master)
         SET_REG_WORD(base + I2S_CER   , 1);              // enable clock generation
}




void DrvI2sInitCh0RxTx(u32 base,         u32 master,
                                         u32 clkCfg,
                                         u32 fifoLvl,
                                         u32 lenCfg,
                                         u32 intMask)
{
      u32 i2sModule;
      i2sModule = ((base - LPB1_CONTROL_ADR) >> 16) & 0x0F;
      if (master)
         switch (i2sModule)
         {
           case 0x0B: SET_REG_WORD(CPR_GEN_CTRL_ADR, GET_REG_WORD_VAL(CPR_GEN_CTRL_ADR) | 0x01); break;
           case 0x0C: SET_REG_WORD(CPR_GEN_CTRL_ADR, GET_REG_WORD_VAL(CPR_GEN_CTRL_ADR) | 0x02); break;
           case 0x0D: SET_REG_WORD(CPR_GEN_CTRL_ADR, GET_REG_WORD_VAL(CPR_GEN_CTRL_ADR) | 0x04); break;
           default:
            break;
         }


      SET_REG_WORD(base + I2S_IER   , 1);        // enable I2S module
      SET_REG_WORD(base + I2S_CER   , 0);        // disable clock generation
      SET_REG_WORD(base + I2S_CCR   , clkCfg);  //clock configuration

      SET_REG_WORD(base + I2S_ITER  , 0);  // disable tx block
      SET_REG_WORD(base + I2S_TER0  , 0);  // transmit disable

      SET_REG_WORD(base + I2S_IRER  , 0);  // disable rx block
      SET_REG_WORD(base + I2S_RER0  , 0);  // receiver disable

      SET_REG_WORD(base + I2S_IMR0  , intMask); // mask interrupts

      // set channel for tx
           SET_REG_WORD(base + I2S_TFCR0 , fifoLvl); // transmit fifo lvl
           SET_REG_WORD(base + I2S_TCR0  , lenCfg);  // data length
           SET_REG_WORD(base + I2S_TXFFR , 1);        // flush the tx fifo

      // set channel for rx
           SET_REG_WORD(base + I2S_RFCR0 , fifoLvl);  // fifo length triger (triggered on value +1)
           SET_REG_WORD(base + I2S_RCR0  , lenCfg);   // data length
           SET_REG_WORD(base + I2S_RXFFR , 1);         // flush the tx fifo

      // enable
           SET_REG_WORD(base + I2S_IRER  , 1);         // enable rx block
           SET_REG_WORD(base + I2S_ITER  , 1);         // enable tx block
           SET_REG_WORD(base + I2S_TER0  , 1);         // enable transmition
           SET_REG_WORD(base + I2S_RER0  , 1);         // enable receiver

      if (master)
         SET_REG_WORD(base + I2S_CER   , 1);              // enable clock generation
}

