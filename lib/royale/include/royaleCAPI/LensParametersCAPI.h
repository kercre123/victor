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

#include <stdint.h>

#include <DefinitionsCAPI.h>
#include <DataStructuresCAPI.h>

ROYALE_CAPI_LINKAGE_TOP

typedef struct
{
    royale_pair_float_float principalPoint;
    royale_pair_float_float focalLength;
    royale_pair_float_float distortionTangential;
    royale_vector_float distortionRadial;
} royale_lens_parameters;

ROYALE_CAPI void royale_free_lens_parameters (royale_lens_parameters *params);

ROYALE_CAPI_LINKAGE_BOTTOM
/** @}*/
