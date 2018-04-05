/**
 ****************************************************************************************
 *
 * @file arch_patch.h
 *
 * @brief ROM code patches.
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

#ifndef _ARCH_PATCH_H_
#define _ARCH_PATCH_H_

#include <stdint.h>
#include "rwip_config.h"
#include "ke_msg.h"


// external function declarations
void patch_llm_task(void);
void patch_gtl_task(void);
void patch_llc_task(void);

#if !defined( __DA14581__)
void patch_atts_task(void);
#endif

#if (BLE_HOST_PRESENT)
void patch_gapc_task(void);
#endif

#if ((BLE_APP_PRESENT == 0 || BLE_INTEGRATED_HOST_GTL == 1) && BLE_HOST_PRESENT )
    #define BLE_GTL_PATCH 1
#else 
    #define BLE_GTL_PATCH 0
#endif

/**
 * @brief Registers a task for receiving ATT read request indications (ATTS_READ_REQ_IND).
 * @param[in] taskid    Id of the task.
 * @return void
 */
void dg_register_task_for_read_request(ke_task_id_t taskid);

/**
 * @brief Un-registers a task from receiving ATT read request indications (ATTS_READ_REQ_IND).
 * @param[in] taskid    Id of the task.
 * @return void
 */
void dg_unregister_task_from_read_request(ke_task_id_t taskid);

/**
 * @brief Native function for replying to an ATT read request indication.
 * @param[in] conidx        The index of the connection that this reply applies to.
 * @param[in] handle        The handle of the read response.
 * @param[in] status_code   If status_code = ATT_ERR_NO_ERROR then an ATT read response is sent after reading the DB
 *                          otherwise an ATT error response with this status_code  is sent.
 * @return void
 *
 * For ATT error codes defined in BT 4.0 specs refer to att.h (ATT_ERR_xxxx).
 */
void dg_atts_read_cfm(uint8_t conidx, uint16_t handle, uint8_t status_code);


#endif // _ARCH_PATCH_H_
/// @} 
