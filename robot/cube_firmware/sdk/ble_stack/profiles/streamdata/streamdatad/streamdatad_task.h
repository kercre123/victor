/**
 ****************************************************************************************
 *
 * @file streamdatad_task.h
 *
 * @brief StreamData Device profile task declaration
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

#ifndef STREAMDATAD_TASK_H_
#define STREAMDATAD_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup STREAMDATADTASK StreamData Device Task
 * @ingroup STREAMDATAD
 * @brief StreamData Device task implementation
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
#include "streamdatad.h"
#if (BLE_STREAMDATA_DEVICE)

/*
 * INSTANCES
 ****************************************************************************************
 */
/// Maximum number of instances of the STREAMDATAD task
#define STREAMDATAD_IDX_MAX 1


/*
 * STATES
 ****************************************************************************************
 */
/// Possible states of the STREAMDATAD task
enum
{
    /// DISABLED state
    STREAMDATAD_DISABLED,
    /// IDLE state
    STREAMDATAD_IDLE,
    /// ACTIVE state
    STREAMDATAD_ACTIVE,
    /// Number of states.
    STREAMDATAD_STATE_MAX
};


/*
 * MESSAGES
 ****************************************************************************************
 */
/// Message API of the STREAMDATAD task
enum
{
    /// Enables the StreamData Device profile. The profile has to be enabled only
    /// once a connection has been established by the application
    STREAMDATAD_ENABLE_REQ = KE_FIRST_MSG(TASK_STREAMDATAD),
    /// Enables the StreamData Device profile. The profile has to be enabled only
    /// once a connection has been established by the application
    STREAMDATAD_DISABLE_REQ,
    /// Send data
    STREAMDATAD_SEND_DATA_PACKETS_REQ,
    /// Indicates to the application that the StreamData Device has to be started
    STREAMDATAD_START_IND,
    /// Indicates to the application that the StreamData Device has to be stopped
    STREAMDATAD_STOP_IND,
	
	/// Indicates to the application that there may be slots available to send data
	//STREAMDATAD_READY_TO_SEND_IND,

    /// Create Stream database request
    STREAMDATAD_CREATE_DB_REQ,
    /// Create Stream database response
    STREAMDATAD_CREATE_DB_CFM,
    /// Send Received Data to app
    STREAMDATAD_RCV_DATA_PACKET_IND,
};

/// @ref STREAMDATAD_ENABLE_REQ parameters structure description.
struct streamdatad_enable_req
{
    /// Application Task Id
    ke_task_id_t appid;
    /// Connection handle
    uint16_t conhdl;
};

/// @ref STREAMDATAD_SEND_DATA_PACKETS_REQ parameters structure description.
struct streamdatad_send_data_packets_req
{
	uint8_t nr_packets;
    uint16_t packets[STREAMDATAD_MAX][STREAMDATAD_PACKET_SIZE/2];
};

/// @ref STREAMDATAD_START_IND parameters structure description.
struct streamdatad_start_ind
{
	uint8_t status;
    //uint16_t streamdatad_en[STREAMDATAD_MAX];
};

///Parameters of the @ref STREAMDATAD_CREATE_DB_REQ message
struct streamdatad_create_db_req
{
    ///Stream Service start handle (0 = automatic handle allocation)
    uint16_t start_hdl;
};

///Parameters of the @ref STREAMDATAD_CREATE_DB_CFM message
struct streamdatad_create_db_cfm
{
    ///Status
    uint8_t status;
};

/// @ref STREAMDATAD_START_IND parameters structure description.
struct streamdatad_ready_to_send_ind
{
	uint8_t status;
};


struct streamdatad_rcv_data_packet_ind
{
    /// Connection handle
    uint16_t conhdl;
    /// Handle of the attribute that has to be written
    uint16_t handle;
	/// sequence number to check for dropped packets
	uint8_t seq;
    /// size of data
    uint8_t size;
    /// value
    uint8_t value[20];
};

/*
 * TASK DESCRIPTOR DECLARATIONS
 ****************************************************************************************
 */
extern const struct ke_state_handler streamdatad_state_handler[STREAMDATAD_STATE_MAX];
extern const struct ke_state_handler streamdatad_default_handler;
extern ke_state_t streamdatad_state[STREAMDATAD_IDX_MAX];


#endif /* BLE_STREAMDATA_DEVICE*/

/// @} STREAMDATADTASK

#endif // STREAMDATAD_TASK_H_
