/****************************************************************************\
 * Copyright (C) 2015 pmdtechnologies ag
 *
 * THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 \****************************************************************************/

#pragma once

#include <royale/Definitions.hpp>
#include <royale/Vector.hpp>
#include <royale/StreamId.hpp>
#include <memory>
#include <cstdint>

namespace royale
{
    /**
     *  Infrared image with 8Bit mono information for every pixel
     */
    struct IRImage
    {
        int64_t                   timestamp;       //!< timestamp for the frame
        StreamId                  streamId;        //!< stream which produced the data
        uint16_t                  width;           //!< width of depth image
        uint16_t                  height;          //!< height of depth image
        royale::Vector<uint8_t>   data;            //!< 8Bit mono IR image
    };

}