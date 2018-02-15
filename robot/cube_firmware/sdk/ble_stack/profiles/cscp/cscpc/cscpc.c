/**
 ****************************************************************************************
 *
 * @file cscpc.c
 *
 * @brief Cycling Speed and Cadence Profile Collector implementation.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $ Rev $
 *
 ****************************************************************************************
 */


/**
 ****************************************************************************************
 * @addtogroup CSCPC
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
 
#include "rwble_config.h" 

#if (BLE_CSC_COLLECTOR)
#include "cscp_common.h"
#include "cscpc.h"
#include "cscpc_task.h"

/*
 * GLOBAL VARIABLES DECLARATION
 ****************************************************************************************
 */

struct cscpc_env_tag **cscpc_envs __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

/// Cycling Speed and Cadence Profile Collector Role Task
static const struct ke_task_desc TASK_DESC_CSCPC = {cscpc_state_handler, &cscpc_default_handler, cscpc_state,
                                                    CSCPC_STATE_MAX, CSCPC_IDX_MAX};

/*
 * GLOBAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

void cscpc_init(void)
{
    // Reset all the profile role tasks
    PRF_CLIENT_RESET(cscpc_envs, CSCPC);
}

void cscpc_cleanup(prf_env_struct *idx_env)
{
    struct cscpc_env_tag *env = (struct cscpc_env_tag *) idx_env;

    if(env->operation != NULL)
    {
        ke_msg_free(ke_param2msg(env->operation));
        env->operation = NULL;
    }
}

void cscpc_send_no_conn_cmp_evt(uint8_t src_id, uint8_t dest_id, uint16_t conhdl, uint8_t operation)
{
    // Send the message
    struct cscpc_cmp_evt *evt = KE_MSG_ALLOC(CSCPC_CMP_EVT,
                                             dest_id, src_id,
                                             cscpc_cmp_evt);

    evt->conhdl     = conhdl;
    evt->operation  = operation;
    evt->status     = PRF_ERR_REQ_DISALLOWED;

    ke_msg_send(evt);
}

void cscpc_send_cmp_evt(struct cscpc_env_tag *cscpc_env, uint8_t operation, uint8_t status)
{
    // Free the stored operation if needed
    if (cscpc_env->operation != NULL)
    {
        ke_msg_free(ke_param2msg(cscpc_env->operation));
        cscpc_env->operation = NULL;
    }

    // Go back to the CONNECTED state if the state is busy
    if (ke_state_get(cscpc_env->con_info.prf_id) == CSCPC_BUSY)
    {
        ke_state_set(cscpc_env->con_info.prf_id, CSCPC_CONNECTED);
    }

    // Send the message
    struct cscpc_cmp_evt *evt = KE_MSG_ALLOC(CSCPC_CMP_EVT,
                                             cscpc_env->con_info.appid, cscpc_env->con_info.prf_id,
                                             cscpc_cmp_evt);

    evt->conhdl     = gapc_get_conhdl(cscpc_env->con_info.conidx);
    evt->operation  = operation;
    evt->status     = status;

    ke_msg_send(evt);
}

#endif //(BLE_CSC_COLLECTOR)

/// @} CSCP
