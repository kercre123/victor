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
#include <stdbool.h>

#include <DepthDataCAPI.h>
#include <IntermediateDataCAPI.h>
#include <RawDataCAPI.h>

ROYALE_CAPI_LINKAGE_TOP

typedef enum royale_callback_data
{
    ROYALE_CB_DATA_NONE = 0x00,             //!< only get the callback but no data delivery
    ROYALE_CB_DATA_RAW = 0x01,              //!< raw frames, if exclusively used no processing pipe is executed (no calibration data is needed)
    ROYALE_CB_DATA_DEPTH = 0x02,            //!< one depth and grayscale image will be delivered for the complete sequence
    ROYALE_CB_DATA_INTERMEDIATE = 0x04      //!< all intermediate data will be delivered which are generated in the processing pipeline
} royale_callback_data;

typedef struct
{
    bool                        has_depth_data;
    bool                        has_intermediate_data;
    bool                        has_raw_data;
    royale_depth_data           *depth_data;
    royale_intermediate_data    *intermediate_data;
    royale_raw_data             *raw_data;
} royale_extended_data;

typedef void (*ROYALE_EXTENDED_DATA_CALLBACK) (royale_extended_data *);

ROYALE_CAPI_LINKAGE_BOTTOM
/** @}*/
