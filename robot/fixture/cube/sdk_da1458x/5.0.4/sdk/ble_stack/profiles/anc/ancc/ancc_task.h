/**
 ****************************************************************************************
 *
 * @file ancc_task.h
 *
 * @brief Header file - ANCS Client Role Task.
 *
 * Copyright (C) 2014. Dialog Semiconductor Ltd, unpublished work. This computer 
 * program includes Confidential, Proprietary Information and is a Trade Secret of 
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited 
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

#ifndef _ANCC_TASK_H_
#define _ANCC_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup ANCCTASK ANCS Client Task
 * @ingroup ANCC
 * @brief ANCS Client Task
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "anc_common.h"

#if (BLE_ANC_CLIENT)

#include "ke_task.h"
#include "gattm_task.h"
#include "ancc.h"
#include "prf_types.h"
#include "prf_utils.h"
#include "prf_utils_128.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// Maximum number of ANCC task instances
#define ANCC_IDX_MAX        (BLE_CONNECTION_MAX)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Possible states of the ANCC task
enum
{
    /// Idle state
    ANCC_IDLE,
    /// Connected state
    ANCC_CONNECTED,
    /// Busy state
    ANCC_BUSY,

    /// Number of defined states.
    ANCC_STATE_MAX
};

/// Message IDs
enum
{
    /// Enable the ANCS Client task - at connection
    ANCC_ENABLE_REQ = KE_FIRST_MSG(TASK_ANCC),

    /// Disable Indication
    ANCC_DISABLE_IND,

    /// Read the value of a Client Characteristic Configuration Descriptor in the peer device database
    ANCC_READ_CMD,
    
    /// Write the value of a Client Characteristic Configuration Descriptor in the peer device database
    ANCC_WRITE_CMD,

    /// Indicate the content of the peer device ANCS
    ANCC_ANC_CONTENT_IND,

    /// Indicate that a Client Characteristic Configuration Descriptor value has been read
    ANCC_CL_CFG_VAL_IND,
    
    /// Indicate that a notification of the Notification Source characteristic has been received
    ANCC_NTF_SRC_IND,
    
    /// Write a "Get Notification Attributes" command in Control Point 
    ANCC_GET_NTF_ATTS_CMD,
    
    /// Indicate that the value of a notification attribute is available
    ANCC_NTF_ATT_IND,
    
    /// Write a "Get App Attributes" command in Control Point 
    ANCC_GET_APP_ATTS_CMD,

    /// Indicate that the value of a notification attribute is available
    ANCC_APP_ATT_IND,

    /// Procedure complete information
    ANCC_CMP_EVT,
};

/// Operation Codes
enum 
{
    /// Reserved operation code
    ANCC_RESERVED_OP_CODE  = 0x00,

    /// Discovery Procedure
    ANCC_ENABLE_OP_CODE,
    
    /// Read Client Characteristic Configuration Descriptor value Procedure
    ANCC_READ_CL_CFG_OP_CODE,
    /// Write Client Characteristic Configuration Descriptor value Procedure
    ANCC_WRITE_CL_CFG_OP_CODE,

    /// Write "Get Notification Attributes" command in Control Point Procedure
    ANCC_GET_NTF_ATTS_CMD_OP_CODE,
    /// Write "Get App Attributes" command in Control Point Procedure
    ANCC_GET_APP_ATTS_CMD_OP_CODE,
};

/// Data Source notification reception states
enum
{
    /**
     * DATA SOURCE NOTIFICATION STATES
     */
    /// Waiting for next notification
    ANCC_DATA_SRC_NTF_IDLE,
    /// Waiting for App Identifier
    ANCC_DATA_SRC_NTF_APP_ID,
    /// Waiting for next attribute
    ANCC_DATA_SRC_NTF_NEXT_ATT,
    /// Waiting for attribute msb
    ANCC_DATA_SRC_NTF_ATT_MSB,
    /// Waiting for attribute length
    ANCC_DATA_SRC_NTF_ATT_LENGTH,
    /// Waiting for attribute value
    ANCC_DATA_SRC_NTF_ATT_VALUE,
};


/*
 * API MESSAGE STRUCTURES
 ****************************************************************************************
 */

