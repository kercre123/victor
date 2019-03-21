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

#include <string>
#include <royale/DepthImage.hpp>

namespace royale
{
    /*!
     * Provides a listener interface for consuming depth images from Royale. A listener needs
     * to implement this interface and register itself as a listener to the ICameraDevice.
     *
     * Consider using an IDepthDataListener instead of this listener.  This callback provides only
     * an array of depth and confidence values.  The mapping of pixels to the scene is similar to
     * the pixels of a two-dimensional camera, and it is unlikely to be a rectilinear projection
     * (although this depends on the exact camera).
     */
    class IDepthImageListener
    {
    public:
        virtual ~IDepthImageListener() {}

        /*!
         * Will be called on every frame update by the Royale framework
         *
         * NOTICE: Calling other framework functions within the data callback
         * can lead to undefined behavior and is therefore unsupported.
         * Call these framework functions from another thread to avoid problems.
         */
        virtual void onNewData (const royale::DepthImage *data) = 0;
    };
}
