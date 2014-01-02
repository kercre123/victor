/**
 * @file
 * @version		2.3.0.0
 * @date 		2011-November-11
 *
 * Copyright(C) Redpine Signals 2011
 * All rights reserved by Redpine Signals.
 *
 * @section License
 * This program should be used on your own responsibility.
 * Redpine Signals assumes no responsibility for any losses
 * incurred by customers or third parties arising from the use of this file.
 *
 * @brief SPI, BOOTLOADER, Loads the bootloader code through the SPI interface
 *
 * @section Description
 * This file contains the SPI BOOTLOADER function.
 *
 *
 */

/**
 * Includes
 */
#include "rsi_global.h"
#include <stdio.h>
#include <string.h>

/**
 * Global Variables
 */

/*=================================================*/
/**
 * @fn          int16 rsi_bootloader(void)
 * @brief       Loads the boot loader code into Wi-Fi module through SPI interface 
 *              (which inturn brings functional firmware from flash). 
 * @brief       Bootloader loads firmware from flash to the Wi-Fi Module. 
 * @param[in]   none
 * @param[out]  none
 * @return      errCode
 *              -1 = SPI busy / Timeout
 *              -2 = SPI Failure
 *              0  = SUCCESS
 * @section description
 * This API is used to load the bootloader code into Wi-Fi module 
 * at specified locations and bring the module out from soft reset. 
 * The bootloader in turn loads the functional firmware from the 
 * moduleCs non-volatile memory and gives control to functional firmware. 
 * This API has to be called only after the rsi_spi_iface_init API. 
 */
