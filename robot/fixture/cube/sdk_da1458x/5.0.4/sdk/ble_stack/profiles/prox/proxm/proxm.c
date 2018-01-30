/**
 ****************************************************************************************
 *
 * @file proxm.c
 *
 * @brief Proximity monitor implementation.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup PROXM
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_PROX_MONITOR)

#include "gattc_task.h"
#include "proxm.h"
#include "proxm_task.h"
#include "prf_utils.h"
#include "gap.h"
#include "gapc.h"

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

struct proxm_env_tag **proxm_envs __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

/// PROXM task descriptor
static const struct ke_task_desc TASK_DESC_PROXM = {proxm_state_handler, &proxm_default_handler, proxm_state, PROXM_STATE_MAX, PROXM_IDX_MAX};


/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void proxm_init(void)
{
    // Reset all the profile role tasks
    PRF_CLIENT_RESET(proxm_envs, PROXM);
}

void proxm_enable_cfm_send(struct proxm_env_tag *proxm_env, struct prf_con_info *con_info, uint8_t status)
{
    //format response to app
    struct proxm_enable_cfm * cfm = KE_MSG_ALLOC(PROXM_ENABLE_CFM,
                                                 con_info->appid, con_info->prf_id,
                                                 proxm_enable_cfm);

    cfm->conhdl = gapc_get_conhdl(con_info->conidx);
    cfm->status = status;

    if (status == PRF_ERR_OK)
    {
        cfm->ias    = proxm_env->ias;
        cfm->lls    = proxm_env->lls;
        cfm->txps   = proxm_env->txps;

        // Go to connected state
        ke_state_set(con_info->prf_id, PROXM_CONNECTED);
    }
    else
    {
        PRF_CLIENT_ENABLE_ERROR(proxm_envs, con_info->prf_id, PROXM);
    }

    ke_msg_send(cfm);
}


void proxm_write_char_rsp_send(struct proxm_env_tag *proxm_env, uint8_t status)
{
    // Send response to app
    struct proxm_wr_char_rsp *wr_cfm = KE_MSG_ALLOC(PROXM_WR_CHAR_RSP,
                                                    proxm_env->con_info.appid, proxm_env->con_info.prf_id,
                                                    proxm_wr_char_rsp);

    wr_cfm->conhdl    = gapc_get_conhdl(proxm_env->con_info.conidx);
    wr_cfm->status    = status;

    ke_msg_send(wr_cfm);
}

#endif //BLE_PROX_MONITOR

/// @} PROXM
