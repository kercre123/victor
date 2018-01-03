/**
 ****************************************************************************************
 *
 * @file app_diss.h
 *
 * @brief Device Information Service Application entry point.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

#ifndef _APP_DISS_H_
#define _APP_DISS_H_

/**
 ****************************************************************************************
 * @addtogroup APP
 *
 * @brief Device Information Service Application entry point.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "user_profiles_config.h"

#if (BLE_DIS_SERVER)
#include <stdint.h>          // Standard Integer Definition
#include <co_bt.h>
#include "ble_580_sw_version.h"
#include "user_config_sw_ver.h"

/*
 * FUNCTIONS DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 *
 * Device Information Service Application Functions
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialize Device Information Service Application.
 * @return void
 ****************************************************************************************
 */
void app_dis_init(void);

/**
 ****************************************************************************************
 * @brief Add a Device Information Service instance in the DB.
 * @return void
 ****************************************************************************************
 */
void app_diss_create_db(void);

/**
 ****************************************************************************************
 * @brief Enable the Device Information Service.
 * @param[in] conhdl Connection handle
 * @return void
 ****************************************************************************************
 */
void app_diss_enable(uint16_t conhdl);

#endif // (BLE_DIS_SERVER)

/// @} APP

#endif // _APP_DISS_H_
