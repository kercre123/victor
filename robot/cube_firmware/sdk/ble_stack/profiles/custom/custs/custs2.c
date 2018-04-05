/**
 ****************************************************************************************
 *
 * @file custs2.c
 *
 * @brief Custom Service profile source file.
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

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
 
#include "rwble_config.h"              // SW configuration
#if (BLE_CUSTOM2_SERVER)
#include "custs2.h"
#include "custs2_task.h"
#include "attm_db.h"
#include "gapc.h"

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

struct custs2_env_tag custs2_env __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

/// Custom Service task descriptor
static const struct ke_task_desc TASK_DESC_CUSTS2 = {custs2_state_handler, &custs2_default_handler, custs2_state, CUSTS2_STATE_MAX, CUSTS2_IDX_MAX};

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void custs2_init(void)
{
    // Reset environment
    memset(&custs2_env, 0, sizeof(custs2_env));

    // Create CUSTS2 task
    ke_task_create(TASK_CUSTS2, &TASK_DESC_CUSTS2);

    // Set task in disabled state
    ke_state_set(TASK_CUSTS2, CUSTS2_DISABLED);
}

void custs2_disable(uint16_t conhdl) 
{
    // Inform the application about the disconnection
    struct custs2_disable_ind *ind = KE_MSG_ALLOC(CUSTS2_DISABLE_IND,
                                                   custs2_env.con_info.appid, custs2_env.con_info.prf_id,
                                                   custs2_disable_ind);

    ind->conhdl = conhdl;

    ke_msg_send(ind);

    //Disable CUSTS2 in database
    attmdb_svc_set_permission(custs2_env.shdl, PERM(SVC, DISABLE));

    //Go to idle state
    ke_state_set(TASK_CUSTS2, CUSTS2_IDLE);
}

#endif // (BLE_CUSTOM2_SERVER)
