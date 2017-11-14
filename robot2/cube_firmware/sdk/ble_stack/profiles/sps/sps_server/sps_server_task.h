/**
 ****************************************************************************************
 *
 * @file sps_server_task.h
 *
 * @brief Serial Port Service Device profile task declaration
 *
 * Copyright (C) 2012. Dialog Semiconductor Ltd, unpublished work. This computer
 * program includes Confidential, Proprietary Information and is a Trade Secret of
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

#ifndef SPS_SERVER_TASK_H_
#define SPS_SERVER_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup SPS_SERVERTASK Serial Port Service Device Task
 * @ingroup SPS_SERVER
 * @brief Serial Port Service Device task implementation
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <stdint.h>
#include "ke_task.h"


/*
 * INSTANCES
 ****************************************************************************************
 */
/// Maximum number of instances of the SPS_SERVER task
#define SPS_SERVER_IDX_MAX 1

/*
 * STATES
 ****************************************************************************************
 */
/// Possible states of the SPS_SERVER task
enum
{
    /// DISABLED state
    SPS_SERVER_DISABLED,
    /// IDLE state
    SPS_SERVER_IDLE,
    /// ACTIVE state
    SPS_SERVER_ACTIVE,
    /// Number of states.
    SPS_SERVER_STATE_MAX
};

/*
 * MESSAGES
 ****************************************************************************************
 */
/// Message API of the SPS_SERVER task
enum
{
    /// Enables the Serial Port Service Device profile. The profile has to be enabled only
    /// once a connection has been established by the application
    SPS_SERVER_ENABLE_REQ = KE_FIRST_MSG(TASK_SPS_SERVER),
    
    //Enable condirmation message
    SPS_SERVER_ENABLE_CFM,
    
    /// Create SPS database request
    SPS_SERVER_CREATE_DB_REQ,
    
    /// Create SPS database response
    SPS_SERVER_CREATE_DB_CFM,
    
    // Request to initialize transmission
    SPS_SERVER_INIT_BLE_TX_REQ,
    
    // Send flow control status
    SPS_SERVER_SEND_FLOW_CONTROL_REQ,
    
    SPS_SERVER_REQ_FLOW_CONTROL_IND,
};

/// @ref SPS_SERVER_ENABLE_REQ parameters structure description.
struct sps_server_enable_req
{
    /// Application Task Id
    ke_task_id_t appid;
    
    /// Connection handle
    uint16_t conhdl;
};

struct sps_server_enable_cfm
{
    /// Connection handle
    uint16_t data_hdl;
};

///Parameters of the @ref SPS_SERVER_CREATE_DB_REQ message
struct sps_server_create_db_req
{
    ///SPS Service start handle (0 = automatic handle allocation)
    uint16_t start_hdl;
};

///Parameters of the @ref SPS_SERVER_CREATE_DB_CFM message
struct sps_server_create_db_cfm
{
    ///Status
    uint8_t status;
};
///Parameters of the @ref SPS_SERVER_CREATE_DB_CFM message

struct sps_server_init_ble_tx_req
{
    ///Status
    uint8_t status;
};

/// @ref SPS_SERVER_SEND_FLOW_CONTROL_REQ parameters structure description,
struct sps_server_send_notify_flow_control_state_req
{
    // flow control state
    uint8_t 	flow_control_state;
};

struct sps_server_request_flow_control_ind
{
    uint8_t status;
};

/*
 * TASK DESCRIPTOR DECLARATIONS
 ****************************************************************************************
 */
extern const struct ke_state_handler sps_server_state_handler[SPS_SERVER_STATE_MAX];
extern const struct ke_state_handler sps_server_default_handler;
extern ke_state_t sps_server_state[SPS_SERVER_IDX_MAX];
extern uint8_t tx_busy_flag;

/// @} SPS_SERVERTASK

#endif // SPS_SERVER_TASK_H_
