/**
 ****************************************************************************************
 *
 * @file custom_common.h
 *
 * @brief Custom Service profile common header file.
 *
 * Copyright (C) 2014. Dialog Semiconductor Ltd, unpublished work. This computer 
 * program includes Confidential, Proprietary Information and is a Trade Secret of 
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited 
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

#ifndef __CUSTOM_COMMON_H
#define __CUSTOM_COMMON_H

#include "gattc_task.h"

/**
 ****************************************************************************************
 * @brief Validates the value of the Client Characteristic CFG.
 * @param[in] bool  indicates whether the CFG is Notification (true) or Indication (false)
 * @param[in] param Pointer to the parameters of the message.
 * @return status.
 ****************************************************************************************
 */

int check_client_char_cfg(bool is_notification, struct gattc_write_cmd_ind const *param);

/**
 ****************************************************************************************
 * @brief  Find the handle of the Characteristic Value having as input the Client Characteristic CFG handle
 * @param[in] handle_cfg  the Client Characteristic CFG handle
 * @return the coresponding value handle
 ****************************************************************************************
 */

uint16_t get_value_handle( uint16_t handle_cfg );

/**
 ****************************************************************************************
 * @brief  Find the handle of Client Characteristic CFG handle having as input the Characteristic value handle
 * @param[in] value_handle  the Characteristic value handle
 * @param[in] max_handle    the last handle of the service
 * @return the coresponding Client Characteristic CFG handle
 ****************************************************************************************
 */
uint16_t get_cfg_handle( uint16_t value_handle, uint16_t last_handle );

#endif // __CUSTOM_COMMON_H
