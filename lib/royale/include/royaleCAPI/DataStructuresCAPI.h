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

ROYALE_CAPI_LINKAGE_TOP

typedef struct
{
    uint32_t first;
    uint32_t second;
} royale_pair_uint32_uint32;

typedef struct
{
    float first;
    float second;
} royale_pair_float_float;

typedef struct
{
    char *first;
    char *second;
} royale_pair_string_string;

typedef struct
{
    char *first;
    uint64_t second;
} royale_pair_string_uint64;

typedef struct
{
    float *values;
    uint32_t nrOfValues;
} royale_vector_float;

typedef uint16_t royale_stream_id;

typedef struct
{
    royale_stream_id *values;
    uint32_t nrOfValues;
} royale_vector_stream_id;

ROYALE_CAPI void royale_free_pair_string_string (royale_pair_string_string *pair);
ROYALE_CAPI void royale_free_pair_string_string_array (royale_pair_string_string **pair_array, uint32_t nr_elements);

ROYALE_CAPI void royale_free_pair_string_uint64 (royale_pair_string_uint64 *pair);
ROYALE_CAPI void royale_free_pair_string_uint64_array (royale_pair_string_uint64 **pair_array, uint32_t nr_elements);

ROYALE_CAPI void royale_free_vector_float (royale_vector_float *vector);

ROYALE_CAPI void royale_free_vector_stream_id (royale_vector_stream_id *vector);

ROYALE_CAPI void royale_free_string_array (char **string_array, uint32_t nr_strings);
ROYALE_CAPI void royale_free_string_array2 (char ***string_array, uint32_t nr_strings);
ROYALE_CAPI void royale_free_string (char *string_ptr);

ROYALE_CAPI_LINKAGE_BOTTOM
/** @}*/
