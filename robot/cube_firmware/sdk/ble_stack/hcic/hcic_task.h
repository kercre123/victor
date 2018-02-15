/**
 ****************************************************************************************
 *
 * @file hci_task.h
 *
 * @brief This file contains definitions related to the Host Controller interface
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

#ifndef HCIC_TASK_H_
#define HCIC_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup HCI Host Controller Interface
 *@{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"     // SW configuration

#if (BLE_HCIC_ITF)

#include "ke_task.h"         // kernel task

/*
 * INSTANCES
 ****************************************************************************************
 */
/// Maximum number of instances of the HCI task
#define HCI_IDX_MAX 1

/*
 * STATES
 ****************************************************************************************
 */
/// Possible states of the HCI task
enum HCI_STATE
{
    /// IDLE state
    HCI_IDLE,
    /// Number of states.
    HCI_STATE_MAX
};

/*
 * MESSAGES
 ****************************************************************************************
 */
/// Message API of the HCI task
enum HCI_MSG
{
    HCI_MSG_ID_FIRST = KE_FIRST_MSG(TASK_HCI),

    #if (DEEP_SLEEP)
    /// to maintain device woken-up during some time
    HCI_POLLING_TO,
    #endif // DEEP_SLEEP

    HCI_MSG_ID_LAST
};

/*
 * TASK DESCRIPTOR DECLARATIONS
 ****************************************************************************************
 */
extern const struct ke_state_handler hci_default_handler;
extern ke_state_t hci_state[HCI_IDX_MAX];

#endif //BLE_HCIC_ITF

/// @} HCI

#endif // HCIC_TASK_H_

