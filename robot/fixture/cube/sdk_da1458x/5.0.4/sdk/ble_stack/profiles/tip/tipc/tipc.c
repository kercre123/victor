/**
 ****************************************************************************************
 *
 * @file tipc.c
 *
 * @brief Time Profile Client implementation.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev$
 *
 ****************************************************************************************
 */


/**
 ****************************************************************************************
 * @addtogroup TIPC
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_TIP_CLIENT)
#include "tipc.h"
#include "tipc_task.h"
#include "gap.h"

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */
struct tipc_env_tag **tipc_envs __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

/// Time Tasks
static const struct ke_task_desc TASK_DESC_TIPC = {tipc_state_handler, &tipc_default_handler, tipc_state, TIPC_STATE_MAX, TIPC_IDX_MAX};


/*
 * EXPORTED FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

void tipc_init(void)
{
    // Reset all the profile role tasks
    PRF_CLIENT_RESET(tipc_envs, TIPC);
}

void tipc_enable_cfm_send(struct tipc_env_tag *tipc_env, struct prf_con_info *con_info, uint8_t status)
{
    // Send to APP the details of the discovered attributes on TIPS
    struct tipc_enable_cfm * rsp = KE_MSG_ALLOC(TIPC_ENABLE_CFM,
                                                con_info->appid, con_info->prf_id,
                                                tipc_enable_cfm);

    rsp->conhdl = gapc_get_conhdl(con_info->conidx);
    rsp->status = status;

    if (status == PRF_ERR_OK)
    {
        rsp->cts    = tipc_env->cts;
        rsp->ndcs   = tipc_env->ndcs;
        rsp->rtus   = tipc_env->rtus;

        //register TIPC task in gatt for indication/notifications
        prf_register_atthdl2gatt(&tipc_env->con_info, &tipc_env->cts.svc);

        // Go to connected state
        ke_state_set(tipc_env->con_info.prf_id, TIPC_CONNECTED);
    }
    else
    {
        PRF_CLIENT_ENABLE_ERROR(tipc_envs, con_info->prf_id, TIPC);
    }

    ke_msg_send(rsp);
}

void tipc_error_ind_send(struct tipc_env_tag *tipc_env, uint8_t status)
{
    struct tipc_error_ind *ind = KE_MSG_ALLOC(TIPC_ERROR_IND,
                                              tipc_env->con_info.appid, tipc_env->con_info.prf_id,
                                              tipc_error_ind);

    ind->conhdl    = gapc_get_conhdl(tipc_env->con_info.conidx);
    // It will be an TIPC status code
    ind->status    = status;

    // send the message
    ke_msg_send(ind);
}

void tipc_unpack_curr_time_value(struct tip_curr_time* p_curr_time_val, uint8_t* packed_ct)
{
    //Date-Time
    prf_unpack_date_time(packed_ct, &(p_curr_time_val->exact_time_256.day_date_time.date_time));

    //Day of Week
    p_curr_time_val->exact_time_256.day_date_time.day_of_week = packed_ct[7];

    //Fraction 256
    p_curr_time_val->exact_time_256.fraction_256 = packed_ct[8];

    //Adjust Reason
    p_curr_time_val->adjust_reason = packed_ct[9];
}

void tipc_unpack_time_dst_value(struct tip_time_with_dst* p_time_dst_val, uint8_t* packed_tdst)
{
    //Date-Time
    prf_unpack_date_time(packed_tdst, &(p_time_dst_val->date_time));

    //DST Offset
    p_time_dst_val->dst_offset = packed_tdst[7];
}

#endif /* (BLE_TIP_CLIENT) */

/// @} TIPC
