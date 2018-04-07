/**
 ****************************************************************************************
 *
 * @file user_config.h
 *
 * @brief User configuration file.
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

#ifndef _USER_CONFIG_H_
#define _USER_CONFIG_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "app_user_config.h"
#include "arch_api.h"
#include "app_default_handlers.h"
#include "app_adv_data.h"

/*
 * VARIABLES
 ****************************************************************************************
 */

/******************************************
 * Default sleep mode. Possible values are:
 *
 * - ARCH_SLEEP_OFF
 * - ARCH_EXT_SLEEP_ON
 * - ARCH_DEEP_SLEEP_ON
 *
 ******************************************
 */
const static sleep_state_t app_default_sleep_mode = ARCH_SLEEP_OFF;

/*
 ****************************************************************************************
 *
 * Advertising configuration
 *
 ****************************************************************************************
 */
static const struct advertise_configuration user_adv_conf = {
    /// Own BD address source of the device:
    /// - GAPM_PUBLIC_ADDR:         Public Address
    /// - GAPM_PROVIDED_RND_ADDR:   Provided random address
    /// - GAPM_GEN_STATIC_RND_ADDR: Generated static random address
    /// - GAPM_GEN_RSLV_ADDR:       Generated resolvable private random address
    /// - GAPM_GEN_NON_RSLV_ADDR:   Generated non-resolvable private random address
    /// - GAPM_PROVIDED_RECON_ADDR: Provided Reconnection address (only for GAPM_ADV_DIRECT)
    .addr_src = GAPM_PUBLIC_ADDR,

    /// Duration of resolvable address before regenerate it.
    .renew_dur = 0,

    /// Minimum interval for advertising
    .intv_min = 160,                    // 100ms (160*0.625ms)

    /// Maximum interval for advertising
    .intv_max = 160,                    // 100ms (160*0.625ms)

    /// Advertising channel map
    .channel_map = 0x7,

    /*************************
     * Advertising information
     *************************
     */

    /// Host information advertising data (GAPM_ADV_NON_CONN and GAPM_ADV_UNDIRECT)
    /// Advertising mode :
    /// - GAP_NON_DISCOVERABLE: Non discoverable mode
    /// - GAP_GEN_DISCOVERABLE: General discoverable mode
    /// - GAP_LIM_DISCOVERABLE: Limited discoverable mode
    /// - GAP_BROADCASTER_MODE: Broadcaster mode
    .mode = GAP_GEN_DISCOVERABLE,

    /// Host information advertising data (GAPM_ADV_NON_CONN and GAPM_ADV_UNDIRECT)
    /// Advertising filter policy:
    /// - ADV_ALLOW_SCAN_ANY_CON_ANY: Allow both scan and connection requests from anyone
    /// - ADV_ALLOW_SCAN_WLST_CON_ANY: Allow both scan req from White List devices only and
    ///   connection req from anyone
    /// - ADV_ALLOW_SCAN_ANY_CON_WLST: Allow both scan req from anyone and connection req
    ///   from White List devices only
    /// - ADV_ALLOW_SCAN_WLST_CON_WLST: Allow scan and connection requests from White List
    ///   devices only
    .adv_filt_policy = ADV_ALLOW_SCAN_ANY_CON_ANY,

    /// Direct address information (GAPM_ADV_DIRECT)
    /// (used only if reconnection address isn't set or privacy disabled)
    /// BD Address of device
    .peer_addr = {0x1, 0x2, 0x3, 0x4, 0x5, 0x6},

    /// Direct address information (GAPM_ADV_DIRECT)
    /// (used only if reconnection address isn't set or privacy disabled)
    /// Address type of the device 0=public/1=private random
    .peer_addr_type = 0,
};

/*
 ****************************************************************************************
 *
 * Common advertising or scan response data for:
 *
 * - Nonconnectable undirected advertising event (ADV_NONCONN_IND). In this mode the scan
 *   response data are useless.
 *
 * - Connectable undirected advertising event (ADV_IND).
 *
 ****************************************************************************************
 */
