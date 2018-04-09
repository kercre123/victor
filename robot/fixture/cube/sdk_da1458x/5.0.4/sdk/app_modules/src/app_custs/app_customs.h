/**
 ****************************************************************************************
 *
 * @file app_customs.h
 *
 * @brief Custom Service Application entry point.
 *
 * Copyright (C) 2015. Dialog Semiconductor Ltd, unpublished work. This computer 
 * program includes Confidential, Proprietary Information and is a Trade Secret of 
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited 
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

#ifndef _APP_CUSTOMS_H_
#define _APP_CUSTOMS_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
 
#include "user_profiles_config.h"

#if (BLE_CUSTOM_SERVER)

#include <stdint.h>
#include "prf_types.h"
#include "attm_db_128.h"

/*
 * FUNCTIONS DECLARATION
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialize Custom Service Application.
 * @return void
 ****************************************************************************************
 */
void app_custs1_init(void);

/**
 ****************************************************************************************
 * @brief Add a Custom Service instance in the DB.
 * @return void
 ****************************************************************************************
 */
void app_custs1_create_db(void);

/**
 ****************************************************************************************
 * @brief Enable the Custom Service.
 * @param[in] conhdl    Connection handle
 * @return void
 ****************************************************************************************
 */
void app_custs1_enable(uint16_t conhdl);

/**
 ****************************************************************************************
 * @brief It is called to validates the characteristic value before it is written to profile database.
 * @param[in] att_idx   Attribute index to be validated before it is written to database
 * @param[in] last      last request of a multiple prepare write request. 
 *                      When "true" it means the entire value has been received
 * @param[in] offset    offset at which the data has to be written (used only for values 
 *                      that are greater than MTU)
 * @param[in] len       Data length to be written
 * @param[in] value     pointer to data to be written in attribute database
 * @return status       validation status
 ****************************************************************************************
 */
uint8 app_custs1_val_wr_validate(uint16_t att_idx, bool last, uint16_t offset, uint16_t len, uint8_t* value);

/**
 ****************************************************************************************
 * @brief Initialize Custom Service Application.
 * @return void
 ****************************************************************************************
 */
void app_custs2_init(void);

/**
 ****************************************************************************************
 * @brief Add a Custom Service instance in the DB.
 * @return void
 ****************************************************************************************
 */
void app_custs2_create_db(void);

/**
 ****************************************************************************************
 * @brief Enable the Custom Service.
 * @param[in] conhdl    Connection handle
 * @return void
 ****************************************************************************************
 */
void app_custs2_enable(uint16_t conhdl);

/**
 ****************************************************************************************
 * @brief It is called to validates the characteristic value before it is written to profile database.
 * @param[in] att_idx   Attribute index to be validated before it is written to database
 * @param[in] last      last request of a multiple prepare write request. 
 *                      When "true" it means the entire value has been received
 * @param[in] offset    offset at which the data has to be written (used only for values 
 *                      that are greater than MTU)
 * @param[in] len       Data length to be written
 * @param[in] value     pointer to data to be written in attribute database
 * @return status       validation status
 ****************************************************************************************
 */
uint8 app_custs2_val_wr_validate(uint16_t att_idx, bool last, uint16_t offset, uint16_t len, uint8_t* value);

#endif // (BLE_CUSTOM_SERVER)

#endif // _APP_CUSTOMS_H_
