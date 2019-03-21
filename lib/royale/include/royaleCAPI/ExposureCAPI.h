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
#include <CAPIVersion.h>
#include <stdint.h>

ROYALE_CAPI_LINKAGE_TOP

ROYALE_CAPI typedef void (*ROYALE_EXPOSURE_CALLBACK_v210) (uint32_t);
ROYALE_CAPI typedef void (*ROYALE_EXPOSURE_CALLBACK_v300) (uint32_t, uint16_t);

ROYALE_CAPI_LINKAGE_BOTTOM
/** @}*/
