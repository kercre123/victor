/**
 ****************************************************************************************
 *
 * @file hogprh_task.h
 *
 * @brief Header file - HID Over GATT Profile Report Host Role Task.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev$
 *
 ****************************************************************************************
 */


#ifndef _HOGPRH_TASK_H_
#define _HOGPRH_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup HOGPRHTASK HID Over GATT Profile Report Host Task
 * @ingroup HOGPRH
 * @brief HID Over GATT Profile Report Host Task
 *
 * The HOGPRHTASK is responsible for handling the messages coming in and out of the
 * @ref HOGPRH monitor block of the BLE Host.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_HID_REPORT_HOST)

#include "ke_task.h"
#include "gattc_task.h"
#include "hogprh.h"
#include "prf_types.h"
#include "prf_utils.h"
#include "hogp_common.h"

/*
 * DEFINES
 ****************************************************************************************
 */

///Maximum number of HID Over GATT Report Host task instances
#define HOGPRH_IDX_MAX    (BLE_CONNECTION_MAX)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Possible states of the HOGPRH task
enum
{
    /// Idle state
    HOGPRH_IDLE,
    /// Connected state
    HOGPRH_CONNECTED,
    /// Discovering state
    HOGPRH_DISCOVERING,

    /// Number of defined states.
    HOGPRH_STATE_MAX
};

enum
{
    /// Start the HID Over GATT profile - at connection
    HOGPRH_ENABLE_REQ = KE_FIRST_MSG(TASK_HOGPRH),
    ///Confirm that cfg connection has finished with discovery results, or that normal cnx started
    HOGPRH_ENABLE_CFM,

    ///Disable profile role
    HOGPRH_DISABLE_REQ,
    ///Disable profile role - at disconnection
    HOGPRH_DISABLE_IND,

    /// Generic message to read a HIDS characteristic/descriptor value
    HOGPRH_RD_CHAR_REQ,
    ///Generic message for configuring the notifications
    HOGPRH_CFG_NTF_REQ,
    /// Write peer HID Control Point value request
    HOGPRH_HID_CTNL_PT_WR_REQ,
    /// Write peer Protocol Mode value request (Report Protocol Mode)
    HOGPRH_SET_REPORT_PROTO_MODE_REQ,
    /// Write peer Report value request
    HOGPRH_REPORT_WR_REQ,

    /// Send Read Protocol Mode value to APP
    HOGPRH_PROTO_MODE_RD_RSP,
    /// Send Read Report Map value to APP
    HOGPRH_REPORT_MAP_RD_RSP,
    /// Send Read Report Map External Report Ref value to APP
    HOGPRH_REPORT_MAP_EXT_REP_REF_RD_RSP,
    /// Send Read Report Report Reference value to APP
    HOGPRH_REPORT_REP_REF_RD_RSP,
    /// Send Read HID Information value to APP
    HOGPRH_HID_INFO_RD_RSP,
    /// Send Read ntf cfg value to APP
    HOGPRH_NTF_CFG_RD_RSP,

    /// Send received report value to APP
    HOGPRH_REPORT_IND,

    ///Generic message for write characteristic response status to APP
    HOGPRH_WR_CHAR_RSP,
    /// Used to inform APP that an error has been raised during read request
    HOGPRH_RD_CHAR_ERR_RSP,
};

/*
 * APIs Structure
 ****************************************************************************************
 */

/// Parameters of the @ref HOGPRH_ENABLE_REQ message
struct hogprh_enable_req
{
    /// Connection handle
    uint16_t conhdl;
    /// Connection type
    uint8_t con_type;

    /// Number of HIDS instances
    uint8_t hids_nb;
    /// Existing handle values hids
    struct hogprh_hids_content hids[HOGPRH_NB_HIDS_INST_MAX];
};

/// Parameters of the @ref HOGPRH_DISABLE_REQ message
struct hogprh_disable_req
{
    /// Connection handle
    uint16_t conhdl;
};

/// Parameters of the @ref HOGPRH_ENABLE_CFM message
struct hogprh_enable_cfm
{
    /// Connection handle
    uint16_t conhdl;
    ///status
    uint8_t status;

    /// Number of HIDS instances
    uint8_t hids_nb;
    /// Existing handle values hids
    struct hogprh_hids_content hids[HOGPRH_NB_HIDS_INST_MAX];
};

///Parameters of the @ref HOGPRH_WR_CHAR_RSP and @ref HOGPRH_RD_CHAR_ERR_RSP message
struct hogprh_char_req_rsp
{
    /// Connection handle
    uint16_t conhdl;
    /// Status
    uint8_t  status;
    /// Attribute Code
    uint8_t att_code;
};

