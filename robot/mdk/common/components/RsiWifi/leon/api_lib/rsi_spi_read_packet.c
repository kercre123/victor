/**
 * @file
 * @version		2.3.0.0
 * @date 		2011-May-30
 *
 * Copyright(C) Redpine Signals 2011
 * All rights reserved by Redpine Signals.
 *
 * @section License
 * This program should be used on your own responsibility.
 * Redpine Signals assumes no responsibility for any losses
 * incurred by customers or third parties arising from the use of this file.
 *
 * @brief Implements common functionality for all the commands to receive response
 *
 * @section Description
 * This API serves as a common API to read the responses from the Wi-Fi module for 
 * all the command (rsi_bootloader, rsi_band, rsi_init, rsi_scan, etc.) packets sent 
 * to it and for the data packets that the module receives over the Wi-Fi network. 
 * This API has to be called after each command API and also regularly to check 
 * if any data is pending from the module (which it has received over the network). 
 */

/**
 * Includes
 */
#include "rsi_global.h"

#include <stdio.h>
#include <string.h>

/*====================================================*/
/**
 * @fn          int16 rsi_read_packet(rsi_uCmdRsp *uCmdRspFrame)
 * @brief	This function is used to read the response from module.
 * @param[in]	rsi_uCmdRsp *uCmdRspFrame, pointer to the global rsi_uCmdRsp structure
 *              which is used to store the response from the module
 * @param[out]  none
 * @return      errCode
 *              -1 = SPI busy / Timeout
 *              -2 = SPI Failure
 *              0  = SUCCESS
 * @section description 
 * This is a common function to read response for all the command and data from Wi-Fi module.
 */
