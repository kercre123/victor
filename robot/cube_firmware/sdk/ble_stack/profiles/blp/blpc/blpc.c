/**
 ****************************************************************************************
 *
 * @file blpc.c
 *
 * @brief Blood Pressure Profile Collector implementation.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev$
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup BLPC
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_BP_COLLECTOR)
#include "blpc.h"
#include "blpc_task.h"
#include "gap.h"

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */
struct blpc_env_tag **blpc_envs __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

/// Blood Pressure task descriptor
static const struct ke_task_desc TASK_DESC_BLPC = {blpc_state_handler, &blpc_default_handler, blpc_state, BLPC_STATE_MAX, BLPC_IDX_MAX};


/*
 * EXPORTED FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

void blpc_init(void)
{
    // Reset all the profile role tasks
    PRF_CLIENT_RESET(blpc_envs, BLPC);
}


void blpc_enable_cfm_send(struct blpc_env_tag *blpc_env, struct prf_con_info *con_info, uint8_t status)
{
    // Send to APP the details of the discovered attributes on BLPS
    struct blpc_enable_cfm * rsp = KE_MSG_ALLOC(BLPC_ENABLE_CFM,
                                                con_info->appid, con_info->prf_id,
                                                blpc_enable_cfm);

    rsp->conhdl = gapc_get_conhdl(con_info->conidx);
    rsp->status = status;

    if (status == PRF_ERR_OK)
    {
        rsp->bps = blpc_env->bps;

        prf_register_atthdl2gatt(&blpc_env->con_info, &blpc_env->bps.svc);

        // Go to connected state
        ke_state_set(blpc_env->con_info.prf_id, BLPC_CONNECTED);
    }
    else
    {
        PRF_CLIENT_ENABLE_ERROR(blpc_envs, con_info->prf_id, BLPC);
    }

    ke_msg_send(rsp);
}

void blpc_unpack_meas_value(struct bps_bp_meas* pmeas_val, uint8_t* packed_bp)
{
    uint8_t cursor;

    // blood pressure measurement flags
    pmeas_val->flags = packed_bp[0];

    // Blood Pressure Measurement Compound Value - Systolic
    pmeas_val->systolic = co_read16p(&(packed_bp[1]));

    // Blood Pressure Measurement Compound Value - Diastolic (mmHg)
    pmeas_val->diastolic = co_read16p(&(packed_bp[3]));

    //  Blood Pressure Measurement Compound Value - Mean Arterial Pressure (mmHg)
    pmeas_val->mean_arterial_pressure = co_read16p(&(packed_bp[5]));

    cursor = 7;

    // time flag set
    if ((pmeas_val->flags & BPS_FLAG_TIME_STAMP_PRESENT) == BPS_FLAG_TIME_STAMP_PRESENT)
    {
        cursor += prf_unpack_date_time(packed_bp + cursor, &(pmeas_val->time_stamp));
    }

    // pulse rate flag set
    if ((pmeas_val->flags & BPS_FLAG_PULSE_RATE_PRESENT) == BPS_FLAG_PULSE_RATE_PRESENT)
    {
        pmeas_val->pulse_rate = co_read16p(&(packed_bp[cursor + 0]));
        cursor += 2;
    }

    // User ID flag set
    if ((pmeas_val->flags & BPS_FLAG_USER_ID_PRESENT) == BPS_FLAG_USER_ID_PRESENT)
    {
        pmeas_val->user_id = packed_bp[cursor + 0];
        cursor += 1;
    }

    // measurement status flag set
    if ((pmeas_val->flags & BPS_FLAG_MEAS_STATUS_PRESENT) == BPS_FLAG_MEAS_STATUS_PRESENT)
    {
        pmeas_val->meas_status = co_read16p(&(packed_bp[cursor + 0]));
        cursor += 2;
    }
}

void blpc_error_ind_send(struct blpc_env_tag *blpc_env, uint8_t status)
{
    struct blpc_error_ind *ind = KE_MSG_ALLOC(BLPC_ERROR_IND,
                                              blpc_env->con_info.appid, blpc_env->con_info.prf_id,
                                              blpc_error_ind);

    ind->conhdl    = gapc_get_conhdl(blpc_env->con_info.conidx);
    // It will be an PRF status code
    ind->status    = status;

    // Send the message
    ke_msg_send(ind);
}

#endif /* (BLE_BP_COLLECTOR) */

/// @} BLPC
