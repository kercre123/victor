/**
 ****************************************************************************************
 *
 * @file htpc_task.h
 *
 * @brief Header file - HTPCTASK.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

#ifndef HTPC_TASK_H_
#define HTPC_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup HTPCTASK Health Thermometer Profile Collector Task
 * @ingroup HTPC
 * @brief Health Thermometer Profile Collector Task
 *
 * The HTPCTASK is responsible for handling the messages coming in and out of the
 * @ref HTPC monitor block of the BLE Host.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#if (BLE_HT_COLLECTOR)
#include "ke_task.h"
#include "gattc_task.h"
#include "htpc.h"
#include "htp_common.h"
#include "prf_types.h"

/*
 * DEFINES
 ****************************************************************************************
 */
///Maximum number of Thermometer Collector task instances
#define HTPC_IDX_MAX        (BLE_CONNECTION_MAX)

/// Possible states of the HTPC task
enum
{
    ///Idle state
    HTPC_IDLE,
    ///Discovering State
    HTPC_DISCOVERING,
    ///Connected state
    HTPC_CONNECTED,

    ///Number of defined states.
    HTPC_STATE_MAX
};

enum
{
    /// Start the Health Thermometer Collector profile - at connection
    HTPC_ENABLE_REQ = KE_FIRST_MSG(TASK_HTPC),
    /// Confirm that cfg connection has finished with discovery results, or that normal cnx started
    HTPC_ENABLE_CFM,
    /// Disable Indication - Send when the connection with a peer peripheral has been turned off
    HTPC_DISABLE_IND,
    /// Generic error message
    HTPC_ERROR_IND,

    /// Generic message to read a HTS characteristic value
    HTPC_RD_CHAR_REQ,
    ///Generic message for read responses for APP
    HTPC_RD_CHAR_RSP,
    ///Generic message for configuring the 3 characteristics that can be
    HTPC_CFG_INDNTF_REQ,
    ///APP request for measurement interval write
    HTPC_WR_MEAS_INTV_REQ,
    ///Generic message for write characteristic response status to APP
    HTPC_WR_CHAR_RSP,

    /// Temperature value send to APP
    HTPC_TEMP_IND,
    /// Measurement interval value indication to APP
    HTPC_MEAS_INTV_IND,
};

/// Parameters of the @ref HTPC_ENABLE_REQ message
struct htpc_enable_req
{
    /// Connection handle
    uint16_t conhdl;
    ///Connection type
    uint8_t con_type;

    ///Existing handle values hts
    struct htpc_hts_content hts;
};

/// Parameters of the @ref HTPC_ENABLE_CFM message
struct htpc_enable_cfm
{
    /// Connection handle
    uint16_t conhdl;
    ///status
    uint8_t status;
    /// HTS handle values and characteristic properties
    struct htpc_hts_content hts;
};

///Parameters of the @ref HTPC_ERROR_IND message
struct htpc_error_ind
{
    ///Connection handle
    uint16_t conhdl;
    ///Status
    uint8_t  status;
};

///Parameters of the @ref HTPC_RD_CHAR_REQ message
struct htpc_rd_char_req
{
    ///Connection handle
    uint16_t conhdl;
    ///Characteristic value code
    uint8_t char_code;
};

///Parameters of the @ref HTPC_RD_CHAR_RSP message
struct htpc_rd_char_rsp
{
    /// Attribute data information
    struct prf_att_info info;
};

///Parameters of the @ref HTPC_CFG_INDNTF_REQ message
struct htpc_cfg_indntf_req
{
    ///Connection handle
    uint16_t conhdl;
    ///Stop/notify/indicate value to configure into the peer characteristic
    uint16_t cfg_val;
    ///Own code for differentiating between Temperature Measurement, Intermediate Temperature and Measurement Interval chars
    uint8_t char_code;
};

///Parameters of the @ref HTPC_WR_MEAS_INTV_REQ message
struct htpc_wr_meas_intv_req
{
    ///Connection handle
    uint16_t conhdl;
    ///Interval
    uint16_t intv;
};

///Parameters of the @ref HTPC_WR_CHAR_RSP message
struct htpc_wr_char_rsp
{
    ///Connection handle
    uint16_t conhdl;
    ///Status
    uint8_t  status;
};

///Parameters of the @ref HTPC_TEMP_IND message
struct htpc_temp_ind
{
    /// Connection handle
    uint16_t conhdl;
    /// Stable or intermediary type of temperature
    uint8_t flag_stable_meas;
    /// Temperature Measurement Structure
    struct htp_temp_meas temp_meas;
};

///Parameters of @ref HTPC_MEAS_INTV_IND message
struct htpc_meas_intv_ind
{
    ///Connection handle
    uint16_t conhdl;
    ///Interval
    uint16_t intv;
};

/*
 * TASK DESCRIPTOR DECLARATIONS
 ****************************************************************************************
 */

extern const struct ke_state_handler htpc_state_handler[HTPC_STATE_MAX];
extern const struct ke_state_handler htpc_default_handler;
extern ke_state_t htpc_state[HTPC_IDX_MAX];

#endif //BLE_HT_COLLECTOR

/// @} HTPCTASK

#endif // HTPC_TASK_H_
