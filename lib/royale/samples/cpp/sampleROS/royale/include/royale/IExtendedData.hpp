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
#include <royale/RawData.hpp>
#include <royale/DepthData.hpp>
#include <royale/IntermediateData.hpp>

#include <memory>
#include <cstdint>
#include <vector>
#include <chrono>

namespace royale
{
    /**
     * Interface for getting additional data to the standard depth data. The
     * retrieval of this data requires L2 access. Please be aware that not
     * all data is filled. Therefore, use the has* calls to check if data is provided.
     */
    class IExtendedData
    {
    public:
        virtual ~IExtendedData() = default;

        /*!
         *  Indicates if the getDepthData() has valid data. If false,
         *  then the getDepthData() will return nullptr.
         */
        virtual bool hasDepthData() const = 0;

        /*!
         *  Indicates if the getRawData() has valid data. If false,
         *  then the getRawData() will return nullptr.
         */
        virtual bool hasRawData() const = 0;

        /*!
         *  Indicates if the getIntermediateData() has valid data. If false,
         *  then the getIntermediateData() will return nullptr.
         */
        virtual bool hasIntermediateData() const = 0;

        /*!
         *  Returns the RawData structure
         *
         *  @return instance of RawData if available, nullptr else
         */
        virtual const royale::RawData *getRawData() const = 0;

        /*!
         *  Returns the DepthData structure
         *
         *  @return instance of DepthData if available, nullptr else
         */
        virtual const royale::DepthData *getDepthData() const = 0;

        /*!
         *  Returns the IntermediateData structure
         *
         *  @return instance of IntermediateData if available, nullptr else
         */
        virtual const royale::IntermediateData *getIntermediateData() const = 0;
    };
}