/**
 ****************************************************************************************
 *
 * @file gap.h
 *
 * @brief Header file - GAP.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */
#ifndef GAP_H_
#define GAP_H_
/**
 ****************************************************************************************
 * @addtogroup HOST
 * @ingroup ROOT
 * @brief Bluetooth Low Energy Host
 *
 * The HOST layer of the stack contains the higher layer protocols (@ref ATT "ATT",
 * @ref SMP "SMP") and transport module (@ref L2C "L2C"). It also includes the Generic
 * Access Profile (@ref GAP "GAP"), used for scanning/connection operations.
 ****************************************************************************************
 */
/**
 ****************************************************************************************
 * @addtogroup GAP Generic Access Profile
 * @ingroup HOST
 * @brief Generic Access Profile.
 *
 * The GAP module is responsible for providing an API to the application in order to
 * configure the device in the desired mode (discoverable, connectable, etc.) and perform
 * required actions (scanning, connection, pairing, etc.). To achieve this, the GAP
 * interfaces with both the @ref SMP "SMP", @ref L2C "L2C" and the @ref CONTROLLER "CONTROLLER"
 *
 * @{
 ****************************************************************************************
 */
#include "rwip_config.h"
#include "co_bt.h"
#include "gap_cfg.h"
#include "smpm.h"

/*
 * DEFINES
 ****************************************************************************************
 */
/// GAP Advertising Flags
enum
{
    /// Flag
    GAP_AD_TYPE_FLAGS                      = 0x01,
    /// Use of more than 16 bits UUID
    GAP_AD_TYPE_MORE_16_BIT_UUID           = 0x02,
    /// Complete list of 16 bit UUID
    GAP_AD_TYPE_COMPLETE_LIST_16_BIT_UUID  = 0x03,
    /// Use of more than 32 bit UUD
    GAP_AD_TYPE_MORE_32_BIT_UUID           = 0x04,
    /// Complete list of 32 bit UUID
    GAP_AD_TYPE_COMPLETE_LIST_32_BIT_UUID  = 0x05,
    /// Use of more than 128 bit UUID
    GAP_AD_TYPE_MORE_128_BIT_UUID          = 0x06,
    /// Complete list of 128 bit UUID
    GAP_AD_TYPE_COMPLETE_LIST_128_BIT_UUID = 0x07,
    /// Shortened device name
    GAP_AD_TYPE_SHORTENED_NAME             = 0x08,
    /// Complete device name
    GAP_AD_TYPE_COMPLETE_NAME              = 0x09,
    /// Transmit power
    GAP_AD_TYPE_TRANSMIT_POWER             = 0x0A,
    /// Class of device
    GAP_AD_TYPE_CLASS_OF_DEVICE            = 0x0D,
    /// Simple Pairing Hash C
    GAP_AD_TYPE_SP_HASH_C                  = 0x0E,
    /// Simple Pairing Randomizer
    GAP_AD_TYPE_SP_RANDOMIZER_R            = 0x0F,
    /// Temporary key value
    GAP_AD_TYPE_TK_VALUE                   = 0x10,
    /// Out of Band Flag
    GAP_AD_TYPE_OOB_FLAGS                  = 0x11,
    /// Slave connection interval range
    GAP_AD_TYPE_SLAVE_CONN_INT_RANGE       = 0x12,
    /// Require 16 bit service UUID
    GAP_AD_TYPE_RQRD_16_BIT_SVC_UUID       = 0x14,
    /// Require 32 bit service UUID
    GAP_AD_TYPE_RQRD_32_BIT_SVC_UUID       = 0x1F,
    /// Require 128 bit service UUID
    GAP_AD_TYPE_RQRD_128_BIT_SVC_UUID      = 0x15,
    /// Service data 16-bit UUID
    GAP_AD_TYPE_SERVICE_16_BIT_DATA        = 0x16,
    /// Service data 32-bit UUID
    GAP_AD_TYPE_SERVICE_32_BIT_DATA        = 0x20,
    /// Service data 128-bit UUID
    GAP_AD_TYPE_SERVICE_128_BIT_DATA       = 0x21,
    /// Public Target Address
    GAP_AD_TYPE_PUB_TGT_ADDR               = 0x17,
    /// Random Target Address
    GAP_AD_TYPE_RAND_TGT_ADDR              = 0x18,
    /// Appearance
    GAP_AD_TYPE_APPEARANCE                 = 0x19,
    /// Advertising Interval
    GAP_AD_TYPE_ADV_INTV                   = 0x1A,
    /// LE Bluetooth Device Address
    GAP_AD_TYPE_LE_BT_ADDR                 = 0x1B,
    /// LE Role
    GAP_AD_TYPE_LE_ROLE                    = 0x1C,
    /// Simple Pairing Hash C-256
    GAP_AD_TYPE_SPAIR_HASH                 = 0x1D,
    /// Simple Pairing Randomizer R-256
    GAP_AD_TYPE_SPAIR_RAND                 = 0x1E,
    /// 3D Information Data
    GAP_AD_TYPE_3D_INFO                    = 0x3D,

