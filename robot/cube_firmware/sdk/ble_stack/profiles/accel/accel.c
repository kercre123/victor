/**
 ****************************************************************************************
 *
 * @file accel.c
 *
 * @brief Accelerometer attribute definitions
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup ACCEL
 * @{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */
 
#include "rwip_config.h"

#if (BLE_ACCEL)
#include "attm_db.h"
#include "accel.h"
#include "accel_task.h"


/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */
struct accel_env_tag accel_env __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

/// Accelerometer task descriptor
static const struct ke_task_desc TASK_DESC_ACCEL = {accel_state_handler, &accel_default_handler, accel_state, ACCEL_STATE_MAX, ACCEL_IDX_MAX};



/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */
void accel_init(void)
{
    // Reset the accelerometer environment
    memset(&accel_env, 0, sizeof(accel_env));

    // Create Accelerometer Task
    ke_task_create(TASK_ACCEL, &TASK_DESC_ACCEL);

    // Go to IDLE state
    ke_state_set(TASK_ACCEL, ACCEL_DISABLED);
}

#endif /* BLE_ACCEL */

/// @} ACCEL
