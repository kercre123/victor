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

#include <royale/Definitions.hpp>
#include <royale/Vector.hpp>
#include <royale/StreamId.hpp>
#include <memory>
#include <cstdint>
#include <chrono>

namespace royale
{
    /**
     *  Encapsulates a 3D point in object space, with coordinates in meters.  In addition to the
     *  X/Y/Z coordinate each point also includes a gray value, a noise standard deviation, and a
     *  depth confidence value.
     */
    struct DepthPoint
    {
        float x;                 //!< X coordinate [meters]
        float y;                 //!< Y coordinate [meters]
        float z;                 //!< Z coordinate [meters]
        float noise;             //!< noise value [meters]
        uint16_t grayValue;      //!< 16-bit gray value
        uint8_t depthConfidence; //!< value from 0 (invalid) to 255 (full confidence)
    };

    /**
     *  This structure defines the depth data which is delivered through the callback.
     *  This data comprises a dense 3D point cloud with the size of the depth image (width, height).
     *  The points vector encodes an array (row-based) with the size of (width x height). Based
     *  on the `depthConfidence`, the user can decide to use the 3D point or not.
     *  The point cloud uses a right handed coordinate system (x -> right, y -> down, z -> in viewing direction).
     *
     *  Although the points are provided as a (width * height) array and are arranged in a grid,
     *  treating them as simple square pixels will provide a distorted image, because they are not
     *  necessarily in a rectilinear projection; it is more likely that the camera would have a
     *  wide-angle or even fish-eye lens.  Each individual DepthPoint provides x and y coordinates
     *  in addition to the z cooordinate, these values in the individual DepthPoints will match the
     *  lens of the camera.
     */
    struct DepthData
    {
        int                                 version;         //!< version number of the data format
        std::chrono::microseconds           timeStamp;       //!< timestamp in microseconds precision (time since epoch 1970)
        StreamId                            streamId;        //!< stream which produced the data
        uint16_t                            width;           //!< width of depth image
        uint16_t                            height;          //!< height of depth image
        royale::Vector<uint32_t>            exposureTimes;   //!< exposureTimes retrieved from CapturedUseCase
        royale::Vector<royale::DepthPoint>  points;          //!< array of points
    };

}
