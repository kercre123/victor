/****************************************************************************\
 * Copyright (C) 2015 Infineon Technologies
 *
 * THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 \****************************************************************************/

#pragma once

namespace royale
{
    /**
     * Specifies the type of data which should be captured and returned as callback.
     */
    enum class CallbackData : uint16_t
    {
        None = 0x00,          //!< only get the callback but no data delivery
        Raw  = 0x01,          //!< raw frames, if exclusively used no processing pipe is executed (no calibration data is needed)
        Depth = 0x02,         //!< one depth and grayscale image will be delivered for the complete sequence
        Intermediate = 0x04   //!< all intermediate data will be delivered which are generated in the processing pipeline
    };
}