/// Parameters of the @ref ANCC_ENABLE_REQ message
struct ancc_enable_req
{
    /// Connection handle
    uint16_t conhdl;
    /// Operation Code
    uint8_t operation;
    /// Connection type
    uint8_t con_type;

    /// Existing handle values ANC
    struct ancc_anc_content anc;
};

/// Parameters of the @ref ANCC_DISABLE_IND message
struct ancc_disable_ind
{
    /// Connection handle
    uint16_t conhdl;
    /// Operation Code
    uint8_t status;
};

/// Parameters of the @ref ANCC_READ_CMD message
struct ancc_read_cmd
{
    /// Connection handle
    uint16_t conhdl;
    /// Operation Code
    uint8_t operation;
    /// Read code
    uint8_t read_code;
};

/// Parameters of the @ref ANCC_WRITE_CMD message
struct ancc_write_cmd
{
    /// Connection handle
    uint16_t conhdl;
    /// Operation Code
    uint8_t operation;
    /// Write code
    uint8_t write_code;
    /// Value
    uint16_t value;
};

/// Parameters of the @ref ANCC_ANC_CONTENT_IND message
struct ancc_anc_content_ind
{
    /// Connection Handle
    uint16_t conhdl;
    /// ANCS Content
    struct ancc_anc_content anc;
};

/// Parameters of the @ref ANCC_CL_CFG_VAL_IND message
struct ancc_cl_cfg_val_ind
{
    /// Connection handle
    uint16_t conhdl;
    /// Attribute Code
    uint8_t att_code;
    /// Value
    uint16_t value;
};

/// Parameters of the @ref ANCC_NTF_SRC_IND message
struct ancc_ntf_src_ind
{
    /// Connection handle
    uint16_t conhdl;
    /// Value
    struct anc_ntf_src value;
};

/// Command Message Basic Structure
struct ancc_tmp_cmd
{
    /// Connection Handle
    uint16_t conhdl;
    /// Operation Code
    uint8_t operation;
};

/// Parameters of the @ref ANCC_GET_NTF_ATTS_CMD message
struct ancc_get_ntf_atts_cmd
{
    /// Connection handle
    uint16_t conhdl;
    /// Operation Code
    uint8_t operation;
    
    /// Notification UID
    uint32_t ntf_uid;
    /// Attribute bit mask
    uint8_t atts;
    /// Title attribute maximum length
    uint16_t title_len;
    /// Subtitle attribute maximum length
    uint16_t subtitle_len;
    /// Message attribute maximum length
    uint16_t message_len;
};

/// Parameters of the @ref ANCC_NTF_ATT_IND message
struct ancc_ntf_att_ind
{
    /// Connection handle
    uint16_t conhdl;
    
    /// Attribute Code
    uint32_t ntf_uid;
    
    /// Attribute ID
    uint8_t att_id;
    
    /// Attribute length
    uint16_t length;
    
    /// Value
    uint8 value[];
};

/// Parameters of the @ref ANCC_GET_APP_ATTS_CMD message
struct ancc_get_app_atts_cmd
{
    /// Connection handle
    uint16_t conhdl;
    /// Operation Code
    uint8_t operation;
    
    // requested attribute bit mask
    uint8_t atts;
    
    // App Identifier length (not including NULL termination)
    uint8_t app_id_length;

    // App Identifier
    uint8_t app_id[];

};

/// Parameters of the @ref ANCC_APP_ATT_IND message
struct ancc_app_att_ind
{
    /// Connection handle
    uint16_t conhdl;
    
    /// app id
    uint8_t app_id_length;
    
    /// app AttributeID
    uint8_t att_id;
    
    /// Attribute length
    uint16_t att_length;
    
    /// Value
    uint8 values[];
};

/// Parameters of the @ref ANCC_CMP_EVT message
struct ancc_cmp_evt
{
    /// Connection handle
    uint16_t conhdl;
    /// Operation code
    uint8_t operation;
    /// Status
    uint8_t status;
};

/*
 * TASK DESCRIPTOR DECLARATIONS
 ****************************************************************************************
 */

extern const struct ke_state_handler ancc_state_handler[ANCC_STATE_MAX];
extern const struct ke_state_handler ancc_default_handler;
extern ke_state_t ancc_state[ANCC_IDX_MAX];

#endif //(BLE_ANC_CLIENT)

/// @} ANCCTASK

#endif //(_ANCC_TASK_H_)
