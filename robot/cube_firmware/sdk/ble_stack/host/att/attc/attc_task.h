/**
 ****************************************************************************************
 *
 * @file attc_task.h
 *
 * @brief Header file - ATTCTASK.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */
#ifndef ATTC_TASK_H_
#define ATTC_TASK_H_
/**
 ****************************************************************************************
 * @addtogroup ATTCTASK Task
 * @ingroup ATTC
 * @brief Handles ALL messages to/from ATTC block.
 *
 * The ATTCTASK is responsible for handling messages intended for
 * the Attribute Profile Client.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"
#if (BLE_ATTC)
#include "attm.h"
#include "attc.h"
#include "l2cc_task.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// Possible states of the ATT task
enum
{
    /// Connected state, not ready to perform actions
//    ATTC_CONNECTED,
    /// Connected state, ready to perform actions
    ATTC_READY,
    /// Busy state
    ATTC_BUSY,
    /// Default state (disconnected) free all received messages
    ATTC_FREE,
    /// total number of ATTC states
    ATTC_STATE_MAX
};
enum
{
    /// Command Completed event
    ATTC_CMP_EVT = KE_FIRST_MSG(TASK_ATTC),
    /// Send an attribute client request to peer service command
    ATTC_SEND_CMD,
    /// Indicate that a message has been received by Attribute client
    ATTC_RECEIVE_IND,
    /// Response timeout indication
    ATTC_RTX_IND,
};

/* **********************************************
 * Data Structures of Attribute client
 * *********************************************/

/// Command Completed event (message send command status)
struct attc_cmp_evt
{
    /// Status of command.
    uint8_t status;
};


/// Send an attribute client request to peer service command
struct attc_send_cmd
{
    /// Offset value used by L2CAP during fragmentation
    uint16_t offset;
    /// PDU data
    struct l2cc_pdu pdu;
};

/// Indicate that a message has been received by Attribute client
struct attc_receive_ind
{
    /// Reception status code.
    uint8_t status;
    /// Remaining length of data to receive
    uint16_t rem_len;
    /// Offset
    uint16_t offset;
    /// PDU data
    struct l2cc_pdu pdu;
};

/*
 * TASK DESCRIPTOR DECLARATIONS
 ****************************************************************************************
 */
extern const struct ke_state_handler attc_state_handler[ATTC_STATE_MAX];
extern const struct ke_state_handler attc_default_handler;
extern ke_state_t attc_state[ATTC_IDX_MAX];
#endif // #if (BLE_ATTC)
/// @} ATTCTASK
#endif // ATT_TASK_H_