int16 rsi_read_packet(rsi_uCmdRsp *uCmdRspFrame)
{
    int16 retval;
    uint16 bufLen;
    rsi_uFrameDsc uFrameDscFrame;				// 16 bytes, send/receive command descriptor frame

    retval = rsi_SpiFrameDscRd(&uFrameDscFrame);					// read in the frame descriptor
    if (retval != 0x00)
    {
#ifdef RSI_DEBUG_PRINT
        RSI_DPRINT(RSI_PL4, "DscRdError=%02x ", (uint16 )retval);
#endif
        return retval;
    }
#ifdef RSI_LITTLE_ENDIAN
    bufLen = *(uint16 *)uFrameDscFrame.uFrmDscBuf;
#else
    bufLen = rsi_bytes2RToUint16(uFrameDscFrame.uFrmDscBuf);
#endif
    if (uFrameDscFrame.frameDscMgmtRsp.frameType[0] == RSI_DATA_PKT_TYPE)
    {
        bufLen = bufLen & 0xFFF;
        // Read in the frame data
        retval = rsi_SpiFrameDataRd(bufLen, (uint8 *) &uCmdRspFrame->uCmdRspBuf);
        if (retval != 0x00)
        {
#ifdef RSI_DEBUG_PRINT
            RSI_DPRINT(RSI_PL4, "FrmDataRdError=%02x ", (uint16 )retval);
#endif
            return retval;
        }
    }
    else
    {
        bufLen = bufLen & 0xFF;
        if (uFrameDscFrame.frameDscMgmtRsp.mgmtPacketType == RSI_RSP_SCAN)
        {
            if (!uFrameDscFrame.uFrmDscBuf[2])
            {
                // Get the length of the returned data
                if (bufLen == 0xff)
                {			        // if byte 0 is < 0xff, this is the length
                    bufLen = uFrameDscFrame.uFrmDscBuf[9];
                    bufLen <<= 8;
                    bufLen &= 0xff00;
                    bufLen |= uFrameDscFrame.uFrmDscBuf[8];
                }
#ifdef RSI_DEBUG_PRINT
                RSI_DPRINT(RSI_PL3, "DataLen=%04x ", (uint16 )bufLen);
#endif
                // Read in the scan data (results)
                if (bufLen > 0)
                {
                    retval = rsi_SpiFrameDataRd(bufLen, (uint8 *) uCmdRspFrame->uCmdRspBuf);
                    memcpy(uCmdRspFrame->scanResponse.scanCount, uCmdRspFrame->rspCode, 2);
                    if (retval != 0x00)
                    {
#ifdef RSI_DEBUG_PRINT
                        RSI_DPRINT(RSI_PL4, "\r\nScanFramDataRdErr %02x ", (uint16 )retval);
#endif
                        return retval;
                    }

                }
            }
            uCmdRspFrame->rspCode[0] = uFrameDscFrame.frameDscMgmtRsp.mgmtPacketType;
            uCmdRspFrame->scanResponse.status = uFrameDscFrame.uFrmDscBuf[2];
        }
        else if (uFrameDscFrame.frameDscMgmtRsp.mgmtPacketType == RSI_RSP_BSSID_NWTYPE)
        {
            if (!uFrameDscFrame.uFrmDscBuf[2])
            {
                // Get the length of the returned data
                if (bufLen == 0xff)
                {										// if byte 0 is < 0xff, this is the length
                    bufLen = uFrameDscFrame.uFrmDscBuf[9];
                    bufLen <<= 8;
                    bufLen &= 0xff00;
                    bufLen |= uFrameDscFrame.uFrmDscBuf[8];
                }
#ifdef RSI_DEBUG_PRINT
                RSI_DPRINT(RSI_PL3, "DataLen=%04x ", (uint16 )bufLen);
#endif
                // Read in the scan data (results)
                if (bufLen > 0)
                {
                    retval = rsi_SpiFrameDataRd(bufLen, (uint8 *) uCmdRspFrame->uCmdRspBuf);
                    memcpy(uCmdRspFrame->bssid_nwtypeFrameRecv.scanCount, uCmdRspFrame->rspCode, 2);
                    if (retval != 0x00)
                    {
#ifdef RSI_DEBUG_PRINT
                        RSI_DPRINT(RSI_PL4, "\r\nBSSIDNwtype FramDataRdErr %02x ", (uint16 )retval);
#endif
                        return retval;
                    }

                }
            }
            uCmdRspFrame->rspCode[0] = uFrameDscFrame.frameDscMgmtRsp.mgmtPacketType;
            uCmdRspFrame->bssid_nwtypeFrameRecv.status = uFrameDscFrame.uFrmDscBuf[2];
        }
        else if (uFrameDscFrame.frameDscMgmtRsp.mgmtPacketType == RSI_RSP_CFG_GET)
        {
            //if(!uFrameDscFrame.uFrmDscBuf[2])
            {
#ifdef RSI_DEBUG_PRINT
                RSI_DPRINT(RSI_PL3, "DataLen=%04x ", (uint16 )bufLen);
#endif
                // Read in config data (results)
                if (bufLen > 0)
                {
                    retval = rsi_SpiFrameDataRd(bufLen, (uint8 *) uCmdRspFrame->uCmdRspBuf);
                    memcpy(uCmdRspFrame->CfgGetFrameRcv.valid_flag, uCmdRspFrame->rspCode, 2);
                    if (retval != 0x00)
                    {
#ifdef RSI_DEBUG_PRINT
                        RSI_DPRINT(RSI_PL4, "\r\nCfgGet FramDataRdErr %02x ", (uint16 )retval);
#endif
                        return retval;
                    }

                }
            }
            uCmdRspFrame->rspCode[0] = uFrameDscFrame.frameDscMgmtRsp.mgmtPacketType;
            uCmdRspFrame->CfgGetFrameRcv.status = uFrameDscFrame.uFrmDscBuf[2];
        }
        else
        {
            uCmdRspFrame->rspCode[0] = uFrameDscFrame.frameDscMgmtRsp.mgmtPacketType;
            uCmdRspFrame->mgmtResponse.status = uFrameDscFrame.uFrmDscBuf[2];
        }
    }
    return retval;
}

