/**
 ****************************************************************************************
 *
 * @file udss_task.h
 *
 * @brief Header file - UDSSTASK.
 *
 * Copyright (C) 2015. Dialog Semiconductor Ltd, unpublished work. This computer
 * program includes Confidential, Proprietary Information and is a Trade Secret of
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

#ifndef UDSS_TASK_H_
#define UDSS_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup UDSSTASK Task
 * @ingroup UDSS
 * @brief User Data Service Server Task
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#if (BLE_UDS_SERVER)

#include "ke_task.h"
#include "udss.h"

/*
 * DEFINES
 ****************************************************************************************
 */

#define UDSS_IDX_MAX        (1)

/// Possible states of the UDSS task
enum
{
    ///Disabled state
    UDSS_DISABLED,
    ///Idle state
    UDSS_IDLE,
    ///Connected state
    UDSS_CONNECTED,

    ///Number of defined states.
    UDSS_STATE_MAX
};

/// Messages for User Data Service Server
enum
{
    ///Add a UDS instance into the database
    UDSS_CREATE_DB_REQ = KE_FIRST_MSG(TASK_UDSS),
    ///Inform APP of database creation status
    UDSS_CREATE_DB_CFM,


    ///Start the User Data Service Task - at connection
    UDSS_ENABLE_REQ,
    /// Inform the application that the profile service role task has been disabled after a disconnection
    UDSS_DISABLE_IND,

    ///Set the value of an attribute
    UDSS_SET_CHAR_VAL_REQ,	
    /// Inform the application about the current user's char values
    UDSS_CHAR_VAL_IND,
    
    /// User Control Point Request
    UDSS_UCP_REQ_IND,
    /// User Control Point Response
    UDSS_UCP_RSP_REQ,
    ///Inform that requested action has been performed
    UDSS_REQ_CMP_EVT,
    
    ///Error indication to Host
    UDSS_ERROR_IND,
};

/*
 * API MESSAGES STRUCTURES
 ****************************************************************************************
 */

/// Parameters of the @ref UDSS_CREATE_DB_REQ message
struct udss_create_db_req
{
    ///Database configuration
    uint32_t features;
};

/// Parameters of the @ref UDSS_CREATE_DB_CFM message
struct udss_create_db_cfm
{
    ///Status
    uint8_t status;
};

/// Parameters of the @ref UDSS_SET_CHAR_VAL_REQ message - shall be dynamically allocated
struct udss_set_char_val_req
{
    /// Characteristic Code
    uint8_t char_code;
    /// Value length
    uint8_t val_len;
    /// Value
    uint8_t val[__ARRAY_EMPTY];
};

/// Parameters of the @ref UDSS_CHAR_VAL_IND message - shall be dynamically allocated
struct udss_char_val_ind
{
    /// Characteristic Code
    uint8_t char_code;
    /// Value length
    uint8_t val_len;
    /// Value
    uint8_t val[__ARRAY_EMPTY];
};

/// Parameters of the @ref UDSS_ENABLE_REQ message
struct udss_enable_req
{
    ///Connection handle
    uint16_t conhdl;
    /// Security level: b0= nothing, b1=unauthenticated, b2=authenticated, b3=authorized; b1 or b2 and b3 can go together
    uint8_t sec_lvl;
    ///Type of connection
    uint8_t con_type;
    /// DB Change notification configuration
    uint16_t db_change_ntf_en;
    /// Control Point indication configuration
    uint16_t cpt_ind_en;
};

/// Parameters of the @ref UDSS_DISABLE_IND message
struct udss_disable_ind
{
    ///Connection handle
    uint16_t conhdl;
};

/// Parameters of the @ref UDSS_UCP_REQ_IND message
struct udss_ucp_req_ind
{
    ///Connection handle
    uint16_t conhdl;
    ///User control point (UCP) request structure
    struct uds_ucp_req ucp_req;
};

///Parameters of the @ref UDSS_UCP_RSP_REQ message
struct udss_ucp_rsp_req
{
    ///Connection handle
    uint16_t conhdl;
    ///User control point (ucp) response structure
    struct uds_ucp_rsp ucp_rsp;
};

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Read characteristic indication callback.
 * This function is called on characteristic read and allows for custom response
 * @param[in] handle Handle for the read characteristic
 * @return Which value (if any) will be sent in read request response
 *  - @ref READ_CLB_RESPOND_WITH_DB_VALUE if value should be taken from the database,
 *  - @ref READ_CLB_DONT_RESPOND if no automatic response should be sent and
 *               ReadClb_response() or other response will be sent manually
 ****************************************************************************************
 */
char uds_read_char_cmd_ind(uint16_t handle);

/*
 * TASK DESCRIPTOR DECLARATIONS
 ****************************************************************************************
 */
extern const struct ke_state_handler udss_state_handler[UDSS_STATE_MAX];
extern const struct ke_state_handler udss_default_handler;
extern ke_state_t udss_state[UDSS_IDX_MAX];

#endif //BLE_UDS_SERVER

/// @} UDSSTASK
#endif // UDSS_TASK_H_
