/**
 ****************************************************************************************
 *
 * @file basc.c
 *
 * @brief Battery Service Client implementation.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev$
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup BASC
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_BAS_CLIENT)
#include "gap.h"
#include "basc.h"
#include "basc_task.h"
#include "prf_utils.h"

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

struct basc_env_tag **basc_envs __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

/// BASC task descriptor
static const struct ke_task_desc TASK_DESC_BASC = {basc_state_handler, &basc_default_handler, basc_state, BASC_STATE_MAX, BASC_IDX_MAX};


/*
 * GLOBAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

void basc_init(void)
{
    // Reset all the profile role tasks
    PRF_CLIENT_RESET(basc_envs, BASC);
}

void basc_enable_cfm_send(struct basc_env_tag *basc_env, struct prf_con_info *con_info, uint8_t status)
{
    // Counter
    uint8_t svc_inst;

    // Send APP the details of the discovered attributes on BASC
    struct basc_enable_cfm * rsp = KE_MSG_ALLOC(BASC_ENABLE_CFM,
                                                con_info->appid, con_info->prf_id,
                                                basc_enable_cfm);

    rsp->conhdl = gapc_get_conhdl(con_info->conidx);
    rsp->status = status;

    if (status == PRF_ERR_OK)
    {
        rsp->bas_nb = basc_env->bas_nb;

        for (svc_inst = 0; svc_inst < basc_env->bas_nb; svc_inst++)
        {
            rsp->bas[svc_inst] = basc_env->bas[svc_inst];

            // Register BASC task in gatt for indication/notifications
            prf_register_atthdl2gatt(&basc_env->con_info, &basc_env->bas[svc_inst].svc);
        }

        // Go to connected state
        ke_state_set(con_info->prf_id, BASC_CONNECTED);
    }
    else
    {
        PRF_CLIENT_ENABLE_ERROR(basc_envs, con_info->prf_id, BASC);
    }

    ke_msg_send(rsp);
}

void basc_error_ind_send(struct basc_env_tag *basc_env, uint8_t status)
{
    struct basc_error_ind *ind = KE_MSG_ALLOC(BASC_ERROR_IND,
                                              basc_env->con_info.appid, basc_env->con_info.prf_id,
                                              basc_error_ind);

    ind->conhdl    = gapc_get_conhdl(basc_env->con_info.conidx);
    //it will be an BASC status code
    ind->status    = status;

    // send the message
    ke_msg_send(ind);
}

#endif /* (BLE_BAS_CLIENT) */

/// @} BASC
