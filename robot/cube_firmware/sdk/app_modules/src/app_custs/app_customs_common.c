/**
 ****************************************************************************************
 *
 * @file app_customs_common.c
 * 
 * @brief Application Custom Service profile common file.
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

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
 
#include "rwble_config.h"              // SW configuration
#if (BLE_CUSTOM_SERVER)
#include "app_customs.h"
#include "app_prf_types.h"
#include "prf_types.h"
#include "attm_db.h"
#include "attm_db_128.h"

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

#if (BLE_CUSTOM1_SERVER)
/**
 ****************************************************************************************
 * @brief It is called to validates the characteristic value before it is written to custom1 profile database
 * 
 * @param[in] att_idx   Attribute index to be validated before it is written to database
 * @param[in] last      last request of a multiple prepare write request. 
 *                      When "true" it means the entire value has been received
 * @param[in] offset    offset at which the data has to be written (used only for values 
 *                      that are greater than MTU)
 * @param[in] len       Data length to be written
 * @param[in] value     pointer to data to be written in attribute database
 *
 * @return status       validation status
 ****************************************************************************************
 */
uint8 app_custs1_val_wr_validate (uint16_t att_idx, bool last, uint16_t offset, uint16_t len, uint8_t* value)
{
    return PRF_ERR_OK;
}
#endif // (BLE_CUSTOM1_SERVER)

#if (BLE_CUSTOM2_SERVER)
/**
 ****************************************************************************************
 * @brief It is called to validates the characteristic value before it is written to custom2 profile database
 * 
 * @param[in] att_idx   Attribute index to be validated before it is written to database
 * @param[in] last      last request of a multiple prepare write request. 
 *                      When "true" it means the entire value has been received
 * @param[in] offset    offset at which the data has to be written (used only for values 
 *                      that are greater than MTU)
 * @param[in] len       Data length to be written
 * @param[in] value     pointer to data to be written in attribute database
 *
 * @return status       validation status
 ****************************************************************************************
 */
uint8 app_custs2_val_wr_validate (uint16_t att_idx, bool last, uint16_t offset, uint16_t len, uint8_t* value)
{
    return PRF_ERR_OK;
}

#endif // (BLE_CUSTOM2_SERVER)

#endif // (BLE_CUSTOM_SERVER)
