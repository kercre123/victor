/**
 ****************************************************************************************
 *
 * @file ctsc_task.h
 *
 * @brief Header file - CTSCTASK.
 *
 * Copyright (C) 2015 Dialog Semiconductor Ltd, unpublished work. This computer
 * program includes Confidential, Proprietary Information and is a Trade Secret of
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

#ifndef CTSC_TASK_H_
#define CTSC_TASK_H_

/**
************************************************************************************
* @addtogroup CTSC Task
* @ingroup CTSC
* @brief Current Time Service Client Task
* @{
************************************************************************************
*/

/*
* INCLUDE FILES
************************************************************************************
*/

#if (BLE_CTS_CLIENT)
#include "prf_types.h"
#include "cts_common.h"
/*
* DEFINES
****************************************************************************************
*/

/// Maximum number of Thermometer Collector task instances
#define CTSC_IDX_MAX        (BLE_CONNECTION_MAX)

/*
* TYPE DEFINITIONS
****************************************************************************************
*/

enum
{
    /// Idle state
    CTSC_IDLE,
    /// Discovering State
    CTSC_DISCOVERING,
    /// Connected state
    CTSC_CONNECTED,

    /// Number of defined states.
    CTSC_STATE_MAX
};

enum
{
    // Start the Current Time Service Client
    CTSC_ENABLE_REQ = KE_FIRST_MSG(TASK_CTSC),
    /// Confirm that cfg connection has finished with discovery results, or that normal cnx started
    CTSC_ENABLE_CFM,
    /// Disable Indication - Send when the connection with a peer peripheral has been turned off
    CTSC_DISABLE_IND,
    /// Generic error message
    CTSC_ERROR_IND,

    /// Generic message to read CTSC characteristics
    CTSC_RD_CHAR_REQ,
    /// Read Local Time Info response
    CTSC_LTI_RD_RSP,
    /// Read Reference Time Info response
    CTSC_RTI_RD_RSP,
    /// Read Current Time notification cfg response
    CTSC_CT_NTF_CFG_RD_RSP,

    /// Message for configuration
    CTSC_CFG_INDNTF_REQ,
    /// Generic message for write characteristic response status to APP
    CTSC_WR_CHAR_RSP,

    /// Current Time value send to APP
    CTSC_CT_IND,
};

enum
{
    CTSC_TYPE_RSP,
    CTSC_TYPE_NTF
};

/// Parameters of the @ref CTSC_ENABLE_REQ message
struct ctsc_enable_req
{
    /// Connection handle
    uint16_t conhdl;
    /// Connection type
    uint8_t con_type;
    /// CTSS handle values and characteristic properties
    struct ctsc_cts_content cts;
};

/// Parameters of the @ref CTSC_ENABLE_CFM message
struct ctsc_enable_cfm
{
    /// Connection handle
    uint16_t conhdl;
    /// Connection type
    uint8_t status;
    /// CTS handle values and characteristic properties
    struct ctsc_cts_content cts;
};

/// Parameters of the @ref CTSC_ERROR_IND message
struct ctsc_error_ind
{
    /// Connection handle
    uint16_t conhdl;
    /// Status
    uint8_t  status;
};

/// Parameters of the @ref CTSC_RD_CHAR_REQ message
struct ctsc_rd_char_req
{
    /// Connection handle
    uint16_t conhdl;
    /// Characteristic value code
    uint8_t  char_code;
};

/// Parameters of the @ref CTSC_CFG_INDNTF_REQ message
struct ctsc_cfg_indntf_req
{
    /// Connection handle
    uint16_t conhdl;
    /// Stop/notify/indicate value to configure into the peer characteristic
    uint16_t cfg_val;
};

/// Parameters of the @ref CTSC_WR_CHAR_RSP message
struct ctsc_wr_char_rsp
{
    /// Connection handle
    uint16_t conhdl;
    /// Status
    uint8_t  status;
};

/// Parameters for the @ref CTSC_CT_IND message
struct ctsc_ct_ind
{
    /// Connection Handle
    uint16_t conhdl;
    /// Indication Type
    uint8_t ind_type;
    /// Current Time Value
    struct cts_curr_time ct_val;
};

/// Parameters for the @ref CTSC_LTI_RD_RSP message
struct ctsc_lti_rd_rsp
{
    /// Connection Handle
    uint16_t conhdl;
    /// Indication Type
    struct cts_loc_time_info lti_val;
};

/// Parameters for the @ref CTSC_RTI_RD_RSP message
struct ctsc_rti_rd_rsp
{
    /// Connection Handle
    uint16_t conhdl;
    /// Indication Type
    struct cts_ref_time_info rti_val;
};

/// Parameters for the @ref CTSC_CT_NTF_CFG_RD_RSP message
struct ctsc_ct_ntf_cfg_rd_rsp
{
    /// Connection Handle
    uint16_t conhdl;
    /// Indication Type
    uint16_t ntf_cfg;
};

/*
 * TASK DESCRIPTOR DECLARATIONS
 ****************************************************************************************
 */
extern const struct ke_state_handler ctsc_state_handler[CTSC_STATE_MAX];
extern const struct ke_state_handler ctsc_default_handler;
extern ke_state_t ctsc_state[CTSC_IDX_MAX];

#endif //BLE_CTS_CLIENT
/// @} CTSCTASK
#endif // WSSC_TASK_H_
