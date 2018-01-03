/**
 ****************************************************************************************
 *
 * @file wpts.c
 *
 * @brief Wireless Power Transfer client.
 *
 * Copyright (C) 2013 Dialog Semiconductor GmbH and its Affiliates, unpublished work
 * This computer program includes Confidential, Proprietary Information and is a Trade Secret 
 * of Dialog Semiconductor GmbH and its Affiliates. All use, disclosure, and/or 
 * reproduction is prohibited unless authorized in writing. All Rights Reserved.
 *
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwble_config.h"

#if (BLE_WPT_CLIENT)

#include "wptc.h"
#include "wptc_task.h"
#include "attm_db.h"
#include "gapc.h"

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

struct wptc_env_tag **wptc_envs __attribute__((section("exchange_mem_case1"))); //@RETENTION MEMORY

/// WPTC task descriptor
static const struct ke_task_desc TASK_DESC_WPTC = {wptc_state_handler, &wptc_default_handler, wptc_state, WPTC_STATE_MAX, WPTC_IDX_MAX};


/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void wptc_init(void)
{
    // Reset all the profile role tasks
    PRF_CLIENT_RESET(wptc_envs, WPTC);
}

void wptc_enable_cfm_send(struct wptc_env_tag *wptc_env, struct prf_con_info *con_info, uint8_t status)
{
    //format response to app
    struct wptc_enable_cfm * cfm = KE_MSG_ALLOC(WPTC_ENABLE_CFM,
                                                 con_info->appid, con_info->prf_id,
                                                 wptc_enable_cfm);

    cfm->conhdl = gapc_get_conhdl(con_info->conidx);
    cfm->status = status;

    if (status == PRF_ERR_OK)
    {
        cfm->wpts = wptc_env->wpts;

        // Register WPT Client task in gatt for indication/notifications
        prf_register_atthdl2gatt(&wptc_env->con_info, &wptc_env->wpts.svc);


        // Go to connected state
        ke_state_set(con_info->prf_id, WPTC_CONNECTED);
    }
    else
    {
        PRF_CLIENT_ENABLE_ERROR(wptc_envs, con_info->prf_id, WPTC);
    }

    ke_msg_send(cfm);
}


void wptc_write_char_rsp_send(struct wptc_env_tag *wptc_env, uint8_t status)
{
    // Send response to app
    struct wptc_wr_char_rsp *wr_cfm = KE_MSG_ALLOC(WPTC_WR_CHAR_RSP,
                                                    wptc_env->con_info.appid, wptc_env->con_info.prf_id,
                                                    wptc_wr_char_rsp);

    wr_cfm->conhdl    = gapc_get_conhdl(wptc_env->con_info.conidx);
    wr_cfm->status    = status;

    ke_msg_send(wr_cfm);
}


#endif //BLE_WPT_CLIENT
