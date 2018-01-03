/**
 ****************************************************************************************
 *
 * @file hogpbh.c
 *
 * @brief HID Over GATT Profile - Boot Host Role Implementation
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev$
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup HOGPBH
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"
#if (BLE_HID_BOOT_HOST)

#include "hogpbh.h"
#include "hogpbh_task.h"
#include "gap.h"

/*
 * GLOBAL VARIABLES DEFINITIONS
 ****************************************************************************************
 */

struct hogpbh_env_tag **hogpbh_envs __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

/// HOGPBH task descriptor
static const struct ke_task_desc TASK_DESC_HOGPBH = {hogpbh_state_handler, &hogpbh_default_handler, hogpbh_state, HOGPBH_STATE_MAX, HOGPBH_IDX_MAX};


/*
 * GLOBAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

void hogpbh_init(void)
{
    // Reset all the profile role tasks
    PRF_CLIENT_RESET(hogpbh_envs, HOGPBH);
}

void hogpbh_enable_cfm_send(struct hogpbh_env_tag *hogpbh_env, struct prf_con_info *con_info, uint8_t status)
{
    // Counter
    uint8_t i;

    // Send APP the details of the discovered attributes on HOGPBH
    struct hogpbh_enable_cfm * rsp = KE_MSG_ALLOC(HOGPBH_ENABLE_CFM,
                                                  con_info->appid, con_info->prf_id,
                                                  hogpbh_enable_cfm);

    rsp->conhdl = gapc_get_conhdl(con_info->conidx);
    rsp->status = status;

    if (status == PRF_ERR_OK)
    {
        rsp->hids_nb = hogpbh_env->hids_nb;

        for (i = 0; i < hogpbh_env->hids_nb; i++)
        {
            rsp->hids[i] = hogpbh_env->hids[i];

            // Register HOGPBH task in gatt for indication/notifications
            prf_register_atthdl2gatt(&hogpbh_env->con_info, &hogpbh_env->hids[i].svc);
        }

        // Go to connected state
        ke_state_set(con_info->prf_id, HOGPBH_CONNECTED);
    }
    else
    {
        PRF_CLIENT_ENABLE_ERROR(hogpbh_envs, con_info->prf_id, HOGPBH);
    }

    ke_msg_send(rsp);
}

void hogpbh_char_req_rsp_send(struct hogpbh_env_tag *hogpbh_env, ke_msg_id_t msg_id, uint8_t status)
{
    struct hogpbh_char_req_rsp *rsp = KE_MSG_ALLOC(msg_id,
                                                   hogpbh_env->con_info.appid, hogpbh_env->con_info.prf_id,
                                                   hogpbh_char_req_rsp);

    rsp->conhdl    = gapc_get_conhdl(hogpbh_env->con_info.conidx);
    rsp->status    = status;
    rsp->att_code  = hogpbh_env->last_char_code;

    // Send the message
    ke_msg_send(rsp);
}

#endif /* (BLE_HID_BOOT_HOST) */

/// @} HOGPBH
