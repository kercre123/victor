/**
 ****************************************************************************************
 *
 * @file gapm_task.h
 *
 * @brief Generic Access Profile Manager Task Header.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 ****************************************************************************************
 */


#ifndef _GAPM_TASK_H_
#define _GAPM_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup GAPM_TASK Generic Access Profile Manager Task
 * @ingroup GAPM
 * @brief  Handles ALL messages to/from GAP Manager block.
 *
 * It handles messages from lower and higher layers not related to an ongoing connection.
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
#include "gapm.h"
#include "attm.h"


/*
 * DEFINES
 ****************************************************************************************
 */

/// Maximum number of GAP Manager process
#define GAPM_IDX_MAX                                 0x01

/// GAPM states
enum gapm_state
{
    /// Idle state - no on going operation
    GAPM_IDLE         = 0x00,
    /// Busy state - Configuration operation on going
    GAPM_BUSY_CFG     = 0x01,
    /// Busy state - Air operation on going
    GAPM_BUSY_AIR     = 0x02,
    /// Busy state - Configuration and air operation on going
    GAPM_BUSY_CFG_AIR = 0x03,
    /// Reset state - Reset operation on going
    GAPM_DEVICE_SETUP,

    GAPM_STATE_MAX
};

/// GAP Manager Message Interface
enum gapm_msg_id
{
    /* Default event */
    /// Command Complete event
    GAPM_CMP_EVT = KE_FIRST_MSG(TASK_GAPM),
    /// Event triggered to inform that lower layers are ready
    GAPM_DEVICE_READY_IND,

    /* Default commands */
    /// Reset link layer and the host command
    GAPM_RESET_CMD,
    /// Cancel ongoing operation
    GAPM_CANCEL_CMD,

    /* Device Configuration */
    /// Set device configuration command
    GAPM_SET_DEV_CONFIG_CMD,
    /// Set device name command
    GAPM_SET_DEV_NAME_CMD,
    /// Set device channel map
    GAPM_SET_CHANNEL_MAP_CMD,

    /* Local device information */
    /// Get local device info command
    GAPM_GET_DEV_INFO_CMD,
    /// Local device name indication event
    GAPM_DEV_NAME_IND,
    /// Local device appearance indication event
    GAPM_APPEARANCE_IND,
    /// Local device version indication event
    GAPM_DEV_VERSION_IND,
    /// Local device BD Address indication event
    GAPM_DEV_BDADDR_IND,

    /* White List */
    /// White List Management Command
    GAPM_WHITE_LIST_MGT_CMD,
    /// White List Size indication event
    GAPM_WHITE_LIST_SIZE_IND,

    /* Air Operations */
    /// Set advertising mode Command
    GAPM_START_ADVERTISE_CMD,

    /// Set Scan mode Command
    GAPM_START_SCAN_CMD,
    /// Advertising or scanning report information event
    GAPM_ADV_REPORT_IND,

    /// Set connection initialization Command
    GAPM_START_CONNECTION_CMD,
    /// Name of peer device indication
    GAPM_PEER_NAME_IND,
    /// Confirm connection to a specific device (Connection Operation in Selective mode)
    GAPM_CONNECTION_CFM,

    /* Privacy update events */
    /// Privacy flag value has been updated
    GAPM_UPDATED_PRIVACY_IND,
    /// Reconnection address has been updated
    GAPM_UPDATED_RECON_ADDR_IND,

    /* Security / Encryption Toolbox */
    /// Resolve address command
    GAPM_RESOLV_ADDR_CMD,
    /// Indicate that resolvable random address has been solved
    GAPM_ADDR_SOLVED_IND,
    /// Generate a random address.
    GAPM_GEN_RAND_ADDR_CMD,
    /// Use the AES-128 block in the controller
    GAPM_USE_ENC_BLOCK_CMD,
    ///  AES-128 block result indication
    GAPM_USE_ENC_BLOCK_IND,
    /// Generate a 8-byte random number
    GAPM_GEN_RAND_NB_CMD,
    /// Random Number Indication
    GAPM_GEN_RAND_NB_IND,

