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
#include <stdint.h>

ROYALE_CAPI_LINKAGE_TOP

typedef enum royale_event_severity
{
    /** Information only event.
    */
    ROYALE_EVENT_SEVERITY_INFO = 0,
    /** Potential issue detected (e.g. soft overtemperature limit reached).
    */
    ROYALE_EVENT_SEVERITY_WARNING = 1,
    /** Errors occurred during operation.
    * The operation (e.g. recording or stream capture) has failed and was stopped.
    */
    ROYALE_EVENT_SEVERITY_ERROR = 2,
    /** Severe error was detected.
    * The corresponding ICameraDevice is no longer in an usable state.
    */
    ROYALE_EVENT_SEVERITY_FATAL = 3
} royale_event_severity;

/**
* Type of an IEvent.
*/
typedef enum royale_event_type
{
    ROYALE_EVENT_TYPE_CAPTURE_STREAM,
    ROYALE_EVENT_TYPE_DEVICE_DISCONNECTED,
    ROYALE_EVENT_TYPE_OVER_TEMPERATURE,
    ROYALE_EVENT_TYPE_RAW_FRAME_STATS
} royale_event_type;

typedef struct
{
    royale_event_severity severity;
    royale_event_type type;
    char *description;
} royale_event;

ROYALE_CAPI typedef void (*ROYALE_EVENT_CALLBACK) (royale_event *);

ROYALE_CAPI_LINKAGE_BOTTOM
/** @}*/
