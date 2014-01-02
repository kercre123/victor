///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt              
///
/// @brief     Definitions and types needed by ETH GRETH GBIT Module
/// 
#ifndef DRV_ETH_DEF_H
#define DRV_ETH_DEF_H 

#include "registersMyriad.h"

#define DRV_ETH_CTRL              ( GETH_BASE_ADR +  0x00)
#define DRV_ETH_STAT              ( GETH_BASE_ADR +  0x04)
#define DRV_ETH_MAC_MSB           ( GETH_BASE_ADR +  0x08)
#define DRV_ETH_MAC_LSB           ( GETH_BASE_ADR +  0x0C)
#define DRV_ETH_MDIO_CTRL         ( GETH_BASE_ADR +  0x10)
#define DRV_ETH_TX_DESC           ( GETH_BASE_ADR +  0x14)
#define DRV_ETH_RX_DESC           ( GETH_BASE_ADR +  0x18)
#define DRV_ETH_EDCL_IP           ( GETH_BASE_ADR +  0x1c)
#define DRV_ETH_HASH_TABLE_MSB    ( GETH_BASE_ADR +  0x20)
#define DRV_ETH_HASH_TABLE_LSB    ( GETH_BASE_ADR +  0x24)
#define DRV_ETH_EDCL_MAC_MSB      ( GETH_BASE_ADR +  0x28)
#define DRV_ETH_EDCL_MAC_LSB      ( GETH_BASE_ADR +  0x2c)


// CTRL Reg - adr 0 ============================================================================
#define DRV_ETH_CTRL_TE  (1<<0)    // Transmit enable 
#define DRV_ETH_CTRL_RE  (1<<1)    // Receive enable 
#define DRV_ETH_CTRL_TI  (1<<2)    // Transmitter interrupt 
#define DRV_ETH_CTRL_RI  (1<<3)    // Receiver interrupt 
#define DRV_ETH_CTRL_FD  (1<<4)    // Full Duplex
#define DRV_ETH_CTRL_PR  (1<<5)    // Promiscuous mode 
#define DRV_ETH_CTRL_RS  (1<<6)    // Reset
#define DRV_ETH_CTRL_SP  (1<<7)    // Speed
#define DRV_ETH_CTRL_GB  (1<<8)    // Gigabit 
#define DRV_ETH_CTRL_BM  (1<<9)    // Burstmode
#define DRV_ETH_CTRL_PI  (1<<10)   // Enable interrupts for detected PHY status changes 
#define DRV_ETH_CTRL_ME  (1<<11)   // Multicast enable 
#define DRV_ETH_CTRL_DD  (1<<12)   // Disable duplex detection 
#define DRV_ETH_CTRL_RD  (1<<13)   // RAM debug enable 
#define DRV_ETH_CTRL_ED_DIS  (1<<14)   // EDCL disable 
// 15 - 24 reserved 
#define DRV_ETH_CTRL_MC  (1<<25)   // Multicast available 
#define DRV_ETH_CTRL_MA  (1<<26)   // MDIO interrupts enabled 
#define DRV_ETH_CTRL_GA  (1<<27)   // Gigabit MAC available 
#define DRV_ETH_CTRL_BS  (7<<28)   // EDCL buffer size 
#define DRV_ETH_CTRL_ED  (1<<31)   // EDCL available 



// STATUS register ============================================================================
#define DRV_ETH_STAT_RE  (1<<0)      //  Receiver error 
#define DRV_ETH_STAT_TE  (1<<1)      //  Transmitter error 
#define DRV_ETH_STAT_RI  (1<<2)      //  Receive successful
#define DRV_ETH_STAT_TI  (1<<3)      //  Transmit successful
#define DRV_ETH_STAT_RA  (1<<4)      //  Receiver AHB error
#define DRV_ETH_STAT_TA  (1<<5)      //  Transmitter AHB error
#define DRV_ETH_STAT_TS  (1<<6)      //  Too small
#define DRV_ETH_STAT_IA  (1<<7)      //  Invalid address
#define DRV_ETH_STAT_PS  (1<<8)      //  PHY status changes 

