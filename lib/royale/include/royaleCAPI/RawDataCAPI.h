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
    long long        timestamp;
    royale_stream_id stream_id;
    uint16_t         width;
    uint16_t         height;
    uint16_t         **raw_data;
    uint16_t         nr_raw_frames;
    uint32_t         nr_data_per_raw_frame;
    uint32_t         *modulation_frequencies;
    uint32_t         nr_mod_frequencies;
    uint32_t         *exposure_times;
    uint32_t         nr_exposure_times;
    float            illumination_temperature;
    uint16_t         *phase_angles;
    uint32_t         nr_phase_angles;
    uint8_t          *illumination_enabled;
    uint32_t         nr_illumination_enabled;
} royale_raw_data;

ROYALE_CAPI typedef void (*ROYALE_RAW_DATA_CALLBACK) (royale_raw_data *);

ROYALE_CAPI_LINKAGE_BOTTOM
/** @}*/
