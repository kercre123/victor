/**
 ****************************************************************************************
 *
 * @file sps_client_task.h
 *
 * @brief Header file - SPS_CLIENTTASK.
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

#ifndef SPS_CLIENT_TASK_H_
#define SPS_CLIENT_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup SPS_CLIENTTASK StreamData client Task
 * @ingroup SPS_CLIENT
 * @brief StreamData Host Task
 *
 * The StreamData client Task is responsible for handling the API messages received from
 * either the Application or internal tasks.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwble_config.h"
#include "ke_task.h"
#include "co_error.h"
#include "sps_client.h"

#if (BLE_SPS_CLIENT)
/*
 * DEFINES
 ****************************************************************************************
 */

///Maximum number of StreamData client task instances
#define SPS_CLIENT_IDX_MAX    	(BLE_CONNECTION_MAX)
/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Possible states of the SPS_CLIENT task
enum
{
    /// IDLE state
    SPS_CLIENT_IDLE,
    
		///Connected state
    SPS_CLIENT_CONNECTED,
    
		///Discovering
    SPS_CLIENT_DISCOVERING,
    
		/// Number of defined states.
    SPS_CLIENT_STATE_MAX
};

///StreamData Host API messages
enum
{
    ///StreamData client role enable request from application.
    SPS_CLIENT_ENABLE_REQ = KE_FIRST_MSG(TASK_SPS_CLIENT),
    
    /// StreamData Host role enable confirmation to application.
    SPS_CLIENT_ENABLE_CFM,

    /// Inform APP that the profile client role has been disabled after a disconnection
    SPS_CLIENT_DISABLE_IND,
    
    // Request to initialize transmission
    SPS_CLIENT_INIT_BLE_TX_REQ,
    
    // Send flow control over ble
    SPS_CLIENT_SEND_FLOW_CONTROL_REQ,

};

/*
 * API Messages Structures
 ****************************************************************************************
 */

/// StreamData client enable command structure
struct sps_client_enable_req
{
    /// Connection handle
    uint16_t conhdl;
    
    /// Connection type
    uint8_t con_type;
    
    /// StreamData Device details kept in APP
    struct sps_client_sps_content sps_server;
};

/// StreamData client enable confirm structure
struct sps_client_enable_cfm
{
    /// Connection handle
    uint16_t conhdl;
   
    /// Status
    uint8_t status;
    
    /// StreamData Device details to keep in APP
    struct sps_client_sps_content sps_client;
};

/// @ref SPS_SERVER_SEND_FLOW_CONTROL_REQ parameters structure description,
struct sps_client_send_notify_flow_control_state_req
{
    // flow control state
    uint8_t 	flow_control_state;
};

struct sps_client_init_ble_tx_req
{
    /// Status
    uint8_t status;
};

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */
extern const struct ke_state_handler sps_client_state_handler[SPS_CLIENT_STATE_MAX];
extern const struct ke_state_handler sps_client_default_handler;
extern ke_state_t sps_client_state[SPS_CLIENT_IDX_MAX];
extern uint8_t tx_busy_flag;

/*
 * Functions
 ****************************************************************************************
 */



#endif //BLE_SPS_CLIENT

/// @} SPS_CLIENTTASK

#endif // SPS_CLIENT_TASK_H_
