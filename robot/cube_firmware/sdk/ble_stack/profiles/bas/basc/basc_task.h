/**
 ****************************************************************************************
 *
 * @file basc_task.h
 *
 * @brief Header file - Battery Service Client Role Task.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev$
 *
 ****************************************************************************************
 */


#ifndef _BASC_TASK_H_
#define _BASC_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup BASCTASK Battery Service Client Task
 * @ingroup BASC
 * @brief Battery Service Client Task
 *
 * The BASCTASK is responsible for handling the messages coming in and out of the
 * @ref BASC block of the BLE Host.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_BAS_CLIENT)

#include "ke_task.h"
#include "gattc_task.h"
#include "basc.h"

/*
 * DEFINES
 ****************************************************************************************
 */

///Maximum number of Battery Client task instances
#define BASC_IDX_MAX     (BLE_CONNECTION_MAX)

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Possible ways for receiving a value
enum
{
    ///Notification
    BASC_IND_NTF        = 0x00,
    ///Read Response
    BASC_IND_RD_RSP,
};

/// Possible states of the BAPC task
enum
{
    /// IDLE state
    BASC_IDLE,
    /// connected state
    BASC_CONNECTED,
    /// Discovering
    BASC_DISCOVERING,

    /// Number of defined states.
    BASC_STATE_MAX
};

enum
{
    /// Start the Battery Service Client Role - at connection
    BASC_ENABLE_REQ = KE_FIRST_MSG(TASK_BASC),
    ///Confirm that cfg connection has finished with discovery results, or that normal cnx started
    BASC_ENABLE_CFM,
    /// Disable Indication - Send when the connection with a peer peripheral has been turned off
    BASC_DISABLE_IND,
    ///Generic error message
    BASC_ERROR_IND,
    ///Inform APP about write command status
    BASC_WR_CHAR_RSP,

    /// Read Characteristic Value Request
    BASC_CHAR_RD_REQ,
    /// Inform APP that Battery Level Notification Configuration has been read
    BASC_BATT_LEVEL_NTF_CFG_RD_RSP,
    /// Write Battery Level Notification Configuration Value request
    BASC_BATT_LEVEL_NTF_CFG_REQ,
    /// Inform APP that Battery Level Characteristic Presentation Format has been read
    BASC_BATT_LEVEL_PRES_FORMAT_RD_RSP,

    /// Indicate to APP that the Battery Level value has been received
    BASC_BATT_LEVEL_IND,
};

/*
 * APIs Structure
 ****************************************************************************************
 */

/// Parameters of the @ref BASC_ENABLE_REQ message
struct basc_enable_req
{
    /// Connection handle
    uint16_t conhdl;
    ///Connection type
    uint8_t con_type;

    /// Number of BAS instances that have previously been found
    uint8_t bas_nb;
    /// Existing handle values bas
    struct bas_content bas[BASC_NB_BAS_INSTANCES_MAX];
};

/// Parameters of the @ref BASC_ENABLE_CFM message
struct basc_enable_cfm
{
    /// Connection handle
    uint16_t conhdl;
    /// Status
    uint8_t status;

    /// Number of BAS that have been found
    uint8_t bas_nb;
    ///Existing handle values bas
    struct bas_content bas[BASC_NB_BAS_INSTANCES_MAX];
};

///Parameters of the @ref BASC_ERROR_IND message
struct basc_error_ind
{
    ///Connection handle
    uint16_t conhdl;
    ///Status
    uint8_t status;
};

///Parameters of the @ref BASC_WR_CHAR_RSP message
struct basc_wr_char_rsp
{
    ///Connection handle
    uint16_t conhdl;
    ///Status
    uint8_t status;
};

///Parameters of the @ref BASC_CHAR_RD_REQ message
struct basc_char_rd_req
{
    ///Connection handle
    uint16_t conhdl;
    ///Characteristic Code
    uint8_t char_code;
    ///Battery Service Instance - From 0 to BASC_NB_BAS_INSTANCES_MAX-1
    uint8_t bas_nb;
};

///Parameters of the @ref BASC_BATT_LEVEL_NTF_CFG_RD_RSP message
struct basc_batt_level_ntf_cfg_rd_rsp
{
    ///Connection handle
    uint16_t conhdl;
    ///Notification Configuration Value
    uint16_t ntf_cfg;
    ///Battery Service Instance
    uint8_t bas_nb;
};

///Parameters of the @ref BASC_BATT_LEVEL_NTF_CFG_REQ message
struct basc_batt_level_ntf_cfg_req
{
    ///Connection handle
    uint16_t conhdl;
    ///Notification Configuration
    uint16_t ntf_cfg;
    ///Battery Service Instance - From 0 to BASC_NB_BAS_INSTANCES_MAX-1
    uint8_t bas_nb;
};

///Parameters of the @ref BASC_BATT_LEVEL_PRES_FORMAT_RD_RSP message
struct basc_batt_level_pres_format_rd_rsp
{
    ///Connection handle
    uint16_t conhdl;
    ///Battery Service Instance - From 0 to BASC_NB_BAS_INSTANCES_MAX-1
    uint8_t bas_nb;
    ///Characteristic Presentation Format
    struct prf_char_pres_fmt char_pres_format;
};

///Parameters of the @ref BASC_BATT_LEVEL_IND message
struct basc_batt_level_ind
{
    ///Connection handle
    uint16_t conhdl;
    ///Indication Type
    uint8_t ind_type;
    ///Battery Level
    uint8_t batt_level;
    ///Battery Service Instance - From 0 to BASC_NB_BAS_INSTANCES_MAX-1
    uint8_t bas_nb;
};

/*
 * TASK DESCRIPTOR DECLARATIONS
 ****************************************************************************************
 */

extern const struct ke_state_handler basc_state_handler[BASC_STATE_MAX];
extern const struct ke_state_handler basc_default_handler;
extern ke_state_t basc_state[BASC_IDX_MAX];

#endif // (BLE_BAS_CLIENT)

/// @} BASCTASK

#endif /* _BASC_TASK_H_ */
