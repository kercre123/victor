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
    float       distance;
    float       amplitude;
    float       intensity;
    uint32_t    flags;
} royale_intermediate_point;

typedef struct
{
    int                         version;
    long long                   timestamp;
    royale_stream_id            stream_id;
    uint16_t                    width;
    uint16_t                    height;
    royale_intermediate_point   *points;
    uint32_t                    nr_points;
    uint32_t                    *modulation_frequencies;
    uint32_t                    nr_mod_frequencies;
    uint32_t                    *exposure_times;
    uint32_t                    nr_exposure_times;
    uint32_t                    nr_frequencies;
} royale_intermediate_data;

ROYALE_CAPI typedef void (*ROYALE_INTERMEDIATE_DATA_CALLBACK) (royale_intermediate_data *);

ROYALE_CAPI_LINKAGE_BOTTOM
/** @}*/
