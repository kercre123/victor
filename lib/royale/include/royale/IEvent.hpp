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
#include <royale/String.hpp>

namespace royale
{
    /**
     * Severity of an IEvent.
     */
    enum class EventSeverity
    {
        /**
         * Information only event.
         */
        ROYALE_INFO = 0,
        /**
         * Potential issue detected (e.g. soft overtemperature limit reached).
         */
        ROYALE_WARNING = 1,
        /**
         * Errors occurred during operation.
         * The operation (e.g. recording or stream capture) has failed and was stopped.
         */
        ROYALE_ERROR = 2,
        /**
         * A severe error was detected.
         * The corresponding ICameraDevice is no longer in a usable state.
         */
        ROYALE_FATAL = 3
    };

    /**
     * Type of an IEvent.
     */
    enum class EventType
    {
        /**
         * The event was detected as part of the mechanism to receive image data.
         *
         * For events of this class, ROYALE_WARNING is likely to indicate dropped frames, and
         * ROYALE_ERROR is likely to indicate that no more frames will be captured until after the
         * next call to ICameraDevice::startCapture().
         */
        ROYALE_CAPTURE_STREAM,
        /**
         * Royale is no longer able to talk to the camera; this is always severity ROYALE_FATAL.
         */
        ROYALE_DEVICE_DISCONNECTED,
        /**
         * The camera has become hot, likely because of the illumination.
         *
         * For events of this class, ROYALE_WARNING indicates that the device is still functioning
         * but is near to the temperature at which it will be shut down for safety, and ROYALE_ERROR
         * indicates that the safety mechanism has been triggered.
         */
        ROYALE_OVER_TEMPERATURE,
        /**
         * This event is sent regularly during capturing.  The trigger for sending this event is
         * implementation defined and varies for different use cases, but the timing is normally
         * around one per second.
         *
         * If all frames were successfully received then it will be ROYALE_INFO, if any were dropped
         * then it will be ROYALE_WARNING.
         */
        ROYALE_RAW_FRAME_STATS,
        /**
         * This event indicates that the camera's internal monitor of the power used by the
         * illumination has been triggered, which is never expected to happen with the use cases
         * in production devices. The capturing should already been stopped when receiving this event,
         * as the illumination will stay turned off and most of the received data will be corrupted.
         *
         * This is always severity ROYALE_FATAL.
         */
        ROYALE_EYE_SAFETY,
        /**
         * The event type is for any event for which there is no official API specification.
         */
        ROYALE_UNKNOWN,
    };

    /**
     * Interface for anything to be passed via IEventListener.
     */
    class IEvent
    {
    public:
        virtual ~IEvent() = default;

        /**
         * Get the severity of this event. The severity of an event denotes the level of urgency the
         * event has. The severity may be used to determine when and where an event description
         * should be presented.
         * @return the severity of this event.
         */
        virtual royale::EventSeverity severity() const = 0;

        /**
         * Returns debugging information intended for developers using the Royale API. The strings
         * returned may change between releases, and are unlikely to be localised, so are neither
         * intended to be parsed automatically, nor intended to be shown to end users.
         * @return the description of this event.
         */
        virtual const royale::String describe() const = 0;

        /**
         * Get the type of this event.
         * @return the type of this event.
         */
        virtual royale::EventType type() const = 0;
    };
}
