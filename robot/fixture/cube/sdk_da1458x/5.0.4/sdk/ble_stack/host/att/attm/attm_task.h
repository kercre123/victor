/**
 ****************************************************************************************
 *
 * @file attm_task.h
 *
 * @brief Header file - ATTMTASK.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

#ifndef ATTM_TASK_H_
#define ATTM_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup ATTMTASK Task
 * @ingroup ATTM
 * @brief Handles ALL messages to/from ATTM block.
 *
 * The ATTMTASK block will be responsible for generic attribute profile
 * requests, and will manage routing messages for @ref ATTS "ATTS"
 * and @ref ATTC "ATTC".
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDES
 ****************************************************************************************
 */
#include "attm_cfg.h"
#include "attm.h"
#if (BLE_CENTRAL || BLE_PERIPHERAL)
/*
 * DEFINES
 ****************************************************************************************
 */
/// Maximum number of ATTM task
#define ATTM_IDX_MAX                                 0x01

/// states of ATTM task
enum attm_state_id
{
    /// idle state
    ATTM_IDLE,
    /// Number of defined states.
    ATTM_STATE_MAX
};


/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */
extern const struct ke_state_handler attm_state_handler[ATTM_STATE_MAX];
extern const struct ke_state_handler attm_default_handler;
extern ke_state_t attm_state[ATTM_IDX_MAX];
#endif // #if (BLE_CENTRAL || BLE_PERIPHERAL)
/// @} ATTMTASK
#endif // ATTM_TASK_H_
