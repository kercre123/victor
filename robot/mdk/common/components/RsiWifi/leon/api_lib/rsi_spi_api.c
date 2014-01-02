/**
 * @file
 * @version		2.0.0.0
 * @date 		2011-May-30
 *
 * Copyright(C) Redpine Signals 2010
 * All rights reserved by Redpine Signals.
 *
 * @section License
 * This program should be used on your own responsibility.
 * Redpine Signals assumes no responsibility for any losses
 * incurred by customers or third parties arising from the use of this file.
 *
 * @brief SPI API This file contain definition of different mangament, control & data commands variables.
 *        These definition are used to construct frames. 
 *
 */


/**
 * Includes
 */
#include "rsi_global.h"
#include "rsi_spi_api.h"


/**
 * Global Variables
 */


/*Management Commands */
const uint8			rsi_frameCmdBand[RSI_BYTES_3] = {0x02, 0x18, 0x04};
/* Band Code=0x18, Data Frame bytes to transfer=0x04, Frame Type, 0x02=Data, 0x04=Management */
const uint8			rsi_frameCmdInit[RSI_BYTES_3] = {0x00, 0x10, 0x04};
/* Init Code=0x10, Data Frame bytes to transfer=0x04, Frame Type, 0x02=Data, 0x04=Management */
const uint8			rsi_frameCmdScan[RSI_BYTES_3] = {0x24, 0x11, 0x04};				
/* 36 bytes, Scan Code=0x11, Data Frame bytes to transfer=0x25, Frame Type, 0x02=Data, 0x04=Management */
#if RSI_63BYTE_PSK_SUPPORT
const uint8			rsi_frameCmdJoin[RSI_BYTES_3] = {0x68, 0x12, 0x04};				
/* 104 bytes, Join Code=0x12, Data Frame bytes to transfer=0x68, Frame Type, 0x02=Data, 0x04=Management */
#else
const uint8			rsi_frameCmdJoin[RSI_BYTES_3] = {0x48, 0x12, 0x04};				
/* 72 bytes, Join Code=0x12, Data Frame bytes to transfer=0x48, Frame Type, 0x02=Data, 0x04=Management */
#endif
const uint8			rsi_frameCmdPower[RSI_BYTES_3] = {0x02, 0x19, 0x04};				
/*Power Code=0x19, Data Frame bytes to transfer=0x04, Frame Type, 0x02=Data, 0x04=Management */
const uint8			rsi_frameCmdFftaim1Upgd[RSI_BYTES_3] = {0x00, 0x13, 0x04};				
/* fftaim1 upgrade Code=0x13, Data Frame bytes to transfer=??, Frame Type, 0x02=Data, 0x04=Management */
const uint8			rsi_frameCmdFftaim2Upgd[RSI_BYTES_3] = {0x00, 0x14, 0x04};				
/* fftaim1 upgrade Code=0x13, Data Frame bytes to transfer=??, Frame Type, 0x02=Data, 0x04=Management */
const uint8			rsi_frameCmdFftadmUpgd[RSI_BYTES_3] = {0x00, 0x15, 0x04};				
/* fftaim1 upgrade Code=0x13, Data Frame bytes to transfer=??, Frame Type, 0x02=Data, 0x04=Management */
const uint8         rsi_frameCmdBssidNwtype[RSI_BYTES_3] = {0x00, 0x23, 0x04};
/* BSSID and NWtype get Code=0x23, Data Frame bytes to transfer=0x00, Frame Type, 0x02=Data, 0x04=Management */
const uint8			rsi_frameCmdCfgEnable[RSI_BYTES_3]  = {0x02, 0x2A, 0x04};
/* Cfg Enable Code=0x2A, Data Frame bytes to transfer=0x00, Frame Type, 0x02=Data, 0x04=Management */
const uint8			rsi_frameCmdCfgSave[RSI_BYTES_3]  =  {0x00, 0x2B, 0x04};			
// Cfg Save Code=0x2B, Data Frame bytes to transfer=??, Frame Type, 0x02=Data, 0x04=Management
const uint8			rsi_frameCmdCfgGet[RSI_BYTES_3]  =  {0x00, 0x2C, 0x04};			
// Cfg Get Code=0x2C, Data Frame bytes to transfer=??, Frame Type, 0x02=Data, 0x04=Management
const uint8         rsi_frameCmdModeSel[RSI_BYTES_3] = {0x04, 0x2D, 0x04};
// ModeSel Code=0x2D, Data Frame bytes to transfer=4 [3 padding], Frame Type, 0x02=Data, 0x04=Management
const uint8         rsi_frameCmdFeatSel[RSI_BYTES_3] = {0x04, 0x24, 0x04};
// FeatSel Code=0x2D, Data Frame bytes to transfer=4, Frame Type, 0x02=Data, 0x04=Management

// Management Data Frames
const uint8			rsi_dFrmBand[RSI_BYTES_2] = {0x18, 0x00};
const uint8			rsi_dFrmInit[RSI_BYTES_2] = {0x10, 0x00};						                                     // PRM says there isn't one, but without it, it hangs
const uint8			rsi_dFrmPower[RSI_BYTES_2] = {0x00, 0x00};						                                     // uint16 + padding
const uint8         rsi_dFrmCfgEnable[RSI_BYTES_2] = {0x2A, 0x00};
const uint8         rsi_dFrmCfgSave[RSI_BYTES_2] = {0x2B, 0x00};

