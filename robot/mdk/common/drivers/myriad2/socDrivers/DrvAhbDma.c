///   
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved. 
///            For License Warranty see: common/license.txt   
///
/// @brief     AHB DMA driver Implementation
/// 
/// Low level driver controlling the functions of the Myriad 2
/// AHB DMA Block
/// 


// 1: Includes
// ----------------------------------------------------------------------------
#include <mv_types.h>
#include <registersMyriad.h>
#include "DrvAhbDma.h"
#include "DrvAhbDmaDefines.h"
#include "assert.h"

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

void DrvAhbDmaByteCopy(u32 ch, u32 dst, u32 src, u32 len)
{
  //   1 : clears interrupt
 
    SET_REG_WORD(DRV_AHB_DMA_CLEARTFR,      0xFFFFFFFF);
    SET_REG_WORD(DRV_AHB_DMA_CLEARBLOCK,    0xFFFFFFFF);
    SET_REG_WORD(DRV_AHB_DMA_CLEARSRCTRAN,  0xFFFFFFFF);
    SET_REG_WORD(DRV_AHB_DMA_CLEARDSTTRAN,  0xFFFFFFFF);
    SET_REG_WORD(DRV_AHB_DMA_CLEARERR,      0xFFFFFFFF);

  //=======================================================
  //3  ) Program a channel:
  //3.a) Write the starting source address in the SARx
    SET_REG_WORD(DRV_AHB_DMA_SAR0 + ch * 0x58, (u32)src);   
  //3.b) Write the destination address in the DARx
    SET_REG_WORD(DRV_AHB_DMA_DAR0 + ch * 0x58, (u32)dst);   
  //3.c) Program the LLPx register with 0
    SET_REG_WORD(DRV_AHB_DMA_LLP0 + ch * 0x58, 0x00000000); 
    
  //3.d) Program the CTLx register (it's a 64bit reg, so gets written in 2 steps)
  // Note1: the dma has a single master interface, so both SRC and DST master select must be 0 below (DMS, SMS)
  // Note2: the dma has 32bit bus, so must set DST/SRC_TR_WIDTH = 2
    SET_REG_WORD(DRV_AHB_DMA_CTL0 + ch * 0x58,   DRV_AHB_DMA_CTL_INT_EN           //INT_EN
                                               | DRV_AHB_DMA_CTL_DST_TR_WIDTH_8  //DST_TR_WIDTH => 8bits (pg.161)
                                               | DRV_AHB_DMA_CTL_SRC_TR_WIDTH_8  //SRC_TR_WIDTH => 8bits
                                               | DRV_AHB_DMA_CTL_DINC_INC         //DST_INC = 0  => increment 
                                               | DRV_AHB_DMA_CTL_SINC_INC         //SRC_INC = 0  => increment
                                               | DRV_AHB_DMA_CTL_DEST_MSIZE_1     //DEST_MSIZE (Dest Burst Transaction Len), [DON'T care] for mem
                                               | DRV_AHB_DMA_CTL_SRC_MSIZE_1      //(Src  Burst Transaction Len), [DON'T care] for mem 
                                               | DRV_AHB_DMA_CTL_TT_FC_D_M2M      //TT_FC (transfer type & flow control) 0 => Memory to Memory
                                               | DRV_AHB_DMA_CTL_DMS_1 //DMS (Dest Master Select)
                                               | DRV_AHB_DMA_CTL_SMS_1 //SMS (Source Master Select)
                ); 
    SET_REG_WORD(DRV_AHB_DMA_CTL0 + ch * 0x58 + 4, len); //BLOCK_TS
  
  //3.e) Configuration information into CFGx
   SET_REG_WORD(DRV_AHB_DMA_CFG0 + ch * 0x58,    DRV_AHB_DMA_CFG_CH_PRIOR_7 //CH_PRIOR (7:highest, 0:lowest)
                                               | DRV_AHB_DMA_CFG_HS_SEL_DST_SW //Dst handshacking select                 [DON'T care] for mem
                                               | DRV_AHB_DMA_CFG_HS_SEL_SRC_SW //Src handshacking select                 [DON'T care] for mem

                );
    SET_REG_WORD(DRV_AHB_DMA_CFG0 + ch * 0x58 + 4, 0); //
                
  //4) Ensure that bit0 of the DmaCfgReg register is enabled before writing to ChEnReg:
   SET_REG_WORD(DRV_AHB_DMA_DMACFGREG,  DRV_AHB_DMA_DMACFGREG_EN);//only bit0 matters (must be 1)
   SET_REG_WORD(DRV_AHB_DMA_CHENREG,    (1<<(ch  )) //CH_EN
                                       |(1<<(ch+8)) //CH_enable write enable
                );
    
  //6) Pool for transfer done:
  //   "Once the transfer completes, hardware sets the interrupts and disables the channel"
   while((GET_REG_WORD_VAL(DRV_AHB_DMA_RAWTFR) & (1<<ch)) == 0)
          ;
}


