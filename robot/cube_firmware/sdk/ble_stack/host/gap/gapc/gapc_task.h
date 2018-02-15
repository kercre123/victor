/**
 ****************************************************************************************
 *
 * @file gapc_task.h
 *
 * @brief Generic Access Profile Controller Task Header.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 ****************************************************************************************
 */
#ifndef _GAPC_TASK_H_
#define _GAPC_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup GAPC_TASK Generic Access Profile Controller Task
 * @ingroup GAPC
 * @brief Handles ALL messages to/from GAP Controller block.
 *
 * It handles messages from lower and higher layers related to an ongoing connection.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"
#include "ke_task.h"
#include "gap.h"

#if (BLE_CENTRAL || BLE_PERIPHERAL)


/*
 * DEFINES
 ****************************************************************************************
 */

/// number of GAP Controller Process
#define GAPC_IDX_MAX                                 BLE_CONNECTION_MAX

/// states of GAP Controller task
enum gapc_state_id
{
    /// Free state
    GAPC_FREE,
    /// Connection ready state
    GAPC_READY,
    /// Connection busy state (operation on going)
    GAPC_BUSY,
    /// Number of defined states.
    GAPC_STATE_MAX
};

/// GAP Controller Task messages
enum gapc_msg_id
{
    /* Default event */
    /// Command Complete event
    GAPC_CMP_EVT = KE_FIRST_MSG(TASK_GAPC),

    /* Connection state information */
    /// Indicate that a connection has been established
    GAPC_CONNECTION_REQ_IND,
    /// Set specific link data configuration.
    GAPC_CONNECTION_CFM,

    /// Indicate that a link has been disconnected
    GAPC_DISCONNECT_IND,

    /* Link management command */
    /// Request disconnection of current link command.
    GAPC_DISCONNECT_CMD,

    /* Peer device info */
    /// Retrieve information command
    GAPC_GET_INFO_CMD,
    /// Name of peer device indication
    GAPC_PEER_NAME_IND,
    /// Indication of peer version info
    GAPC_PEER_VERSION_IND,
    /// Indication of peer features info
    GAPC_PEER_FEATURES_IND,

    /// Indication of ongoing connection RSSI
    GAPC_CON_RSSI_IND,
    /// Indication of peer privacy info
    GAPC_PRIVACY_IND,
    /// Indication of peer reconnection address info
    GAPC_RECON_ADDR_IND,

    /* Privacy configuration */
    /// Set Privacy flag command.
    GAPC_SET_PRIVACY_CMD,
    /// Set Reconnection Address Value command.
    GAPC_SET_RECON_ADDR_CMD,

    /* Connection parameters update */
    /// Perform update of connection parameters command
    GAPC_PARAM_UPDATE_CMD,
    /// Request of updating connection parameters indication
    GAPC_PARAM_UPDATE_REQ_IND,
    /// Master confirm or not that parameters proposed by slave are accepted or not
    GAPC_PARAM_UPDATE_CFM,
    /// Connection parameters updated indication
    GAPC_PARAM_UPDATED_IND,

    /* Bonding procedure */
    /// Start Bonding command procedure
    GAPC_BOND_CMD,
    /// Bonding requested by peer device indication message.
    GAPC_BOND_REQ_IND,
    /// Confirm requested bond information.
    GAPC_BOND_CFM,
    /// Bonding information indication message
    GAPC_BOND_IND,

    /* Encryption procedure */
    /// Start Encryption command procedure
    GAPC_ENCRYPT_CMD,
    /// Encryption requested by peer device indication message.
    GAPC_ENCRYPT_REQ_IND,
    /// Confirm requested Encryption information.
    GAPC_ENCRYPT_CFM,
    /// Encryption information indication message
    GAPC_ENCRYPT_IND,

    /* Security request procedure */
    /// Start Security Request command procedure
    GAPC_SECURITY_CMD,
    /// Security requested by peer device indication message
    GAPC_SECURITY_IND,

