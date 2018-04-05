/**
 ****************************************************************************************
 *
 * @file hrpc.c
 *
 * @brief Heart Rate Profile Collector implementation.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev$
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup HRPC
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_HR_COLLECTOR)
#include "hrpc.h"
#include "hrpc_task.h"
#include "gap.h"

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */
struct hrpc_env_tag **hrpc_envs __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

/// Heart Rate Task
static const struct ke_task_desc TASK_DESC_HRPC = {hrpc_state_handler, &hrpc_default_handler, hrpc_state, HRPC_STATE_MAX, HRPC_IDX_MAX};


/*
 * EXPORTED FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

void hrpc_init(void)
{
    // Reset all the profile role tasks
    PRF_CLIENT_RESET(hrpc_envs, HRPC);
}

void hrpc_enable_cfm_send(struct hrpc_env_tag *hrpc_env, struct prf_con_info *con_info, uint8_t status)
{
    //send APP the details of the discovered attributes on HTPT
    struct hrpc_enable_cfm * rsp = KE_MSG_ALLOC(HRPC_ENABLE_CFM,
                                                con_info->appid, con_info->prf_id,
                                                hrpc_enable_cfm);

    rsp->conhdl = gapc_get_conhdl(con_info->conidx);
    rsp->status = status;

    if (status == PRF_ERR_OK)
    {
        rsp->hrs = hrpc_env->hrs;

        //register HRPC task in gatt for indication/notifications
        prf_register_atthdl2gatt(&hrpc_env->con_info, &hrpc_env->hrs.svc);

        // Go to connected state
        ke_state_set(hrpc_env->con_info.prf_id, HRPC_CONNECTED);
    }
    else
    {
        PRF_CLIENT_ENABLE_ERROR(hrpc_envs, con_info->prf_id, HRPC);
    }

    ke_msg_send(rsp);
}

void hrpc_unpack_meas_value(struct hrs_hr_meas* pmeas_val, uint8_t* packed_hr, uint8_t size)
{
    int8_t cursor = 0;
    int8_t i = 0;

    // Heart Rate measurement flags
    pmeas_val->flags = packed_hr[0];
    cursor += 1;

    if ((pmeas_val->flags & HRS_FLAG_HR_16BITS_VALUE) == HRS_FLAG_HR_16BITS_VALUE)
    {
        // Heart Rate Measurement Value 16 bits
        pmeas_val->heart_rate = co_read16p(&packed_hr[cursor]);
        cursor += 2;
    }
    else
    {
        // Heart Rate Measurement Value 8 bits
        pmeas_val->heart_rate = (uint16_t) packed_hr[cursor];
        cursor += 1;
    }

    if ((pmeas_val->flags & HRS_FLAG_ENERGY_EXPENDED_PRESENT) == HRS_FLAG_ENERGY_EXPENDED_PRESENT)
    {
        // Energy Expended present
        pmeas_val->energy_expended = co_read16p(&packed_hr[cursor]);
        cursor += 2;
    }

    // retrieve number of rr intervals
    pmeas_val->nb_rr_interval = ((size - cursor) > 0 ? ((size - cursor)/ 2) : (0));

    for(i = 0 ; (i < (pmeas_val->nb_rr_interval)) && (i < (HRS_MAX_RR_INTERVAL)) ; i++)
    {
        // RR-Intervals
        pmeas_val->rr_intervals[i] = co_read16p(&packed_hr[cursor]);
        cursor += 2;
    }
}

void hrpc_error_ind_send(struct hrpc_env_tag *hrpc_env, uint8_t status)
{
    struct hrpc_error_ind *ind = KE_MSG_ALLOC(HRPC_ERROR_IND,
                                              hrpc_env->con_info.appid, hrpc_env->con_info.prf_id,
                                              hrpc_error_ind);

    ind->conhdl    = gapc_get_conhdl(hrpc_env->con_info.conidx);
    // It will be an HTPC status code
    ind->status    = status;

    // Send the message
    ke_msg_send(ind);
}

#endif /* (BLE_HR_COLLECTOR) */

/// @} HRPC
