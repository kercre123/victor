/**
 ****************************************************************************************
 *
 * @file scppc_task.h
 *
 * @brief Header file - Scan Parameters Profile Client Role Task.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev$
 *
 ****************************************************************************************
 */


#ifndef _SCPPC_TASK_H_
#define _SCPPC_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup SCPPCTASK Scan Parameters Profile Client Task
 * @ingroup SCPPC
 * @brief Scan Parameters Profile Client Task
 *
 * The SCPPCTASK is responsible for handling the messages coming in and out of the
 * @ref SCPPC monitor block of the BLE Host.
 *
 * @{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_SP_CLIENT)

#include "ke_task.h"
#include "gattc_task.h"
#include "scppc.h"
#include "prf_types.h"
#include "prf_utils.h"

/*
 * DEFINES
 ****************************************************************************************
 */

///Maximum number of Scan Parameters Client task instances
#define SCPPC_IDX_MAX    (BLE_CONNECTION_MAX)

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Possible states of the SCPPC task
enum
{
    /// IDLE state
    SCPPC_IDLE,
    /// connected state
    SCPPC_CONNECTED,
    /// Discovering
    SCPPC_DISCOVERING,

    /// Number of defined states.
    SCPPC_STATE_MAX
};

enum
{
    /// Start the Scan Parameters profile Client Role - at connection
    SCPPC_ENABLE_REQ = KE_FIRST_MSG(TASK_SCPPC),
    ///Confirm that cfg connection has finished with discovery results, or that normal cnx started
    SCPPC_ENABLE_CFM,
    /// Disable Indication - Send when the connection with a peer peripheral has been turned off
    SCPPC_DISABLE_IND,
    ///Generic error message
    SCPPC_ERROR_IND,
    ///Inform APP about write command status - For Scan Refresh CCC
    SCPPC_WR_CHAR_RSP,

    /// Write Scan Interval Window Characteristic Value Request - Write Without Response
    SCPPC_SCAN_INTV_WD_WR_REQ,
    /// Read Scan Refresh Notification Configuration Value request
    SCPPC_SCAN_REFRESH_NTF_CFG_RD_REQ,
    /// Inform APP that Scan Refresh Notification Configuration has been read
    SCPPC_SCAN_REFRESH_NTF_CFG_RD_RSP,
    /// Write Scan Refresh Notification Configuration Value request
    SCPPC_SCAN_REFRESH_NTF_CFG_REQ,
};

/*
 * APIs Structure
 ****************************************************************************************
 */

/// Parameters of the @ref SCPPC_ENABLE_REQ message
struct scppc_enable_req
{
    /// Connection handle
    uint16_t conhdl;
    ///Connection type
    uint8_t con_type;
    // Last Scan Interval Window value to write after discovery
    struct scan_intv_wd scan_intv_wd;
    ///Existing handle values scps
    struct scps_content scps;
};

/// Parameters of the @ref SCPPC_ENABLE_CFM message
struct scppc_enable_cfm
{
    /// Connection handle
    uint16_t conhdl;
    ///status
    uint8_t status;
    ///Existing handle values scps
    struct scps_content scps;
};

///Parameters of the @ref SCPPC_ERROR_IND message
struct scppc_error_ind
{
    ///Connection handle
    uint16_t conhdl;
    ///Status
    uint8_t status;
};

///Parameters of the @ref SCPPC_WR_CHAR_RSP message
struct scppc_wr_char_rsp
{
    ///Connection handle
    uint16_t conhdl;
    ///Status
    uint8_t status;
};

///Parameters of the @ref SCPPC_SCAN_INTV_WD_WR_REQ message
struct scppc_scan_intv_wd_wr_req
{
    ///Connection handle
    uint16_t conhdl;
    ///Scan Interval Window
    struct scan_intv_wd scan_intv_wd;
};

///Parameters of the @ref SCPPC_SCAN_REFRESH_NTF_CFG_RD_REQ message
struct scppc_scan_refresh_ntf_cfg_rd_req
{
    ///Connection handle
    uint16_t conhdl;
};

///Parameters of the @ref SCPPC_SCAN_REFRESH_NTF_CFG_RD_RSP message
struct scppc_scan_refresh_ntf_cfg_rd_rsp
{
    ///Connection handle
    uint16_t conhdl;
    ///Notification COnfiguration Value
    uint16_t ntf_cfg;
};

///Parameters of the @ref SCPPC_SCAN_REFRESH_NTF_CFG_REQ message
struct scppc_scan_refresh_ntf_cfg_req
{
    ///Connection handle
    uint16_t conhdl;
    ///Notification Configuration
    uint16_t ntf_cfg;
};

/*
 * TASK DESCRIPTOR DECLARATIONS
 ****************************************************************************************
 */

extern const struct ke_state_handler scppc_state_handler[SCPPC_STATE_MAX];
extern const struct ke_state_handler scppc_default_handler;
extern ke_state_t scppc_state[SCPPC_IDX_MAX];

#endif // (BLE_SP_CLIENT)

/// @} SCPPCTASK

#endif /* _SCPPC_TASK_H_ */
