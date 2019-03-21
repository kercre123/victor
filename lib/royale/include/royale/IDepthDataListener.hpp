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

#include <string>
#include <royale/DepthData.hpp>

namespace royale
{
    /*!
     * Provides the listener interface for consuming depth data from Royale. A listener needs
     * to implement this interface and register itself as a listener to the ICameraDevice.
     */
    class IDepthDataListener
    {
    public:
        virtual ~IDepthDataListener() {}

        /*!
         * Will be called on every frame update by the Royale framework
         *
         * NOTICE: Calling other framework functions within the data callback
         * can lead to undefined behavior and is therefore unsupported.
         * Call these framework functions from another thread to avoid problems.
         */
        virtual void onNewData (const royale::DepthData *data) = 0;
    };
}