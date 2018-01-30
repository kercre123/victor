/**
 ****************************************************************************************
 *
 * @file lanc.c
 *
 * @brief Location and Navigation Profile Collector implementation.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $ Rev $
 *
 ****************************************************************************************
 */


/**
 ****************************************************************************************
 * @addtogroup LANC
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwble_config.h"  

#if (BLE_LN_COLLECTOR)
#include "lan_common.h"
#include "lanc.h"
#include "lanc_task.h"

/*
 * GLOBAL VARIABLES DECLARATION
 ****************************************************************************************
 */

struct lanc_env_tag **lanc_envs __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

/// Location and Navigation Profile Collector Role Task
static const struct ke_task_desc TASK_DESC_LANC = {lanc_state_handler, &lanc_default_handler, lanc_state,
                                                    LANC_STATE_MAX, LANC_IDX_MAX};

/*
 * GLOBAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

void lanc_init(void)
{
    // Reset all the profile role tasks
    PRF_CLIENT_RESET(lanc_envs, LANC);
}

void lanc_cleanup(prf_env_struct *idx_env)
{
    struct lanc_env_tag *env = (struct lanc_env_tag *) idx_env;

    if(env->operation != NULL)
    {
        ke_msg_free(ke_param2msg(env->operation));
        env->operation = NULL;
    }
}


void lanc_send_no_conn_cmp_evt(uint8_t src_id, uint8_t dest_id, uint16_t conhdl, uint8_t operation)
{
    // Send the message
    struct lanc_cmp_evt *evt = KE_MSG_ALLOC(LANC_CMP_EVT,
                                             dest_id, src_id,
                                             lanc_cmp_evt);

    evt->conhdl     = conhdl;
    evt->operation  = operation;
    evt->status     = PRF_ERR_REQ_DISALLOWED;

    ke_msg_send(evt);
}

void lanc_send_cmp_evt(struct lanc_env_tag *lanc_env, uint8_t operation, uint8_t status)
{
    // Free the stored operation if needed
    if (lanc_env->operation != NULL)
    {
        ke_msg_free(ke_param2msg(lanc_env->operation));
        lanc_env->operation = NULL;
    }

    // Go back to the CONNECTED state if the state is busy
    if (ke_state_get(lanc_env->con_info.prf_id) == LANC_BUSY)
    {
        ke_state_set(lanc_env->con_info.prf_id, LANC_CONNECTED);
    }

    // Send the message
    struct lanc_cmp_evt *evt = KE_MSG_ALLOC(LANC_CMP_EVT,
                                             lanc_env->con_info.appid, lanc_env->con_info.prf_id,
                                             lanc_cmp_evt);

    evt->conhdl     = gapc_get_conhdl(lanc_env->con_info.conidx);
    evt->operation  = operation;
    evt->status     = status;

    ke_msg_send(evt);
}

#endif //(BLE_LN_COLLECTOR)

/// @} LAN
