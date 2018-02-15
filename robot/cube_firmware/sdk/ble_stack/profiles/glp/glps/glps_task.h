/**
 ****************************************************************************************
 *
 * @file glps_task.h
 *
 * @brief Header file - Glucose Profile Sensor Task.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev$
 *
 ****************************************************************************************
 */


#ifndef _GLPS_TASK_H_
#define _GLPS_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup GLPSTASK Task
 * @ingroup GLPS
 * @brief Glucose Profile Task.
 *
 * The GLPSTASK is responsible for handling the messages coming in and out of the
 * @ref GLPS collector block of the BLE Host.
 *
 * @{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#if (BLE_GL_SENSOR)
#include <stdint.h>
#include "ke_task.h"
#include "glps.h"

/*
 * DEFINES
 ****************************************************************************************
 */

///Maximum number of Glucose task instances
#define GLPS_IDX_MAX     0x01

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Possible states of the GLPS task
enum glps_state
{
    /// Disabled state
    GLPS_DISABLED,
    /// Idle state
    GLPS_IDLE,
    /// Connected state
    GLPS_CONNECTED,

    /// Number of defined states.
    GLPS_STATE_MAX
};

/// Messages for Glucose Profile Sensor
enum glps_msg_id
{
    ///Add GLS into the database
    GLPS_CREATE_DB_REQ = KE_FIRST_MSG(TASK_GLPS),
    ///Inform APP about DB creation status
    GLPS_CREATE_DB_CFM,

    /// Start the Glucose Profile Sensor - at connection
    GLPS_ENABLE_REQ,
    /// Confirm that Glucose Profile Sensor is started.
    GLPS_ENABLE_CFM,

    /// Disable confirmation with configuration to save after profile disabled
    GLPS_DISABLE_IND,

    ///Inform APP of new configuration value
    GLPS_CFG_INDNTF_IND,

    /// Record Access Control Point Request
    GLPS_RACP_REQ_IND,
    /// Record Access Control Point Resp
    GLPS_RACP_RSP_REQ,

    /// Send Glucose measurement with context information
    GLPS_SEND_MEAS_WITH_CTX_REQ,
    /// Send Glucose measurement without context information
    GLPS_SEND_MEAS_WITHOUT_CTX_REQ,

    ///Inform that requested action has been performed
    GLPS_REQ_CMP_EVT,
    /// Error indication to Host
    GLPS_ERROR_IND,
};

///Parameters of the @ref GLPS_CREATE_DB_REQ message
struct glps_create_db_req
{
    ///Glucose Service start handle (0 = automatic handle allocation)
    uint16_t start_hdl;
    ///Measurement context supported
    uint8_t meas_ctx_supported;
};

///Parameters of the @ref GLPS_CREATE_DB_CFM message
struct glps_create_db_cfm
{
    ///Status
    uint8_t status;
};

/// Parameters of the @ref GLPS_ENABLE_REQ message
struct glps_enable_req
{
    ///Connection handle
    uint16_t conhdl;

    /// Glucose Feature
    uint16_t features;

    /// Glucose sensor security level (see SVC attribute permissions)
    uint8_t sec_lvl;
    ///Type of connection - will someday depend on button press length; can be CFG or DISCOVERY
    uint8_t con_type;

    /// Glucose indication/notification configuration
    uint16_t evt_cfg;
};

///Parameters of the @ref GLPS_ENABLE_CFM message
struct glps_enable_cfm
{
    ///Connection handle
    uint16_t conhdl;
    ///Status
    uint8_t status;
};

///Parameters of the @ref GLPS_DISABLE_IND message
struct glps_disable_ind
{
    ///Connection handle
    uint16_t conhdl;
    /// Glucose indication/notification configuration
    uint8_t evt_cfg;
};
//
///Parameters of the @ref GLPS_CFG_INDNTF_IND message
struct glps_cfg_indntf_ind
{
    ///Connection handle
    uint16_t conhdl;
    /// Glucose indication/notification configuration
    uint8_t evt_cfg;
};

///Parameters of the @ref GLPS_SEND_MEAS_WITH_CTX_REQ message
struct glps_send_meas_with_ctx_req
{
    ///Connection handle
    uint16_t conhdl;
    /// Sequence Number
    uint16_t seq_num;
    /// Glucose measurement
    struct glp_meas meas;
    /// Glucose measurement context
    struct glp_meas_ctx ctx;
};


///Parameters of the @ref GLPS_SEND_MEAS_WITHOUT_CTX_REQ message
struct glps_send_meas_without_ctx_req
{
    ///Connection handle
    uint16_t conhdl;
    /// Sequence Number
    uint16_t seq_num;
    /// Glucose measurement
    struct glp_meas meas;
};


///Parameters of the @ref GLPS_RACP_REQ_IND message
struct glps_racp_req_ind
{
    ///Connection handle
    uint16_t conhdl;
    ///RACP Request
    struct glp_racp_req racp_req;
};

///Parameters of the @ref GLPS_RACP_RSP_REQ message
struct glps_racp_rsp_req
{
    ///Connection handle
    uint16_t conhdl;

    ///Number of records.
    uint16_t num_of_record;

    /// operation code
    uint8_t op_code;

    ///Command status
    uint8_t status;
};

///Parameters of the @ref GLPS_REQ_CMP_EVT message
struct glps_req_cmp_evt
{
    ///Connection handle
    uint16_t conhdl;

    /// completed request
    uint8_t request;

    ///Command status
    uint8_t status;
};


/*
 * TASK DESCRIPTOR DECLARATIONS
 ****************************************************************************************
 */
extern const struct ke_state_handler glps_state_handler[GLPS_STATE_MAX];
extern const struct ke_state_handler glps_default_handler;
extern ke_state_t glps_state[GLPS_IDX_MAX];

#endif /* #if BLE_GL_SENSOR */

/// @} GLPSTASK

#endif /* _GLPS_TASK_H_ */
