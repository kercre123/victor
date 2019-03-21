/****************************************************************************\
 * Copyright (C) 2016 Infineon Technologies
 *
 * THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 \****************************************************************************/

#pragma once

#include <royale/Definitions.hpp>
#include <memory>

namespace royale
{
    class IEvent;

    /**
    * This interface allows observers to receive events.
    */
    class IEventListener
    {
    public:
        virtual ~IEventListener() = default;

        /**
        * Will be called when an event occurs.
        *
        * Note there are some constraints on what the user is allowed to do
        * in the callback.
        * - Actually the royale API does not claim to be reentrant (and probably isn't),
        *   so the user is not supposed to call any API function from this callback
        *   besides stopCapture
        * - Deleting the ICameraDevice from the callback will most certainly
        *   lead to a deadlock.
        *   This has the interesting side effect that calling exit() or equivalent
        *   from the callback may cause issues.
        *
        * \param event      The event.
        */
        virtual void onEvent (std::unique_ptr<royale::IEvent> &&event) = 0;
    };
}
