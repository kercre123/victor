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
     *  The sparse point cloud gives XYZ and confidence for every valid
     *  point.
     *  It is given as an array of packed coordinate quadruplets (x,y,z,c)
     *  as floating point values. The x, y and z coordinates are in meters.
     *  The confidence (c) has a floating point value in [0.0, 1.0], where 1
     *  corresponds to full confidence.
     */
    struct SparsePointCloud
    {
        int64_t                 timestamp;     //!< timestamp for the frame
        StreamId                streamId;      //!< stream which produced the data
        uint32_t                numPoints;     //!< the number of valid points
        royale::Vector<float>   xyzcPoints;    //!< XYZ and confidence for every valid point
    };
}