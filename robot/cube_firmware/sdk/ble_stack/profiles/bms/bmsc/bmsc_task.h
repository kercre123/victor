/**
 ****************************************************************************************
 *
 * @file bmsc_task.h
 *
 * @brief Header file - Bond Management Service Client Role Task Declaration.
 *
 * Copyright (C) 2015. Dialog Semiconductor Ltd, unpublished work. This computer
 * program includes Confidential, Proprietary Information and is a Trade Secret of
 * Dialog Semiconductor Ltd. All use, disclosure, and/or reproduction is prohibited
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

#ifndef _BMSC_TASK_H_
#define _BMSC_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup BMSCTASK Bond Management Service Client Task
 * @ingroup BMSC
 * @brief Bond Management Service Client Task
 *
 * The BMSCTASK is responsible for handling the messages coming in and out of the
 * @ref BMSC monitor block of the BLE Host.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_BM_CLIENT)
#include "ke_task.h"
#include "gattc_task.h"
#include "co_error.h"
#include "bmsc.h"
#include "prf_types.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// Maximum number of Bond Management Service Client task instances
#define BMSC_IDX_MAX    (BLE_CONNECTION_MAX)

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Possible states of the BMSC task
enum bmsc_state
{
    /// IDLE state
    BMSC_IDLE,
    /// Connected state
    BMSC_CONNECTED,
    /// Discovering Bond Management SVC and CHars
    BMSC_DISCOVERING,

    /// Number of defined states
    BMSC_STATE_MAX
};

enum bmsc_msg_id
{
    /// Start the Bond Management Service Client - at connection
    BMSC_ENABLE_REQ = KE_FIRST_MSG(TASK_BMSC),
    /// Confirm that cfg connection has finished with discovery results, or that normal cnx started
    BMSC_ENABLE_CFM,

    /// Disable profile role - Indication
    BMSC_DISABLE_IND,
     /// Generic error message
    BMSC_ERROR_IND,

    /// APP request for control point write
    BMSC_WR_CTRL_POINT_REQ,
    /// Write response to APP
    BMSC_WR_CTRL_POINT_RSP,

    /// APP request for features read
    BMSC_RD_FEATURES_REQ,
    /// Read response to APP
    BMSC_RD_FEATURES_RSP,
};

/// Parameters of the @ref BMSC_ENABLE_REQ message
struct bmsc_enable_req
{
    /// Connection handle
    uint16_t conhdl;

    /// Connection type
    uint8_t con_type;

    /// Existing handle values bms
    struct bms_content bms;
};

/// Parameters of the @ref BMSC_ENABLE_CFM message
struct bmsc_enable_cfm
{
    /// Connection handle
    uint16_t conhdl;

    /// status
    uint8_t status;

    /// Existing handle values bms
    struct bms_content bms;
};

///Parameters of the @ref BMSC_ERROR_IND message
struct bmsc_error_ind
{
    /// Connection handle
    uint16_t conhdl;

    /// Status
    uint8_t  status;
};

/// Parameters of the @ref BMSC_RD_FEATURES_REQ message
struct bmsc_rd_features_req
{
    /// Connection handle
    uint16_t conhdl;
};

/// Parameters of the @ref BMSC_RD_FEATURES_RSP message - dynamically allocated
struct bmsc_rd_features_rsp
{
    /// Connection handle
    uint16_t conhdl;

    /// Status
    uint8_t status;

    /// Bond Management server features length
    uint8_t length;

    /// Bond Management server features
    uint8_t features[__ARRAY_EMPTY];
};

/// Parameters of the @ref BMSC_WR_CTRL_POINT_REQ message - dynamically allocated
struct bmsc_wr_ctrl_point_req
{
    /// Connection handle
    uint16_t conhdl;

    /// Operation code and operand
    struct bms_ctrl_point_op operation;
};

/// Parameters of the @ref BMSC_WR_CTRL_POINT_RSP message
struct bmsc_wr_ctrl_point_rsp
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

extern const struct ke_state_handler bmsc_state_handler[BMSC_STATE_MAX];
extern const struct ke_state_handler bmsc_default_handler;
extern ke_state_t bmsc_state[BMSC_IDX_MAX];

#endif // (BLE_BM_CLIENT)

/// @} BMSCTASK

#endif /* _BMSC_TASK_H_ */
