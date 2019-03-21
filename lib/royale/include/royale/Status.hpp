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

#include <royale/String.hpp>

namespace royale
{
    enum class CameraStatus
    {
        SUCCESS                           = 0,    //!< Indicates that there isn't an error

        RUNTIME_ERROR                     = 1024, //!< Something unexpected happened
        DISCONNECTED                      = 1026, //!< Camera device is disconnected
        INVALID_VALUE                     = 1027, //!< The value provided is invalid
        TIMEOUT                           = 1028, //!< The connection got a timeout

        LOGIC_ERROR                       = 2048, //!< This does not make any sense here
        NOT_IMPLEMENTED                   = 2049, //!< This feature is not implemented yet
        OUT_OF_BOUNDS                     = 2050, //!< Setting/parameter is out of specified range

        RESOURCE_ERROR                    = 4096, //!< Cannot access resource
        FILE_NOT_FOUND                    = 4097, //!< Specified file was not found
        COULD_NOT_OPEN                    = 4098, //!< Cannot open resource
        DATA_NOT_FOUND                    = 4099, //!< No data available where expected
        DEVICE_IS_BUSY                    = 4100, //!< Another action is in progress
        WRONG_DATA_FORMAT_FOUND           = 4101, //!< A resource was expected to be in one data format, but was in a different (recognisable) format

        USECASE_NOT_SUPPORTED             = 5001, //!< This use case is not supported
        FRAMERATE_NOT_SUPPORTED           = 5002, //!< The specified frame rate is not supported
        EXPOSURE_TIME_NOT_SUPPORTED       = 5003, //!< The exposure time is not supported
        DEVICE_NOT_INITIALIZED            = 5004, //!< The device seems to be uninitialized
        CALIBRATION_DATA_ERROR            = 5005, //!< The calibration data is not readable
        INSUFFICIENT_PRIVILEGES           = 5006, //!< The camera access level does not allow to call this operation
        DEVICE_ALREADY_INITIALIZED        = 5007, //!< The camera was already initialized
        EXPOSURE_MODE_INVALID             = 5008, //!< The current set exposure mode does not support this operation
        NO_CALIBRATION_DATA               = 5009, //!< The method cannot be called since no calibration data is available
        INSUFFICIENT_BANDWIDTH            = 5010, //!< The interface to the camera module does not provide a sufficient bandwidth
        DUTYCYCLE_NOT_SUPPORTED           = 5011, //!< The duty cycle is not supported
        SPECTRE_NOT_INITIALIZED           = 5012, //!< Spectre was not initialized properly
        NO_USE_CASES                      = 5013, //!< The camera offers no use cases
        NO_USE_CASES_FOR_LEVEL            = 5014, //!< The camera offers no use cases for the current access level

        FSM_INVALID_TRANSITION            = 8096, //!< Camera module state machine does not support current transition

        UNKNOWN                           = 0x7fffff01 //!< Catch-all failure
    };

    /*!
     * Get a human-readable description for a given error message.
     *
     * Note: These descriptions are in English and are intended to help developers,
     * they're not translated to the current locale.
     */
    ROYALE_API royale::String getStatusString (royale::CameraStatus status);

    ROYALE_API ::std::ostream &operator<< (::std::ostream &os, royale::CameraStatus status);
}
