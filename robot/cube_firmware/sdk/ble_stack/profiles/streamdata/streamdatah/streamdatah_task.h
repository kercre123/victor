/**
 ****************************************************************************************
 *
 * @file streamdatah_task.h
 *
 * @brief Header file - STREAMDATAHTASK.
 *
 * Copyright (C) 2012. Dialog Semiconductor Ltd, unpublished work. This computer 
 * program includes Confidential, Proprietary Information and is a Trade Secret of 
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited 
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

#ifndef STREAMDATAH_TASK_H_
#define STREAMDATAH_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup STREAMDATAHTASK StreamData Host Task
 * @ingroup STREAMDATAH
 * @brief StreamData Host Task
 *
 * The StreamData Host Task is responsible for handling the API messages received from
 * either the Application or internal tasks.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwble_config.h"

#if (BLE_STREAMDATA_HOST)

#include "ke_task.h"
#include "co_error.h"
#include "streamdatah.h"

/*
 * DEFINES
 ****************************************************************************************
 */

///Maximum number of StreamData Host task instances
#define STREAMDATAH_IDX_MAX    (BLE_CONNECTION_MAX)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Possible states of the STREAMDATAH task
enum
{
    /// IDLE state
    STREAMDATAH_IDLE,
    ///Connected state
    STREAMDATAH_CONNECTED,
    ///Discovering
    STREAMDATAH_DISCOVERING,

    /// Number of defined states.
    STREAMDATAH_STATE_MAX
};

///StreamData Host API messages
enum
{
    ///StreamData Host role enable request from application.
    STREAMDATAH_ENABLE_REQ = KE_FIRST_MSG(TASK_STREAMDATAH),
    /// StreamData Host role enable confirmation to application.
    STREAMDATAH_ENABLE_CFM,
	
	STREAMDATAH_RD_STREAM_ENABLE_REQ,
	STREAMDATAH_RCV_DATA_PACKET_IND,
	
	STREAMDATAH_STREAMON_REQ,  // Turn the incoming stream on
	STREAMDATAH_STREAMON_RSP,
	STREAMDATAH_STREAMOFF_REQ, // Turn the incoming stream off
	STREAMDATAH_STREAMOFF_RSP,
	
    /// Inform APP that the profile client role has been disabled after a disconnection
    STREAMDATAH_DISABLE_IND,
	
    ///Send read Characteristic Response to APP
    STREAMDATAH_RD_CHAR_RSP,
    ///Send write Characteristic Response to APP
    STREAMDATAH_WR_CHAR_RSP,
};

/*
 * API Messages Structures
 ****************************************************************************************
 */

/// StreamData Host enable command structure
struct streamdatah_enable_req
{
    /// Connection handle
    uint16_t conhdl;
    /// Connection type
    uint8_t con_type;

    /// StreamData Device details kept in APP
    struct streamdatah_streamdata_content streamdatah;
};

/// StreamData Host enable confirm structure
struct streamdatah_enable_cfm
{
    /// Connection handle
    uint16_t conhdl;
    /// Status
    uint8_t status;

    /// StreamData Device details to keep in APP
    struct streamdatah_streamdata_content streamdatah;
};

///Parameters of the @ref STREAMDATAH_RD_ALERT_LVL_REQ message
struct streamdatah_rd_alert_lvl_req
{
    /// Connection handle
    uint16_t conhdl;
};

///Parameters of the @ref STREAMDATAH_WR_ALERT_LVL_REQ message
struct streamdatah_wr_alert_lvl_req
{
    /// Connection handle
    uint16_t conhdl;
    /// 0=LLS or 1=IAS, code for the service in which the alert level should be written
    uint8_t  svc_code;
    /// Alert level
    uint8_t  lvl;
};

///Parameters of the @ref STREAMDATAH_RD_CHAR_RSP message
struct streamdatah_rd_char_rsp
{
    /// Connection handle
    uint16_t conhdl;
    /// Read characteristic response status code, may be GATT code or ATT error code.
    uint8_t  status;
    /// Char. Code
    uint8_t char_code;
    /// Value
    uint8_t val;
};

///Parameters of the @ref STREAMDATAH_WR_CHAR_RSP message
struct streamdatah_wr_char_rsp
{
    /// Connection handle
    uint16_t conhdl;
    /// Write characteristic response status code, may be GATT code or ATT error code.
    uint8_t  status;
};

///Parameters of the @ref STREAMDATAH_RD_TXPW_LVL_REQ message
struct streamdatah_rd_txpw_lvl_req
{
    /// Connection handle
    uint16_t conhdl;
};

struct streamdatah_rd_streamon_enable_req
{
    /// Connection handle
    uint16_t conhdl;
};

struct streamdatah_streamon_req
{
    /// Connection handle
    uint16_t conhdl;
};

struct streamdatah_streamoff_req
{
    /// Connection handle
    uint16_t conhdl;
};

struct streamdatah_rcv_data_packet_ind
{
    /// Connection handle
    uint16_t conhdl;
	
	/// sequence number to check for dropped packets
	uint8_t seq;
    /// size of data
    uint8_t size;
    /// value
    uint8_t value[20];
};

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

extern const struct ke_state_handler streamdatah_state_handler[STREAMDATAH_STATE_MAX];
extern const struct ke_state_handler streamdatah_default_handler;
extern ke_state_t streamdatah_state[STREAMDATAH_IDX_MAX];

#endif //BLE_STREAMDATA_HOST

/// @} STREAMDATAHTASK

#endif // STREAMDATAH_TASK_H_