void DrvAhbDmaTransfer(u32 ch, u32 dst, u32 src, u32 len, u32 cfg, u32 enAndWait)
{
  //   1 : clears interrupt
 
    SET_REG_WORD(DRV_AHB_DMA_CLEARTFR,      0xFFFFFFFF);
    SET_REG_WORD(DRV_AHB_DMA_CLEARBLOCK,    0xFFFFFFFF);
    SET_REG_WORD(DRV_AHB_DMA_CLEARSRCTRAN,  0xFFFFFFFF);
    SET_REG_WORD(DRV_AHB_DMA_CLEARDSTTRAN,  0xFFFFFFFF);
    SET_REG_WORD(DRV_AHB_DMA_CLEARERR,      0xFFFFFFFF);

  //=======================================================
  //3  ) Program a channel:
  //3.a) Write the starting source address in the SARx
    SET_REG_WORD(DRV_AHB_DMA_SAR0 + ch * 0x58, (u32)src);   
  //3.b) Write the destination address in the DARx
    SET_REG_WORD(DRV_AHB_DMA_DAR0 + ch * 0x58, (u32)dst);   
  //3.c) Program the LLPx register with 0
    SET_REG_WORD(DRV_AHB_DMA_LLP0 + ch * 0x58, 0x00000000); 
    
  //3.d) Program the CTLx register (it's a 64bit reg, so gets written in 2 steps)
  // Note1: the dma has a single master interface, so both SRC and DST master select must be 0 below (DMS, SMS)
  // Note2: the dma has 32bit bus, so must set DST/SRC_TR_WIDTH = 2
    SET_REG_WORD(DRV_AHB_DMA_CTL0 + ch * 0x58, cfg);

    SET_REG_WORD(DRV_AHB_DMA_CTL0 + ch * 0x58 + 4, len); //BLOCK_TS
  
  //3.e) Configuration information into CFGx
   SET_REG_WORD(DRV_AHB_DMA_CFG0 + ch * 0x58,    DRV_AHB_DMA_CFG_CH_PRIOR_7 //CH_PRIOR (7:highest, 0:lowest)
                                               | DRV_AHB_DMA_CFG_HS_SEL_DST_SW //Dst handshacking select                 [DON'T care] for mem
                                               | DRV_AHB_DMA_CFG_HS_SEL_SRC_SW //Src handshacking select                 [DON'T care] for mem

                );
    SET_REG_WORD(DRV_AHB_DMA_CFG0 + ch * 0x58 + 4, 0); //
                
  //4) Ensure that bit0 of the DmaCfgReg register is enabled before writing to ChEnReg:
   SET_REG_WORD(DRV_AHB_DMA_DMACFGREG,  DRV_AHB_DMA_DMACFGREG_EN);//only bit0 matters (must be 1)
   
   
   // enable 
   if ((enAndWait == DRV_AHB_DMA_START_WAIT) || (enAndWait == DRV_AHB_DMA_START_NO_WAIT))
   {
         SET_REG_WORD(DRV_AHB_DMA_CHENREG,    (1<<(ch  ))      //CH_EN
                                             |(1<<(ch+8)));    //CH_enable write enable
                     
    
         //6) Pool for transfer done:
         //   "Once the transfer completes, hardware sets the interrupts and disables the channel"
         if (enAndWait == DRV_AHB_DMA_START_WAIT)
                  while((GET_REG_WORD_VAL(DRV_AHB_DMA_RAWTFR) & (1<<ch)) == 0)
                         ;
    }
}



