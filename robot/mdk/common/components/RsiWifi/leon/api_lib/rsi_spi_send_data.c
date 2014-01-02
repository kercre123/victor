/**
 * @file
 * @version		2.0.0.0
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
 * @brief SPI, SEND, send udp or tcp data to an existing connection
 *
 * @section Description
 * This file contains the SEND function.
 *
 *
 */

/**
 * Includes
 */
#include "rsi_global.h"

#include <stdio.h>
#include <string.h>

/*
 * Global Variables 
 */

/*=============================================================*/
/**
 * @fn          int16 rsi_send_data(uint16 socketDescriptor, uint8 *payload, uint32 payloadLen,uint8 protocol)
 * @brief       SEND Packet command
 * @param[in]   uint16 socketDescriptor, socket descriptor for the socket to send data to
 * @param[in]   uint8 *payload, pointer to the data to send payload buffer
 * @param[in]   uint32 payloadLen, length of the data to send payload
 * @param[in]   uint8  protocol, TCP or UDP
 * @param[out]  none
 * @return      errCode
 *              -1 = SPI busy / Timeout
 *              -2 = SPI Failure
 *              -3 = Buffer Full
 *              -4 = Module in power save
 *              0  = SUCCESS
 *
 * @section description 
 * This API used to send TCP/UDP data using an already opened socket. This function 
 * should be called after successfully opening a socket using the rsi_socket API.
 * If this API return error codes like -1,-3,-4, Application need to retry this function until 
 * successfully send the packet over WiFi module.
 */
int16 rsi_send_data(uint16 socketDescriptor, uint8 *payload, uint32 payloadLen, uint8 protocol)
{
    int16 retval;
    uint16 headerLen;
    // frame sent for the send command,  includes data
    rsi_uSend uSendFrame;
    // length of frame without padding bytes
    uint32 frameLen, hdrLen;
    // length to pad the transfer so it lines up on a 4 byte boundary
    uint32 padding = 0;
    rsi_uFrameDsc uFrameDscFrame;
#ifdef RSI_DEBUG_PRINT
    uint8 strBuf[32];
#endif
    if (rsi_checkBufferFullIrq() == RSI_TRUE)
    {
        return RSI_BUFFER_FULL;
    }
    if ((rsi_pwstate.current_mode == 1) && (rsi_checkPowerModeIrq() == RSI_FALSE))
    {
        return RSI_IN_SLEEP;
    }

    headerLen =
            (protocol == RSI_PROTOCOL_UDP) ? RSI_UDP_FRAME_HEADER_LEN : RSI_TCP_FRAME_HEADER_LEN;
#ifdef RSI_LITTLE_ENDIAN
    *(uint16 *)uSendFrame.sendFrameSnd.sendDataOffsetSize = headerLen;
#else
    rsi_uint16To2Bytes(uSendFrame.sendFrameSnd.sendDataOffsetSize, headerLen);
#endif
    frameLen = payloadLen + headerLen;
    uSendFrame.uSendBuf[headerLen] = payload[0];
    uSendFrame.uSendBuf[headerLen + 1] = payload[1];
    payload += 2;
    hdrLen = headerLen + 2;
    if (((frameLen) % 4) == 0)
    {
        padding = 0;
    }
    else
    {
        padding = 4 - (frameLen % 4);
    }

#ifdef RSI_DEBUG_PRINT
    RSI_DPRINT(RSI_PL3, "\r\nPaddingLen=%08x", (uint16 )padding);
    RSI_DPRINT(RSI_PL3, " PayloadLen=%08x", (uint16 )payloadLen);
    RSI_DPRINT(RSI_PL3, " FrameLen=%08x ", (uint16 )(frameLen + padding));
#endif

    // Create the SEND Command Frame Descriptor
    // build the SEND frame descriptor
    rsi_buildFrameDescriptor(&uFrameDscFrame, (uint8 *) rsi_frameCmdSend);
#ifdef RSI_LITTLE_ENDIAN
    *(uint16 *)uFrameDscFrame.frameDscDataSnd.dataFrmBodyLen = (uint16)(frameLen & 0x7fff);
#else 
    // set the frame length LSB
    uFrameDscFrame.frameDscDataSnd.dataFrmBodyLen[0] = (uint8) (frameLen & 0x00ff);
    // set the frame length MSB
    uFrameDscFrame.frameDscDataSnd.dataFrmBodyLen[1] = (uint8) ((frameLen >> 8) & 0x007f);
#endif
    // Write out the command frame descriptor
    retval = rsi_SpiFrameDscWr(&uFrameDscFrame, RSI_C2DATA);
    // Init the SEND Frame Data
    uSendFrame.sendFrameSnd.sendCmd[0] = rsi_dFrmSend[0];
    uSendFrame.sendFrameSnd.sendCmd[1] = rsi_dFrmSend[1];
#ifdef RSI_LITTLE_ENDIAN
    *(uint16 *)uSendFrame.sendFrameSnd.socketDescriptor = socketDescriptor;
    *(uint16 *)uSendFrame.sendFrameSnd.sendBufLen = payloadLen;
#else
    rsi_uint16To2Bytes(uSendFrame.sendFrameSnd.socketDescriptor, socketDescriptor);
    rsi_uint16To2Bytes(uSendFrame.sendFrameSnd.sendBufLen, payloadLen);
#endif
    // Write the Frame Data
    if (payloadLen > 2)
    {
        payloadLen -= 2;
        payloadLen += padding;
    }
    else
    {
        payloadLen = 0;
    }
    retval = rsi_SpiFrameDataWr(hdrLen, (uint8 *) &uSendFrame.uSendBuf, (payloadLen),
            (uint8 *) payload, RSI_C2DATA);
    if (retval != 0x00)
    {
#ifdef RSI_DEBUG_PRINT
        RSI_DPRINT(RSI_PL4, "SndFrmDescErr=%02x", (uint16 )retval);
#endif
        return retval;
    }
    return retval;
}

