/****************************************************************************\
 * Copyright (C) 2018 pmdtechnologies ag
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
    /*!
     *  Royale allows to set different filter levels. Internally these represent
     *  different configurations of the processing pipeline.
     */
    enum class FilterLevel
    {
        Off = 0,        ///< Turn off all filtering of the data (validation will still be enabled)
        Legacy = 200,   ///< Standard settings for older cameras
        Full = 255,     ///< Enable all filters that are available for this camera
        Custom = 256    ///< Value returned by getFilterLevel if the processing parameters differ from all of the presets
    };
}