    /* Signature procedure */
    /// Indicate the current sign counters to the application
    GAPC_SIGN_COUNTER_IND,

    /* Device information */
    /// Indication of ongoing connection Channel Map
    GAPC_CON_CHANNEL_MAP_IND,


    /* Internal messages for timer events, not part of API*/
    /// Parameter update procedure timeout indication
    GAPC_PARAM_UPDATE_TO_IND,

};


/// request operation type - application interface
enum gapc_operation
{
    /*                 Operation Flags                  */
    /* No Operation (if nothing has been requested)     */
    /* ************************************************ */
    /// No operation
    GAPC_NO_OP                                    = 0x00,

    /* Connection management */
    /// Disconnect link
    GAPC_DISCONNECT,

    /* Connection information */
    /// Retrieve name of peer device.
    GAPC_GET_PEER_NAME,
    /// Retrieve peer device version info.
    GAPC_GET_PEER_VERSION,
    /// Retrieve peer device features.
    GAPC_GET_PEER_FEATURES,
    /// Retrieve connection RSSI.
    GAPC_GET_CON_RSSI,
    /// Retrieve Privacy Info.
    GAPC_GET_PRIVACY,
    /// Retrieve Reconnection Address Value.
    GAPC_GET_RECON_ADDR,

    /* Privacy Configuration */
    /// Set Privacy flag.
    GAPC_SET_PRIVACY,
    /// Set Reconnection Address Value.
    GAPC_SET_RECON_ADDR,

    /* Connection parameters update */
    /// Perform update of connection parameters.
    GAPC_UPDATE_PARAMS,

    /* Security procedures */
    /// Start bonding procedure.
    GAPC_BOND,
    /// Start encryption procedure.
    GAPC_ENCRYPT,
    /// Start security request procedure
    GAPC_SECURITY_REQ,

    /* Connection information */
    /// Retrieve Connection Channel MAP.
    GAPC_GET_CON_CHANNEL_MAP,


    /// Last GAPC operation flag
    GAPC_LAST
};

/// Bond event type.
enum gapc_bond
{
    /// Bond Pairing request
    GAPC_PAIRING_REQ,
    /// Respond to Pairing request
    GAPC_PAIRING_RSP,

    /// Pairing Finished information
    GAPC_PAIRING_SUCCEED,
    /// Pairing Failed information
    GAPC_PAIRING_FAILED,

    /// Used to retrieve pairing Temporary Key
    GAPC_TK_EXCH,
    /// Used for Identity Resolving Key exchange
    GAPC_IRK_EXCH,
    /// Used for Connection Signature Resolving Key exchange
    GAPC_CSRK_EXCH,
    /// Used for Long Term Key exchange
    GAPC_LTK_EXCH,

    /// Bond Pairing request issue, Repeated attempt
    GAPC_REPEATED_ATTEMPT,
};

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Operation command structure in order to keep requested operation.
struct gapc_operation_cmd
{
    /// GAP request type
    uint8_t operation;
};


/// Command complete event data structure
struct gapc_cmp_evt
{
    /// GAP request type
    uint8_t operation;
    /// Status of the request
    uint8_t status;
};

/// Indicate that a connection has been established
struct gapc_connection_req_ind
{
    /// Connection handle
    uint16_t conhdl;
    /// Connection interval
    uint16_t con_interval;
    /// Connection latency
    uint16_t con_latency;
    /// Link supervision timeout
    uint16_t sup_to;
    /// Clock accuracy
    uint8_t clk_accuracy;
    /// Peer address type
    uint8_t peer_addr_type;
    /// Peer BT address
    struct bd_addr peer_addr;
};


/// Set specific link data configuration.
struct gapc_connection_cfm
{
    /// Local CSRK value
    struct gap_sec_key lcsrk;
    /// Local signature counter value
    uint32_t lsign_counter;

    /// Remote CSRK value
    struct gap_sec_key rcsrk;
    /// Remote signature counter value
    uint32_t rsign_counter;