    /// Manufacturer specific data
    GAP_AD_TYPE_MANU_SPECIFIC_DATA         = 0xFF,
};


/// Random Address type
enum gap_rnd_addr_type
{
    /// Static random address
    GAP_STATIC_ADDR     = SMPM_ADDR_TYPE_STATIC,
    /// Private non resolvable address
    GAP_NON_RSLV_ADDR   = SMPM_ADDR_TYPE_PRIV_NON_RESOLV,
    /// Private resolvable address
    GAP_RSLV_ADDR       = SMPM_ADDR_TYPE_PRIV_RESOLV,
};

/// GAP Specific Error
enum gap_err_code
{
    /// No error
    GAP_ERR_NO_ERROR        = 0x00,
    /// Invalid parameters set
    GAP_ERR_INVALID_PARAM   = 0x40,
    /// Problem with protocol exchange, get unexpected response
    GAP_ERR_PROTOCOL_PROBLEM,
    /// Request not supported by software configuration
    GAP_ERR_NOT_SUPPORTED,
    /// Request not allowed in current state.
    GAP_ERR_COMMAND_DISALLOWED,
    /// Requested operation canceled.
    GAP_ERR_CANCELED,
    /// Requested operation timeout.
    GAP_ERR_TIMEOUT,
    /// Link connection lost during operation.
    GAP_ERR_DISCONNECTED,
    /// Search algorithm finished, but no result found
    GAP_ERR_NOT_FOUND,
    /// Request rejected by peer device
    GAP_ERR_REJECTED,
    /// Problem with privacy configuration
    GAP_ERR_PRIVACY_CFG_PB,
    /// Duplicate or invalid advertising data
    GAP_ERR_ADV_DATA_INVALID,
};

/// Boolean value set
enum
{
    /// Disable
    GAP_DISABLE = 0x00,
    /// Enable
    GAP_ENABLE
};


#if (BLE_ATTS)
/// GAP Attribute database handles
/// Generic Access Profile Service
enum
{
    GAP_IDX_PRIM_SVC,
    GAP_IDX_CHAR_DEVNAME,
    GAP_IDX_DEVNAME,
    GAP_IDX_CHAR_ICON,
    GAP_IDX_ICON,
    #if (BLE_PERIPHERAL)
    GAP_IDX_CHAR_PRIVY_FLAG,
    GAP_IDX_PRIVY_FLAG,
    GAP_IDX_CHAR_SLAVE_PREF_PARAM,
    GAP_IDX_SLAVE_PREF_PARAM,
    GAP_IDX_CHAR_RECON_ADDR,
    GAP_IDX_RECON_ADDR,
    #endif /* #if (BLE_PERIPHERAL) */
    GAP_IDX_NUMBER,
};
#endif /* (BLE_ATTS)*/

/****************** GAP Role **********************/
enum gap_role
{
    /// No role set yet
    GAP_NO_ROLE    = 0x00,

    /// Observer role
    GAP_OBSERVER_SCA    = 0x01,

    /// Broadcaster role
    GAP_BROADCASTER_ADV = 0x02,

    /// Master/Central role
    GAP_CENTRAL_MST     = (0x04 | GAP_OBSERVER_SCA),

    /// Peripheral/Slave role
    GAP_PERIPHERAL_SLV  = (0x08 | GAP_BROADCASTER_ADV),
};

/// Advertising mode
enum gap_adv_mode
{
    /// Mode in non-discoverable
    GAP_NON_DISCOVERABLE,
    /// Mode in general discoverable
    GAP_GEN_DISCOVERABLE,
    /// Mode in limited discoverable
    GAP_LIM_DISCOVERABLE,
    /// Broadcaster mode which is a non discoverable and non connectable mode.
    GAP_BROADCASTER_MODE,
};

/// Scan mode
enum gap_scan_mode
{
    /// Mode in general discovery
    GAP_GEN_DISCOVERY,
    /// Mode in limited discovery
    GAP_LIM_DISCOVERY,
    /// Observer mode
    GAP_OBSERVER_MODE,
    /// Invalid mode
    GAP_INVALID_MODE,
};



/// IO Capability Values
enum gap_io_cap
{
    /// Display Only
    GAP_IO_CAP_DISPLAY_ONLY = 0x00,
    /// Display Yes No
    GAP_IO_CAP_DISPLAY_YES_NO,
    /// Keyboard Only
    GAP_IO_CAP_KB_ONLY,
    /// No Input No Output
    GAP_IO_CAP_NO_INPUT_NO_OUTPUT,
    /// Keyboard Display
    GAP_IO_CAP_KB_DISPLAY,
    GAP_IO_CAP_LAST,
};