    /* Debug  */
    /// Indication containing information about memory usage.
    GAPM_DBG_MEM_INFO_IND,

    /* Local device information -cont */
    /// Advertising channel Tx power level
    GAPM_DEV_ADV_TX_POWER_IND,

    /* Internal messages for timer events, not part of API*/
    /// Limited discoverable timeout indication
    GAPM_LIM_DISC_TO_IND,
    /// Scan timeout indication
    GAPM_SCAN_TO_IND,
    /// Address renewal timeout indication
    GAPM_ADDR_RENEW_TO_IND,
};



/// GAP Manager operation type - application interface
enum gapm_operation
{
    /* No Operation (if nothing has been requested)     */
    /* ************************************************ */
    /// No operation.
    GAPM_NO_OP                                     = 0x00,

    /* Default operations                               */
    /* ************************************************ */
    /// Reset BLE subsystem: LL and HL.
    GAPM_RESET,
    /// Cancel currently executed operation.
    GAPM_CANCEL,

    /* Configuration operations                         */
    /* ************************************************ */
    /// Set device configuration
    GAPM_SET_DEV_CONFIG,
    /// Set device name
    GAPM_SET_DEV_NAME,
    /// Set device channel map
    GAPM_SET_CHANNEL_MAP,

    /* Retrieve device information                      */
    /* ************************************************ */
    /// Get Local device name
    GAPM_GET_DEV_NAME,
    /// Get Local device version
    GAPM_GET_DEV_VERSION,
    /// Get Local device BD Address
    GAPM_GET_DEV_BDADDR,

    /* Operation on White list                          */
    /* ************************************************ */
    /// Get White List Size.
    GAPM_GET_WLIST_SIZE,
    /// Add devices in white list.
    GAPM_ADD_DEV_IN_WLIST,
    /// Remove devices form white list.
    GAPM_RMV_DEV_FRM_WLIST,
    /// Clear all devices from white list.
    GAPM_CLEAR_WLIST,

    /* Advertise mode operations                        */
    /* ************************************************ */
    /// Start non connectable advertising
    GAPM_ADV_NON_CONN,
    /// Start undirected connectable advertising
    GAPM_ADV_UNDIRECT,
    /// Start directed connectable advertising
    GAPM_ADV_DIRECT,

    /* Scan mode operations                             */
    /* ************************************************ */
    /// Start active scan operation
    GAPM_SCAN_ACTIVE,
    /// Start passive scan operation
    GAPM_SCAN_PASSIVE,

    /* Connection mode operations                       */
    /* ************************************************ */
    /// Direct connection operation
    GAPM_CONNECTION_DIRECT,
    /// Automatic connection operation
    GAPM_CONNECTION_AUTO,
    /// Selective connection operation
    GAPM_CONNECTION_SELECTIVE,
    /// Name Request operation (requires to start a direct connection)
    GAPM_CONNECTION_NAME_REQUEST,

    /* Security / Encryption Toolbox                    */
    /* ************************************************ */
    /// Resolve device address
    GAPM_RESOLV_ADDR,
    /// Generate a random address
    GAPM_GEN_RAND_ADDR,
    /// Use the controller's AES-128 block
    GAPM_USE_ENC_BLOCK,
    /// Generate a 8-byte random number
    GAPM_GEN_RAND_NB,

    /* DEBUG                                            */
    /* ************************************************ */
    /// Get memory usage
    GAPM_DBG_GET_MEM_INFO,
    /// Perform a platform reset
    GAPM_PLF_RESET,

    /* Retrieve device information - cont               */
    /* ************************************************ */
    /// Get device advertising power level
    GAPM_GET_DEV_ADV_TX_POWER,

    /// Last GAPM operation flag
    GAPM_LAST
};


