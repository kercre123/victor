/**
 ****************************************************************************************
 *
 * @file atts_task.h
 *
 * @brief Header file - ATTSTASK.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

#ifndef ATTS_TASK_H_
#define ATTS_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup ATTSTASK Task
 * @ingroup ATTS
 * @brief Handles ALL messages to/from ATTS block.
 *
 * The ATTSTASK is responsible for handling messages intended for
 * the Attribute Protocol Server.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip.h"
#include "attm.h"
#include "atts.h"
#if (BLE_ATTS)
/*
 * DEFINES
 ****************************************************************************************
 */
/// Maximum number of ATTS instance
#define ATTS_IDX_MAX                                 BLE_CONNECTION_MAX

/// ATTS task states
enum
{
    /// Free state (No Connection)
    ATTS_FREE,
    /// Connection started but ATTS not ready
    ATTS_CONNNECTED,
    /// Connected and ready to receive requests
    ATTS_READY,
    /// Connected but waiting for Indication confirmation
    ATTS_INDICATE,
    /// Waiting for confirmation of notification transmission
    ATTS_NOTIFY,
    /// total number of ATTS states.
    ATTS_STATE_MAX
};

/// ATTS messages
enum
{
    /// Command completed event
    ATTS_CMP_EVT  = KE_FIRST_MSG(TASK_ATTS),
    /// Sends a value notification/indication command
    ATTS_SEND_EVENT_CMD = KE_FIRST_MSG(TASK_ATTS),
    /// Response timeout indication
    ATTS_RTX_IND,

    /// Read request indication
    ATTS_READ_REQ_IND,
};

/* **********************************************************
 * Data Structures of Attribute server
 * **********************************************************/
/// Command completed event
struct atts_cmp_evt
{
    /// error status
    uint8_t status;
};

/// Sends a value notification/indication command
struct atts_send_event_cmd
{
    /// characteristic handle
    uint16_t handle;
    /// true to trigger a notification, False else
    bool     notify;
};

/// Read request indication.
struct atts_read_req_ind
{
    /// Handle of the attribute to be read.
    uint16_t handle;
};

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */
extern struct atts_env_tag* atts_env[ATTS_IDX_MAX];

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */
extern const struct ke_state_handler atts_state_handler[ATTS_STATE_MAX];
extern const struct ke_state_handler atts_default_handler;
extern ke_state_t atts_state[ATTS_IDX_MAX];
#endif // #if (BLE_ATTS)
/// @} ATTSTASK
#endif // ATTS_TASK_H_