/// Advertising service data
/// Advertising AD type flags, shall not be set in advertising data
#define USER_ADVERTISE_DATA    ("\x03"\
                                ADV_TYPE_COMPLETE_LIST_16BIT_SERVICE_IDS\
                                ADV_UUID_DEVICE_INFORMATION_SERVICE\
                                "\x11"\
                                ADV_TYPE_COMPLETE_LIST_128BIT_SERVICE_IDS\
                                "\x2F\x2A\x93\xA6\xBD\xD8\x41\x52\xAC\x0B\x10\x99\x2E\xC6\xFE\xED")

/// Advertising data length - maximum 28 bytes, 3 bytes are reserved to set
#define USER_ADVERTISE_DATA_LEN (sizeof(USER_ADVERTISE_DATA)-1)

/// Scan response data
#define USER_ADVERTISE_SCAN_RESPONSE_DATA ""

/// Scan response data length- maximum 31 bytes
#define USER_ADVERTISE_SCAN_RESPONSE_DATA_LEN (sizeof(USER_ADVERTISE_SCAN_RESPONSE_DATA)-1)

/// Device name to be placed in advertising or scan response data
#define USER_DEVICE_NAME    ("DIALOG-PRFL")

/// Device name length
#define USER_DEVICE_NAME_LEN (sizeof(USER_DEVICE_NAME)-1)

/*
 ****************************************************************************************
 *
 * GAPM configuration
 *
 ****************************************************************************************
 */
static const struct gapm_configuration user_gapm_conf = {
     /// Device Role: Central, Peripheral, Observer or Broadcaster
    .role = GAP_PERIPHERAL_SLV,

    /// Device IRK used for resolvable random BD address generation (LSB first)
    .irk = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},

    /// Device Appearance (0x0000 - Unknown appearance)
    //Fill in according to https://developer.bluetooth.org/gatt/characteristics/Pages/CharacteristicViewer.aspx?u=org.bluetooth.characteristic.gap.appearance.xml
    .appearance = 0,

    /// Device Appearance write permission requirements for peer device (@see gapm_write_att_perm)
    .appearance_write_perm = GAPM_WRITE_DISABLE,

    /// Device Name write permission requirements for peer device (@see gapm_write_att_perm)
    .name_write_perm = GAPM_WRITE_DISABLE,

    /// Maximal MTU
    .max_mtu = 23,

    /// Peripheral only: *****************************************************************
    /// Slave preferred Minimum of connection interval  measured in ble double slots (1.25ms)
    /// use the macro MS_TO_DOUBLESLOTS to convert from milliseconds (ms) to double slots
    .con_intv_min = MS_TO_DOUBLESLOTS(10),

    /// Slave preferred Maximum of connection interval  measured in ble double slots (1.25ms)
    /// use the macro MS_TO_DOUBLESLOTS to convert from milliseconds (ms) to double slots
    .con_intv_max = MS_TO_DOUBLESLOTS(20),

    /// Slave preferred Connection latency. It is measured in connection events skipped
    .con_latency = 0,

    /// Slave preferred Link supervision timeout measured in timer units (10 ms)
    /// use the macro MS_TO_TIMERUNITS to convert from milliseconds (ms) to timer units
    .superv_to = MS_TO_TIMERUNITS(1000),

    /// Privacy settings bit field (0b1 = enabled, 0b0 = disabled)
    ///  - [bit 0]: Privacy Support
    ///  - [bit 1]: Multiple Bond Support (Peripheral only); If enabled, privacy flag is
    ///             read only.
    ///  - [bit 2]: Reconnection address visible.
    .flags = 0
};

/*
 ****************************************************************************************
 *
 * Parameter update configuration
 *
 ****************************************************************************************
 */
static const struct connection_param_configuration user_connection_param_conf = {
    /// Connection interval minimum measured in ble double slots (1.25ms)
    /// use the macro MS_TO_DOUBLESLOTS to convert from milliseconds (ms) to double slots
    .intv_min = MS_TO_DOUBLESLOTS(10),

    /// Connection interval maximum measured in ble double slots (1.25ms)
    /// use the macro MS_TO_DOUBLESLOTS to convert from milliseconds (ms) to double slots
    .intv_max = MS_TO_DOUBLESLOTS(20),

    /// Latency measured in connection events
    .latency = 0,

    /// Supervision timeout measured in timer units (10 ms)
    /// use the macro MS_TO_TIMERUNITS to convert from milliseconds (ms) to timer units
    .time_out = MS_TO_TIMERUNITS(1250),

    /// Minimum Connection Event Duration measured in ble double slots (1.25ms)
    /// use the macro MS_TO_DOUBLESLOTS to convert from milliseconds (ms) to double slots
    .ce_len_min = MS_TO_DOUBLESLOTS(0),

    /// Maximum Connection Event Duration measured in ble double slots (1.25ms)
    /// use the macro MS_TO_DOUBLESLOTS to convert from milliseconds (ms) to double slots
    .ce_len_max = MS_TO_DOUBLESLOTS(0),
};

