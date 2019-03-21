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
    int64_t          timestamp;
    royale_stream_id stream_id;
    uint16_t         width;
    uint16_t         height;
    uint8_t          *data;
    uint32_t         nr_data_entries;
} royale_ir_image;

ROYALE_CAPI typedef void (*ROYALE_IR_IMAGE_CALLBACK) (royale_ir_image *);

ROYALE_CAPI_LINKAGE_BOTTOM
/** @}*/