void DrvAhbDmaTransferHw(u32 ch, u32 dst, u32 src, u32 len, u32 ctl0, u32 cfg0, u32 cfg1, u32 enAndWait)
{
  //   1 : clears interrupt
 
    SET_REG_WORD(DRV_AHB_DMA_CLEARTFR,      0xFFFFFFFF);
    SET_REG_WORD(DRV_AHB_DMA_CLEARBLOCK,    0xFFFFFFFF);
    SET_REG_WORD(DRV_AHB_DMA_CLEARSRCTRAN,  0xFFFFFFFF);
    SET_REG_WORD(DRV_AHB_DMA_CLEARDSTTRAN,  0xFFFFFFFF);
    SET_REG_WORD(DRV_AHB_DMA_CLEARERR,      0xFFFFFFFF);

  //=======================================================
  //3  ) Program a channel:
  //3.a) Write the starting source address in the SARx
    SET_REG_WORD(DRV_AHB_DMA_SAR0 + ch * 0x58, (u32)src);   
  //3.b) Write the destination address in the DARx
    SET_REG_WORD(DRV_AHB_DMA_DAR0 + ch * 0x58, (u32)dst);   
  //3.c) Program the LLPx register with 0
    SET_REG_WORD(DRV_AHB_DMA_LLP0 + ch * 0x58, 0x00000000); 
    
  //3.d) Program the CTLx register (it's a 64bit reg, so gets written in 2 steps)
  // Note1: the dma has a single master interface, so both SRC and DST master select must be 0 below (DMS, SMS)
  // Note2: the dma has 32bit bus, so must set DST/SRC_TR_WIDTH = 2
    SET_REG_WORD(DRV_AHB_DMA_CTL0 + ch * 0x58, ctl0);

    SET_REG_WORD(DRV_AHB_DMA_CTL0 + ch * 0x58 + 4, len); //BLOCK_TS
  
  //3.e) Configuration information into CFGx
    SET_REG_WORD(DRV_AHB_DMA_CFG0 + ch * 0x58,     cfg0);
    SET_REG_WORD(DRV_AHB_DMA_CFG0 + ch * 0x58 + 4, cfg1); //
                
  //4) Ensure that bit0 of the DmaCfgReg register is enabled before writing to ChEnReg:
   SET_REG_WORD(DRV_AHB_DMA_DMACFGREG,  DRV_AHB_DMA_DMACFGREG_EN);//only bit0 matters (must be 1)
   
   
   // enable 
   if ((enAndWait == DRV_AHB_DMA_START_WAIT) || (enAndWait == DRV_AHB_DMA_START_NO_WAIT))
   {
         SET_REG_WORD(DRV_AHB_DMA_CHENREG,    (1<<(ch  ))      //CH_EN
                                             |(1<<(ch+8)));    //CH_enable write enable, without writing this bit the enable bit is not changed, there is no need to read before write
                     
    
         //6) Pool for transfer done:
         //   "Once the transfer completes, hardware sets the interrupts and disables the channel"
         if (enAndWait == DRV_AHB_DMA_START_WAIT)
                  while((GET_REG_WORD_VAL(DRV_AHB_DMA_RAWTFR) & (1<<ch)) == 0)
                         ;
    }
}
