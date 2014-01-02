/**
 * @file
 * @version		2.1.0.0
 * @date 		2011-May-25
 *
 * Copyright(C) Redpine Signals 2011
 * All rights reserved by Redpine Signals.
 *
 * @section License
 * This program should be used on your own responsibility.
 * Redpine Signals assumes no responsibility for any losses
 * incurred by customers or third parties arising from the use of this file.
 *
 * @brief SPI, FWUPGRADER, Upgrade the firmware over the SPI interface
 *
 * @section Description
 * This file contains the function to upgrade the module firmware over the SPI interface
 * This code is contained in files referenced by the code as an array initialization
 * In order to call this function, the firmware upgrade files must be updated
 * and the source code cleaned and compiled.
 *
 *
 */




/**
 * Includes
 */
#include "rsi_global.h"

#include <stdio.h>
#include <string.h>

#if RSI_FIRMWARE_UPGRADE

/**
 * External Variables
 */


/*=================================================*/
/**
 * @fn                  int rsi_fwupgrade(void)
 * @brief               Upgrades the firmware for the Wi-Fi Module via SPI
 * @param[in]           none
 * @param[out]          none
 * @return              errCode
 *                      -1 = SPI busy / Timeout
 *                      -2 = SPI Failure
 *                      0  = SUCCESS
 * @section description  	
 * This API is used to upgrade the firmware in the Wi-Fi module. It is enabled only 
 * if the RSI_FIRMWARE_UPGRADE macro is set to C1C in rsi_config.h.This function 
 * should be called immediately after spi interface initialization.After successful 
 * firmware upgradation application need to reset the Wi-Fi module for normal operation. 
 */
