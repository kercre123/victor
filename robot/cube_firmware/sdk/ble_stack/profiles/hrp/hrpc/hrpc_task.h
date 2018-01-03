/**
 ****************************************************************************************
 *
 * @file hrpc_task.h
 *
 * @brief Header file - Heart Rate Profile Collector Role Task.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev$
 *
 ****************************************************************************************
 */


#ifndef _HRPC_TASK_H_
#define _HRPC_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup HRPCTASK Heart Rate Profile Collector Task
 * @ingroup HRPC
 * @brief Heart Rate Profile Collector Task
 *
 * The HRPCTASK is responsible for handling the messages coming in and out of the
 * @ref HRPC monitor block of the BLE Host.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_HR_COLLECTOR)
#include "ke_task.h"
#include "gattc_task.h"
#include "co_error.h"
#include "hrpc.h"
#include "prf_types.h"
#include "prf_utils.h"

/*
 * DEFINES
 ****************************************************************************************
 */
///Maximum number of Heart Rate Collector task instances
#define HRPC_IDX_MAX    (BLE_CONNECTION_MAX)

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Possible states of the HRPC task
enum
{
    /// IDLE state
    HRPC_IDLE,
    /// connected state
    HRPC_CONNECTED,
    /// Discovering Heart Rate SVC and CHars
    HRPC_DISCOVERING,

    /// Number of defined states.
    HRPC_STATE_MAX
};

enum
{
    /// Start the Heart Rate profile - at connection
    HRPC_ENABLE_REQ = KE_FIRST_MSG(TASK_HRPC),
    ///Confirm that cfg connection has finished with discovery results, or that normal cnx started
    HRPC_ENABLE_CFM,
    /// Disable Indication - Send when the connection with a peer peripheral has been turned off
    HRPC_DISABLE_IND,
    ///Generic error message
    HRPC_ERROR_IND,

    /// Generic message to read a HRS or DIS characteristic value
    HRPC_RD_CHAR_REQ,
    ///Generic message for read responses for APP
    HRPC_RD_CHAR_RSP,
    ///Generic message for configuring the 2 characteristics that can be handled
    HRPC_CFG_INDNTF_REQ,
    ///APP request for control point write (to reset Energy Expanded)
    HRPC_WR_CNTL_POINT_REQ,
    ///Generic message for write characteristic response status to APP
    HRPC_WR_CHAR_RSP,

    /// Heart Rate value send to APP
    HRPC_HR_MEAS_IND,
};


/// Parameters of the @ref HRPC_ENABLE_REQ message
struct hrpc_enable_req
{
    /// Connection handle
    uint16_t conhdl;
    ///Connection type
    uint8_t con_type;
    ///Existing handle values hrs
    struct hrs_content hrs;
};

/// Parameters of the @ref HRPC_ENABLE_CFM message
struct hrpc_enable_cfm
{
    /// Connection handle
    uint16_t conhdl;
    ///status
    uint8_t status;
    ///Existing handle values hrs
    struct hrs_content hrs;
};

///Parameters of the @ref HRPC_ERROR_IND message
struct hrpc_error_ind
{
    ///Connection handle
    uint16_t conhdl;
    ///Status
    uint8_t  status;
};

///Parameters of the @ref HRPC_RD_CHAR_REQ message
struct hrpc_rd_char_req
{
    ///Connection handle
    uint16_t conhdl;
    ///Characteristic value code
    uint8_t  char_code;
};

///Parameters of the @ref HRPC_RD_CHAR_RSP message
struct hrpc_rd_char_rsp
{
    /// Attribute data information
    struct prf_att_info info;
};


///Parameters of the @ref HRPC_CFG_INDNTF_REQ message
struct hrpc_cfg_indntf_req
{
    ///Connection handle
    uint16_t conhdl;
    ///Stop/notify/indicate value to configure into the peer characteristic
    uint16_t cfg_val;
};

///Parameters of the @ref HTPC_WR_CNTL_POINT_REQ message
struct htpc_wr_cntl_point_req
{
    ///Connection handle
    uint16_t conhdl;
    ///value
    uint8_t val;
};

///Parameters of the @ref HRPC_WR_CHAR_RSP message
struct hrpc_wr_char_rsp
{
    ///Connection handle
    uint16_t conhdl;
    ///Status
    uint8_t  status;
};

///Parameters of the @ref HRPC_MEAS_IND message
struct hrpc_meas_ind
{
    ///Connection handle
    uint16_t conhdl;
    ///Heart Rate measurement
    struct hrs_hr_meas meas_val;
};

/*
 * TASK DESCRIPTOR DECLARATIONS
 ****************************************************************************************
 */

extern const struct ke_state_handler hrpc_state_handler[HRPC_STATE_MAX];
extern const struct ke_state_handler hrpc_default_handler;
extern ke_state_t hrpc_state[HRPC_IDX_MAX];

#endif //(BLE_HR_COLLECTOR)

/// @} HRPCTASK

#endif /* _HRPC_TASK_H_ */