int16 rsi_send_data_nonblk(uint16 socketDescriptor, uint8 *payload, uint32 payloadLen,
        uint8 protocol, void *callback)
{
    int16 retval;
    uint16 headerLen;
    // frame sent for the send command,  includes data
    rsi_uSend uSendFrame;
    // length of frame without padding bytes
    uint32 frameLen, hdrLen;
    // length to pad the transfer so it lines up on a 4 byte boundary
    uint32 padding = 0;
    rsi_uFrameDsc uFrameDscFrame;
#ifdef RSI_DEBUG_PRINT
    uint8 strBuf[32];
#endif
    if (rsi_checkBufferFullIrq() == RSI_TRUE)
    {
        return RSI_BUFFER_FULL;
    }
    if ((rsi_pwstate.current_mode == 1) && (rsi_checkPowerModeIrq() == RSI_FALSE))
    {
        return RSI_IN_SLEEP;
    }

    headerLen =
            (protocol == RSI_PROTOCOL_UDP) ? RSI_UDP_FRAME_HEADER_LEN : RSI_TCP_FRAME_HEADER_LEN;
#ifdef RSI_LITTLE_ENDIAN
    *(uint16 *)uSendFrame.sendFrameSnd.sendDataOffsetSize = headerLen;
#else
    rsi_uint16To2Bytes(uSendFrame.sendFrameSnd.sendDataOffsetSize, headerLen);
#endif
    frameLen = payloadLen + headerLen;
    uSendFrame.uSendBuf[headerLen] = payload[0];
    uSendFrame.uSendBuf[headerLen + 1] = payload[1];
    payload += 2;
    hdrLen = headerLen + 2;
    if (((frameLen) % 4) == 0)
    {
        padding = 0;
    }
    else
    {
        padding = 4 - (frameLen % 4);
    }

#ifdef RSI_DEBUG_PRINT
    RSI_DPRINT(RSI_PL3, "\r\nPaddingLen=%08x", (uint16 )padding);
    RSI_DPRINT(RSI_PL3, " PayloadLen=%08x", (uint16 )payloadLen);
    RSI_DPRINT(RSI_PL3, " FrameLen=%08x ", (uint16 )(frameLen + padding));
#endif

    // Create the SEND Command Frame Descriptor
    // build the SEND frame descriptor
    rsi_buildFrameDescriptor(&uFrameDscFrame, (uint8 *) rsi_frameCmdSend);
#ifdef RSI_LITTLE_ENDIAN
    *(uint16 *)uFrameDscFrame.frameDscDataSnd.dataFrmBodyLen = (uint16)(frameLen & 0x7fff);
#else 
    // set the frame length LSB
    uFrameDscFrame.frameDscDataSnd.dataFrmBodyLen[0] = (uint8) (frameLen & 0x00ff);
    // set the frame length MSB
    uFrameDscFrame.frameDscDataSnd.dataFrmBodyLen[1] = (uint8) ((frameLen >> 8) & 0x007f);
#endif
    // Write out the command frame descriptor
    retval = rsi_SpiFrameDscWr(&uFrameDscFrame, RSI_C2DATA);
    // Init the SEND Frame Data
    uSendFrame.sendFrameSnd.sendCmd[0] = rsi_dFrmSend[0];
    uSendFrame.sendFrameSnd.sendCmd[1] = rsi_dFrmSend[1];
#ifdef RSI_LITTLE_ENDIAN
    *(uint16 *)uSendFrame.sendFrameSnd.socketDescriptor = socketDescriptor;
    *(uint16 *)uSendFrame.sendFrameSnd.sendBufLen = payloadLen;
#else
    rsi_uint16To2Bytes(uSendFrame.sendFrameSnd.socketDescriptor, socketDescriptor);
    rsi_uint16To2Bytes(uSendFrame.sendFrameSnd.sendBufLen, payloadLen);
#endif
    // Write the Frame Data
    if (payloadLen > 2)
    {
        payloadLen -= 2;
        payloadLen += padding;
    }
    else
    {
        payloadLen = 0;
    }
    retval = rsi_SpiFrameDataWrNonBlk(hdrLen, (uint8 *) &uSendFrame.uSendBuf, (payloadLen),
            (uint8 *) payload, RSI_C2DATA, callback);
    if (retval != 0x00)
    {
#ifdef RSI_DEBUG_PRINT
        RSI_DPRINT(RSI_PL4, "SndFrmDescErr=%02x", (uint16 )retval);
#endif
        return retval;
    }
    return retval;
}

