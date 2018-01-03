/**
 ****************************************************************************************
 *
 * @file hogpbh_task.h
 *
 * @brief Header file - HID Over GATT Profile Boot Host Role Task.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev$
 *
 ****************************************************************************************
 */


#ifndef _HOGPBH_TASK_H_
#define _HOGPBH_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup HOGPBHTASK HID Over GATT Profile Boot Host Task
 * @ingroup HOGPBH
 * @brief HID Over GATT Profile Boot Host Task
 *
 * The HOGPBHTASK is responsible for handling the messages coming in and out of the
 * @ref HOGPBH monitor block of the BLE Host.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_HID_BOOT_HOST)

#include "ke_task.h"
#include "gattc_task.h"
#include "hogpbh.h"
#include "prf_types.h"
#include "prf_utils.h"

/*
 * DEFINES
 ****************************************************************************************
 */

///Maximum number of HID Over GATT Boot Device task instances
#define HOGPBH_IDX_MAX    (BLE_CONNECTION_MAX)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Possible states of the HOGPBH task
enum
{
    /// IDLE state
    HOGPBH_IDLE,
    /// connected state
    HOGPBH_CONNECTED,
    /// Discovering Heart Rate SVC and CHars
    HOGPBH_DISCOVERING,

    /// Number of defined states.
    HOGPBH_STATE_MAX
};

enum
{
    /// Start the HID Over GATT profile - at connection
    HOGPBH_ENABLE_REQ = KE_FIRST_MSG(TASK_HOGPBH),
    ///Confirm that cfg connection has finished with discovery results, or that normal cnx started
    HOGPBH_ENABLE_CFM,

    ///Disable profile role
    HOGPBH_DISABLE_REQ,
    ///Disable profile role - at disconnection
    HOGPBH_DISABLE_IND,

    /// Generic message to read a HIDS or DIS or BAS characteristic value
    HOGPBH_RD_CHAR_REQ,
    ///Generic message for configuring the characteristics
    HOGPBH_CFG_NTF_REQ,
    /// Write peer Boot Report Char. value
    HOGPBH_BOOT_REPORT_WR_REQ,
    /// Set Boot Protocol Mode on peer device
    HOGPBH_SET_BOOT_PROTO_MODE_REQ,

    /// Send read notification configuration value to APP
    HOGPBH_CFG_NTF_RD_RSP,
    /// Send Read Protocol Mode value to APP
    HOGPBH_PROTO_MODE_RD_RSP,

    /// Boot Report value send to APP (after Read Request or Notification)
    HOGPBH_BOOT_REPORT_IND,

    ///Generic message for write characteristic response status to APP
    HOGPBH_WR_CHAR_RSP,
    ///Message sent to the app to inform that a read procedure has failed.
    HOGPBH_RD_CHAR_ERR_RSP,
};

/*
 * APIs Structure
 ****************************************************************************************
 */

/// Parameters of the @ref HOGPBH_ENABLE_REQ message
struct hogpbh_enable_req
{
    /// Connection handle
    uint16_t conhdl;
    /// Connection type
    uint8_t con_type;

    /// Number of HIDS instances
    uint8_t hids_nb;
    /// Existing handle values hids
    struct hids_content hids[HOGPBH_NB_HIDS_INST_MAX];
};

/// Parameters of the @ref HOGPBH_ENABLE_CFM message
struct hogpbh_enable_cfm
{
    /// Connection handle
    uint16_t conhdl;
    ///status
    uint8_t status;

    /// Number of HIDS instances
    uint8_t hids_nb;
    /// Existing handle values hids
    struct hids_content hids[HOGPBH_NB_HIDS_INST_MAX];
};

/// Parameters of the @ref HOGPBH_DISABLE_REQ message
struct hogpbh_disable_req
{
    /// Connection handle
    uint16_t conhdl;
};

///Parameters of the @ref HOGPBH_RD_CHAR_REQ message
struct hogpbh_rd_char_req
{
    ///Connection handle
    uint16_t conhdl;
    /// HIDS Instance
    uint8_t hids_nb;
    ///Characteristic value code
    uint8_t char_code;
};

///Parameters of the @ref HOGPBH_CFG_NTF_REQ message
struct hogpbh_cfg_ntf_req
{
    /// Connection handle
    uint16_t conhdl;
    /// Stop/notify value to configure into the peer characteristic
    uint16_t ntf_cfg;
    /// Client Characteristic Configuration Code
    uint8_t desc_code;
    /// HIDS instance
    uint8_t hids_nb;
};

///Parameters of the @ref HOGPBH_CFG_NTF_RD_RSP message
struct hogpbh_cfg_ntf_rd_rsp
{
    ///Connection handle
    uint16_t conhdl;
    /// Stop/notify value to configure into the peer characteristic
    uint16_t cfg_val;
    /// Client Characteristic Configuration Code
    uint8_t desc_code;
    /// HIDS instance
    uint8_t hids_nb;
};

///Parameters of the @ref HOGPBH_WR_CHAR_RSP and @ref HOGPBH_RD_CHAR_ERR_RSP message
struct hogpbh_char_req_rsp
{
    /// Connection handle
    uint16_t conhdl;
    /// Status
    uint8_t  status;
    /// Attribute Code
    uint8_t att_code;
};

///Parameters of the @ref HOGPBH_PROTO_MODE_RD_RSP message
struct hogpbh_proto_mode_rd_rsp
{
    ///Connection handle
    uint16_t conhdl;
    ///Protocol Mode
    uint8_t proto_mode;
    ///HIDS Instance
    uint8_t hids_nb;
};

///Parameters of the @ref HOGPBH_BOOT_REPORT_WR_REQ message
struct hogpbh_boot_report_wr_req
{
    ///Connection handle
    uint16_t conhdl;
    /// HIDS Instance
    uint8_t hids_nb;
    /// Characteristic Code
    uint8_t char_code;
    /// Write type (Write or Write without Response)
    uint8_t wr_type;
    /// Report Length
    uint8_t report_length;
    ///Boot Report
    uint8_t report[1];
};

///Parameters of the @ref HOGPBH_BOOT_REPORT_IND message
struct hogpbh_boot_report_ind
{
    /// Connection handle
    uint16_t conhdl;
    /// HIDS Instance
    uint8_t hids_nb;
    /// Read Response or Notification
    uint8_t ind_type;
    /// Char Code
    uint8_t char_code;
    /// Report Length
    uint8_t report_length;
    ///Boot Report
    uint8_t report[1];
};

///Parameters of the @ref HOGPBH_SET_BOOT_PROTO_MODE_REQ message
struct hogpbh_set_boot_proto_mode_req
{
    /// Connection handle
    uint16_t conhdl;
    /// HIDS Instance
    uint8_t hids_nb;
};

/*
 * TASK DESCRIPTOR DECLARATIONS
 ****************************************************************************************
 */

extern const struct ke_state_handler hogpbh_state_handler[HOGPBH_STATE_MAX];
extern const struct ke_state_handler hogpbh_default_handler;
extern ke_state_t hogpbh_state[HOGPBH_IDX_MAX];

#endif //(BLE_HID_BOOT_HOST)

/// @} HOGPBHTASK

#endif /* _HOGPBH_TASK_H_ */
