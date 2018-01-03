/**
 ****************************************************************************************
 *
 * @file hogprh.c
 *
 * @brief HID Over GATT Profile - Report Host Role Implementation
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev$
 *
 ****************************************************************************************
 */


/**
 ****************************************************************************************
 * @addtogroup HOGPRH
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_HID_REPORT_HOST)

#include "hogprh.h"
#include "hogprh_task.h"
#include "prf_utils.h"
#include "gap.h"

/*
 * GLOBAL VARIABLES DEFINITIONS
 ****************************************************************************************
 */

struct hogprh_env_tag **hogprh_envs __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

/// HOGPRH task descriptor
static const struct ke_task_desc TASK_DESC_HOGPRH = {hogprh_state_handler, &hogprh_default_handler, hogprh_state, HOGPRH_STATE_MAX, HOGPRH_IDX_MAX};



/*
 * GLOBAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

void hogprh_init(void)
{
    // Reset all the profile role tasks
    PRF_CLIENT_RESET(hogprh_envs, HOGPRH);
}


void hogprh_enable_cfm_send(struct hogprh_env_tag *hogprh_env, struct prf_con_info *con_info, uint8_t status)
{
    // Counter
    uint8_t i;

    // Send APP the details of the discovered attributes on HOGPBH
    struct hogprh_enable_cfm * rsp = KE_MSG_ALLOC(HOGPRH_ENABLE_CFM,
                                                  con_info->appid, con_info->prf_id,
                                                  hogprh_enable_cfm);

    rsp->conhdl = gapc_get_conhdl(con_info->conidx);
    rsp->status = status;

    if (status == PRF_ERR_OK)
    {
        rsp->hids_nb = hogprh_env->hids_nb;

        for (i = 0; i < hogprh_env->hids_nb; i++)
        {
            rsp->hids[i]    = hogprh_env->hids[i];

            // Register HOGPRH task in gatt for indication/notifications
            prf_register_atthdl2gatt(&hogprh_env->con_info, &hogprh_env->hids[i].svc);
        }

        // Go to connected state
        ke_state_set(con_info->prf_id, HOGPRH_CONNECTED);
    }
    else
    {
        PRF_CLIENT_ENABLE_ERROR(hogprh_envs, con_info->prf_id, HOGPRH);
    }

    ke_msg_send(rsp);
}

void hogprh_char_req_rsp_send(struct hogprh_env_tag *hogprh_env, ke_msg_id_t msg_id, uint8_t status,
                              uint8_t att_code)
{
    struct hogprh_char_req_rsp *rsp = KE_MSG_ALLOC(msg_id,
                                                   hogprh_env->con_info.appid, hogprh_env->con_info.prf_id,
                                                   hogprh_char_req_rsp);

    rsp->conhdl    = gapc_get_conhdl(hogprh_env->con_info.conidx);
    rsp->status    = status;
    rsp->att_code  = hogprh_env->last_char_code;

    // Send the message
    ke_msg_send(rsp);
}

#endif /* (BLE_HID_REPORT_HOST) */

/// @} HOGPRH