/* Data-Control Commands */
const uint8			rsi_frameCmdIpparam[RSI_BYTES_3] = {0x14, 0x00, 0x02};				
/* 20 bytes, Byte0=bits[7:0], byte1=bits[11:8] of data frame length, byte2=0x02, Data frame */
const uint8			rsi_frameCmdSocket[RSI_BYTES_3] = {0x0c, 0x00, 0x02};				
/* 12 bytes, Byte0=bits[7:0], byte1=bits[11:8] of data frame length, byte2=0x02, Data frame */
const uint8			rsi_frameCmdSocketClose[RSI_BYTES_3] = {0x04, 0x00, 0x02};				
/* 4 bytes, 2 bytes command, 2 bytes socket descriptor */
const uint8			rsi_frameCmdRssi[RSI_BYTES_3] = {0x02, 0x00, 0x02};				
/* 4 bytes, 2 bytes command, 2 bytes padding */
const uint8			rsi_frameCmdSend[RSI_BYTES_3] = {0x00, 0x00, 0x02};				
/* Length is variable */
const uint8			rsi_frameCmdRecv[RSI_BYTES_3] = {0x00, 0x00, 0x02};				
/* Length comes from the received frame descriptor */
const uint8			rsi_frameCmdConnStatus[RSI_BYTES_3] = {0x02, 0x00, 0x02};				
/* 4 bytes, 2 bytes command, 2 bytes padding */
const uint8			rsi_frameCmdQryNetParms[RSI_BYTES_3] = {0x02, 0x00, 0x02};				
/* 4 bytes, 2 bytes command, 2 bytes padding */
const uint8			rsi_frameCmdDisconnect[RSI_BYTES_3] = {0x02, 0x00, 0x02};				
/* 4 bytes, 2 bytes command, 2 bytes padding */
const uint8			rsi_frameCmdQryDhcpParms[RSI_BYTES_3]	= {0x02, 0x00, 0x02};				
/* 4 bytes, 2 bytes command, 2 bytes padding */
const uint8			rsi_frameCmdQryFwVer[RSI_BYTES_3] = {0x02, 0x00, 0x02};				
/* 4 bytes, 2 bytes command, 2 bytes padding */
const uint8			rsi_frameCmdSetMacAddr[RSI_BYTES_3] = {0x08, 0x00, 0x02};				
/* 8 bytes, 6 bytes MAC Address, 2 bytes padding */
const uint8			rsi_frameCmdDnsGet[RSI_BYTES_3] = {0x2C, 0x00, 0x02};
/* 44 bytes, 42 bytes domain name, 2 bytes command */
const uint8			rsi_frameCmdSetListenInterval[RSI_BYTES_3] = {0x04, 0x00, 0x02};				
/* 4 bytes, 2 bytes Listen interval, 2 bytes command */
const uint8         rsi_frameCmdReqHttp[RSI_BYTES_3] = {0xBC, 0x04, 0x02};
/* 1212 bytes, 2 bytes command, 1208 bytes command data, 2 bytes padding */
const uint8			rsi_frameCmdQryMacAddress[RSI_BYTES_3]	= {0x02, 0x00, 0x02};				
/* 4 bytes, 2 bytes command, 2 bytes padding */

/* Data-Control Data Frames */
/* These can be used as-is, or copied into bytes 0/1 of a larger data frame */
const uint8			rsi_dFrmIpparam[RSI_BYTES_2] = {0x01, 0x00};
const uint8			rsi_dFrmSocket[RSI_BYTES_2] = {0x02, 0x00};
const uint8			rsi_dFrmSend[RSI_BYTES_2] = {0x03, 0x00};
const uint8			rsi_dFrmSocketClose[RSI_BYTES_2] = {0x04, 0x00};
const uint8			rsi_dFrmRssi[RSI_BYTES_2] = {0x05, 0x00};
const uint8			rsi_dFrmQryNetParms[RSI_BYTES_2] = {0x06, 0x00};
const uint8			rsi_dFrmConnStatus[RSI_BYTES_2]	= {0x07, 0x00};
const uint8			rsi_dFrmDisconnect[RSI_BYTES_2]	= {0x08, 0x00};
const uint8			rsi_dFrmQryDhcpParms[RSI_BYTES_2] = {0x09, 0x00};
const uint8			rsi_dFrmQryFwVer[RSI_BYTES_2] = {0x0d, 0x00};
const uint8			rsi_dFrmSetMacAddr[RSI_BYTES_2] = {0x0e, 0x00};
const uint8			rsi_dFrmDnsGet[RSI_BYTES_2] = {0x11, 0x00};
const uint8			rsi_dFrmSetListenInterval[RSI_BYTES_2] = {0x12, 0x00};
const uint8         rsi_dFrmGetHttp[RSI_BYTES_2] = {0x0a, 0x00};
const uint8			rsi_dFrmQryMacAddress[RSI_BYTES_2] = {0x0f, 0x00};
/* Management Response list */
const uint8			rsi_mgmtRspList[] = {0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x64};	
/* 9 bytes, list of management frame response codes */