    /// Authentication (@see gap_auth)
    uint8_t auth;
    /// Peer device is authorized to access database parameters requiring authorizations.
    ///  - GAP_AUTHZ_NOT_SET: Authorization not set
    ///  - GAP_AUTHZ_ACCEPT:  Authorization automatically accepted
    ///  - GAP_AUTHZ_REJECT:  Authorization automatically rejected
    uint8_t authorize;
};


/// Request disconnection of current link command.
struct gapc_disconnect_cmd
{
    /// GAP request type:
    /// - GAPC_DISCONNECT: Disconnect link.
    uint8_t operation;

    /// Reason of disconnection
    uint8_t reason;
};


/// Indicate that a link has been disconnected
struct gapc_disconnect_ind
{
    /// Connection handle
    uint16_t conhdl;
    /// Reason of disconnection
    uint8_t reason;
};


/// Retrieve information command
struct gapc_get_info_cmd
{
    /// GAP request type:
    /// - GAPC_GET_PEER_NAME: Retrieve name of peer device.
    /// - GAPC_GET_PEER_VERSION: Retrieve peer device version info.
    /// - GAPC_GET_PEER_FEATURES: Retrieve peer device features.
    /// - GAPC_GET_CON_RSSI: Retrieve connection RSSI.
    /// - GAPC_GET_PRIVACY: Retrieve Privacy Info.
    /// - GAPC_GET_RECON_ADDR: Retrieve Reconnection Address Value.
    uint8_t operation;
};


/// Name of peer device indication
struct gapc_peer_name_ind
{
    /// peer device name length
    uint8_t name_len;
    /// peer device name
    uint8_t name[__ARRAY_EMPTY];
};

/// Indication of peer version info
struct gapc_peer_version_ind
{
    /// Manufacturer name
    uint16_t compid;
    /// LMP subversion
    uint16_t lmp_subvers;
    /// LMP version
    uint8_t  lmp_vers;
};

/// Indication of peer features info
struct gapc_peer_features_ind
{
    /// 8-byte array for LE features
    uint8_t features[LE_FEATS_LEN];
};

/// Indication of ongoing connection RSSI
struct gapc_con_rssi_ind
{
    /// RSSI value
    uint8_t rssi;
};
/// Indication of ongoing connection Channel Map
struct gapc_con_channel_map_ind
{
    /// channel map value
    struct le_chnl_map ch_map;
};

/// Indication of peer privacy info
struct gapc_privacy_ind
{
    /// attribute handle
    uint16_t handle;
    /// privacy flag value
    uint8_t privacy;
};

/// Indication of peer reconnection address info
struct gapc_recon_addr_ind
{
    /// attribute handle
    uint16_t handle;
    /// reconnection address
    struct bd_addr recon_addr;
};

/// Sign counter value changed due to packet signing or signature verification.
struct gapc_sign_counter_updated_ind
{
    /// New Local signature counter value
    uint32_t lsign_counter;
    /// New Remote signature counter value
    uint32_t rsign_counter;
};

/// Set Privacy flag command.
struct gapc_set_privacy_cmd
{
    /// GAP request type:
    /// - GAPC_SET_PRIVACY: Set Privacy flag.
    uint8_t operation;
    /// privacy flag value
    uint8_t privacy;
    /// attribute handle - 0x0000 an automatic discovery will be performed
    uint16_t handle;
};

/// Set Reconnection Address Value command.
struct gapc_set_recon_addr_cmd
{
    /// GAP request type:
    /// - GAPC_SET_RECON_ADDR: Set Reconnection Address Value.
    uint8_t operation;
    /// attribute handle - 0x0000 an automatic discovery will be performed
    uint16_t handle;
    /// reconnection address
    struct bd_addr recon_addr;
};


