/**
 ****************************************************************************************
 *
 * @file hogpd_task.h
 *
 * @brief Header file - HID Over GATT Profile Device Task.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev$
 *
 ****************************************************************************************
 */


#ifndef _HOGPD_TASK_H_
#define _HOGPD_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup HOGPDTASK Task
 * @ingroup HOGPD
 * @brief HID Over GATT Profile Task.
 *
 * The HOGPD_TASK is responsible for handling the messages coming in and out of the
 * @ref HOGPD block of the BLE Host.
 *
 * @{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#if (BLE_HID_DEVICE)

#include <stdint.h>
#include "ke_task.h"
#include "hogpd.h"

/*
 * DEFINES
 ****************************************************************************************
 */

///Maximum number of HID Over GATT Device task instances
#define HOGPD_IDX_MAX                   (0x01)

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Possible states of the HOGPD task
enum
{
    /// Disabled state
    HOGPD_DISABLED,
    /// Idle state
    HOGPD_IDLE,
    /// Connected state
    HOGPD_CONNECTED,

    /// Number of defined states.
    HOGPD_STATE_MAX
};

/// Messages for HID Over GATT Profile Device Role
enum
{
    /// Add a HIDS instance in the database
    HOGPD_CREATE_DB_REQ = KE_FIRST_MSG(TASK_HOGPD),
    /// Inform APP of database creation status
    HOGPD_CREATE_DB_CFM,

    /// Set a Report Map value in the database
    HOGPD_SET_REPORT_MAP_REQ,

    /// Start the HID Over GATT Profile Device Role Task
    HOGPD_ENABLE_REQ,
    /// Disable confirmation with configuration to save after profile disabled
    HOGPD_DISABLE_IND,
    /// Error indication to Host
    HOGPD_ERROR_IND,

    /// Inform Device APP that a Client Characteristic Configuration has been modified
    HOGPD_NTF_CFG_IND,
    /// Inform Device APP that Protocol Mode Characteristic Value has been written on Device
    HOGPD_PROTO_MODE_IND,

    /// Request sending of a report to the host - notification
    HOGPD_REPORT_UPD_REQ,
    /// Inform Device APP about received report notification from host
    HOGPD_REPORT_IND,

    /// Request sending of a boot report to the host - notification
    HOGPD_BOOT_REPORT_UPD_REQ,
    /// Inform APP that a Boot Report has been written in the database
    HOGPD_BOOT_REPORT_IND,

    /// Inform APP that HID Control Point Characteristic Value has been written on Device
    HOGPD_CTNL_PT_IND,

    /// Inform APP if a notification has been sent to the peer device or not
    HOGPD_NTF_SENT_CFM,
};

/*
 * APIs Structures
 ****************************************************************************************
 */

/// Parameters of the @ref HOGPD_CREATE_DB_REQ message
struct hogpd_create_db_req
{
    /// Number of HIDS to add
    uint8_t hids_nb;
    /// Initial configuration for each HIDS instance
    struct hogpd_hids_cfg cfg[HOGPD_NB_HIDS_INST_MAX];
};

/// Parameters of the @ref HOGPD_CREATE_DB_CFM message
struct hogpd_create_db_cfm
{
    /// Status
    uint8_t status;
};

///Parameters of the @ref HOGPD_SET_REPORT_MAP_REQ - Need to be dynamically allocated
struct hogpd_set_report_map_req
{
    /// Report Map Length
    uint16_t report_map_len;
    /// HIDS Instance
    uint8_t hids_nb;
    /// Report Map value
    uint8_t report_map[1];
};

/// Parameters of the @ref HOGPD_ENABLE_REQ message
struct hogpd_enable_req
{
    ///Connection handle
    uint16_t conhdl;
    /// security level
    uint8_t sec_lvl;
    ///Type of connection - can be CFG or DISCOVERY
    uint8_t con_type;
    /// Notification Configurations
    struct hogpd_hids_ntf_cfg ntf_cfg[HOGPD_NB_HIDS_INST_MAX];
};

///Parameters of the @ref HOGPD_DISABLE_IND message
struct hogpd_disable_ind
{
    /// Connection handle
    uint16_t conhdl;
    /// Notification Configurations
    struct hogpd_hids_ntf_cfg ntf_cfg[HOGPD_NB_HIDS_INST_MAX];
};

///Parameters of the @ref HOGPD_NTF_CFG_IND message
struct hogpd_ntf_cfg_ind
{
    /// Connection Handle
    uint16_t conhdl;
    /// New Notification Configuration Value
    uint16_t ntf_en;
    /// Cfg. Code
    uint8_t cfg_code;
    /// HIDS Instance
    uint8_t hids_nb;
    /// Report Char. Instance
    uint8_t report_nb;
};

///Parameters of the @ref HOGPD_PROTO_MODE_IND message
struct hogpd_proto_mode_ind
{
    /// Connection Handle
    uint16_t conhdl;
    /// HIDS Instance
    uint8_t hids_nb;
    /// New Protocol Mode Characteristic Value
    uint8_t proto_mode;
};

///Parameters of the @ref HOGPD_REPORT_UPD_REQ and @ref HOGPD_REPORT_IND - Need to be dynamically allocated
struct hogpd_report_info
{
    /// Connection Handle
    uint16_t conhdl;
    /// HIDS Instance
    uint8_t hids_nb;
    /// Report Char. Instance
    uint8_t report_nb;
    /// Report Length (uint8_t)
    uint16_t report_length;
    /// Report
    uint8_t report[1];
};

///Parameters of the @ref HOGPD_BOOT_REPORT_UPD_REQ and @ref HOGPD_BOOT_KB_IN_IND - Need to be dynamically allocated
struct hogpd_boot_report_info
{
    /// Connection Handle
    uint16_t conhdl;
    /// HIDS Instance
    uint8_t hids_nb;
    /// Char Code
    uint8_t char_code;
    /// Report Length
    uint8_t report_length;
    /// Boot Report
    uint8_t boot_report[1];
};

///Parameters of the @ref HOGPD_CTNL_PT_IND message
struct hogpd_ctnl_pt_ind
{
    /// Connection Handle
    uint16_t conhdl;
    /// HIDS Instance
    uint8_t hids_nb;
    /// New HID Control Point Characteristic Value
    uint8_t hid_ctnl_pt;
};

///Parameters of the @ref HOGPD_NTF_SENT_CFM message
struct hogpd_ntf_sent_cfm
{
    /// Connection Handle
    uint16_t conhdl;
    /// Status
    uint8_t status;
    /// HIDS Instance
    uint8_t hids_nb;
    /// Characteristic Code
    uint8_t char_code;
    /// Report Instance
    uint8_t report_nb;
};

/*
 * TASK DESCRIPTOR DECLARATIONS
 ****************************************************************************************
 */

extern const struct ke_state_handler hogpd_state_handler[HOGPD_STATE_MAX];
extern const struct ke_state_handler hogpd_default_handler;
extern ke_state_t hogpd_state[HOGPD_IDX_MAX];

#endif /* #if BLE_HID_DEVICE */

/// @} HOGPDTASK

#endif /* _HOGPD_TASK_H_ */