int16 rsi_fwupgrade(void)
{	
	int16                           retval;
	int16                           i;
	uint8                           dBuf[4];
	rsi_uFrameDsc                   uFrameDscFrame;



	// firmware update program
	static const uint8 iuinst1[] = {
#include "Firmware/iuinst1"
	};
	static const uint8 iuinst2[] = {
#include "Firmware/iuinst2"
	};
	static const uint8 iudata[] = {
#include "Firmware/iudata"
	};

	// firmware update to write to flash
	static const uint8 fftaim1[] = {
#include "Firmware/ffinst1"
	};
	static const uint8 fftaim2[] = {
#include "Firmware/ffinst2"
	};
	static const uint8 fftadm[] = {
#include "Firmware/ffdata"
	};



#ifdef RSI_DEBUG_PRINT
	RSI_DPRINT(RSI_PL3,"\r\n\nFW Upgrade Start ");

	// first we have to put the chip in soft reset??
	RSI_DPRINT(RSI_PL3,"\r\nSet Soft Reset ");
#endif
	for (i = 0; i < sizeof(dBuf); i++) { dBuf[i] = 0; }																// zero out dBuf
	dBuf[0] = RSI_RST_SOFT_SET;


	// 0x74, 0x00, 0x22000004, 0x04, 0x00000001
	retval = rsi_memWr(RSI_RST_SOFT_ADDR, 0x0004, dBuf);						
	if(retval !=0)
	{
#ifdef RSI_DEBUG_PRINT
		RSI_DPRINT(RSI_PL4,"\r\n Fwupgrade setting soft reset failed %d", retval);
#endif     
		return retval;
	}

#ifdef RSI_DEBUG_PRINT
	RSI_DPRINT(RSI_PL3,"\r\nWriting IUINST1, is %d Bytes", sizeof(iuinst1));
#endif
	retval = rsi_memWr(RSI_IUINST1_ADDR, sizeof(iuinst1), (uint8 *) iuinst1);
	if(retval !=0)
	{
#ifdef RSI_DEBUG_PRINT
		RSI_DPRINT(RSI_PL4,"\r\n FwUpgrade iuinst1 masterwr failed %d", retval);
#endif     
		return retval;
	}

#ifdef RSI_DEBUG_PRINT	
	RSI_DPRINT(RSI_PL3,"\r\nWriting IUINST2, is %d Bytes", sizeof(iuinst2));
#endif
	retval = rsi_memWr(RSI_IUINST2_ADDR, sizeof(iuinst2), (uint8 *)iuinst2);	
	if(retval !=0)
	{
#ifdef RSI_DEBUG_PRINT  
		RSI_DPRINT(RSI_PL4,"\r\n FwUpgrade iuinst2 masterwr failed %d", retval);
#endif     
		return retval;
	}

	// write iudata
#ifdef RSI_DEBUG_PRINT	
	RSI_DPRINT(RSI_PL3,"\r\nWriting IUDATA, is %d Bytes", sizeof(iudata));
#endif
	retval = rsi_memWr(RSI_IUDATA_ADDR, sizeof(iudata), (uint8 *) iudata);
	if(retval !=0)
	{
#ifdef RSI_DEBUG_PRINT
		RSI_DPRINT(RSI_PL4,"\r\n FwUpgrade iudata masterwr failed %d", retval);
#endif     
		return retval;
	}

#ifdef RSI_DEBUG_PRINT	
	RSI_DPRINT(RSI_PL3,"\r\nNegate Soft Reset");
#endif
	dBuf[0] = RSI_RST_SOFT_CLR;
	// 0x74, 0x00, 0x22000004, 0x04, 0x00000000
	retval = rsi_memWr(RSI_RST_SOFT_ADDR, 0x0004, dBuf);						
	if(retval !=0)
	{
#ifdef RSI_DEBUG_PRINT
		RSI_DPRINT(RSI_PL4,"\r\n Fwupgrade: setting out of soft reset failed %d", retval);
#endif     
		return retval;
	}
#ifdef RSI_DEBUG_PRINT
	RSI_DPRINT(RSI_PL3,"\r\nWait for Card Ready");
#endif

	RSI_RESPONSE_TIMEOUT(RSI_FWUPTIMEOUT);
	// Read in the Frame Descriptor
	retval =rsi_SpiFrameDscRd(&uFrameDscFrame);
	if(retval !=0)
	{
#ifdef RSI_DEBUG_PRINT
		RSI_DPRINT(RSI_PL4,"\r\n Fwupgrade: Card ready mSpiFrameDscRd failed %d", retval);
#endif     
		return retval;
	}
#ifdef RSI_DEBUG_PRINT
	RSI_DPRINT(RSI_PL3,"\r\nFrameDescriptorManagementResponse Type=%02x", 
			(uint16)uFrameDscFrame.frameDscMgmtRsp.mgmtPacketType);
#endif
	// 0x89, make sure we received the correct frame type
	if (uFrameDscFrame.frameDscMgmtRsp.mgmtPacketType != RSI_RSP_CARD_READY) {											
#ifdef RSI_DEBUG_PRINT	
		RSI_DPRINT(RSI_PL4,"\r\nCard (NOT) Ready=%02x ", (uint16)uFrameDscFrame.uFrmDscBuf[1]);
#endif		
		return RSI_BUSY;																									// we have an error
	}

	/*=================================*/
	// Write the flash with the upgrade images
	// write fftaim1
#ifdef RSI_DEBUG_PRINT	
	RSI_DPRINT(RSI_PL3,"\r\nWriting FFTAIM1 is %d Bytes", sizeof(fftaim1));
#endif	
	retval = rsi_memWr(RSI_FFTAIM1_ADDR, sizeof(fftaim1), (uint8 *)fftaim1);
	if(retval !=0)
	{
#ifdef RSI_DEBUG_PRINT
		RSI_DPRINT(RSI_PL4,"\r\n FwUpgrade: ffinst1 masterwr failed %d", retval);
#endif    
		return retval;
	}
	rsi_buildFrameDescriptor(&uFrameDscFrame, (uint8 *)rsi_frameCmdFftaim1Upgd);
	rsi_setMFrameFwUpgradeLen(&uFrameDscFrame, (uint16)sizeof(fftaim1));
	retval = rsi_SpiFrameDscWr(&uFrameDscFrame,RSI_C2MGMT);
	if(retval !=0)
	{
#ifdef RSI_DEBUG_PRINT
		RSI_DPRINT(RSI_PL4,"\r\n FwUpgrade: ffinst1 upgrade frame desc write failed %d", retval);
#endif     
		return retval;
	}
	RSI_RESPONSE_TIMEOUT(RSI_FWUPTIMEOUT);	
	retval = rsi_SpiFrameDscRd(&uFrameDscFrame);	
	if(retval !=0)
	{
#ifdef RSI_DEBUG_PRINT
		RSI_DPRINT(RSI_PL4,"\r\n FwUpgrade: ffinst1 upgrade done frame desc read failed %d", retval);
#endif    
		return retval;
	}
#ifdef RSI_DEBUG_PRINT
	RSI_DPRINT(RSI_PL3,"\r\nFrameDescriptorManagementResponse Type=%02x", 
			(uint16)uFrameDscFrame.frameDscMgmtRsp.mgmtPacketType);
	RSI_DPRINT(RSI_PL3,"\r\nWriting FFTAIM2 is %d Bytes", sizeof(fftaim2));
#endif
	retval = rsi_memWr(RSI_FFTAIM2_ADDR, sizeof(fftaim2), (uint8 *)fftaim2);	
	if(retval !=0)
	{
#ifdef RSI_DEBUG_PRINT
		RSI_DPRINT(RSI_PL4,"\r\n FwUpgrade: ffinst2 masterwr failed %d", retval);
#endif     
		return retval;
	}
	rsi_buildFrameDescriptor(&uFrameDscFrame, (uint8 *)rsi_frameCmdFftaim2Upgd);
	rsi_setMFrameFwUpgradeLen(&uFrameDscFrame, (uint16)sizeof(fftaim2));
	retval = rsi_SpiFrameDscWr(&uFrameDscFrame,RSI_C2MGMT);
	if(retval !=0)
	{
#ifdef RSI_DEBUG_PRINT
		RSI_DPRINT(RSI_PL4,"\r\n FwUpgrade: ffinst2 upgrade frame desc Wr failed %d", retval);
#endif
		return retval;
	}
	RSI_RESPONSE_TIMEOUT(RSI_FWUPTIMEOUT);
	retval = rsi_SpiFrameDscRd(&uFrameDscFrame);
	if(retval !=0)
	{
#ifdef RSI_DEBUG_PRINT
		RSI_DPRINT(RSI_PL4,"\r\n FwUpgrade: ffinst2 upgrade done frame desc read failed %d", retval);
#endif
		return retval;
	}
#ifdef RSI_DEBUG_PRINT  
	RSI_DPRINT(RSI_PL3,"\r\nFrameDescriptorManagementResponse Type=%02x", 
			(uint16)uFrameDscFrame.frameDscMgmtRsp.mgmtPacketType);
	RSI_DPRINT(RSI_PL3,"\r\nWriting FFTADM is %d Bytes", sizeof(fftadm));
#endif
	retval = rsi_memWr(RSI_FFTADM_ADDR, sizeof(fftadm), (uint8 *)fftadm);
	if(retval !=0)
	{
#ifdef RSI_DEBUG_PRINT
		RSI_DPRINT(RSI_PL4,"\r\n FwUpgrade: ffdata masterwr failed %d", retval);
#endif
		return retval;
	}
	rsi_buildFrameDescriptor(&uFrameDscFrame, (uint8 *)rsi_frameCmdFftadmUpgd);
	rsi_setMFrameFwUpgradeLen(&uFrameDscFrame, (uint16)sizeof(fftadm));
	retval = rsi_SpiFrameDscWr(&uFrameDscFrame,RSI_C2MGMT);
	if(retval !=0)
	{
#ifdef RSI_DEBUG_PRINT
		RSI_DPRINT(RSI_PL4,"\r\n FwUpgrade: ffdata upgrade frame desc Wr failed %d", retval);
#endif
		return retval;
	}
	RSI_RESPONSE_TIMEOUT(RSI_FWUPTIMEOUT);
	retval = rsi_SpiFrameDscRd(&uFrameDscFrame);
	if(retval !=0)
	{
#ifdef RSI_DEBUG_PRINT
		RSI_DPRINT(RSI_PL4,"\r\n FwUpgrade: ffdata upgrade done frame desc Rd failed %d", retval);
#endif
		return retval;
	}
#ifdef RSI_DEBUG_PRINT
	RSI_DPRINT(RSI_PL3,"\r\nFrameDescriptorManagementResponse Type=%02x", 
			(uint16)uFrameDscFrame.frameDscMgmtRsp.mgmtPacketType);
	// Power cycle the module for the new firmware to take effect
	RSI_DPRINT(RSI_PL3,"\r\nCycling power to complete upgrade ");
#endif
	// cycle the power to the module, not sure how long it needs to be off for
	retval =rsi_module_power_cycle();
#ifdef RSI_DEBUG_PRINT		
	RSI_DPRINT(RSI_PL3,"\r\nFirmware Upgrade Done ");
#endif	
	return retval;
}



/*==================================================*/
/**
 * @fn          rsi_setMFrameFwUpgradeLen(rsi_uFrameDsc *uFrameDscFrame, uint16 len)
 * @brief       Sets the length values for a firmware upgrade management frame
 * @param[in]   rsi_uFrameDsc *uFrameDscFrame,Pointer to Frame Descriptor
 * @param[in]   uint16 len, length to be  set in the management frame
 * @param[out]  none
 * @return      none
 */
void rsi_setMFrameFwUpgradeLen(rsi_uFrameDsc *uFrameDscFrame, uint16 len)
{
	uFrameDscFrame->uFrmDscBuf[2] = (uint8)len;
	uFrameDscFrame->uFrmDscBuf[3] = (uint8)(len >> 8);
	return;
}

#endif