/// Connection Parameter used to update connection parameters
struct gapc_conn_param
{
    /// Connection interval minimum
    uint16_t intv_min;
    /// Connection interval maximum
    uint16_t intv_max;
    /// Latency
    uint16_t latency;
    /// Supervision timeout
    uint16_t time_out;
};


/// Perform update of connection parameters command
struct gapc_param_update_cmd
{
    /// GAP request type:
    /// - GAPC_UPDATE_PARAMS: Perform update of connection parameters.
    uint8_t operation;
    /// Internal parameter used to manage internally l2cap packet identifier for signaling
    uint8_t pkt_id;
#ifndef __DA14581__
    /// New Connection parameters
    struct gapc_conn_param params;
#else
    /// Connection interval minimum
    uint16_t intv_min;
    /// Connection interval maximum
    uint16_t intv_max;
    /// Latency
    uint16_t latency;
    /// Supervision timeout
    uint16_t time_out;
    /// Minimum Connection Event Duration
    uint16_t ce_len_min;
    /// Maximum Connection Event Duration
    uint16_t ce_len_max;
#endif
};

/// Request of updating connection parameters indication
struct gapc_param_update_req_ind
{
#ifndef __DA14581__
    /// New requested connection parameters
    struct gapc_conn_param params;
#else
    /// Connection interval minimum
    uint16_t intv_min;
    /// Connection interval maximum
    uint16_t intv_max;
    /// Latency
    uint16_t latency;
    /// Supervision timeout
    uint16_t time_out;
#endif
};

/// Connection parameters updated indication
struct gapc_param_updated_ind
{
    ///Connection interval value
    uint16_t            con_interval;
    ///Connection latency value
    uint16_t            con_latency;
    ///Supervision timeout
    uint16_t            sup_to;
};


/// Master confirm or not that parameters proposed by slave are accepted or not
struct gapc_param_update_cfm
{
    /// True to accept slave connection parameters, False else.
    bool accept;

#ifdef __DA14581__
    /// Minimum Connection Event Duration
    uint16_t ce_len_min;
    /// Maximum Connection Event Duration
    uint16_t ce_len_max;
#endif
};

/// Pairing parameters
struct gapc_pairing
{
    /// IO capabilities (@see gap_io_cap)
    uint8_t iocap;
    /// OOB information (@see gap_oob)
    uint8_t oob;
    /// Authentication (@see gap_auth)
    uint8_t auth;
    /// Encryption key size (7 to 16)
    uint8_t key_size;
    ///Initiator key distribution (@see gap_kdist)
    uint8_t ikey_dist;
    ///Responder key distribution (@see gap_kdist)
    uint8_t rkey_dist;

    /// Device security requirements (minimum security level). (@see gap_sec_req)
    uint8_t sec_req;
};

/// Long Term Key information
struct gapc_ltk
{
    /// Long Term Key
    struct gap_sec_key ltk;
    /// Encryption Diversifier
    uint16_t ediv;
    /// Random Number
    struct rand_nb randnb;
    /// Encryption key size (7 to 16)
    uint8_t key_size;
};

/// Identity Resolving Key information
struct gapc_irk
{
    /// Identity Resolving Key
    struct gap_sec_key irk;
    /// Device BD Address
    struct gap_bdaddr addr;
};


/// Start Bonding command procedure
struct gapc_bond_cmd
{
    /// GAP request type:
    /// - GAPC_BOND:  Start bonding procedure.
    uint8_t operation;
    /// Pairing information
    struct gapc_pairing pairing;
};

/// Bonding requested by peer device indication message.
struct gapc_bond_req_ind
{
    /// Bond request type (@see gapc_bond)
    uint8_t request;

    /// Bond procedure requested information data
    union gapc_bond_req_data
    {
        /// Authentication level (@see gap_auth) (if request = GAPC_PAIRING_REQ)
        uint8_t auth_req;
        /// LTK Key Size (if request = GAPC_LTK_EXCH)
        uint8_t key_size;
        /// Device IO used to get TK: (if request = GAPC_TK_EXCH)
        ///  - GAP_TK_OOB:       TK get from out of band method
        ///  - GAP_TK_DISPLAY:   TK generated and shall be displayed by local device
        ///  - GAP_TK_KEY_ENTRY: TK shall be entered by user using device keyboard
        uint8_t tk_type;
    } data;
};