/// Configuration flag bit field description
enum gapm_cfg_flag
{
    /// Privacy Support enabled
    GAPM_CFG_PRIVACY_EN            = (1 << 0),
    /// Multiple Bond support enabled
    GAPM_CFG_MUL_BOND_SUPP_EN      = (1 << 1),
    /// Set Visible reconnection address support enable
    GAPM_CFG_VISIBLE_RECON_ADDR_EN = (1 << 2),
};


/// Own BD address source of the device
enum gapm_own_addr_src
{
   /// Public Address
   GAPM_PUBLIC_ADDR,
   /// Provided random address
   GAPM_PROVIDED_RND_ADDR,
   /// Provided static random address
   GAPM_GEN_STATIC_RND_ADDR,
   /// Generated resolvable private random address
   GAPM_GEN_RSLV_ADDR,
   /// Generated non-resolvable private random address
   GAPM_GEN_NON_RSLV_ADDR,
   /// Provided Reconnection address
   GAPM_PROVIDED_RECON_ADDR,
};

/// Device Attribute write permission requirement
enum gapm_write_att_perm
{
    /// Disable write access
    GAPM_WRITE_DISABLE  = PERM(WR, DISABLE),
    /// Enable write access
    GAPM_WRITE_ENABLE   = PERM(WR, ENABLE),
    /// Write access requires unauthenticated link
    GAPM_WRITE_UNAUTH   = PERM(WR, UNAUTH),
    /// Write access requires authenticated link
    GAPM_WRITE_AUTH     = PERM(WR, AUTH),
};


/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Operation command structure in order to keep requested operation.
struct gapm_operation_cmd
{
    /// GAP request type
    uint8_t operation;
};

/// Command complete event data structure
struct gapm_cmp_evt
{
    /// GAP requested operation
    uint8_t operation;
    /// Status of the request
    uint8_t status;
};

///  Reset link layer and the host command
struct gapm_reset_cmd
{
    /// GAPM requested operation:
    /// - GAPM_RESET: Reset BLE subsystem: LL and HL.
    uint8_t operation;
};

/// Set device configuration command
struct gapm_set_dev_config_cmd
{
    /// GAPM requested operation:
    ///  - GAPM_SET_DEV_CONFIG: Set device configuration
    uint8_t operation;
    /// Device Role: Central, Peripheral, Observer or Broadcaster
    uint8_t role;
    /// Device IRK used for resolvable random BD address generation (LSB first)
    struct gap_sec_key irk;

    /// Device Appearance (0x0000 - Unknown appearance)
    uint16_t appearance;
    /// Device Appearance write permission requirements for peer device (@see gapm_write_att_perm)
    uint8_t  appearance_write_perm;
    /// Device Name write permission requirements for peer device (@see gapm_write_att_perm)
    uint8_t  name_write_perm;

    /// Maximal MTU
    uint16_t max_mtu;

    /// Peripheral only: *****************************************************************
    /// Slave preferred Minimum of connection interval
    uint16_t con_intv_min;
    /// Slave preferred Maximum of connection interval
    uint16_t con_intv_max;
    /// Slave preferred Connection latency
    uint16_t con_latency;
    /// Slave preferred Link supervision timeout
    uint16_t superv_to;

    /// Privacy settings bit field (0b1 = enabled, 0b0 = disabled)
    ///  - [bit 0]: Privacy Support
    ///  - [bit 1]: Multiple Bond Support (Peripheral only); If enabled, privacy flag is
    ///             read only.
    ///  - [bit 2]: Reconnection address visible.
    uint8_t flags;
};

/// Set Local device name command
struct gapm_set_dev_name_cmd
{
    /// GAPM requested operation:
    ///  - GAPM_SET_DEV_NAME: Set Local device name
    uint8_t operation;
    /// name length
    uint8_t length;
    /// name value
    uint8_t name[__ARRAY_EMPTY];
};


/// Set device channel map
struct gapm_set_channel_map_cmd
{
    /// GAPM requested operation:
    ///  - GAPM_SET_CHANNEL_MAP: Set device channel map.
    uint8_t operation;
    /// Channel map
    struct le_chnl_map chmap;
};



