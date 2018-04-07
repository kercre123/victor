/**
 ****************************************************************************************
 *
 * @file app_findme.c
 *
 * @brief Findme locator and target application.
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

#include "rwip_config.h"
#include "app_findme.h"

#if BLE_FINDME_LOCATOR
#include "findl_task.h"
#include "app.h"
#endif

#if BLE_FINDME_TARGET
#include "findt_task.h"
#include "app.h"
#endif

/*
 * LOCAL VARIABLES DEFINITION
 ****************************************************************************************
 */

#if (BLE_FINDME_LOCATOR || BLE_FINDME_TARGET)
static uint16_t active_conhdl __attribute__((section("retention_mem_area0"),zero_init));
#endif

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

#if BLE_FINDME_TARGET
void app_findt_enable(uint16_t conhdl)
{
    // Allocate the message
    struct findt_enable_req *req = KE_MSG_ALLOC(FINDT_ENABLE_REQ, TASK_FINDT, TASK_APP,
                                                 findt_enable_req);

    // Fill in the parameter structure
    active_conhdl = conhdl;
    req->conhdl = conhdl;
    req->sec_lvl = get_user_prf_srv_perm(TASK_FINDT);

    // Send the message
    ke_msg_send(req);
}

void default_findt_alert_ind_hnd(uint8_t connection_idx, uint8_t alert_lvl)
{
    if (alert_lvl)
    {
        app_proxr_alert_start(alert_lvl);
    }
    else
    {
        app_proxr_alert_stop();
    }
}
#endif // BLE_FINDME_TARGET

#if BLE_FINDME_LOCATOR
void app_findl_init(void)
{
}

void app_findl_enable(uint16_t conhdl)
{
    // Allocate the message
    struct findl_enable_req *req = KE_MSG_ALLOC(FINDL_ENABLE_REQ, TASK_FINDL, TASK_APP,
                                                 findl_enable_req);
    active_conhdl = conhdl;

    // Fill in the parameter structure
    req->conhdl = conhdl;
    req->con_type = PRF_CON_DISCOVERY;

    // Send the message
    ke_msg_send(req);
}

void app_findl_set_alert(void)
{
    struct findl_set_alert_req *req = KE_MSG_ALLOC(FINDL_SET_ALERT_REQ, TASK_FINDL, TASK_APP,
                                                   findl_set_alert_req);

    req->conhdl = active_conhdl;
    req->alert_lvl = FINDL_ALERT_HIGH;

    // Send the message
    ke_msg_send(req);
}
#endif // BLE_FINDME_LOCATOR

/// @} APP
