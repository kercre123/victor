/**
 ****************************************************************************************
 *
 * @file app_msg_utils.c
 *
 * @brief Application entry point
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

#if (BLE_APP_PRESENT)
#include "app_task.h"                // Application task Definition
#include "app.h"                     // Application Definition
#include "ke_msg.h"
#include "app_msg_utils.h"
#include "arch_api.h"
/*
 * ENUMERATIONS
 ****************************************************************************************
 */


/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */
 
#include "rwip.h"

/**
 ****************************************************************************************
 * @brief Check if the BLE is active or not.
 * @return Returns true if BLE is active.
 ****************************************************************************************
 */
bool app_check_BLE_active(void)
{
    return ((GetBits16(CLK_RADIO_REG, BLE_ENABLE) == 1) &&\
            (GetBits32(BLE_DEEPSLCNTL_REG, DEEP_SLEEP_STAT) == 0) &&\
            !(rwip_prevent_sleep_get() & RW_WAKE_UP_ONGOING)) ;
}

/**
 ****************************************************************************************
 * @brief Sends a message and wakes BLE up if not active.
 * @param[in] cmd Pointer to the message.
 * @return None.
 ****************************************************************************************
 */
void app_msg_send_wakeup_ble(void *cmd)
{
   if (!app_check_BLE_active())
      arch_ble_force_wakeup();
    ke_msg_send(cmd);
}




 
 
 #endif //(BLE_APP_PRESENT)

/// @} APP
