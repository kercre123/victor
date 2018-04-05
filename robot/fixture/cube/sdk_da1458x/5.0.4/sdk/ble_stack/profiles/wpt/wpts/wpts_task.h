/**
 ****************************************************************************************
 *
 * @file wpts_task.h
 *
 * @brief Header file - wpts_task.
 * @brief 128 UUID service. sample code
 *
 * Copyright (C) 2013 Dialog Semiconductor GmbH and its Affiliates, unpublished work
 * This computer program includes Confidential, Proprietary Information and is a Trade Secret 
 * of Dialog Semiconductor GmbH and its Affiliates. All use, disclosure, and/or 
 * reproduction is prohibited unless authorized in writing. All Rights Reserved.
 *
 ****************************************************************************************
 */

#ifndef WPTS_TASK_H_
#define WPTS_TASK_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
*/

#if (BLE_WPTS)

#include <stdint.h>
#include "ke_task.h"
#include "wpts.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// Maximum number of Sample128 task instances
#define WPTS_IDX_MAX                 (1)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Possible states of the WPTS task
enum
{
    /// Disabled State
    WPTS_DISABLED,
    /// Idle state
    WPTS_IDLE,
    /// Connected state
    WPTS_CONNECTED,

    /// Number of defined states.
    WPTS_STATE_MAX
};

/// Messages for WPTS
enum
{
    /// Start wpts. Device connection
    WPTS_ENABLE_REQ = KE_FIRST_MSG(TASK_WPTS),
    /// Disable confirm.
    WPTS_DISABLE_IND,
    /// Send Alert bit fields to PTU Unit
    WPTS_ALERT_SEND_REQ,
    /// Inform App about Alert send status
    WPTS_ALERT_SEND_CFM,
    /// Save static configuration of PRU in DataBase
    WPTS_PRU_STATIC_VAL_UPD_DB_REQ,
    /// Inform App about DB status
    WPTS_PRU_STATIC_VAL_UPD_DB_CFM,
    /// Save/Update dynamic configuration of PRU in DataBase
    WPTS_PRU_DYNAMIC_VAL_UPD_DB_REQ,
    /// Inform App about DB status
    WPTS_PRU_DYNAMIC_VAL_UPD_DB_CFM,
    /// Inform App about characteristic has been read
    WPTS_PRU_DYNAMIC_VAL_READ_DB_IND,
    /// Start/Stop alert notification
    WPTS_ALERT_CFG_INDNTF_IND,
    /// Change control behavior
    WPTS_CTRL_CHANGE_IND,
    /// Get PTU static configuration value
    WPTS_PTU_STATIC_VAL_IND,
    /// Create DataBase
    WPTS_CREATE_DB_REQ,
    /// Inform APP of database creation status
    WPTS_CREATE_DB_CFM,
    /// Error Indication
    WPTS_ERROR_IND,
};

/*
 * API MESSAGES STRUCTURES
 ****************************************************************************************
 */

/// Parameters of the @ref WPTS_ENABLE_REQ message
struct wpts_enable_req
{
    /// Connection Handle
    uint16_t conhdl;
    /// Security level
    uint8_t sec_lvl;

    /// Saved LLS alert level to set in ATT DB
    uint8_t wpts_1_val;
};

/// Parameters of the @ref WPTS_DISABLE_IND message
struct wpts_disable_ind
{
    /// Connection Handle
    uint16_t conhdl;
};

/// Parameters of the @ref WPTS_ALERT_CFG_INDNTF_IND message
struct wpts_alert_cfg_indntf_ind
{
    /// Connection handle
    uint16_t conhdl;
    /// Stop/notify/indicate value to configure into the peer characteristic
    uint16_t cfg_val;
};

#pragma pack(push)
#pragma pack(1)
/// Parameters of the @ref WPTS_CTRL_CHANGE_IND message
struct wpts_ctrl_change_ind
{
    /// Connection handle
    uint16_t conhdl;
    /// bit field contains enable/disable charging configuration
    uint8_t enables;
    /// bit field contains permission configuration
    uint8_t permission;
    /// required amount of time to start charging
    uint8_t time_set;
    /// RFU
    uint16_t rfu_1;
};

/// Parameters of the @ref WPTS_PTU_STATIC_VAL_IND message
struct wpts_ptu_static_val_ind
{
    /// Connection handle
    uint16_t conhdl;
    /// Defined which fields are valid
    uint8_t valid;
    /// Power of PTU
    uint8_t power;
    /// Maximum source impedance of PTU
    uint8_t max_impedance;
    /// Maximum load resistance of PTU
    uint8_t max_resistance;
    /// RFU field
    uint16_t rfu_1;
    /// PTU class
    uint8_t class;
    /// Revision of PTU hardware
    uint8_t hw_ver;
    /// Revision of PTU firmware
    uint8_t sw_ver;
    /// A4WP supported revision
    uint8_t prtcl_ver;
    /// Maximum number of devices
    uint8_t max_dev;
    /// RFUs
    uint16_t rfu_2;
    uint16_t rfu_3;
    uint16_t rfu_4;
};

