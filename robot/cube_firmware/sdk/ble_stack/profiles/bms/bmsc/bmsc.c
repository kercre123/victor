/**
 ****************************************************************************************
 *
 * @file bmsc.c
 *
 * @brief C file - Bond Management Service Client Implementation.
 *
 * Copyright (C) 2015. Dialog Semiconductor Ltd, unpublished work. This computer
 * program includes Confidential, Proprietary Information and is a Trade Secret of
 * Dialog Semiconductor Ltd. All use, disclosure, and/or reproduction is prohibited
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup BMSC
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_BM_CLIENT)
#include "bmsc.h"
#include "bmsc_task.h"
#include "gap.h"
#include "prf_utils.h"

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

struct bmsc_env_tag **bmsc_envs __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

/// BMSC task descriptor
static const struct ke_task_desc TASK_DESC_BMSC = {bmsc_state_handler, &bmsc_default_handler, bmsc_state, BMSC_STATE_MAX, BMSC_IDX_MAX};

/*
 * EXPORTED FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

void bmsc_init(void)
{
    // Reset all the profile role tasks
    PRF_CLIENT_RESET(bmsc_envs, BMSC);
}

void bmsc_enable_cfm_send(struct bmsc_env_tag *bmsc_env, struct prf_con_info *con_info, uint8_t status)
{
    // Send to APP the details of the discovered attributes on BMSS
    struct bmsc_enable_cfm *rsp = KE_MSG_ALLOC(BMSC_ENABLE_CFM,
                                               con_info->appid,
                                               con_info->prf_id,
                                               bmsc_enable_cfm);

    rsp->conhdl = gapc_get_conhdl(con_info->conidx);
    rsp->status = status;

    if (status == PRF_ERR_OK)
    {
        rsp->bms = bmsc_env->bms;

        // Register BMSC task in gatt for indication/notifications
        prf_register_atthdl2gatt(&bmsc_env->con_info, &bmsc_env->bms.svc); // WARNING is it required?

        // Go to connected state
        ke_state_set(con_info->prf_id, BMSC_CONNECTED);
    }
    else
    {
        PRF_CLIENT_ENABLE_ERROR(bmsc_envs, con_info->prf_id, BMSC);
    }

    ke_msg_send(rsp);
}

uint8_t bmsc_validate_request(struct bmsc_env_tag *bmsc_env, uint16_t conhdl, uint8_t char_code)
{
    uint8_t status = PRF_ERR_OK;

    // Check if client enabled
    if (conhdl == gapc_get_conhdl(bmsc_env->con_info.conidx))
    {
        // check if feature val characteristic exists
        if (bmsc_env->bms.chars[char_code].val_hdl == ATT_INVALID_HANDLE)
        {
            status = PRF_ERR_INEXISTENT_HDL;
        }
    }
    else
    {
        status = PRF_ERR_INVALID_PARAM;
    }

    return status;
}

void bmsc_error_ind_send(struct bmsc_env_tag *bmsc_env, uint8_t status)
{
    struct bmsc_error_ind *ind = KE_MSG_ALLOC(BMSC_ERROR_IND,
                                              bmsc_env->con_info.appid,
                                              bmsc_env->con_info.prf_id,
                                              bmsc_error_ind);

    ind->conhdl = gapc_get_conhdl(bmsc_env->con_info.conidx);
    // It will be an BMSC status code
    ind->status = status;

    // Send the message
    ke_msg_send(ind);
}

#endif // (BLE_BM_CLIENT)

/// @} BMSC