/*
 ****************************************************************************************
 *
 * Default handlers configuration
 *
 ****************************************************************************************
 */
static const struct default_handlers_configuration  user_default_hnd_conf = {
    //Configure the advertise operation used by the default handlers
    //Possible values:
    //  - DEF_ADV_FOREVER
    //  - DEF_ADV_WITH_TIMEOUT
    .adv_scenario = DEF_ADV_FOREVER,

    //Configure the advertise period in case of DEF_ADV_WITH_TIMEOUT.
    //It is measured in timer units (10ms). Use MS_TO_TIMERUNITS macro to convert
    //from milliseconds (ms) to timer units.
    .advertise_period = MS_TO_TIMERUNITS(10000),

    //Configure the security start operation of the default handlers
    //if the security is enabled (CFG_APP_SECURITY)
    .security_request_scenario = DEF_SEC_REQ_NEVER
};

/*
 ****************************************************************************************
 *
 * Central configuration (not used by current example)
 *
 ****************************************************************************************
 */
static const struct central_configuration user_central_conf = {
    /// GAPM requested operation:
    /// - GAPM_CONNECTION_DIRECT: Direct connection operation
    /// - GAPM_CONNECTION_AUTO: Automatic connection operation
    /// - GAPM_CONNECTION_SELECTIVE: Selective connection operation
    /// - GAPM_CONNECTION_NAME_REQUEST: Name Request operation (requires to start a direct
    ///   connection)
    .code = GAPM_CONNECTION_DIRECT,

    /// Own BD address source of the device:
    /// - GAPM_PUBLIC_ADDR: Public Address
    /// - GAPM_PROVIDED_RND_ADDR: Provided random address
    /// - GAPM_GEN_STATIC_RND_ADDR: Generated static random address
    /// - GAPM_GEN_RSLV_ADDR: Generated resolvable private random address
    /// - GAPM_GEN_NON_RSLV_ADDR: Generated non-resolvable private random address
    /// - GAPM_PROVIDED_RECON_ADDR: Provided Reconnection address (only for GAPM_ADV_DIRECT)
    .addr_src = GAPM_PUBLIC_ADDR,

    /// Duration of resolvable address before regenerate it.
    .renew_dur = 100,

    /// Scan interval
    .scan_interval = 0x180,

    /// Scan window size
    .scan_window = 0x160,

     /// Minimum of connection interval
    .con_intv_min = 100,

    /// Maximum of connection interval
    .con_intv_max = 100,

    /// Connection latency
    .con_latency = 0,

    /// Link supervision timeout
    .superv_to = 0x1F4,

     /// Minimum CE length
    .ce_len_min = 0,

    /// Maximum CE length
    .ce_len_max = 0x5,

    /**************************************************************************************
     * Peer device information, only for:
     *
     * - GAPM_CONNECTION_AUTO
     * - GAPM_CONNECTION_SELECTIVE
     *
     * White list with peer addresses and the respective peer address type
     **************************************************************************************
     */

    /// BD Address of device
    .peer_addr_0 = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0},

    /// Address type of the device 0=public/1=private random
    .peer_addr_0_type = 0,

    /// BD Address of device
    .peer_addr_1 = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0},

    /// Address type of the device 0=public/1=private random
    .peer_addr_1_type = 0,

    /// BD Address of device
    .peer_addr_2 = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0},

    /// Address type of the device 0=public/1=private random
    .peer_addr_2_type = 0,

    /// BD Address of device
    .peer_addr_3 = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0},

    /// Address type of the device 0=public/1=private random
    .peer_addr_3_type = 0,

    /// BD Address of device
    .peer_addr_4 = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0},

    /// Address type of the device 0=public/1=private random
    .peer_addr_4_type = 0,

    /// BD Address of device
    .peer_addr_5 = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0},

    /// Address type of the device 0=public/1=private random
    .peer_addr_5_type = 0,

    /// BD Address of device
    .peer_addr_6 = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0},

    /// Address type of the device 0=public/1=private random
    .peer_addr_6_type = 0,

    /// BD Address of device
    .peer_addr_7 = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0},

    /// Address type of the device 0=public/1=private random
    .peer_addr_7_type = 0,
};

