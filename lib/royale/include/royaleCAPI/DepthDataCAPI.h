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
#include <DataStructuresCAPI.h>

ROYALE_CAPI_LINKAGE_TOP

typedef struct
{
    float       x;
    float       y;
    float       z;
    float       noise;
    uint16_t    gray_value;
    uint8_t     depth_confidence;
} royale_depth_point;

typedef struct
{
    int                 version;
    long long           timestamp;
    royale_stream_id    stream_id;
    uint16_t            width;
    uint16_t            height;
    uint32_t            *exposure_times;
    uint32_t            nr_exposure_times;
    royale_depth_point  *points;
    uint32_t            nr_points;
} royale_depth_data;

ROYALE_CAPI typedef void (*ROYALE_DEPTH_DATA_CALLBACK) (royale_depth_data *);

ROYALE_CAPI_LINKAGE_BOTTOM
/** @}*/
