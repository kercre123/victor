/**
 ****************************************************************************************
 *
 * @file user_proxr_ext_spi.c
 *
 * @brief Proximity reporter external processor over spi user application source code.
 *
 * Copyright (C) 2012. Dialog Semiconductor Ltd, unpublished work. This computer 
 * program includes Confidential, Proprietary Information and is a Trade Secret of 
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited 
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup APP
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"             // SW configuration
#include "user_periph_setup.h"             // SW configuration
#include "user_proxr_ext_spi.h"
#include "arch_api.h"
#include "user_config.h"

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
*/

/**
 ****************************************************************************************
 * @brief User code initiliazation function.
 *
 * @void 
 *
 * @return void.
 ****************************************************************************************
*/

void user_on_init(void)
{	
    arch_set_sleep_mode(app_default_sleep_mode);
}

/// @} APP