/// Confirm requested bond information.
struct gapc_bond_cfm
{
    /// Bond request type (@see gapc_bond)
    uint8_t request;
    /// Request accepted
    uint8_t accept;

    /// Bond procedure information data
    union gapc_bond_cfm_data
    {
        /// Pairing Features (request = GAPC_PAIRING_RSP)
        struct gapc_pairing pairing_feat;
        /// LTK (request = GAPC_LTK_EXCH)
        struct gapc_ltk ltk;
        /// CSRK (request = GAPC_CSRK_EXCH)
        struct gap_sec_key csrk;
        /// TK (request = GAPC_TK_EXCH)
        struct gap_sec_key tk;
    } data;
};


/// Bonding information indication message
struct gapc_bond_ind
{
    /// Bond information type (@see gapc_bond)
    uint8_t info;

    /// Bond procedure information data
    union gapc_bond_data
    {
        /// Authentication information (@see gap_auth)
        /// (if info = GAPC_PAIRING_SUCCEED)
        uint8_t auth;
        /// Pairing failed reason  (if info = GAPC_PAIRING_FAILED)
        uint8_t reason;
        /// Long Term Key information (if info = GAPC_LTK_EXCH)
        struct gapc_ltk ltk;
        /// Identity Resolving Key information (if info = GAPC_IRK_EXCH)
        struct gapc_irk irk;
        /// Connection Signature Resolving Key information (if info = GAPC_CSRK_EXCH)
        struct gap_sec_key csrk;
    } data;
};

/// Start Encryption command procedure
struct gapc_encrypt_cmd
{
    /// GAP request type:
    /// - GAPC_ENCRYPT:  Start encryption procedure.
    uint8_t operation;
    /// Long Term Key information
    struct gapc_ltk ltk;
};

/// Encryption requested by peer device indication message.
struct gapc_encrypt_req_ind
{
    /// Encryption Diversifier
    uint16_t ediv;
    /// Random Number
    struct rand_nb rand_nb;
};

/// Confirm requested Encryption information.
struct gapc_encrypt_cfm
{
    /// Indicate if a LTK has been found for the peer device
    uint8_t found;
    /// Long Term Key
    struct gap_sec_key ltk;
    /// LTK Key Size
    uint8_t key_size;
};

/// Encryption information indication message
struct gapc_encrypt_ind
{
    /// Authentication  level (@see gap_auth)
    uint8_t auth;
};

/// Start Security Request command procedure
struct gapc_security_cmd
{
    /// GAP request type:
    /// - GAPC_SECURITY_REQ: Start security request procedure
    uint8_t operation;
    /// Authentification level (@see gap_auth)
    uint8_t auth;
};
/// Security requested by peer device indication message
struct gapc_security_ind
{
    /// Authentification level (@see gap_auth)
    uint8_t auth;
};

/// Parameters of the @ref GAPC_SIGN_COUNTER_IND message
struct gapc_sign_counter_ind
{
    /// Local SignCounter value
    uint32_t local_sign_counter;
    /// Peer SignCounter value
    uint32_t peer_sign_counter;
};

/*
 * MACROS
 ****************************************************************************************
 */


/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/*
 * TASK DESCRIPTOR DECLARATIONS
 ****************************************************************************************
 */
extern const struct ke_state_handler gapc_state_handler[GAPC_STATE_MAX];
extern const struct ke_state_handler gapc_default_handler;
extern ke_state_t gapc_state[GAPC_IDX_MAX];

#endif // (BLE_CENTRAL || BLE_PERIPHERAL)

/// @} GAPC_TASK

#endif /* _GAPC_TASK_H_ */