/// TK Type
enum gap_tk_type
{
    ///  TK get from out of band method
    GAP_TK_OOB         = 0x00,
    /// TK generated and shall be displayed by local device
    GAP_TK_DISPLAY,
    /// TK shall be entered by user using device keyboard
    GAP_TK_KEY_ENTRY,
};

/// OOB Data Present Flag Values
enum gap_oob
{
    /// OOB Data not present
    GAP_OOB_AUTH_DATA_NOT_PRESENT = 0x00,
    /// OOB data present
    GAP_OOB_AUTH_DATA_PRESENT,
    GAP_OOB_AUTH_DATA_LAST,
};

/// Authentication mask
enum gap_auth_mask
{
    /// No Flag set
    GAP_AUTH_NONE = 0,
    /// Bond authentication
    GAP_AUTH_BOND = (1 << 0),
    /// Man In the middle protection
    GAP_AUTH_MITM = (1 << 2),
};


/// Authentication Requirements
enum gap_auth
{
    /// No MITM No Bonding
    GAP_AUTH_REQ_NO_MITM_NO_BOND = (GAP_AUTH_NONE),
    /// No MITM Bonding
    GAP_AUTH_REQ_NO_MITM_BOND    = (GAP_AUTH_BOND),
    /// MITM No Bonding
    GAP_AUTH_REQ_MITM_NO_BOND    = (GAP_AUTH_MITM),
    /// MITM and Bonding
    GAP_AUTH_REQ_MITM_BOND       = (GAP_AUTH_MITM | GAP_AUTH_BOND),
    GAP_AUTH_REQ_LAST
};

/// Key Distribution Flags
enum gap_kdist
{
    /// No Keys to distribute
    GAP_KDIST_NONE = 0x00,
    /// Encryption key in distribution
    GAP_KDIST_ENCKEY = (1 << 0),
    /// IRK (ID key)in distribution
    GAP_KDIST_IDKEY  = (1 << 1),
    /// CSRK(Signature key) in distribution
    GAP_KDIST_SIGNKEY= (1 << 2),

    GAP_KDIST_LAST =   (1 << 3),
};

/// Security Defines
enum gap_sec_req
{
    /// No security (no authentication and encryption)
    GAP_NO_SEC = 0x00,
    /// Unauthenticated pairing with encryption
    GAP_SEC1_NOAUTH_PAIR_ENC,
    /// Authenticated pairing with encryption
    GAP_SEC1_AUTH_PAIR_ENC,
    /// Unauthenticated pairing with data signing
    GAP_SEC2_NOAUTH_DATA_SGN,
    /// Authentication pairing with data signing
    GAP_SEC2_AUTH_DATA_SGN,
    /// Unrecognized security
    GAP_SEC_UNDEFINED
};

/// Authorization setting
enum gap_authz
{
    /// Authorization not set, application informed when authorization requested
    GAP_AUTHZ_NOT_SET = 0x00,
    /// Authorization request automatically accepted
    GAP_AUTHZ_ACCEPT  = 0x01,
    /// Authorization request automatically rejected
    GAP_AUTHZ_REJECT  = 0x02,
};

///***** AD Type Flag - Bit set *******/
/// Limited discovery flag - AD Flag
#define GAP_LE_LIM_DISCOVERABLE_FLG             0x01
/// General discovery flag - AD Flag
#define GAP_LE_GEN_DISCOVERABLE_FLG             0x02
/// Legacy BT not supported - AD Flag
#define GAP_BR_EDR_NOT_SUPPORTED                0x04
/// Dual mode for controller supported (BR/EDR/LE) - AD Flag
#define GAP_SIMUL_BR_EDR_LE_CONTROLLER          0x08
/// Dual mode for host supported (BR/EDR/LE) - AD Flag
#define GAP_SIMUL_BR_EDR_LE_HOST                0x10

/*********** GAP Miscellaneous Defines *************/
/// Invalid connection index
#define GAP_INVALID_CONIDX                      0xFF

/// Invalid connection handle
#define GAP_INVALID_CONHDL                      0xFFFF


/*************** GAP Structures ********************/


/// Address information about a device address
struct gap_bdaddr
{
    /// BD Address of device
    struct bd_addr addr;
    /// Address type of the device 0=public/1=private random
    uint8_t addr_type;
};


/// Generic Security key structure
struct gap_sec_key
{
    /// Key value MSB -> LSB
    uint8_t key[KEY_LEN];
};


/// @} GAP
#endif // GAP_H_
