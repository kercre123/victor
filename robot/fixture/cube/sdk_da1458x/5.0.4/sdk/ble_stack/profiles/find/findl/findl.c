/**
 ****************************************************************************************
 *
 * @file findl.c
 *
 * @brief Find Me Locator implementation.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup FINDL
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_FINDME_LOCATOR)
#include "findl.h"
#include "findl_task.h"
#include "prf_utils.h"
#include "gap.h"

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

struct findl_env_tag **findl_envs __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

/// FIND ME task descriptor
static const struct ke_task_desc TASK_DESC_FINDL = {findl_state_handler, &findl_default_handler, findl_state, FINDL_STATE_MAX, FINDL_IDX_MAX};


/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void findl_init(void)
{
    // Reset all the profile role tasks
    PRF_CLIENT_RESET(findl_envs, FINDL);
}

void findl_enable_cfm_send(struct findl_env_tag *findl_env, struct prf_con_info *con_info, uint8_t status)
{
    //send response to app
    struct findl_enable_cfm * cfm = KE_MSG_ALLOC(FINDL_ENABLE_CFM,
                                                 con_info->appid, con_info->prf_id,
                                                 findl_enable_cfm);

    cfm->conhdl = gapc_get_conhdl(con_info->conidx);
    cfm->status = status;

    if (status == PRF_ERR_OK)
    {
        cfm->ias = findl_env->ias;

        // Go to connected state
        ke_state_set(con_info->prf_id, FINDL_CONNECTED);
    }
    else
    {
        PRF_CLIENT_ENABLE_ERROR(findl_envs, con_info->prf_id, FINDL);
    }

    ke_msg_send(cfm);
}

void findl_error_ind_send(struct findl_env_tag *findl_env, uint8_t status)
{
    struct findl_error_ind *ind = KE_MSG_ALLOC(FINDL_ERROR_IND,
                                               findl_env->con_info.appid, findl_env->con_info.prf_id,
                                               findl_error_ind);

    ind->conhdl    = gapc_get_conhdl(findl_env->con_info.conidx);
    // It will be a FINDL status code
    ind->status    = status;

    // Send the message
    ke_msg_send(ind);
}

#endif //BLE_FINDME_LOCATOR

/// @} FINDL
