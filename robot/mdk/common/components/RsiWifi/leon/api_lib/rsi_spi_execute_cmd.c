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
 * @brief Implements common functionality for all the commands
 *
 * @section Description
 * This file contains common api needed for all the commands
 *
 *
 */

/**
 * Includes
 */
#include "rsi_global.h"

#include <stdio.h>
#include <string.h>

/*====================================================*/
/**
 * @fn          int16 rsi_execute_cmd(uint8 *descparam,uint8 *payloadparam,uint16 size_param)
 * @brief       Common function for all the commands.
 * @param[in]   uint8 *descparam, pointer to the frame descriptor parameter structure
 * @param[in]   uint8 *payloadparam, pointer to the command payload parameter structure
 * @param[in]   uint16 size_param, size of the payload for the command
 * @return      errCode
 *              -1 = SPI busy / Timeout
 *              -2 = SPI Failure
 *              0  = SUCCESS
 * @section description 
 * This is a common function used to process a command to the Wi-Fi module.
 */
int16 rsi_execute_cmd(uint8 *descparam, uint8 *payloadparam, uint16 size_param)
{
    int16 retval;
    rsi_uFrameDsc uFrameDscFrame;				// 16 bytes, send/receive command descriptor frame
    // Create the Command Frame Descriptor
    rsi_buildFrameDescriptor(&uFrameDscFrame, descparam);
#ifdef RSI_DEBUG_PRINT
    RSI_DPRINT(RSI_PL3, "Descriptor write");
#endif
    // Write out the Command Frame Descriptor
    retval = rsi_SpiFrameDscWr(&uFrameDscFrame, RSI_C2MGMT);
    if (retval != 0x00)
    {
#ifdef RSI_DEBUG_PRINT
        RSI_DPRINT(RSI_PL4, "Descriptor write failErr=%02x", (uint16 )retval);
#endif
        return retval;
    }

    // Write the Command Frame Data
    if (size_param)
    {
        size_param = (size_param + 3) & ~3;
        retval = rsi_SpiFrameDataWr(size_param, payloadparam, 0, NULL, RSI_C2MGMT);
        if (retval != 0x00)
        {
#ifdef RSI_DEBUG_PRINT
            RSI_DPRINT(RSI_PL4, "FramePayload writeErr=%02x", (uint16 )retval);
#endif
            return retval;
        }
    }
    return retval;
}

