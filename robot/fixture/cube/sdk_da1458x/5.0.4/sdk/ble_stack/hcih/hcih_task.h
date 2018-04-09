/**
 ****************************************************************************************
 *
 * @file hcih_task.h
 *
 * @brief This file contains definitions related to the Host Controller interface
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

#ifndef HCIH_TASK_H_
#define HCIH_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup HCIH Host Controller Interface
 *@{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"     // SW configuration

#if (BLEHL_HCIH_ITF)

#include "ke_task.h"         // kernel task

/*
 * INSTANCES
 ****************************************************************************************
 */
/// Maximum number of instances of the HCIH task
#define HCIH_IDX_MAX 1

/*
 * STATES
 ****************************************************************************************
 */
/// Possible states of the HCIH task
enum HCIH_STATE
{
    /// IDLE state
    HCIH_IDLE,
    /// Number of states.
    HCIH_STATE_MAX
};

/*
 * MESSAGES
 ****************************************************************************************
 */
/// Message API of the HCIH task
enum HCIH_MSG
{
    HCIH_MSG_ID_FIRST = KE_FIRST_MSG(TASK_HCIH),

    HCIH_MSG_ID_LAST
};

/*
 * TASK DESCRIPTOR DECLARATIONS
 ****************************************************************************************
 */
extern const struct ke_state_handler hcih_default_handler;
extern ke_state_t hcih_state[HCIH_IDX_MAX];

#endif //BLEHL_HCIH_ITF

/// @} HCIH

#endif // HCIH_TASK_H_

