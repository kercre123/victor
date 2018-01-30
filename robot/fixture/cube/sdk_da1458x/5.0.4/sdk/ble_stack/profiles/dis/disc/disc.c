/**
 ****************************************************************************************
 *
 * @file disc.c
 *
 * @brief Device Information Service Client Implementation.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup DISC
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_DIS_CLIENT)
#include "disc.h"
#include "disc_task.h"
#include "gap.h"
#include "prf_utils.h"
/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

struct disc_env_tag **disc_envs __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

/// Device Information Service task descriptor
static const struct ke_task_desc TASK_DESC_DISC = {disc_state_handler, &disc_default_handler, disc_state, DISC_STATE_MAX, DISC_IDX_MAX};


/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void disc_init(void)
{
    // Reset all the profile role tasks
    PRF_CLIENT_RESET(disc_envs, DISC);
}

void disc_enable_cfm_send(struct disc_env_tag *disc_env, struct prf_con_info *con_info, uint8_t status)
{
    // Send APP the details of the discovered attributes on DISS
    struct disc_enable_cfm * rsp = KE_MSG_ALLOC(DISC_ENABLE_CFM,
                                                con_info->appid, con_info->prf_id,
                                                disc_enable_cfm);

    rsp->conhdl = gapc_get_conhdl(con_info->conidx);
    rsp->status = status;

    if (status == PRF_ERR_OK)
    {
        rsp->dis = disc_env->dis;

        // Go to connected state
        ke_state_set(con_info->prf_id, DISC_CONNECTED);
    }
    else
    {
        PRF_CLIENT_ENABLE_ERROR(disc_envs, con_info->prf_id, DISC);
    }

    ke_msg_send(rsp);
}

#endif //BLE_DIS_CLIENT

/// @} DISC
