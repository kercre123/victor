/**
 ****************************************************************************************
 *
 * @file htpc.c
 *
 * @brief Health Thermometer Profile Collector implementation.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup HTPC
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_HT_COLLECTOR)
#include "htpc.h"
#include "htpc_task.h"
#include "gap.h"
#include "prf_utils.h"

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

struct htpc_env_tag **htpc_envs __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

/// Health Thermometer task descriptor
static const struct ke_task_desc TASK_DESC_HTPC = {htpc_state_handler, &htpc_default_handler, htpc_state, HTPC_STATE_MAX, HTPC_IDX_MAX};


/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void htpc_init(void)
{
    // Reset all the profile role tasks
    PRF_CLIENT_RESET(htpc_envs, HTPC);
}

void htpc_error_ind_send(struct htpc_env_tag *htpc_env, uint8_t status)
{
    struct htpc_error_ind *ind = KE_MSG_ALLOC(HTPC_ERROR_IND,
                                              htpc_env->con_info.appid, htpc_env->con_info.prf_id,
                                              htpc_error_ind);

    // Connection handle
    ind->conhdl    = gapc_get_conhdl(htpc_env->con_info.conidx);
    // Error status
    ind->status    = status;

    // Send the message
    ke_msg_send(ind);
}

void htpc_enable_cfm_send(struct htpc_env_tag *htpc_env, struct prf_con_info *con_info, uint8_t status)
{
    // Inform the APP about the status of the enabling of the Health Thermometer Profile Collector role 5awk
    struct htpc_enable_cfm *cfm = KE_MSG_ALLOC(HTPC_ENABLE_CFM,
                                               con_info->appid, con_info->prf_id,
                                               htpc_enable_cfm);

    // Connection Handle
    cfm->conhdl = gapc_get_conhdl(con_info->conidx);
    // Status
    cfm->status = status;

    // If status is PRF_ERR_OK, hts is non NULL
    if (status == PRF_ERR_OK)
    {
        // Attributes discovered in the peer device database
        cfm->hts = htpc_env->hts;

        // Register the profile task in GATT in order to receive notifications/indications
        prf_register_atthdl2gatt(con_info, &htpc_env->hts.svc);

        // Go to connected state
        ke_state_set(con_info->prf_id, HTPC_CONNECTED);
    }
    else
    {
        PRF_CLIENT_ENABLE_ERROR(htpc_envs, con_info->prf_id, HTPC);
    }

    // Send the confirmation to the application
    ke_msg_send(cfm);
}

void htpc_unpack_temp(struct htpc_env_tag *htpc_env, uint8_t *packed_temp,
                      uint8_t length, uint8_t flag_stable_meas)
{
    // Cursor used to read parameter in the packed temperature value
    uint8_t cursor = 0;

    // Checked the length of the received value
    if (length >= HTPC_PACKED_TEMP_MIN_LEN)
    {
        // Prepare the message to send to the application
        struct htpc_temp_ind *ind = KE_MSG_ALLOC(HTPC_TEMP_IND,
                                                 htpc_env->con_info.appid, htpc_env->con_info.prf_id,
                                                 htpc_temp_ind);

        // Connection handle
        ind->conhdl = gapc_get_conhdl(htpc_env->con_info.conidx);

        // Unpack Temp Measurement
        ind->temp_meas.flags = *(packed_temp + cursor);
        cursor += 1;

        ind->temp_meas.temp = co_read32p(packed_temp + cursor);
        cursor += 4;

        // Time Flag Set
        if ((ind->temp_meas.flags & HTPT_FLAG_TIME) == HTPT_FLAG_TIME)
        {
            cursor += prf_unpack_date_time(packed_temp+cursor, &(ind->temp_meas.time_stamp));
        }

        // Type Flag set
        if ((ind->temp_meas.flags & HTPT_FLAG_TYPE) == HTPT_FLAG_TYPE)
        {
            ind->temp_meas.type    = *(packed_temp + cursor);
        }

        // Stable or intermediary type of temperature
        ind->flag_stable_meas = flag_stable_meas;

        //Send request to ATT
        ke_msg_send(ind);
    }
}

uint8_t htpc_get_next_desc_char_code(struct htpc_env_tag *htpc_env,
                                     const struct prf_char_desc_def *desc_def)
{
    // Counter
    uint8_t i;
    uint8_t next_char_code;

    for (i=0; i<HTPC_DESC_HTS_MAX; i++)
    {
        next_char_code = desc_def[i].char_code;

        if (next_char_code > htpc_env->last_char_code)
        {
            //If Char. has been found and number of attributes is upper than 2
            if ((htpc_env->hts.chars[next_char_code].char_hdl != 0)
                    & (htpc_env->hts.chars[next_char_code].char_ehdl_off > 2))
            {
                return next_char_code;
            }
        }
    }

    return i;
}

#endif //BLE_HT_COLLECTOR
/// @} HTPC