/// Parameters of the @ref WPTS_CREATE_DB_REQ message
struct wpts_create_db_req
{
    /// Indicate if TXPS is supported or not
    uint8_t features;
};

/// Parameters of the @ref WPTS_CREATE_DB_CFM message
struct wpts_create_db_cfm
{
    /// Status
    uint8_t status;
};

/// Parameters of the @ref WPTS_ALERT_SEND_REQ message
struct wpts_alert_send_req
{
    /// Connection handle
    uint16_t conhdl;
    /// notification/indication request
    uint8_t is_ind;
    /// Alert bit fields
    uint8_t alert_val;
    /// Device address
    uint8_t dev_address[WPTS_BD_ADDR_LEN];
};

/// Parameters of the @ref WPTS_ALERT_SEND_CFM message
struct wpts_alert_send_cfm
{
    /// Connection handle
    uint16_t conhdl;
    /// Status
    uint8_t status;
};

///Parameters of the @ref WPTS_PRU_STATIC_VAL_UPD_DB_REQ message
struct wpts_pru_static_val_req
{
    /// Defined which fields are valid
    uint8_t valid;
    /// A4WP supported revision
    uint8_t prtcl_ver;
    /// Reserved for future use
    uint8_t rfu_1;
    /// Category of PRU
    uint8_t cat;
    /// Capabilities of PRU (bit field)
    uint8_t info;
    /// Revision of the PRU hardware
    uint8_t hw_rev;
    /// Revision of the PRU firmware
    uint8_t sw_rev;
    /// P_RECT_MAX of the PRU (mW * 100)
    uint8_t p_rect_max;
    /// V_RECT_MIN (static, first estimate) (mV)
    uint16_t v_rect_min;
    /// V_RECT_HIGH (static, first estimate) (mV)
    uint16_t v_rect_high;
    /// V_RECT_SET (mV)
    uint16_t v_rect_set;
    /// Delta R1 caused by PRU (.01 ohms)
    uint16_t v_delta_r1;
    /// Reserved for future use
    uint16_t rfu_2;
    /// Reserved for future use
    uint16_t rfu_3;
};

///Parameters of the @ref WPTS_PRU_STATIC_VAL_UPD_DB_CFM message
struct wpts_pru_static_val_cfm
{
    /// Connection handle
    uint16_t conhdl;
    /// Status
    uint8_t status;
};

///Parameters of the @ref WPTS_PRU_DYNAMIC_VAL_UPD_DB_REQ message
struct wpts_pru_dynamic_val_req
{
    /// Connection handle 
    uint16_t conhdl;
    /// Defined which fields are valid
    uint8_t valid;
    /// DC voltage at the output of the rectifier
    uint16_t v_rect;
    /// DC current at the output of the rectifier
    uint16_t i_rect;
    /// Voltage at charge/battery port
    uint16_t v_out;
    /// Current at charge/battery port
    uint16_t i_out;
    /// Temperature of PRU
    uint8_t temperature;
    /// The current dynamic minimum rectifier voltage desired
    uint16_t v_rect_min_dyn;
    /// Desired VRECT (dynamic value)
    uint16_t v_rect_set_dyn;
    /// The current dynamic maximum rectifier voltage desired
    uint16_t v_rect_high_dyn;
    /// Warnings
    uint8_t alert;
    /// 3 bytes RFU
    uint8_t rfu_1;
    uint8_t rfu_2;
    uint8_t rfu_3;
};

///Parameters of the @ref WPTS_PRU_DYNAMIC_VAL_UPD_DB_CFM message
struct wpts_pru_dynamic_val_cfm
{
    /// Connection handle
    uint16_t conhdl;
    /// Status
    uint8_t status;
};

struct wpts_error_ind
{
    uint8_t error_id;
    uint8_t reason;
};

#pragma pack(pop)

/*
 * GLOBAL VARIABLES DECLARATIONS
 ****************************************************************************************
 */

extern const struct ke_state_handler wpts_state_handler[WPTS_STATE_MAX];
extern const struct ke_state_handler wpts_default_handler;
extern ke_state_t wpts_state[WPTS_IDX_MAX];

#endif //BLE_WPTS

#endif // WPTS_TASK_H_
