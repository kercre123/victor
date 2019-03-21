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
     *  The depth image represents the depth and confidence for every pixel.
     *  The least significant 13 bits are the depth (z value along the optical axis) in
     *  millimeters. 0 stands for invalid measurement / no data.
     *
     *  The most significant 3 bits correspond to a confidence value. 0 is the highest confidence, 7
     *  the second highest, and 1 the lowest.
     *
     *  *note* The meaning of the confidence bits changed between Royale v3.14.0 and v3.15.0. Before
     *  v3.15.0, zero was lowest and 7 was highest. Because of this, the member was renamed from
     *  "data" to "cdData".
     */
    struct DepthImage
    {
        int64_t                   timestamp;       //!< timestamp for the frame
        StreamId                  streamId;        //!< stream which produced the data
        uint16_t                  width;           //!< width of depth image
        uint16_t                  height;          //!< height of depth image
        royale::Vector<uint16_t>  cdData;          //!< depth and confidence for the pixel
    };
}