/// Get local device info command
struct gapm_get_dev_info_cmd
{
    /// GAPM requested operation:
    ///  - GAPM_GET_DEV_NAME: Get Local device name
    ///  - GAPM_GET_DEV_VERSION: Get Local device version
    ///  - GAPM_GET_DEV_BDADDR: Get Local device BD Address
    uint8_t operation;
};

/// Local device name indication event
struct gapm_dev_name_ind
{
    /// name length
    uint8_t length;
    /// name value
    uint8_t name[__ARRAY_EMPTY];
};

/// Local device appearance indication event
struct gapm_appearance_ind
{
    /// Device Appearance
    uint16_t appearance;
};

/// Local device version indication event
struct gapm_dev_version_ind
{
    /// HCI version
    uint8_t hci_ver;
    /// LMP version
    uint8_t lmp_ver;
    /// Host version
    uint8_t host_ver;
    /// HCI revision
    uint16_t hci_subver;
    /// LMP subversion
    uint16_t lmp_subver;
    /// Host revision
    uint16_t host_subver;
    /// Manufacturer name
    uint16_t manuf_name;
};

/// Local device BD Address indication event
struct gapm_dev_bdaddr_ind
{
    /// Local device address information
    struct gap_bdaddr addr;
};

/// Advertising channel Tx power level indication event
struct gapm_dev_adv_tx_power_ind
{
    /// Advertising channel Tx power level
#ifndef __DA14581__
    uint8_t     power_lvl;
#else
    int8_t     power_lvl;
#endif
};

/// Cancel ongoing operation
struct gapm_cancel_cmd
{
    /// GAPM requested operation
    /// - GAPM_CANCEL: Cancel running operation
    uint8_t operation;
};


/// White List Management Command
struct gapm_white_list_mgt_cmd
{
    /// GAPM requested operation:
    ///  - GAPM_GET_WLIST_SIZE: Get White List Size.
    ///  - GAPM_ADD_DEV_IN_WLIST: Add devices in white list.
    ///  - GAPM_RMV_DEV_FRM_WLIST: Remove devices form white list.
    ///  - GAPM_CLEAR_WLIST: Clear all devices from white list.
    uint8_t operation;
    /// Number of device information present in command
    uint8_t nb;
    /// Device address information that can be used to add or remove element in device list.
    struct gap_bdaddr devices[__ARRAY_EMPTY];
};

/// White List Size indication event
struct gapm_white_list_size_ind
{
    /// White List size
    uint8_t size;
};

/// Resolve Address command
struct gapm_resolv_addr_cmd
{
    /// GAPM requested operation:
    ///  - GAPM_RESOLV_ADDR: Resolve device address
    uint8_t operation;
    /// Number of provided IRK (sahlle be > 0)
    uint8_t nb_key;
    /// Resolvable random address to solve
    struct bd_addr addr;
    /// Array of IRK used for address resolution (MSB -> LSB)
    struct gap_sec_key irk[__ARRAY_EMPTY];
};


/// Indicate that resolvable random address has been solved
struct gapm_addr_solved_ind
{
    /// Resolvable random address solved
    struct bd_addr addr;
    /// IRK that correctly solved the random address
    struct gap_sec_key irk;
};


/// Advertising data that contains information set by host.
struct gapm_adv_host
{
    /// Advertising mode :
    /// - GAP_NON_DISCOVERABLE: Non discoverable mode
    /// - GAP_GEN_DISCOVERABLE: General discoverable mode
    /// - GAP_LIM_DISCOVERABLE: Limited discoverable mode
    /// - GAP_BROADCASTER_MODE: Broadcaster mode
    uint8_t              mode;

    /// Advertising filter policy:
    /// - ADV_ALLOW_SCAN_ANY_CON_ANY: Allow both scan and connection requests from anyone
    /// - ADV_ALLOW_SCAN_WLST_CON_ANY: Allow both scan req from White List devices only and
    ///   connection req from anyone
    /// - ADV_ALLOW_SCAN_ANY_CON_WLST: Allow both scan req from anyone and connection req
    ///   from White List devices only
    /// - ADV_ALLOW_SCAN_WLST_CON_WLST: Allow scan and connection requests from White List
    ///   devices only
    uint8_t              adv_filt_policy;

