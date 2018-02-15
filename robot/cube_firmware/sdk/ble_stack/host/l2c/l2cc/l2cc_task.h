/**
 ****************************************************************************************
 *
 * @file l2cc_task.h
 *
 * @brief Header file - L2CCTASK.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

#ifndef L2CC_TASK_H_
#define L2CC_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup L2CCTASK Task
 * @ingroup L2CC
 * @brief Handles ALL messages to/from L2CC block.
 *
 * The L2CC task is responsible for L2CAP attribute and security block handling.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_L2CC)

#include "l2cc_pdu.h"
#include "ke_task.h"

/*
 * MACRO DEFINITIONS
 ****************************************************************************************
 */

#define L2CC_SEND_PDU_TO_PACKET(pdu) \
    ((struct l2cc_pdu_send_req *)(((uint8_t *)pdu) - (3*sizeof(uint16_t))))

#define L2CC_RECV_PDU_TO_PACKET(pdu) \
    ((struct l2cc_pdu_recv_ind *)(((uint8_t *)pdu) - (5*sizeof(uint16_t))))

/*
 * STATES
 ****************************************************************************************
 */

/// L2CC states
enum
{
    /// Free state
    L2CC_FREE,
    /// At least one buffer is available to send a PDU
    L2CC_READY,
    /// No more buffers available
    L2CC_BUSY,

    /// Total number of defined L2CC states */
    L2CC_STATE_MAX
};

/*
 * MESSAGES
 ****************************************************************************************
 */

/// Message API of the L2CC task
enum l2cc_msg_id
{
    /// Send a PDU packet
    L2CC_PDU_SEND_REQ = KE_FIRST_MSG(TASK_L2CC),
    /// Reception of a PDU packet
    L2CC_PDU_RECV_IND,
    /// Inform that packet been sent.
    L2CC_DATA_SEND_RSP,
};

/// pass from l2cc to upper layer
struct l2cc_data_send_rsp
{
    /// Status of request.
    uint8_t status;
};

/// Send a PDU packet
struct l2cc_pdu_send_req
{
    /// Offset value used by L2CAP during fragmentation
    uint16_t offset;
    /// PDU data
    struct l2cc_pdu pdu;
};

/// Send a PDU packet
struct l2cc_pdu_recv_ind
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

extern const struct ke_state_handler l2cc_state_handler[L2CC_STATE_MAX];
extern const struct ke_state_handler l2cc_default_handler;
extern ke_state_t l2cc_state[L2CC_IDX_MAX];

#endif //(BLE_L2CC)

/// @} L2CCTASK

#endif // L2CC_TASK_H_
