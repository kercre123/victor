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
#include <royale/String.hpp>
#include <royale/StreamId.hpp>
#include <memory>
#include <cstdint>
#include <chrono>

namespace royale
{
    /**
     *  This structure defines the raw data which is delivered through the callback only
     *  exposed for access LEVEL 2.
     *  This data comprises the raw phase images coming directly from the imager.
     */
    struct RawData
    {
        ROYALE_API explicit RawData();
        ROYALE_API explicit RawData (size_t rawVectorSize);

        std::chrono::microseconds           timeStamp;                //!< timestamp in microseconds precision (time since epoch 1970)
        StreamId                            streamId;                 //!< stream which produced the data
        uint16_t                            width;                    //!< width of raw frame
        uint16_t                            height;                   //!< height of raw frame
        royale::Vector<const uint16_t *>    rawData;                  //!< pointer to each raw frame
        royale::Vector<royale::String>      exposureGroupNames;       //!< name of each exposure group
        royale::Vector<size_t>              rawFrameCount;            //!< raw frame count of each exposure group
        royale::Vector<uint32_t>            modulationFrequencies;    //!< modulation frequencies for each sequence
        royale::Vector<uint32_t>            exposureTimes;            //!< integration times for each sequence
        float                               illuminationTemperature;  //!< temperature of illumination
        royale::Vector<uint16_t>            phaseAngles;              //!< phase angles for each raw frame
        royale::Vector<uint8_t>             illuminationEnabled;      //!< status of the illumination for each raw frame (1-enabled/0-disabled)
    };

}
