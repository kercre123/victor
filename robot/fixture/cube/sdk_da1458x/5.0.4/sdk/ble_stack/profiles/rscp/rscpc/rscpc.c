/**
 ****************************************************************************************
 *
 * @file rscpc.c
 *
 * @brief Running Speed and Cadence Profile Collector implementation.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $ Rev $
 *
 ****************************************************************************************
 */


/**
 ****************************************************************************************
 * @addtogroup RSCPC
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwble_config.h" 

#if (BLE_RSC_COLLECTOR)
#include "rscp_common.h"
#include "rscpc.h"
#include "rscpc_task.h"

/*
 * GLOBAL VARIABLES DECLARATION
 ****************************************************************************************
 */

struct rscpc_env_tag **rscpc_envs __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

/// Running Speed and Cadence Profile Collector Role Task
static const struct ke_task_desc TASK_DESC_RSCPC = {rscpc_state_handler, &rscpc_default_handler, rscpc_state,
                                                    RSCPC_STATE_MAX, RSCPC_IDX_MAX};


/*
 * GLOBAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

void rscpc_init(void)
{
    // Reset all the profile role tasks
    PRF_CLIENT_RESET(rscpc_envs, RSCPC);
}

void rscpc_cleanup(prf_env_struct *idx_env)
{
    struct rscpc_env_tag *env = (struct rscpc_env_tag *) idx_env;

    if(env->operation != NULL)
    {
        ke_msg_free(ke_param2msg(env->operation));
        env->operation = NULL;
    }
}

void rscps_send_no_conn_cmp_evt(uint8_t src_id, uint8_t dest_id, uint16_t conhdl, uint8_t operation)
{
    // Send the message
    struct rscpc_cmp_evt *evt = KE_MSG_ALLOC(RSCPC_CMP_EVT,
                                             dest_id, src_id,
                                             rscpc_cmp_evt);

    evt->conhdl     = conhdl;
    evt->operation  = operation;
    evt->status     = PRF_ERR_REQ_DISALLOWED;

    ke_msg_send(evt);
}

void rscpc_send_cmp_evt(struct rscpc_env_tag *rscpc_env, uint8_t operation, uint8_t status)
{
    // Free the stored operation if needed
    if (rscpc_env->operation != NULL)
    {
        ke_msg_free(ke_param2msg(rscpc_env->operation));
        rscpc_env->operation = NULL;
    }

    // Go back to the CONNECTED state if the state is busy
    if (ke_state_get(rscpc_env->con_info.prf_id) == RSCPC_BUSY)
    {
        ke_state_set(rscpc_env->con_info.prf_id, RSCPC_CONNECTED);
    }

    // Send the message
    struct rscpc_cmp_evt *evt = KE_MSG_ALLOC(RSCPC_CMP_EVT,
                                             rscpc_env->con_info.appid, rscpc_env->con_info.prf_id,
                                             rscpc_cmp_evt);

    evt->conhdl     = gapc_get_conhdl(rscpc_env->con_info.conidx);
    evt->operation  = operation;
    evt->status     = status;

    ke_msg_send(evt);
}

#endif //(BLE_RSC_COLLECTOR)

/// @} RSCP