// MDIO status/control ============================================================================
#define DRV_ETH_MDIO_CS_WR (1<<0)    // Write
#define DRV_ETH_MDIO_CS_RD (1<<1)    // Read
#define DRV_ETH_MDIO_CS_LF (1<<2)    // Link fail
#define DRV_ETH_MDIO_CS_BU (1<<3)    // Busy
#define DRV_ETH_MDIO_CS_NV (1<<4)    // Not valid 
// bit 5 reserved
#define DRV_ETH_MDIO_CS_REGADDR_MASK (0x1F<<6)   // Register address mask
#define DRV_ETH_MDIO_CS_REGADDR_POS  (6)         
#define DRV_ETH_MDIO_CS_PHYADDR_MASK (0x1F<<11) // PHY address mask
#define DRV_ETH_MDIO_CS_PHYADDR_POS  (11)
#define DRV_ETH_MDIO_CS_DATA_MASK (0xFFFF<<16)   // Data being written or read form the PHY
#define DRV_ETH_MDIO_CS_DATA_POS  (16)


// Receiver & Transmitter descriptor table register ============================================================================
#define DRV_ETH_TX_DESCRIPTOR_DESCPNT_MASK  (0xFF<<3)
#define DRV_ETH_TX_DESCRIPTOR_DESCPNT_POS   (3)
#define DRV_ETH_TX_DESCRIPTOR_BASEADDR_MASK  (0x3FFFFF<<10)
#define DRV_ETH_TX_DESCRIPTOR_BASEADDR_POS   (10)

// TX Descriptor defines ============================================================================
#define DRV_ETH_TX_DESC_LENGTH_MASK     (0x7FF<<0)  
#define DRV_ETH_TX_DESC_LENGTH_POS      (0)  
#define DRV_ETH_TX_DESC_EN              (1<<11) // enable  
#define DRV_ETH_TX_DESC_WR              (1<<12) // Wrap to zero
#define DRV_ETH_TX_DESC_IE              (1<<13) // enable interrupt 
#define DRV_ETH_TX_DESC_UE              (1<<14) // underrun error
#define DRV_ETH_TX_DESC_AL              (1<<15) // maximum number of attempts reached 
#define DRV_ETH_TX_DESC_LC              (1<<16) // late colision
#define DRV_ETH_TX_DESC_MO              (1<<17) // more descriptors
#define DRV_ETH_TX_DESC_IC              (1<<18) // calculate and insert the IP header check sum 
#define DRV_ETH_TX_DESC_TC              (1<<19) // calculate and insert the TCP 
#define DRV_ETH_TX_DESC_UC              (1<<20) // udp check sum 


// RX Descriptor defines ============================================================================
#define DRV_ETH_RX_DESC_LENGTH_MASK     (0x7FF<<0)  
#define DRV_ETH_RX_DESC_LENGTH_POS      (0)  
#define DRV_ETH_RX_DESC_EN              (1<<11) // enable 
#define DRV_ETH_RX_DESC_WR              (1<<12) // Wrap to zero 
#define DRV_ETH_RX_DESC_IE              (1<<13) // enable interrupt 
#define DRV_ETH_RX_DESC_AE              (1<<14) // alignment error 
#define DRV_ETH_RX_DESC_FT              (1<<15) // frame too long 
#define DRV_ETH_RX_DESC_CE              (1<<16) // crc error 
#define DRV_ETH_RX_DESC_OE              (1<<17) // fifo overrun error 
#define DRV_ETH_RX_DESC_LE              (1<<18) // length error 
#define DRV_ETH_RX_DESC_ID              (1<<19) // IP packet detected   
#define DRV_ETH_RX_DESC_IR              (1<<20) // IP cheksum error detected 
#define DRV_ETH_RX_DESC_UD              (1<<21) // UDP packet detected 
#define DRV_ETH_RX_DESC_UR              (1<<22) // UDP checksum error detected 
#define DRV_ETH_RX_DESC_TD              (1<<23) // TCP packet detected 
#define DRV_ETH_RX_DESC_TR              (1<<24) // TCP checksum error 
#define DRV_ETH_RX_DESC_IF              (1<<25) // fragmented packet
#define DRV_ETH_RX_DESC_MC              (1<<26) // multicast address






#endif // DRV_ETH_DEF_H