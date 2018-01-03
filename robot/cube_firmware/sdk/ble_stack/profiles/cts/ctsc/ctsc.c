/**
 ****************************************************************************************
 *
 * @file ctsc.c
 *
 * @brief Current Time Service Client Implementation.
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
 * @addtogroup ctsc
 * @{
 ****************************************************************************************
 */

 /*
  * INCLUDE FILES
  ****************************************************************************************
  */
#include "rwip_config.h"
#if (BLE_CTS_CLIENT)
#include "ctsc.h"
#include "ctsc_task.h"
#include "gap.h"
#include "prf_utils.h"

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

struct ctsc_env_tag **ctsc_envs __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

/// Current Time Service Client task descriptor
static const struct ke_task_desc TASK_DESC_CTSC = {ctsc_state_handler, &ctsc_default_handler, ctsc_state, CTSC_STATE_MAX, CTSC_IDX_MAX};

/*
 * EXPORTED FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

void ctsc_init(void)
{
    PRF_CLIENT_RESET(ctsc_envs, CTSC);
}

void ctsc_enable_cfm_send(struct ctsc_env_tag *ctsc_env, struct prf_con_info *con_info, uint8_t status)
{
    struct ctsc_enable_cfm *cfm = KE_MSG_ALLOC(CTSC_ENABLE_CFM,
                                               con_info->appid,
                                               con_info->prf_id,
                                               ctsc_enable_cfm);

    cfm->conhdl = gapc_get_conhdl(con_info->conidx);
    cfm->status = status;

    if(status == PRF_ERR_OK)
    {
        cfm->cts = ctsc_env->cts;
        prf_register_atthdl2gatt(&ctsc_env->con_info, &ctsc_env->cts.svc);
        // Go to connected state
        ke_state_set(con_info->prf_id, CTSC_CONNECTED);
    }
    else
    {
        PRF_CLIENT_ENABLE_ERROR(ctsc_envs, con_info->prf_id, CTSC);
    }

    ke_msg_send(cfm);
}

void ctsc_error_ind_send(struct ctsc_env_tag *ctsc_env, uint8_t status)
{
    struct ctsc_error_ind *ind = KE_MSG_ALLOC(CTSC_ERROR_IND,
                                              ctsc_env->con_info.appid, ctsc_env->con_info.prf_id,
                                              ctsc_error_ind);

    ind->conhdl    = gapc_get_conhdl(ctsc_env->con_info.conidx);
    ind->status    = status;

    // Send the message
    ke_msg_send(ind);
}

void ctsc_unpack_curr_time_value(struct cts_curr_time* p_ct_val, uint8_t* packed_ct)
{
    // Date and time
    prf_unpack_date_time(packed_ct,&(p_ct_val->exact_time_256.day_date_time.date_time));
    // Day of week
    p_ct_val->exact_time_256.day_date_time.day_of_week = packed_ct[7];
    // Fraction 256
    p_ct_val->exact_time_256.fraction_256 = packed_ct[8];
    // Adjust reason
    p_ct_val->adjust_reason = packed_ct[9];
}
#endif //BLE_CTS_CLIENT
/// @} WSSC
