/**
 ****************************************************************************************
 *
 * @file l2cm_task.h
 *
 * @brief Header file - L2CMTASK.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

#ifndef L2CM_TASK_H_
#define L2CM_TASK_H_
/**
 ****************************************************************************************
 * @addtogroup L2CMTASK Task
 * @ingroup L2CM
 * @brief Handles ALL messages to/from L2CM block.
 *
 * The L2CM task is responsible for L2CAP signaling and resource allocation.
 * It handles the message signals between two L2CAP peers (CID 0x0005) and the
 * creation of logical link channel.
 *
 * L2CM task has only one state after successful initialization.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "llm_task.h"
#include "llc_task.h"
#include "l2cm.h"
#if (BLE_CENTRAL || BLE_PERIPHERAL)
/*
 * INSTANCES
 ****************************************************************************************
 */
/// Maximum number of instances of the L2CM task
#define L2CM_IDX_MAX                        0x01

/*
 * STATES
 ****************************************************************************************
 */
/// Possible states of the L2CM task
enum
{
    /// idle state
    L2CM_IDLE,
    /// Number of defined states.
    L2CM_STATE_MAX
};
/*
 * MESSAGES
 ****************************************************************************************
 */

/// Message API of the L2CM task
enum l2cm_msg_id
{
    /// Buffer size indication
    L2CM_BUFF_SIZE_IND = KE_FIRST_MSG(TASK_L2CM),
};



/// Buffer size indication
struct l2cm_buff_size_ind
{
    /// Data packet length
    uint16_t data_pk_len;
    /// total number of data packets
    uint16_t tot_nb_data_pkts;
};


/*
 * TASK DESCRIPTOR DECLARATIONS
 ****************************************************************************************
 */
extern const struct ke_state_handler l2cm_state_handler[L2CM_STATE_MAX];
extern const struct ke_state_handler l2cm_default_handler;
extern ke_state_t l2cm_state[L2CM_IDX_MAX];
#endif // #if (BLE_CENTRAL || BLE_PERIPHERAL)
/// @} L2CMTASK
#endif // L2CM_TASK_H_
