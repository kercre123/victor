/****************************************************************************\
 * Copyright (C) 2017 pmdtechnologies ag
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

ROYALE_CAPI_LINKAGE_TOP

typedef enum royale_trigger_mode
{
    ROYALE_TRIGGER_MASTER,         //!< Camera set to master mode
    ROYALE_TRIGGER_SLAVE           //!< Camera set to slave mode
} royale_trigger_mode;

ROYALE_CAPI_LINKAGE_BOTTOM
/** @}*/