///Parameters of the @ref HOGPRH_RD_CHAR_REQ message
struct hogprh_rd_char_req
{
    /// Connection handle
    uint16_t conhdl;
    /// HIDS Instance
    uint8_t hids_nb;
    /// Read code
    uint8_t read_code;
    /// Report Char. Instance
    uint8_t report_nb;
};

///Parameters of the @ref HOGPRH_CFG_NTF_REQ message
struct hogprh_cfg_ntf_req
{
    /// Connection handle
    uint16_t conhdl;
    /// Stop/notify value to configure into the peer characteristic
    uint16_t ntf_cfg;
    /// Report instance (The Report Char. is the only one for which we can receive notifications)
    uint8_t report_nb;
    /// HIDS instance
    uint8_t hids_nb;
};

///Parameters of the @ref HOGPRH_HID_CTNL_PT_WR_REQ message
struct hogprh_hid_ctnl_pt_wr_req
{
    /// Connection handle
    uint16_t conhdl;
    /// HIDS instance
    uint8_t hids_nb;
    /// HID Control Point value to write
    uint8_t hid_ctnl_pt;
};

///Parameters of the @ref HOGPRH_SET_REPORT_PROTO_MODE_REQ message
struct hogprh_set_report_proto_mode_req
{
    /// Connection handle
    uint16_t conhdl;
    /// HIDS instance
    uint8_t hids_nb;
};

///Parameters of the @ref HOGPRH_REPORT_WR_REQ message - Need to dynamically allocated
struct hogprh_report_wr_req
{
    /// Connection handle
    uint16_t conhdl;
    /// HIDS instance
    uint8_t hids_nb;
    /// Report instance
    uint8_t report_nb;
    /// Report Length
    uint16_t report_length;
    /// Output Report Write Type
    uint8_t out_report_type;
    /// Report value
    uint8_t report[1];
};

///Parameters of the @ref HOGPRH_PROTO_MODE_RD_RSP message
struct hogprh_proto_mode_rd_rsp
{
    ///Connection handle
    uint16_t conhdl;
    ///Protocol Mode
    uint8_t proto_mode;
    ///HIDS Instance
    uint8_t hids_nb;
};

///Parameters of the @ref HOGPRH_REPORT_MAP_RD_RSP
struct hogprh_report_map_rd_rsp
{
    /// Connection handle
    uint16_t conhdl;
    /// Report Map Length
    uint16_t report_map_length;
    /// HIDS Instance
    uint8_t hids_nb;
    /// Report Map value
    uint8_t report_map[1];
};

///Parameters of the @ref HOGPRH_REPORT_MAP_EXT_REP_REF_RD_RSP
struct hogprh_report_map_ext_rep_ref_rd_rsp
{
    /// Connection handle
    uint16_t conhdl;
    /// Report Map Char. External Report Reference Descriptor value
    uint16_t ext_report_ref;
    /// HIDS Instance
    uint8_t hids_nb;
};

///Parameters of the @ref HOGPRH_REPORT_REF_RD_RSP message
struct hogprh_report_ref_rd_rsp
{
    /// Connection handle
    uint16_t conhdl;
    /// HIDS Instance
    uint8_t hids_nb;
    /// Report Char. Instance
    uint8_t report_nb;
    /// Report Char. Report Reference Descriptor value
    struct hids_report_ref report_ref;
};

///Parameters of the @ref HOGPRH_HID_INFO_RD_RSP message
struct hogprh_hid_info_rd_rsp
{
    /// Connection handle
    uint16_t conhdl;
    /// HIDS Instance
    uint8_t hids_nb;
    /// HID Information value
    struct hids_hid_info hid_info;
};

///Parameters of the @ref HOGPRH_CFG_NTF_RD_RSP message
struct hogprh_cfg_ntf_rd_rsp
{
    /// Connection handle
    uint16_t conhdl;
    /// Stop/notify value to configure into the peer characteristic
    uint16_t cfg_val;
    /// Report instance
    uint8_t report_nb;
    /// HIDS instance
    uint8_t hids_nb;
};

///Parameters of the @ref HOGPRH_REPORT_IND message
struct hogprh_report_ind
{
    /// Connection handle
    uint16_t conhdl;
    /// Report Length
    uint16_t report_length;
    /// HIDS Instance
    uint8_t hids_nb;
    /// Report Instance
    uint8_t report_nb;
    /// Indication Type
    uint8_t ind_type;
    /// Report value
    uint8_t report[1];
};

/*
 * TASK DESCRIPTOR DECLARATIONS
 ****************************************************************************************
 */

extern const struct ke_state_handler hogprh_state_handler[HOGPRH_STATE_MAX];
extern const struct ke_state_handler hogprh_default_handler;
extern ke_state_t hogprh_state[HOGPRH_IDX_MAX];

#endif //(BLE_HID_REPORT_HOST)

/// @} HOGPRHTASK

#endif /* _HOGPRH_TASK_H_ */