    /// Advertising data length - maximum 28 bytes, 3 bytes are reserved to set
    /// Advertising AD type flags, shall not be set in advertising data
    uint8_t              adv_data_len;
    /// Advertising data
    uint8_t              adv_data[ADV_DATA_LEN-3];
    /// Scan response data length- maximum 31 bytes
    uint8_t              scan_rsp_data_len;
    /// Scan response data
    uint8_t              scan_rsp_data[SCAN_RSP_DATA_LEN];
};

/// Set advertising mode Command
struct gapm_start_advertise_cmd
{
    /// GAPM requested operation:
    /// - GAPM_ADV_NON_CONN: Start non connectable advertising
    /// - GAPM_ADV_UNDIRECT: Start undirected connectable advertising
    /// - GAPM_ADV_DIRECT: Start directed connectable advertising
    struct gapm_air_operation op;

    /// Minimum interval for advertising
    uint16_t             intv_min;
    /// Maximum interval for advertising
    uint16_t             intv_max;

    ///Advertising channel map
    uint8_t              channel_map;

    /// Advertising information
    union gapm_adv_info
    {
        /// Host information advertising data (GAPM_ADV_NON_CONN and GAPM_ADV_UNDIRECT)
        struct gapm_adv_host host;
        ///  Direct address information (GAPM_ADV_DIRECT)
        /// (used only if reconnection address isn't set or privacy disabled)
        struct gap_bdaddr direct;
    } info;
};

/// Set scan mode Command
struct gapm_start_scan_cmd
{
    /// GAPM requested operation:
    /// - GAPM_SCAN_ACTIVE: Start active scan operation
    /// - GAPM_SCAN_PASSIVE: Start passive scan operation
    struct gapm_air_operation op;

    /// Scan interval
    uint16_t             interval;
    /// Scan window size
    uint16_t             window;

    /// Scanning mode :
    /// - GAP_GEN_DISCOVERY: General discovery mode
    /// - GAP_LIM_DISCOVERY: Limited discovery mode
    /// - GAP_OBSERVER_MODE: Observer mode
    uint8_t              mode;

    /// Scan filter policy:
    /// - SCAN_ALLOW_ADV_ALL: Allow advertising packets from anyone
    /// - SCAN_ALLOW_ADV_WLST: Allow advertising packets from White List devices only
    uint8_t              filt_policy;
    /// Scan duplicate filtering policy:
    /// - SCAN_FILT_DUPLIC_DIS: Disable filtering of duplicate packets
    /// - SCAN_FILT_DUPLIC_EN: Enable filtering of duplicate packets
    uint8_t              filter_duplic;
};


/// Advertising or scanning report information event
struct gapm_adv_report_ind
{
    /// Advertising report structure
    struct adv_report report;
};


/// Set connection initialization Command
struct gapm_start_connection_cmd
{
    /// GAPM requested operation:
    /// - GAPM_CONNECTION_DIRECT: Direct connection operation
    /// - GAPM_CONNECTION_AUTO: Automatic connection operation
    /// - GAPM_CONNECTION_SELECTIVE: Selective connection operation
    /// - GAPM_CONNECTION_NAME_REQUEST: Name Request operation (requires to start a direct
    ///   connection)
    struct gapm_air_operation op;

    /// Scan interval
    uint16_t             scan_interval;
    /// Scan window size
    uint16_t             scan_window;

    /// Minimum of connection interval
    uint16_t             con_intv_min;
    /// Maximum of connection interval
    uint16_t             con_intv_max;
    /// Connection latency
    uint16_t             con_latency;
    /// Link supervision timeout
    uint16_t             superv_to;
    /// Minimum CE length
    uint16_t             ce_len_min;
    /// Maximum CE length
    uint16_t             ce_len_max;

