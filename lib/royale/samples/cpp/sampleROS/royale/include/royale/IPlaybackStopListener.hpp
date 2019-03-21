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

#include <cstdint>

namespace royale
{
    class IPlaybackStopListener
    {
    public :

        virtual ~IPlaybackStopListener() = default;

        /**
        * Will be called if the playback is stopped.
        */
        virtual void onPlaybackStopped () = 0;
    };

}