/**
 ****************************************************************************************
 *
 * @file ancc.c
 *
 * @brief ANCS Client Role implementation.
 *
 * Copyright (C) 2014. Dialog Semiconductor Ltd, unpublished work. This computer 
 * program includes Confidential, Proprietary Information and is a Trade Secret of 
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited 
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */


/**
 ****************************************************************************************
 * @addtogroup ANCC
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "anc_common.h"

#if (BLE_ANC_CLIENT)

#include "ancc.h"
#include "ancc_task.h"

/*
 * GLOBAL VARIABLES DECLARATION
 ****************************************************************************************
 */

struct ancc_env_tag **ancc_envs __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

/// ANCS Client Role Task
static const struct ke_task_desc TASK_DESC_ANCC = {ancc_state_handler, &ancc_default_handler, ancc_state,
                                                   ANCC_STATE_MAX, ANCC_IDX_MAX};

/*
 * GLOBAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

void ancc_init(void)
{
    // Reset all the profile role tasks
    PRF_CLIENT_RESET(ancc_envs, ANCC);
}


void ancc_cleanup(prf_env_struct *idx_env)
{
    struct ancc_env_tag *env = (struct ancc_env_tag *) idx_env;

    if(env->operation != NULL)
    {
        ke_msg_free(ke_param2msg(env->operation));
        env->operation = NULL;
    }
}

void ancc_send_no_conn_cmp_evt(uint8_t src_id, uint8_t dest_id, uint16_t conhdl, uint8_t operation)
{
    // Send the message
    struct ancc_cmp_evt *evt = KE_MSG_ALLOC(ANCC_CMP_EVT,
                                            dest_id, src_id,
                                            ancc_cmp_evt);

    evt->conhdl     = conhdl;
    evt->operation  = operation;
    evt->status     = PRF_ERR_REQ_DISALLOWED;

    ke_msg_send(evt);
}

void ancc_send_cmp_evt(struct ancc_env_tag *ancc_env, uint8_t operation, uint8_t status)
{
    // Free the stored operation if needed
    if (ancc_env->operation != NULL)
    {
        ke_msg_free(ke_param2msg(ancc_env->operation));
        ancc_env->operation = NULL;
    }

    // Go back to the CONNECTED state if the state is busy
    if (ke_state_get(ancc_env->con_info.prf_id) == ANCC_BUSY)
    {
        ke_state_set(ancc_env->con_info.prf_id, ANCC_CONNECTED);
    }

    // Send the message
    struct ancc_cmp_evt *evt = KE_MSG_ALLOC(ANCC_CMP_EVT,
                                            ancc_env->con_info.appid, ancc_env->con_info.prf_id,
                                            ancc_cmp_evt);

    evt->conhdl     = gapc_get_conhdl(ancc_env->con_info.conidx);
    evt->operation  = operation;
    evt->status     = status;

    ke_msg_send(evt);
}

#endif //(BLE_ANC_CLIENT)

/// @} ANCC
