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

typedef enum royale_variant_type
{
    ROYALE_VARIANT_TYPE_INT,
    ROYALE_VARIANT_TYPE_FLOAT,
    ROYALE_VARIANT_TYPE_BOOL
} royale_variant_type;

typedef struct royale_variant
{
    royale_variant_type type;

    union
    {
        float f;
        int i;
        bool b;
    } data;

    int int_min;
    int int_max;

    float float_min;
    float float_max;
} royale_variant;

ROYALE_CAPI royale_variant royale_variant_create_float (float f);
ROYALE_CAPI royale_variant royale_variant_create_int (int i);
ROYALE_CAPI royale_variant royale_variant_create_bool (bool b);

ROYALE_CAPI royale_variant royale_variant_create_float_with_limits (float f, float min, float max);
ROYALE_CAPI royale_variant royale_variant_create_int_with_limits (int i, int min, int max);

ROYALE_CAPI void royale_variant_set_float (royale_variant *v, float f);
ROYALE_CAPI void royale_variant_set_int (royale_variant *v, int i);
ROYALE_CAPI void royale_variant_set_bool (royale_variant *v, bool b);

ROYALE_CAPI_LINKAGE_BOTTOM
/** @}*/