/*
 ****************************************************************************************
 *
 * Security related configuration
 *
 ****************************************************************************************
 */
static const struct security_configuration user_security_conf = {
    /**************************************************************************************
     * IO capabilities (@see gap_io_cap)
     *
     * - GAP_IO_CAP_DISPLAY_ONLY          Display Only
     * - GAP_IO_CAP_DISPLAY_YES_NO        Display Yes No
     * - GAP_IO_CAP_KB_ONLY               Keyboard Only
     * - GAP_IO_CAP_NO_INPUT_NO_OUTPUT    No Input No Output
     * - GAP_IO_CAP_KB_DISPLAY            Keyboard Display
     *
     **************************************************************************************
     */
    .iocap          = GAP_IO_CAP_NO_INPUT_NO_OUTPUT,

    /**************************************************************************************
     * OOB information (@see gap_oob)
     *
     * - GAP_OOB_AUTH_DATA_NOT_PRESENT    OOB Data not present
     * - GAP_OOB_AUTH_DATA_PRESENT        OOB data present
     *
     **************************************************************************************
     */
    .oob            = GAP_OOB_AUTH_DATA_NOT_PRESENT,

    /**************************************************************************************
     * Authentication (@see gap_auth)
     *
     * - GAP_AUTH_REQ_NO_MITM_NO_BOND     No MITM No Bonding
     * - GAP_AUTH_REQ_NO_MITM_BOND        No MITM Bonding
     * - GAP_AUTH_REQ_MITM_NO_BOND        MITM No Bonding
     * - GAP_AUTH_REQ_MITM_BOND           MITM and Bonding
     *
     **************************************************************************************
     */
    .auth           = GAP_AUTH_REQ_NO_MITM_BOND,

    /**************************************************************************************
     * Device security requirements (minimum security level). (@see gap_sec_req)
     *
     * - GAP_NO_SEC                       No security (no authentication and encryption)
     * - GAP_SEC1_NOAUTH_PAIR_ENC         Unauthenticated pairing with encryption
     * - GAP_SEC1_AUTH_PAIR_ENC           Authenticated pairing with encryption
     * - GAP_SEC2_NOAUTH_DATA_SGN         Unauthenticated pairing with data signing
     * - GAP_SEC2_AUTH_DATA_SGN           Authentication pairing with data signing
     * - GAP_SEC_UNDEFINED                Unrecognized security
     *
     **************************************************************************************
     */
    .sec_req        = GAP_NO_SEC,

     /// Encryption key size (7 to 16) - LTK Key Size
    .key_size       = KEY_LEN,

    /**************************************************************************************
     * Initiator key distribution (@see gap_kdist)
     *
     * - GAP_KDIST_NONE                   No Keys to distribute
     * - GAP_KDIST_ENCKEY                 LTK (Encryption key) in distribution
     * - GAP_KDIST_IDKEY                  IRK (ID key)in distribution
     * - GAP_KDIST_SIGNKEY                CSRK (Signature key) in distribution
     * - Any combination of the above
     *
     **************************************************************************************
     */
    .ikey_dist      = GAP_KDIST_SIGNKEY,

    /**************************************************************************************
     * Responder key distribution (@see gap_kdist)
     *
     * - GAP_KDIST_NONE                   No Keys to distribute
     * - GAP_KDIST_ENCKEY                 LTK (Encryption key) in distribution
     * - GAP_KDIST_IDKEY                  IRK (ID key)in distribution
     * - GAP_KDIST_SIGNKEY                CSRK (Signature key) in distribution
     * - Any combination of the above
     *
     **************************************************************************************
     */
    .rkey_dist      = GAP_KDIST_ENCKEY,
};

#endif // _USER_CONFIG_H_
