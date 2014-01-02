///   
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved. 
///            For License Warranty see: common/license.txt   
///
/// @brief     Driver for GRETH GBIT module
/// 
/// 
/// 


// 1: Includes
// ----------------------------------------------------------------------------

#include "registersMyriad.h"
#include "mv_types.h"
#include "stdio.h"
#include "DrvEth.h"
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

void DrvEthSetDescriptor(u32 Length, u32 CtrlWord, u32 AddressOfDescriptor, u32 AddressPointedByDescriptor)
{
     *((u32 *)AddressOfDescriptor)        = (Length & DRV_ETH_TX_DESC_LENGTH_MASK) | (CtrlWord & (~DRV_ETH_TX_DESC_LENGTH_MASK));
     *((u32 *)(AddressOfDescriptor + 4))  = AddressPointedByDescriptor;
}

void DrvEthSetMAC(u32 macHi, u32 macLo)
{
     SET_REG_WORD(DRV_ETH_MAC_MSB, macHi);
     SET_REG_WORD(DRV_ETH_MAC_LSB, macLo);     
}

void DrvEthInit()
{
     // reset 
     SET_REG_WORD(DRV_ETH_CTRL, DRV_ETH_CTRL_RS);
     // wait while the ctrl resets itself 
     while ((GET_REG_WORD_VAL(DRV_ETH_CTRL) & DRV_ETH_CTRL_RS) == DRV_ETH_CTRL_RS) ; 
  
     SET_REG_WORD(DRV_ETH_CTRL, DRV_ETH_CTRL_TI      // tx int enable 
                                | DRV_ETH_CTRL_RI    // rx int enable 
                                | DRV_ETH_CTRL_FD    // full duplex
                                | DRV_ETH_CTRL_PR    // promiscuous mode enabled       
                                | DRV_ETH_CTRL_PI  );  // enable PHY int 
                                
     SET_REG_WORD(DRV_ETH_STAT, 0xFFFFFFFF); // this should clear every bit and also make the sim not see XXX
}

void DrvEthDescriptorInit(u32 CtrlWord, u32 DescTxPointer, u32 DescRxPointer)
{
     SET_REG_WORD(DRV_ETH_TX_DESC, DescTxPointer);
     SET_REG_WORD(DRV_ETH_RX_DESC, DescRxPointer);          
     SET_REG_WORD(DRV_ETH_CTRL, GET_REG_WORD_VAL(DRV_ETH_CTRL) | CtrlWord);     
}


        
u32 DrvEthMdioCom(u32 data, u8 phyAddr, u8 regAddr, u8 WrNRd)
{
       u32 retVal;
       
       retVal = 0;
       // wait while the MDIO is busy
       while ((GET_REG_WORD_VAL(DRV_ETH_MDIO_CTRL) & DRV_ETH_MDIO_CS_BU) == DRV_ETH_MDIO_CS_BU);

       // write the data in the proper addr reg
       SET_REG_WORD(DRV_ETH_MDIO_CTRL, ((data   <<   DRV_ETH_MDIO_CS_DATA_POS) & DRV_ETH_MDIO_CS_DATA_MASK)    |
                                       ((phyAddr<<DRV_ETH_MDIO_CS_PHYADDR_POS) & DRV_ETH_MDIO_CS_PHYADDR_MASK) |
                                       ((regAddr<<DRV_ETH_MDIO_CS_REGADDR_POS) & DRV_ETH_MDIO_CS_REGADDR_MASK) |
                                                                                  (WrNRd & DRV_ETH_MDIO_CS_WR) | 
                                                                            ((~WrNRd << 1) & DRV_ETH_MDIO_CS_RD) );

       // wait to complete the operation                                       
       while ((GET_REG_WORD_VAL(DRV_ETH_MDIO_CTRL) & DRV_ETH_MDIO_CS_BU) == DRV_ETH_MDIO_CS_BU);                                       
       
       if (!WrNRd) 
       {
               retVal = GET_REG_WORD_VAL(DRV_ETH_MDIO_CTRL);
       }

       return retVal;
}

