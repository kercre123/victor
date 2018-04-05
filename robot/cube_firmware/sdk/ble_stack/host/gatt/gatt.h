/**
 ****************************************************************************************
 *
 * @file gatt.h
 *
 * @brief Header file - GATT.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

#ifndef GATT_H_
#define GATT_H_

/**
 ****************************************************************************************
 * @addtogroup GATT Generic Attribute Profile
 * @ingroup HOST
 * @brief Generic Attribute Profile.
 *
 * The GATT module is responsible for providing an API for all attribute related operations.
 * It is responsible for all the service framework activities using the Attribute protocol
 * for discovering services and for reading and writing characteristic values on a peer device.
 * To achieve this, the GATT interfaces with @ref ATTM "ATTM", @ref ATTC "ATTC" and the
 * @ref ATTS "ATTS".
 *
 * @{
 ****************************************************************************************
 */
/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"


/*
 * DEFINE DECLARATIONS
 ****************************************************************************************
 */

/// size for 16-bit UUID
#define GATT_UUID_16_LEN                       0x02

/// size for 32-bit UUID
#define GATT_UUID_32_LEN                       0x04

/// size for 128-bit UUID
#define GATT_UUID_128_LEN                      0x10

/// size of Attribute handle
#define GATT_ATTR_HDL_LEN                           0x02

/// size of Attribute property len
#define GATT_PROP_LEN                               0x01

/// Invalid attribute handle
#define GATT_INVALID_ATTR_HDL                       0x0000

/// Maximum possible attribute handle
#define GATT_MAX_ATTR_HDL                           0xFFFF


#if (BLE_ATTS)
/// GATT Attribute database handles
/// Generic Attribute Profile Service
enum gatt_db_att
{
    GATT_IDX_PRIM_SVC,
    GATT_IDX_CHAR_SVC_CHANGED,
    GATT_IDX_SVC_CHANGED,
    GATT_IDX_SVC_CHANGED_CFG,

    GATT_IDX_NUMBER,
};
#endif /* (BLE_ATTS)*/

/// GATT Specific Error
enum gatt_error
{
    /// No Error
    GATT_ERR_NO_ERROR             = 0x00,
    /// Invalid parameters set
    GATT_ERR_INVALID_PARAM        = 0x50,
    /// Problem with ATTC protocol response
    GATT_ERR_INVALID_ATT_LEN,
    /// Insufficient resources for request.
    GATT_ERR_INSUF_RESOURCE,
    /// Error in service search
    GATT_ERR_INVALID_TYPE_IN_SVC_SEARCH,
    /// Invalid write data
    GATT_ERR_WRITE,
    /// Signed write error
    GATT_ERR_SIGNED_WRITE,
    /// Problem with ATT protocol
    GATT_ERR_PROTOCOL_PROBLEM,
    /// Operation request not completed.
    GATT_ERR_TIMEOUT,
    /// No attribute client defined
    GATT_ERR_ATT_CLIENT_MISSING,
    /// No attribute server defined
    GATT_ERR_ATT_SERVER_MISSING,
};

/// GATT Client Characteristic Configuration Codes
enum gatt_ccc_val
{
    /// Stop notification/indication
    GATT_CCC_STOP_NTFIND = 0x0000,
    /// Start notification
    GATT_CCC_START_NTF,
    /// Start indication
    GATT_CCC_START_IND,
};

/*
 * Type Definition
 ****************************************************************************************
 */

/// @} GATT
#endif // GATT_H_
