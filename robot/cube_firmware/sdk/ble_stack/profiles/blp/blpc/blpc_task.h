/**
 ****************************************************************************************
 *
 * @file blpc_task.h
 *
 * @brief Header file - Blood Pressure Profile Collector Role Task.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev$
 *
 ****************************************************************************************
 */

#ifndef _BLPC_TASK_H_
#define _BLPC_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup BLPCTASK Blood Pressure Profile Collector Task
 * @ingroup BLPC
 * @brief Blood Pressure Profile Collector Task
 *
 * The BLPCTASK is responsible for handling the messages coming in and out of the
 * @ref BLPC monitor block of the BLE Host.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_BP_COLLECTOR)
#include "ke_task.h"
#include "gattc_task.h"
#include "co_error.h"
#include "blpc.h"
#include "blps.h"
#include "prf_types.h"
#include "prf_utils.h"

/*
 * DEFINES
 ****************************************************************************************
 */

///Maximum number of Blood Pressure Collector task instances
#define BLPC_IDX_MAX    (BLE_CONNECTION_MAX)

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Possible states of the BLPC task
enum
{
    /// IDLE state
    BLPC_IDLE,
    /// connected state
    BLPC_CONNECTED,
    /// Discovering Blood Pressure SVC and CHars
    BLPC_DISCOVERING,

    /// Number of defined states.
    BLPC_STATE_MAX
};

enum
{
    /// Start the blood pressure profile - at connection
    BLPC_ENABLE_REQ = KE_FIRST_MSG(TASK_BLPC),
    ///Confirm that cfg connection has finished with discovery results, or that normal cnx started
    BLPC_ENABLE_CFM,
    /// Disable Indication - Send when the connection with a peer peripheral has been turned off
    BLPC_DISABLE_IND,
    ///Generic error message
    BLPC_ERROR_IND,

    /// Generic message to read a BPS or DIS characteristic value
    BLPC_RD_CHAR_REQ,
    ///Generic message for read responses for APP
    BLPC_RD_CHAR_RSP,
    ///Generic message for configuring the 2 characteristics that can be handled
    BLPC_CFG_INDNTF_REQ,
    ///Generic message for write characteristic response status to APP
    BLPC_WR_CHAR_RSP,

    /// Blood pressure value send to APP
    BLPC_BP_MEAS_IND,
};

/// Parameters of the @ref BLPC_ENABLE_REQ message
struct blpc_enable_req
{
    /// Connection handle
    uint16_t conhdl;
    ///Connection type
    uint8_t con_type;
    ///Existing handle values bps
    struct bps_content bps;
};

/// Parameters of the @ref BLPC_ENABLE_CFM message
struct blpc_enable_cfm
{
    /// Connection handle
    uint16_t conhdl;
    ///status
    uint8_t status;
    ///Existing handle values bps
    struct bps_content bps;
};

///Parameters of the @ref BLPC_ERROR_IND message
struct blpc_error_ind
{
    ///Connection handle
    uint16_t conhdl;
    ///Status
    uint8_t  status;
};

///Parameters of the @ref BLPC_RD_CHAR_REQ message
struct blpc_rd_char_req
{
    ///Connection handle
    uint16_t conhdl;
    ///Characteristic value code
    uint8_t  char_code;
};

///Parameters of the @ref BLPC_RD_CHAR_RSP message
struct blpc_rd_char_rsp
{
    /// Attribute data information
    struct prf_att_info info;
};

///Parameters of the @ref BLPC_CFG_INDNTF_REQ message
struct blpc_cfg_indntf_req
{
    ///Connection handle
    uint16_t conhdl;
    ///Own code for differentiating between blood pressure and
    ///intermediate cuff pressure measurements
    uint8_t char_code;
    ///Stop/notify/indicate value to configure into the peer characteristic
    uint16_t cfg_val;
};

///Parameters of the @ref BLPC_WR_CHAR_RSP message
struct blpc_wr_char_rsp
{
    ///Connection handle
    uint16_t conhdl;
    ///Status
    uint8_t  status;
};

///Parameters of the @ref BLPC_MEAS_IND message
struct blpc_meas_ind
{
    ///Connection handle
    uint16_t conhdl;
    /// Flag indicating if it is a intermediary cuff pressure measurement (0) or
    /// stable blood pressure measurement (1).
    uint16_t flag_interm_cp;
    ///Blood Pressure measurement
    struct bps_bp_meas meas_val;
};

/*
 * TASK DESCRIPTOR DECLARATIONS
 ****************************************************************************************
 */

extern const struct ke_state_handler blpc_state_handler[BLPC_STATE_MAX];
extern const struct ke_state_handler blpc_default_handler;
extern ke_state_t blpc_state[BLPC_IDX_MAX];

#endif //(BLE_BP_COLLECTOR)

/// @} BLPCTASK

#endif /* _BLPC_TASK_H_ */
