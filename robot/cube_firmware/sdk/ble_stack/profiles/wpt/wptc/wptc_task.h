/**
 ****************************************************************************************
 *
 * @file wptc_task.h
 *
 * @brief Header file - WPTCTASK.
 *
 * Copyright (C) 2013 Dialog Semiconductor GmbH and its Affiliates, unpublished work
 * This computer program includes Confidential, Proprietary Information and is a Trade Secret
 * of Dialog Semiconductor GmbH and its Affiliates. All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing. All Rights Reserved.
 *
 ****************************************************************************************
 */

#ifndef WPTC_TASK_H_
#define WPTC_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup WPTCTASK Wireless Transfer Profile Client Task
 * @ingroup PROXC
 * @brief  Wireless Transfer Profile Client Task
 *
 * The Wireless Transfer Profile Client Task is responsible for handling the API
 *  messages received from either the Application or internal tasks.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#if (BLE_WPT_CLIENT)

#include "ke_task.h"
#include "co_error.h"
#include "wptc.h"
#include "wpt_common.h"

/*
 * DEFINES
 ****************************************************************************************
 */

///Maximum number of WPT Client Connections
#define WPTC_IDX_MAX     (BLE_CONNECTION_MAX)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Possible states of the WPTC task
enum
{
    /// IDLE state
    WPTC_IDLE,
    ///Discovering
    WPTC_DISCOVERING,
    ///Connected state
    WPTC_CONNECTED,

    /// Number of defined states.
    WPTC_STATE_MAX
};

/// Possible read/write characteristics (used in callbacks to determine which
/// characteristic was read)
enum
{
    WTPC_RD_PRU_DYNAMIC_PARAM,
    WTPC_RD_PRU_STATIC_PARAM,
    WPTC_WR_PTU_STATIC_PARAM,
    WPTC_WR_PRU_CONTROL_PARAM,
    WPTC_WR_ENABLE_NOTIFICATIONS
};

///WPT Client API messages
enum
{
    /// Wireless Power Transfer Client role enable request from application.
    WPTC_ENABLE_REQ = KE_FIRST_MSG(TASK_WPTC),
    /// OFFLINE (Without dynamic discovering characteristic) profile enabling. This should be used during registration phase
    /// This request uses a wptc_offline_enable_req structure
    WPTC_OFFLINE_ENABLE_REQ,
    /// WPT Client role enable confirmation to application.
    WPTC_ENABLE_CFM,

    /// Inform APP that the profile client role has been disabled after a disconnection
    WPTC_DISABLE_IND,

    /// Set PRU Control Parameter
    WPTC_WR_PRU_CONTROL_REQ,
    /// Read PRU DYNAMIC PARAMETER
    WPTC_RD_PRU_DYNAMIC_PARAMETER_REQ,
    /// Write PTU Static Parameter
    WPTC_WR_PTU_STATIC_PARAMETER_REQ,
    /// Read PRU Static Parameter
    WPTC_RD_PRU_STATIC_PARAMETER_REQ,

    /// Enable/Disable PRU Alert Notification
    WPTC_SEND_ALERT_CFG_INDNTF_REQ,
    /// Alert notification about what kind of error occured on PRU side
    WPTC_PRU_ALERT_REPORT_NTF,

    /// Send read Characteristic Response to APP
    WPTC_RD_CHAR_RSP,
    /// Send write Characteristic Response to APP
    WPTC_WR_CHAR_RSP,
};

//WPT Client Discovery State
enum
{
    WPTC_DISCOVERY_MODE_PRIMARY_SERVICE,
    WPTC_DISCOVERY_MODE_ALL_CHARACTERISTICS,
    WPTC_DISCOVERY_MODE_DESCRIPTORS
};
/*
 * API Messages Structures
 ****************************************************************************************
 */

/// Parameters of the @ref WPTC_SEND_ALERT_CFG_INDNTF_REQ message
struct wptc_alert_cfg_indntf_req
{
    /// Connection handle
    uint16_t conhdl;
    /// Stop/notify/indicate value to configure into the peer characteristic
    uint16_t cfg_val;
};

/// WTP Client enable command structure
struct wptc_enable_req
{
    /// Connection handle
    uint16_t conhdl;
    /// Connection type
    uint8_t con_type;

    /// Service for Wireless Power Transfer details kept in APP
    struct svc_content_wpts wpts;
};

/// WTP Client "offline-enable" command structure
struct wptc_offline_enable_req
{
    /// Connection handle
    uint16_t conhdl;
    /// Connection type
    uint16_t wpt_uuid_start;

    /// Service for Wireless Power Transfer details kept in APP
    struct svc_content_wpts wpts;
};

/// WTP Client enable confirm structure
struct wptc_enable_cfm
{
    /// Connection handle
    uint16_t conhdl;
    /// Status
    uint8_t status;

    /// Service for Wireless Power Transfer in APP
    struct svc_content_wpts wpts;
};


///Parameters of the @ref WPTC_WR_PRU_CONTROL_REQ message
struct wptc_wr_pru_control_req
{
    /// Connection handle
    uint16_t conhdl;

    /// struct for dynamic value param
    struct pru_control_value pru_control_val;
};

///Parameters of the @ref WPTC_WR_PTU_STATIC_PARAMETER_REQ message
struct wptc_wr_ptu_static_req
{
    /// Connection handle
    uint16_t conhdl;

    /// struct for dynamic value param
    struct ptu_static_value ptu_static_val;

};

///Parameters of the @ref WPTC_RD_PRU_STATIC_PARAMETER_REQ message
struct wptc_rd_pru_dynamic_param_req
{
    /// Connection handle
    uint16_t conhdl;
};

///Parameters of the @ref WPTC_RD_PRU_STATIC_PARAMETER_REQ message
struct wptc_rd_pru_static_param_req
{
    /// Connection handle
    uint16_t conhdl;
};

///Parameters of the @ref WPTC_RD_CHAR_RSP message
struct wptc_rd_char_rsp
{
  struct prf_att_info info;
};

///Parameters of the @ref WPTC_WR_CHAR_RSP message
struct wptc_wr_char_rsp
{
    /// Connection handle
    uint16_t conhdl;
    /// Write characteristic response status code, may be GATT code or ATT error code.
    uint8_t  status;
};

struct wptc_alert_report_ntf
{
    /// Connection handle
    uint16_t conhdl;
    /// Alert bit fields
    uint8_t alert_val;
    uint8_t device_address[BD_ADDR_LEN];
};

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

extern const struct ke_state_handler wptc_state_handler[WPTC_STATE_MAX];
extern const struct ke_state_handler wptc_default_handler;
extern ke_state_t wptc_state[WPTC_IDX_MAX];

#endif //BLE_WPT_CLIENT

/// @} WPTCTASK

#endif // WPTC_TASK_H_