    /// Number of peer device information present in message.
    ///  Shall be 1 for GAPM_CONNECTION_DIRECT or GAPM_CONNECTION_NAME_REQUEST operations
    ///  Shall be greater than 0 for other operations
    uint8_t              nb_peers;

    /// Peer device information
    struct gap_bdaddr   peers[__ARRAY_EMPTY];
};


/// Name of peer device indication
struct gapm_peer_name_ind
{
    /// peer device bd address
    struct bd_addr addr;
    /// peer device address type
    uint8_t addr_type;
    /// peer device name length
    uint8_t name_len;
    /// peer device name
    uint8_t name[__ARRAY_EMPTY];
};

/// Confirm connection to a specific device (Connection Operation in Selective mode)
struct gapm_connection_cfm
{
    /// peer device bd address
    struct bd_addr addr;
    /// peer device address type
    uint8_t addr_type;

    /// Minimum of connection interval
    uint16_t             con_intv_min;
    /// Maximum of connection interval
    uint16_t             con_intv_max;
    /// Connection latency
    uint16_t             con_latency;
    /// Link supervision timeout
    uint16_t             superv_to;
    /// Minimum CE length
    uint16_t             ce_len_min;
    /// Maximum CE length
    uint16_t             ce_len_max;
};

/// Privacy flag value has been updated
struct gapm_updated_privacy_ind
{
    /// privacy flag value
    uint8_t privacy;
};

/// Reconnection address has been updated
struct gapm_updated_recon_addr_ind
{
    /// reconnection address
    struct bd_addr recon_addr;
};

/// Generate a random address.
struct gapm_gen_rand_addr_cmd
{
    /// GAPM requested operation:
    ///  - GAPM_GEN_RAND_ADDR: Generate a random address
    uint8_t operation;
    /// Random address type @see gap_rnd_addr_type
    ///  - GAP_STATIC_ADDR: Static random address
    ///  - GAP_NON_RSLV_ADDR: Private non resolvable address
    ///  - GAP_RSLV_ADDR: Private resolvable address
    uint8_t rnd_type;
};

/// Parameters of the @ref GAPM_USE_ENC_BLOCK_CMD message
struct gapm_use_enc_block_cmd
{
    /// Command Operation Code (shall be GAPM_USE_ENC_BLOCK)
    uint8_t operation;
    /// Operand 1
    uint8_t operand_1[KEY_LEN];
    /// Operand 2
    uint8_t operand_2[KEY_LEN];
};

/// Parameters of the @ref GAPM_USE_ENC_BLOCK_IND message
struct gapm_use_enc_block_ind
{
    /// Result (16 bytes)
    uint8_t result[KEY_LEN];
};

/// Parameters of the @ref GAPM_GEN_RAND_NB_CMD message
struct gapm_gen_rand_nb_cmd
{
    /// Command Operation Code (shall be GAPM_GEN_RAND_NB)
    uint8_t operation;
};

/// Parameters of the @ref GAPM_GEN_RAND_NB_IND message
struct gapm_gen_rand_nb_ind
{
    /// Generation Random Number (8 bytes)
    struct rand_nb randnb;
};

#if (KE_PROFILING)
/// Retrieve information about memory usage
struct gapm_dbg_get_mem_info_cmd
{
    /// GAPM requested operation:
    ///  - GAPM_DBG_GET_MEM_INFO: Get memory usage
    uint8_t operation;
};

/// Indication containing information about memory usage.
struct gapm_dbg_mem_info_ind
{
    /// peak of memory usage measured
    uint32_t max_mem_used;
    /// memory size currently used into each heaps.
    uint16_t mem_used[KE_MEM_BLOCK_MAX];
};
#endif // (KE_PROFILING)


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
extern const struct ke_state_handler gapm_state_handler[GAPM_STATE_MAX];
extern const struct ke_state_handler gapm_default_handler;
extern ke_state_t gapm_state[GAPM_IDX_MAX];

/// @} GAPM_TASK

#endif /* _GAPM_TASK_H_ */
