/**
 ****************************************************************************************
 *
 * @file cppc.c
 *
 * @brief Cycling Power Profile Collector implementation.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $ Rev $
 *
 ****************************************************************************************
 */


/**
 ****************************************************************************************
 * @addtogroup CPPC
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwble_config.h"  

#if (BLE_CP_COLLECTOR)
#include "cpp_common.h"
#include "cppc.h"
#include "cppc_task.h"

/*
 * GLOBAL VARIABLES DECLARATION
 ****************************************************************************************
 */

struct cppc_env_tag **cppc_envs __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

/// Cycling Power Profile Collector Role Task
static const struct ke_task_desc TASK_DESC_CPPC = {cppc_state_handler, &cppc_default_handler, cppc_state,
                                                    CPPC_STATE_MAX, CPPC_IDX_MAX};

/*
 * GLOBAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

void cppc_init(void)
{
    // Reset all the profile role tasks
    PRF_CLIENT_RESET(cppc_envs, CPPC);
}

void cppc_cleanup(prf_env_struct *idx_env)
{
    struct cppc_env_tag *env = (struct cppc_env_tag *) idx_env;

    if(env->operation != NULL)
    {
        ke_msg_free(ke_param2msg(env->operation));
        env->operation = NULL;
    }
}

void cppc_send_no_conn_cmp_evt(uint8_t src_id, uint8_t dest_id, uint16_t conhdl, uint8_t operation)
{
    // Send the message
    struct cppc_cmp_evt *evt = KE_MSG_ALLOC(CPPC_CMP_EVT,
                                             dest_id, src_id,
                                             cppc_cmp_evt);

    evt->conhdl     = conhdl;
    evt->operation  = operation;
    evt->status     = PRF_ERR_REQ_DISALLOWED;

    ke_msg_send(evt);
}

void cppc_send_cmp_evt(struct cppc_env_tag *cppc_env, uint8_t operation, uint8_t status)
{
    // Free the stored operation if needed
    if (cppc_env->operation != NULL)
    {
        ke_msg_free(ke_param2msg(cppc_env->operation));
        cppc_env->operation = NULL;
    }

    // Go back to the CONNECTED state if the state is busy
    if (ke_state_get(cppc_env->con_info.prf_id) == CPPC_BUSY)
    {
        ke_state_set(cppc_env->con_info.prf_id, CPPC_CONNECTED);
    }

    // Send the message
    struct cppc_cmp_evt *evt = KE_MSG_ALLOC(CPPC_CMP_EVT,
                                             cppc_env->con_info.appid, cppc_env->con_info.prf_id,
                                             cppc_cmp_evt);

    evt->conhdl     = gapc_get_conhdl(cppc_env->con_info.conidx);
    evt->operation  = operation;
    evt->status     = status;

    ke_msg_send(evt);
}

#endif //(BLE_CP_COLLECTOR)

/// @} CPP
