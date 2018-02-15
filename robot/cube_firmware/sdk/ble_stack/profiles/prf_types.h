/**
 ****************************************************************************************
 *
 * @file prf_types.h
 *
 * @brief Header file - Profile Types
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev$
 *
 ****************************************************************************************
 */


#ifndef _PRF_TYPES_H_
#define _PRF_TYPES_H_

/**
 ****************************************************************************************
 * @addtogroup PROFILE PROFILES
 * @ingroup ROOT
 * @brief Bluetooth Low Energy Host Profiles
 *
 * The PROFILE of the stack contains the profile layers (@ref PROX "PROXIMITY",
 * @ref HTP "HTP",@ref FIND "FIND ME" @ref BPS "Blood Pressure").
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup PRF_TYPES
 * @ingroup PROFILE
 * @brief Definitions of shared profiles types
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */


#include "ke_msg.h"
#include "attm.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// Attribute is mandatory
#define ATT_MANDATORY   (0xFF)
/// Attribute is optional
#define ATT_OPTIONAL    (0x00)

/// Characteristic Presentation Format Descriptor Size
#define PRF_CHAR_PRES_FMT_SIZE  (7)

/// Profiles specific error codes
enum prf_err_code
{
    /// No error
    PRF_ERR_OK                             = 0x00,
    /// Application Error
    PRF_APP_ERROR                          = 0x80,
    /// Invalid parameter in request
    PRF_ERR_INVALID_PARAM,
    /// Inexistent handle for sending a read/write characteristic request
    PRF_ERR_INEXISTENT_HDL,
    /// Discovery stopped due to missing attribute according to specification
    PRF_ERR_STOP_DISC_CHAR_MISSING,
    /// Too many SVC instances found -> protocol violation
    PRF_ERR_MULTIPLE_SVC,
    /// Discovery stopped due to found attribute with incorrect properties
    PRF_ERR_STOP_DISC_WRONG_CHAR_PROP,
    /// Too many Char. instances found-> protocol violation
    PRF_ERR_MULTIPLE_CHAR,
    /// Attribute write not allowed
    PRF_ERR_NOT_WRITABLE,
    /// Attribute read not allowed
    PRF_ERR_NOT_READABLE,
    /// Request not allowed
    PRF_ERR_REQ_DISALLOWED,
    /// Notification Not Enabled
    PRF_ERR_NTF_DISABLED,
    /// Indication Not Enabled
    PRF_ERR_IND_DISABLED,
    /// Feature not supported by profile
    PRF_ERR_FEATURE_NOT_SUPPORTED,
    /// Read value has an unexpected length
    PRF_ERR_UNEXPECTED_LEN,
    /// Disconnection occurs
    PRF_ERR_DISCONNECTED,
    /// Procedure Timeout
    PRF_ERR_PROC_TIMEOUT
};

/// Possible values for setting client configuration characteristics
enum prf_cli_conf
{
    /// Stop notification/indication
    PRF_CLI_STOP_NTFIND = 0x0000,
    /// Start notification
    PRF_CLI_START_NTF,
    /// Start indication
    PRF_CLI_START_IND,
};

/// Possible values for setting server configuration characteristics
enum prf_srv_conf
{
    /// Stop Broadcast
    PRF_SRV_STOP_BCST = 0x0000,
    /// Start Broadcast
    PRF_SRV_START_BCST,
};

/// Connection type
enum prf_con_type
{
    ///Discovery type connection
    PRF_CON_DISCOVERY = 0x00,
    /// Normal type connection
    PRF_CON_NORMAL    = 0x01,
};

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/**
 * Characteristic Presentation Format Descriptor structure
 * Packed size is PRF_CHAR_PRES_FMT_SIZE
 */
/// characteristic presentation information
struct prf_char_pres_fmt
{
    /// Unit (The Unit is a UUID)
    uint16_t unit;
    /// Description
    uint16_t description;
    /// Format
    uint8_t format;
    /// Exponent
    uint8_t exponent;
    /// Name space
    uint8_t name_space;
};

/**
 * date and time structure
 * size = 7 bytes
 */
/// Time profile information
struct prf_date_time
{
    /// year time element
    uint16_t year;
    /// month time element
    uint8_t month;
    /// day time element
    uint8_t day;
    /// hour time element
    uint8_t hour;
    /// minute time element
    uint8_t min;
    /// second time element
    uint8_t sec;
};

/**
 *  SFLOAT: Short Floating Point Type
 *
 *        +----------+----------+---------+
 *        | Exponent | Mantissa |  Total  |
 * +------+----------+----------+---------+
 * | size |  4 bits  | 12 bits  | 16 bits |
 * +------+----------+----------+---------+
 */
typedef uint16_t prf_sfloat;

/// Profile Connection Info
struct prf_con_info
{
    /// Application Task Id
    ke_task_id_t appid;
    /// Profile Task Id
    ke_task_id_t prf_id;
    /// Connection index
    uint8_t conidx;
};

/// Profile Environment Data
typedef struct prf_data
{
    /// Profile connection env
    struct prf_con_info con_info;
} prf_env_struct;

/// Attribute information
struct prf_att_info
{
    /// Connection handle
    uint16_t conhdl;
    /// Attribute Handle
    uint16_t handle;
    /// Attribute length
    uint16_t length;
    /// Status of request
    uint8_t  status;
    /// Attribute value
    uint8_t value[__ARRAY_EMPTY];
};

/**
 * define default cleanup function used by a profile to clean-up its parameters.
 */
typedef void (*prf_cleanup_cb) (prf_env_struct* env);


/// service handles
struct prf_svc
{
    /// start handle
    uint16_t shdl;
    /// end handle
    uint16_t ehdl;
};

/// service handles
struct prf_incl_svc
{
    /// attribute handle
    uint16_t handle;
    /// included service start handle
    uint16_t start_hdl;
    /// included service  end handle
    uint16_t end_hdl;
    /// UUID length
    uint8_t uuid_len;
    /// UUID
    uint8_t uuid[ATT_UUID_128_LEN];
};

/// characteristic info
struct prf_char_inf
{
    /// Characteristic handle
    uint16_t char_hdl;
    /// Value handle
    uint16_t val_hdl;
    /// Characteristic properties
    uint8_t prop;
    /// End of characteristic offset
    uint8_t char_ehdl_off;
};

/// characteristic description
struct prf_char_desc_inf
{
    /// Descriptor handle
    uint16_t desc_hdl;
};


/// Characteristic definition
struct prf_char_def
{
    /// Characteristic UUID
    uint16_t uuid;
    /// Requirement Attribute Flag
    uint8_t req_flag;
    /// Mandatory Properties
    uint8_t prop_mand;
};

/// Characteristic Descriptor definition
struct prf_char_desc_def
{
    /// Characteristic Descriptor uuid
    uint16_t uuid;
    /// requirement attribute flag
    uint8_t req_flag;
    /// Corresponding characteristic code
    uint8_t char_code;
};

/// Message structure used to inform APP that a profile client role has been disabled
struct prf_client_disable_ind
{
    /// Connection Handle
    uint16_t conhdl;
    /// Status
    uint8_t status;
};

/// Message structure used to inform APP that an error has occured in the profile server role task
struct prf_server_error_ind
{
    /// Connection Handle
    uint16_t conhdl;
    /// Message ID
    uint16_t msg_id;
    /// Status
    uint8_t status;
};


/// @} PRF_TYPES

#endif /* _PRF_TYPES_H_ */
