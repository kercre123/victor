/**
 ****************************************************************************************
 *
 * @file custs1.c
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
#if (BLE_CUSTOM1_SERVER)
#include "custs1.h"
#include "custs1_task.h"
#include "attm_db.h"
#include "gapc.h"

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

struct custs1_env_tag custs1_env __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

/// Custom Service task descriptor
static const struct ke_task_desc TASK_DESC_CUSTS1 = {custs1_state_handler, &custs1_default_handler, custs1_state, CUSTS1_STATE_MAX, CUSTS1_IDX_MAX};

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void custs1_init(void)
{
    // Reset environment
    memset(&custs1_env, 0, sizeof(custs1_env));

    // Create CUSTS1 task
    ke_task_create(TASK_CUSTS1, &TASK_DESC_CUSTS1);

    // Set task in disabled state
    ke_state_set(TASK_CUSTS1, CUSTS1_DISABLED);
}

void custs1_disable(uint16_t conhdl)
{
    // Inform the application about the disconnection
    struct custs1_disable_ind *ind = KE_MSG_ALLOC(CUSTS1_DISABLE_IND,
                                                   custs1_env.con_info.appid, custs1_env.con_info.prf_id,
                                                   custs1_disable_ind);

    ind->conhdl = conhdl;

    ke_msg_send(ind);

    //Disable CUSTS1 in database
    attmdb_svc_set_permission(custs1_env.shdl, PERM(SVC, DISABLE));

    //Go to idle state
    ke_state_set(TASK_CUSTS1, CUSTS1_IDLE);
}

#endif // (BLE_CUSTOM1_SERVER)