int16 rsi_bootloader(void)
{
    int16 retval;
    int16 i;
    uint8 dBuf[4];
    rsi_uFrameDsc uFrameDscFrame;

    static uint8 sbinst1[] = {
#include "Firmware/sbinst1"
            };
    static uint8 sbinst2[] = {
#include "Firmware/sbinst2"
            };
    static uint8 sbdata1[] = {
#include "Firmware/sbdata1"
            };
#if RSI_LOAD_SBDATA2_FROM_HOST  
    static uint8 sbdata2[] = {
#include "Firmware/sbdata2"
            };
#endif

    // Software Bootload files
    // sbinst1,sbinst2,sbdata1,sbdata2 are software bootload files provided with the release package
#ifdef RSI_DEBUG_PRINT
    RSI_DPRINT(RSI_PL3, "\r\nBootLoader Start ");

    // first we have to put the chip in soft reset??
    RSI_DPRINT(RSI_PL3, "\r\nSet Soft Reset ");
#endif
    for (i = 0; i < sizeof(dBuf); i++)
    {
        dBuf[i] = 0;
    }															// zero out dBuf
    dBuf[0] = RSI_RST_SOFT_SET;
    retval = rsi_memWr(RSI_RST_SOFT_ADDR, sizeof(dBuf), dBuf);	  // 0x74, 0x00, 0x22000004, 0x04, 0x00000001
    if (retval != 0)
    {
#ifdef RSI_DEBUG_PRINT
        RSI_DPRINT(RSI_PL4, "\r\n Soft Reset FAILED %d", retval);
#endif
        return retval;
    }
#ifdef RSI_DEBUG_PRINT
    RSI_DPRINT(RSI_PL3, "\r\nLoading SBINST1, is %d Bytes", sizeof(sbinst1));
#endif
    retval = rsi_memWr(RSI_SBINST1_ADDR, sizeof(sbinst1), sbinst1);		// 0x00000000

    if (retval != 0)
    {
#ifdef RSI_DEBUG_PRINT
        RSI_DPRINT(RSI_PL4, "\r\n sbinst1 memory write FAILED %d", retval);
#endif
        return retval;
    }

#ifdef RSI_DEBUG_PRINT
    RSI_DPRINT(RSI_PL3, "\r\nLoading SBINST2, is %d Bytes", sizeof(sbinst2));
#endif
    // write sbinst2
    retval = rsi_memWr(RSI_SBINST2_ADDR, sizeof(sbinst2), sbinst2);		// 0x02014010

    if (retval != 0)
    {
#ifdef RSI_DEBUG_PRINT
        RSI_DPRINT(RSI_PL4, "\r\n sbinst2 memory write FAILED %d", retval);
#endif
        return retval;
    }

#ifdef RSI_DEBUG_PRINT
    RSI_DPRINT(RSI_PL3, "\r\nLoading SBDATA1, is %d Bytes", sizeof(sbdata1));
#endif
    // write sbdata1

    retval = rsi_memWr(RSI_SBDATA1_ADDR, sizeof(sbdata1), sbdata1);		// 0x20003100

    if (retval != 0)
    {
#ifdef RSI_DEBUG_PRINT
        RSI_DPRINT(RSI_PL4, "\r\n sbdata1 memory write FAILED %d", retval);
#endif
        return retval;
    }

#ifdef RSI_DEBUG_PRINT
    // Take the module out of soft reset
    RSI_DPRINT(RSI_PL3, "\r\nNegate Soft Reset ");
#endif
    for (i = 0; i < sizeof(dBuf); i++)
    {
        dBuf[i] = 0;
    }															// zero out dBuf
    dBuf[0] = RSI_RST_SOFT_CLR;
    retval = rsi_memWr(RSI_RST_SOFT_ADDR, 0x0004, dBuf);	// 0x74, 0x00, 0x22000004, 0x04, 0x00000000

    if (retval != 0)
    {
#ifdef RSI_DEBUG_PRINT
        RSI_DPRINT(RSI_PL4, "\r\n Bring module out of reset FAILED %d", retval);
#endif
        return retval;
    }

    rsi_delayMs(100);
#ifdef RSI_DEBUG_PRINT
    // Check for a card ready
    RSI_DPRINT(RSI_PL3, "\r\nWait for Card Ready ");

    RSI_DPRINT(RSI_PL3, "\r\n Read Card Ready Response ");
#endif
    // Read in the Frame Descriptor
    retval = rsi_SpiFrameDscRd(&uFrameDscFrame);
    if (retval != 0)
    {
#ifdef RSI_DEBUG_PRINT
        RSI_DPRINT(RSI_PL4, "\r\n Card ready frame descriptor read failed ");
#endif
        return retval;
    }
    // read in the frame descriptor
    if (uFrameDscFrame.uFrmDscBuf[1] != RSI_RSP_CARD_READY)
    {	     // make sure we received the correct frame type
#ifdef RSI_DEBUG_PRINT    
        RSI_DPRINT(RSI_PL4, "\r\nCard (NOT) Ready=%02x ", (uint16 )uFrameDscFrame.uFrmDscBuf[1]);
#endif
        return RSI_BUSY;														// we have an error
    }

#if RSI_LOAD_SBDATA2_FROM_HOST	
    // write sbdata2
    retval = rsi_memWr(RSI_SBDATA2_ADDR, sizeof(sbdata2), sbdata2);		// 0x02010000

    if (retval != 0)
    {
#ifdef RSI_DEBUG_PRINT
        RSI_DPRINT(RSI_PL4, "\r\n sbdata2 memory write FAILED %d", retval);
#endif
        return retval;
    }
#endif
#ifdef RSI_DEBUG_PRINT
    RSI_DPRINT(RSI_PL3, "\r\nBootLoader End");
#endif

#ifdef RSI_INTERRUPTS
    // Cannot initialize interrupts until the chip has booted
    // SPI_IRQ Initialization
#ifdef RSI_DEBUG_PRINT
    RSI_DPRINT(RSI_PL3, "\r\nEnabling Interrupts");
#endif
    rsi_spiIrqStart();								// Enable the interrupt from the Wi-Fi Module
#ifdef RSI_DEBUG_PRINT
    RSI_DPRINT(RSI_PL3, "\r\nInterrupts Enabled");
#endif
#endif

    retval = RSI_SUCCESS;

    return retval;																	// everything ok
}

