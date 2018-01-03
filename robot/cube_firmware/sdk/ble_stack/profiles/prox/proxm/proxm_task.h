/**
 ****************************************************************************************
 *
 * @file proxm_task.h
 *
 * @brief Header file - PROXMTASK.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

#ifndef PROXM_TASK_H_
#define PROXM_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup PROXMTASK Proximity Monitor Task
 * @ingroup PROXM
 * @brief Proximity Monitor Task
 *
 * The Proximity Monitor Task is responsible for handling the API messages received from
 * either the Application or internal tasks.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#if (BLE_PROX_MONITOR)

#include "ke_task.h"
#include "co_error.h"
#include "proxm.h"

/*
 * DEFINES
 ****************************************************************************************
 */

///Maximum number of Proximity Monitor task instances
#define PROXM_IDX_MAX    (BLE_CONNECTION_MAX)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Possible states of the PROXM task
enum
{
    /// IDLE state
    PROXM_IDLE,
    ///Connected state
    PROXM_CONNECTED,
    ///Discovering
    PROXM_DISCOVERING,

    /// Number of defined states.
    PROXM_STATE_MAX
};

///Proximity Monitor API messages
enum
{
    ///Proximity Monitor role enable request from application.
    PROXM_ENABLE_REQ = KE_FIRST_MSG(TASK_PROXM),
    /// Proximity Monitor role enable confirmation to application.
    PROXM_ENABLE_CFM,

    /// Inform APP that the profile client role has been disabled after a disconnection
    PROXM_DISABLE_IND,

    ///Read LLS Alert level (for IAS  not allowed)
    PROXM_RD_ALERT_LVL_REQ,
    ///Set Alert level
    PROXM_WR_ALERT_LVL_REQ,
    /// Read TX Power Level
    PROXM_RD_TXPW_LVL_REQ,

    ///Send read Characteristic Response to APP
    PROXM_RD_CHAR_RSP,
    ///Send write Characteristic Response to APP
    PROXM_WR_CHAR_RSP,
};

/*
 * API Messages Structures
 ****************************************************************************************
 */

/// Proximity monitor enable command structure
struct proxm_enable_req
{
    /// Connection handle
    uint16_t conhdl;
    /// Connection type
    uint8_t con_type;

    /// Reporter LLS details kept in APP
    struct svc_content lls;
    /// Reporter IAS details kept in APP
    struct svc_content ias;
    /// Reporter TPS details kept in APP
    struct svc_content txps;
};

/// Proximity monitor enable confirm structure
struct proxm_enable_cfm
{
    /// Connection handle
    uint16_t conhdl;
    /// Status
    uint8_t status;

    /// Reporter LLS details to keep in APP
    struct svc_content lls;
    /// Reporter IAS details to keep in APP
    struct svc_content ias;
    /// Reporter TPS details to keep in APP
    struct svc_content txps;
};

///Parameters of the @ref PROXM_RD_ALERT_LVL_REQ message
struct proxm_rd_alert_lvl_req
{
    /// Connection handle
    uint16_t conhdl;
};

///Parameters of the @ref PROXM_WR_ALERT_LVL_REQ message
struct proxm_wr_alert_lvl_req
{
    /// Connection handle
    uint16_t conhdl;
    /// 0=LLS or 1=IAS, code for the service in which the alert level should be written
    uint8_t  svc_code;
    /// Alert level
    uint8_t  lvl;
};

///Parameters of the @ref PROXM_RD_CHAR_RSP message
struct proxm_rd_char_rsp
{
    struct prf_att_info info;
};

///Parameters of the @ref PROXM_WR_CHAR_RSP message
struct proxm_wr_char_rsp
{
    /// Connection handle
    uint16_t conhdl;
    /// Write characteristic response status code, may be GATT code or ATT error code.
    uint8_t  status;
};

///Parameters of the @ref PROXM_RD_TXPW_LVL_REQ message
struct proxm_rd_txpw_lvl_req
{
    /// Connection handle
    uint16_t conhdl;
};

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

extern const struct ke_state_handler proxm_state_handler[PROXM_STATE_MAX];
extern const struct ke_state_handler proxm_default_handler;
extern ke_state_t proxm_state[PROXM_IDX_MAX];

#endif //BLE_PROX_MONITOR

/// @} PROXMTASK

#endif // PROXM_TASK_H_
