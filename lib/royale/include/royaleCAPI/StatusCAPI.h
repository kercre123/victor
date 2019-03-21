/****************************************************************************\
* Copyright (C) 2016 Infineon Technologies
*
* THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
* KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
* PARTICULAR PURPOSE.
*
\****************************************************************************/

/**
* \addtogroup royaleCAPI
* @{
*/

#pragma once

#include <DefinitionsCAPI.h>

ROYALE_CAPI_LINKAGE_TOP

typedef enum royale_camera_status
{
    ROYALE_STATUS_SUCCESS                           = 0,    //!< Indicates that there isn't an error

    ROYALE_STATUS_RUNTIME_ERROR                     = 1024, //!< Something unexpected happened
    ROYALE_STATUS_DISCONNECTED                      = 1026, //!< Camera device is disconnected
    ROYALE_STATUS_INVALID_VALUE                     = 1027, //!< The value provided is invalid
    ROYALE_STATUS_TIMEOUT                           = 1028, //!< The connection got a timeout

    ROYALE_STATUS_LOGIC_ERROR                       = 2048, //!< This does not make any sense here
    ROYALE_STATUS_NOT_IMPLEMENTED                   = 2049, //!< This feature is not implemented yet
    ROYALE_STATUS_OUT_OF_BOUNDS                     = 2050, //!< Setting/parameter is out of specified range

    ROYALE_STATUS_RESOURCE_ERROR                    = 4096, //!< Cannot access resource
    ROYALE_STATUS_FILE_NOT_FOUND                    = 4097, //!< Specified file was not found
    ROYALE_STATUS_COULD_NOT_OPEN                    = 4098, //!< Cannot open file
    ROYALE_STATUS_DATA_NOT_FOUND                    = 4099, //!< No data available where expected
    ROYALE_STATUS_DEVICE_IS_BUSY                    = 4100, //!< Another action is in progress
    ROYALE_STATUS_WRONG_DATA_FORMAT_FOUND           = 4101, //!< A resource was expected to be in one data format, but was in a different (recognisable) format

    ROYALE_STATUS_USECASE_NOT_SUPPORTED             = 5001, //!< This use case is not supported
    ROYALE_STATUS_FRAMERATE_NOT_SUPPORTED           = 5002, //!< The specified frame rate is not supported
    ROYALE_STATUS_EXPOSURE_TIME_NOT_SUPPORTED       = 5003, //!< The exposure time is not supported
    ROYALE_STATUS_DEVICE_NOT_INITIALIZED            = 5004, //!< The device seems to be uninitialized
    ROYALE_STATUS_CALIBRATION_DATA_ERROR            = 5005, //!< The calibration data is not readable
    ROYALE_STATUS_INSUFFICIENT_PRIVILEGES           = 5006, //!< The camera access level does not allow to call this operation
    ROYALE_STATUS_DEVICE_ALREADY_INITIALIZED        = 5007, //!< The camera was already initialized
    ROYALE_STATUS_EXPOSURE_MODE_INVALID             = 5008, //!< The current set exposure mode does not support this operation
    ROYALE_STATUS_NO_CALIBRATION_DATA               = 5009, //!< The method cannot be called since no calibration data is available
    ROYALE_STATUS_INSUFFICIENT_BANDWIDTH            = 5010, //!< The interface to the camera module does not provide a sufficient bandwidth
    ROYALE_STATUS_DUTYCYCLE_NOT_SUPPORTED           = 5011, //!< The duty cycle is not supported
    SPECTRE_NOT_INITIALIZED                         = 5012, //!< Spectre was not initialized properly

    ROYALE_STATUS_FSM_INVALID_TRANSITION            = 8096, //!< Camera module state machine does not support current transition

    ROYALE_STATUS_UNKNOWN                           = 0x7fffff01, //!< Catch-all failure
    ROYALE_STATUS_INVALID_HANDLE                    = 0x7fffff02 //!< no instance for the provided handle
} royale_camera_status;

/*!
 * Get a human-readable description for a given error message.
 *
 * Note: These descriptions are in English and are intended to help developers,
 * they're not translated to the current locale.
 *
 * free() has to be used on the resulting string.
 */
ROYALE_CAPI char *royale_status_get_error_string (royale_camera_status status);

ROYALE_CAPI_LINKAGE_BOTTOM
/** @}*/
