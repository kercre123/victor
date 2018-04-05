/**
 ****************************************************************************************
 *
 * @file wssc_task.h
 *
 * @brief Header file - WSSCTASK.
 *
 * Copyright (C) 20124 Dialog Semiconductor Ltd, unpublished work. This computer 
 * program includes Confidential, Proprietary Information and is a Trade Secret of 
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited 
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

#ifndef WSSC_TASK_H_
#define WSSC_TASK_H_

/**
************************************************************************************
* @addtogroup WSSCTASK Task
* @ingroup WSSS
* @brief Weight Scale Service Collector Task
* @{
************************************************************************************
*/

/*
* INCLUDE FILES
************************************************************************************
*/

#if (BLE_WSS_COLLECTOR)
#include "prf_types.h"
#include "wssc.h"
#include "wss_common.h"

/*
* DEFINES
****************************************************************************************
*/

/// Maximum number of Thermometer Collector task instances
#define WSSC_IDX_MAX        (BLE_CONNECTION_MAX)

/*
* TYPE DEFINITIONS
****************************************************************************************
*/

/// Possible states of the HTPC task
enum
{
    /// Idle state
    WSSC_IDLE,
    /// Discovering State
    WSSC_DISCOVERING,
    /// Connected state
    WSSC_CONNECTED,

    /// Number of defined states.
    WSSC_STATE_MAX
};

enum
{
    /// Start the Weight Scale Collector profile - at connection
    WSSC_ENABLE_REQ = KE_FIRST_MSG(TASK_WSSC),
    /// Confirm that cfg connection has finished with discovery results, or that normal cnx started
    WSSC_ENABLE_CFM,
    /// Disable Indication - Send when the connection with a peer peripheral has been turned off
    WSSC_DISABLE_IND,
    /// Generic error message
    WSSC_ERROR_IND,

    /// Generic message to read WSS characteristics
    WSSC_RD_CHAR_REQ,
    /// GEneric message for read response
    WSSC_RD_CHAR_RSP,
    /// Message for configuration
    WSSC_CFG_INDNTF_REQ,
    /// Generic message for write characteristic response status to APP
    WSSC_WR_CHAR_RSP,

    /// Weight Scale value send to APP
    WSSC_WS_IND,
};

/// Parameters of the @ref WSSC_ENABLE_REQ message
struct wssc_enable_req
{
    /// Connection handle
    uint16_t conhdl;
    /// Connection type
    uint8_t con_type;
    /// HTS handle values and characteristic properties
    struct wssc_wss_content wss;
};

/// Parameters of the @ref WSSC_ENABLE_CFM message
struct wssc_enable_cfm
{
    /// Connection handle
    uint16_t conhdl;
    /// Connection type
    uint8_t status;
    /// HTS handle values and characteristic properties
    struct wssc_wss_content wss;
};

/// Parameters of the @ref WSSC_ERROR_IND message
struct wssc_error_ind
{
    /// Connection handle
    uint16_t conhdl;
    /// Status
    uint8_t  status;
};

/// Parameters of the @ref WSSC_RD_CHAR_REQ message
struct wssc_rd_char_req
{
    /// Connection handle
    uint16_t conhdl;
    /// Characteristic value code
    uint8_t char_code;
};

/// Parameters of the @ref WSSC_RD_CHAR_RSP message
struct wssc_rd_char_rsp
{
    /// Attribute data information
    struct prf_att_info info;
};

/// Parameters of the @ref WSSC_CFG_INDNTF_REQ message
struct wssc_cfg_indntf_req
{
    /// Connection handle
    uint16_t conhdl;
    /// Stop/notify/indicate value to configure into the peer characteristic
    uint16_t cfg_val;
};

/// Parameters of the @ref WSSC_WS_IND message
struct wssc_ws_ind
{
    /// Connection handle
    uint16_t conhdl;
    /// Weight measurment value
    struct wss_wt_meas meas_val;
};

/// Parameters of the @ref WSSC_WR_CHAR_RSP message
struct wssc_wr_char_rsp
{
    /// Connection handle
    uint16_t conhdl;
    /// Status
    uint8_t  status;
};

/*
 * TASK DESCRIPTOR DECLARATIONS
 ****************************************************************************************
 */
extern const struct ke_state_handler wssc_state_handler[WSSC_STATE_MAX];
extern const struct ke_state_handler wssc_default_handler;
extern ke_state_t wssc_state[WSSC_IDX_MAX];

#endif //BLE_WSS_COLLECTOR
/// @} WSSSTASK
#endif // WSSC_TASK_H_
