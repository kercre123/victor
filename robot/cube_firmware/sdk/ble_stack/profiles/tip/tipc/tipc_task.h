/**
 ****************************************************************************************
 *
 * @file tipc_task.h
 *
 * @brief Header file - Time Profile Client Role Task.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev$
 *
 ****************************************************************************************
 */

#ifndef _TIPC_TASK_H_
#define _TIPC_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup TIPCTASK Time Profile Client Task
 * @ingroup TIPC
 * @brief Time Profile Client Task
 *
 * The TIPCTASK is responsible for handling the messages coming in and out of the
 * @ref TIPC monitor block of the BLE Host.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_TIP_CLIENT)
#include "ke_task.h"
#include "gattc_task.h"
#include "co_error.h"
#include "tipc.h"
#include "prf_types.h"
#include "tip_common.h"
#include "prf_utils.h"

/*
 * DEFINES
 ****************************************************************************************
 */

///Maximum number of Time Client task instances
#define TIPC_IDX_MAX    (BLE_CONNECTION_MAX)

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Possible states of the TIPC task
enum
{
    /// IDLE state
    TIPC_IDLE,
    /// connected state
    TIPC_CONNECTED,
    /// Discovering Time SVC and Chars
    TIPC_DISCOVERING,

    /// Number of defined states.
    TIPC_STATE_MAX
};

enum
{
    /// Start the time profile - at connection
    TIPC_ENABLE_REQ = KE_FIRST_MSG(TASK_TIPC),
    ///Confirm that cfg connection has finished with discovery results, or that normal cnx started
    TIPC_ENABLE_CFM,
    /// Disable Indication - Send when the connection with a peer peripheral has been turned off
    TIPC_DISABLE_IND,
    ///Generic error message
    TIPC_ERROR_IND,

    /// Generic message to read a CTS or NDCS or RTUS characteristic value
    TIPC_RD_CHAR_REQ,

    ///Generic message for configuring the Current Time Characteristic on the Server
    TIPC_CT_NTF_CFG_REQ,
    /// Generic message for writing the Time Update Control Point Characteristic Value on a peer device
    TIPC_WR_TIME_UPD_CTNL_PT_REQ,
    ///Generic message for writing characteristic response status to APP
    TIPC_WR_CHAR_RSP,

    /// Received Current Time value sent to APP (Read Response or Notification)
    TIPC_CT_IND,
    /// Read Current Time Notification Configuration sent to APP
    TIPC_CT_NTF_CFG_RD_RSP,
    /// Read Local Time Info value sent to APP
    TIPC_LTI_RD_RSP,
    /// Read Reference Time Info value sent to APP
    TIPC_RTI_RD_RSP,
    /// Read Time With DST value sent to APP
    TIPC_TDST_RD_RSP,
    /// Read Time Update State value sent to APP
    TIPC_TUS_RD_RSP,
};


/// Parameters of the @ref TIPC_ENABLE_REQ message
struct tipc_enable_req
{
    /// Connection handle
    uint16_t conhdl;
    ///Connection type
    uint8_t con_type;

    ///Existing handle values cts
    struct tipc_cts_content cts;
    ///Existing handle values ndcs
    struct tipc_ndcs_content ndcs;
    ///Existing handle values rtus
    struct tipc_rtus_content rtus;
};

/// Parameters of the @ref TIPC_ENABLE_CFM message
struct tipc_enable_cfm
{
    /// Connection handle
    uint16_t conhdl;
    ///status
    uint8_t status;

    ///Existing handle values cts
    struct tipc_cts_content cts;
    ///Existing handle values ndcs
    struct tipc_ndcs_content ndcs;
    ///Existing handle values rtus
    struct tipc_rtus_content rtus;
};

///Parameters of the @ref TIPC_ERROR_IND message
struct tipc_error_ind
{
    ///Connection handle
    uint16_t conhdl;
    ///Status
    uint8_t status;
};

///Parameters of the @ref TIPC_RD_CHAR_REQ message
struct tipc_rd_char_req
{
    ///Connection handle
    uint16_t conhdl;
    ///Characteristic value code
    uint8_t  char_code;
};

///Parameters of the @ref TIPC_CT_NTF_CFG_REQ message
struct tipc_ct_ntf_cfg_req
{
    ///Connection handle
    uint16_t conhdl;
    ///Stop/notify/indicate value to configure into the peer characteristic
    uint16_t cfg_val;
};

///Parameters of the @ref TIPC_WR_TIME_UPD_CTNL_PT_REQ message
struct tipc_wr_time_udp_ctnl_pt_req
{
    ///Connection handle
    uint16_t conhdl;
    ///Value
    uint8_t value;
};

///Parameters of the @ref TIPC_WR_CHAR_RSP message
struct tipc_wr_char_rsp
{
    ///Connection handle
    uint16_t conhdl;
    ///Status
    uint8_t  status;
};

///Parameters of the @ref TIPC_CT_IND message
struct tipc_ct_ind
{
    ///Connection handle
    uint16_t conhdl;
    ///Indication type
    uint8_t ind_type;
    ///Current Time Value
    struct tip_curr_time ct_val;
};

///Parameters of the @ref TIPC_CT_NTF_CFG_RD_RSP message
struct tipc_ct_ntf_cfg_rd_rsp
{
    ///Connection handle
    uint16_t conhdl;
    ///Notification Configuration
    uint16_t ntf_cfg;
};

///Parameters of the @ref TIPC_LTI_IND message
struct tipc_lti_rd_rsp
{
    ///Connection handle
    uint16_t conhdl;
    ///Current Time Value
    struct tip_loc_time_info lti_val;
};

///Parameters of the @ref TIPC_RTI_IND message
struct tipc_rti_rd_rsp
{
    ///Connection handle
    uint16_t conhdl;
    ///Current Time Value
    struct tip_ref_time_info rti_val;
};

///Parameters of the @ref TIPC_TDST_IND message
struct tipc_tdst_rd_rsp
{
    ///Connection handle
    uint16_t conhdl;
    ///Current Time Value
    struct tip_time_with_dst tdst_val;
};

///Parameters of the @ref TIPC_TUS_RD_RSP message
struct tipc_tus_rd_rsp
{
    ///Connection handle
    uint16_t conhdl;
    ///Current Time Value
    struct tip_time_upd_state tus_val;
};

/*
 * TASK DESCRIPTOR DECLARATIONS
 ****************************************************************************************
 */

extern const struct ke_state_handler tipc_state_handler[TIPC_STATE_MAX];
extern const struct ke_state_handler tipc_default_handler;
extern ke_state_t tipc_state[TIPC_IDX_MAX];

#endif //(BLE_TIP_CLIENT)

/// @} TIPCTASK

#endif /* _TIPC_TASK_H_ */
