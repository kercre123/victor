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
    long long                   timestamp;
    royale_stream_id            stream_id;
    uint32_t                    nr_points;
    float                      *xyzcPoints;
} royale_sparse_point_cloud;

ROYALE_CAPI typedef void (*ROYALE_SPC_DATA_CALLBACK) (royale_sparse_point_cloud *);

ROYALE_CAPI_LINKAGE_BOTTOM
/** @}*/
