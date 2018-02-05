/**
 ****************************************************************************************
 *
 * @file wpt_common.h
 *
 * @brief WPT Common files that defines structures used in communication between
 * WPT Client(PTU Role) and WPT Service(PRU Role)
 *
 * Copyright (C) 2013 Dialog Semiconductor GmbH and its Affiliates, unpublished work
 * This computer program includes Confidential, Proprietary Information and is a Trade Secret 
 * of Dialog Semiconductor GmbH and its Affiliates. All use, disclosure, and/or 
 * reproduction is prohibited unless authorized in writing. All Rights Reserved.
 *
 ****************************************************************************************
 */

#ifndef WPT_COMMON_H_
#define WPT_COMMON_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */


#include "prf_types.h"
#include "ke_task.h"
#include "atts.h"
#if (BLE_WPTS || BLE_WPT_CLIENT)

/*
 * PROFILE STRUCTURE & SIZE DEFINITIONS
 ****************************************************************************************
 */

#define WPTS_PRU_CHARGING_CTRL_CHAR_LEN 5
#define WPTS_PRU_ALERT_CHAR_LEN 1 + 6 // WHEN BT addr is returned it has to be 7(1 + 6byte BT addr)
                                  // NOTE: Remember to change overall attr size in DB as well when changing this size
#define WPTS_PTU_STATIC_CHAR_LEN 17
#define WPTS_PRU_STATIC_CHAR_LEN 20
#define WPTS_PRU_DYNAMIC_CHAR_LEN 20
#define WPTS_PRU_ALERT_CONFIR_CHAR_LEN 2
#define WPTS_BD_ADDR_LEN 6

#define WPTS_SERVICE_UUID_16 (0xFFFE)
#define WPTS_PRU_CHARGING_CTRL_UUID_128 {0x67, 0x9A, 0x0C, 0x20, 0x00, 0x08, 0x96, 0x9E, 0xE2, 0x11, 0x46, 0xA1, 0x70, 0xE6, 0x55, 0x64}
#define WPTS_PTU_STATIC_UUID_128 {0x68, 0x9A, 0x0C, 0x20, 0x00, 0x08, 0x96, 0x9E, 0xE2, 0x11, 0x46, 0xA1, 0x70, 0xE6, 0x55, 0x64}
#define WPTS_PRU_ALERT_UUID_128 {0x69, 0x9A, 0x0C, 0x20, 0x00, 0x08, 0x96, 0x9E, 0xE2, 0x11, 0x46, 0xA1, 0x70, 0xE6, 0x55, 0x64}
#define WPTS_PRU_STATIC_UUID_128 {0x6A, 0x9A, 0x0C, 0x20, 0x00, 0x08, 0x96, 0x9E, 0xE2, 0x11, 0x46, 0xA1, 0x70, 0xE6, 0x55, 0x64}
#define WPTS_PRU_DYNAMIC_UUID_128  {0x6B, 0x9A, 0x0C, 0x20, 0x00, 0x08, 0x96, 0x9E, 0xE2, 0x11, 0x46, 0xA1, 0x70, 0xE6, 0x55, 0x64}

//////////////////////// PRU Control /////////////////////////
/// PRU Control Enables bit field
enum
{
    WPTS_CTRL_ENABLE_PRU_OUTPUT         = 0x80,
    WPTS_CTRL_DISABLE_PRU_OUTPUT        = 0x00,
    
    WPTS_CTRL_ENABLE_PRU_CHARGE_IND     = 0x40,
    WPTS_CTRL_DISABLE_PRU_CHARGE_IND    = 0x00,
    
    WPTS_CTRL_ADJ_POW_2_5_W             = 0x30,
    WPTS_CTRL_ADJ_POW_33                = 0x20,
    WPTS_CTRL_ADJ_POW_66                = 0x10,
    WPTS_CTRL_ADJ_POW_MAX               = 0x00,
};

/// PRU Control Permission bit field
enum
{
    WPTS_CTRL_PERM_WITHOUT_REASON       = 0x00,
    WPTS_CTRL_PERM_TIME_AVAIL_POW       = 0x01,
    WPTS_CTRL_PERM_DENIED_LATCHING      = 0x80,
    WPTS_CTRL_PERM_DENIED_LIMIT_POW     = 0x81,
    WPTS_CTRL_PERM_DENIED_LIMIT_DEV_NUM = 0x82,
    WPTS_CTRL_PERM_DENIED_CLASS_SUPPORT = 0x83,
};

/// PRU Control charging timer
enum
{
    // to get time in ms, multiply by 10
    WPTS_CTRL_TIME_10_MS = 0x01,
    WPTS_CTRL_TIME_20_MS,
    WPTS_CTRL_TIME_30_MS,
    WPTS_CTRL_TIME_40_MS,
    WPTS_CTRL_TIME_50_MS,
    WPTS_CTRL_TIME_60_MS,
    WPTS_CTRL_TIME_70_MS,
    WPTS_CTRL_TIME_80_MS,
};

//////////////////////// PRU Control /////////////////////////

///////////////////// PRU Static Value ///////////////////////
/// PRU Optional fields validity
enum
{
    WPTS_STATIC_VALIDITY_DELTA_R1 = 0x80,
};

/// PRU Protocol revision
/// look at PTU/PRU Common section - WPT_PROTOCOL_A4WP_1

/// PRU Category
enum
{
    WPTS_STATIC_CATEGORY_UNDEF = 0,
    WPTS_STATIC_CATEGORY_1,
    WPTS_STATIC_CATEGORY_2,
    WPTS_STATIC_CATEGORY_3,
    WPTS_STATIC_CATEGORY_4,
    WPTS_STATIC_CATEGORY_5, // NOTE: Category 6 & 7 are mentioned in spec, but not in definition of Category field in PRU static
    WPTS_STATIC_CATEGORY_END,
};

/// PRU Information about capabilities
enum
{
    /// NFC receiver
    WPTS_STATIC_INFO_NFC_SUPPORTED                  = 0x80,
    WPTS_STATIC_INFO_NFC_NOT_SUPPORTED              = 0x00,
    /// Separate BLE radio in PRU
    WPTS_STATIC_INFO_SEPARATE_BLE_SUPPORTED         = 0x40,
    WPTS_STATIC_INFO_SEPARATE_BLE_NOT_SUPPORTED     = 0x00,
    /// Power control algorithm preference
    WPTS_STATIC_INFO_POWER_MIN_ERROR                = 0x20,
    WPTS_STATIC_INFO_POWER_MAX_EFFICIENCY           = 0x00,
    /// Adjust power capabitily
    WPTS_STATIC_INFO_ADJUST_POWER_SUPPORTED         = 0x10,
    WPTS_STATIC_INFO_ADJUST_POWER_NOT_SUPPORTED     = 0x00,
    /// Charge complete connected mode
    WPTS_STATIC_INFO_CHARGE_COMPLETE_SUPPORTED      = 0x08,
    WPTS_STATIC_INFO_CHARGE_COMPLETE_NOT_SUPPORTED  = 0x00,
};

///////////////////// PRU Static Value ///////////////////////

///////////////////// PRU Dynamic Value //////////////////////
/// PRU Optional fields validity
enum
{
    WPTS_DYNAMIC_VALIDITY_V_OUT_SUPPORTED               = 0x80,
    WPTS_DYNAMIC_VALIDITY_V_OUT_NOT_SUPPORTED           = 0x00,
    
    WPTS_DYNAMIC_VALIDITY_I_OUT_SUPPORTED               = 0x40,
    WPTS_DYNAMIC_VALIDITY_I_OUT_NOT_SUPPORTED           = 0x00,
    
    WPTS_DYNAMIC_VALIDITY_TEMPERATURE_SUPPORTED         = 0x20,
    WPTS_DYNAMIC_VALIDITY_TEMPERATURE_NOT_SUPPORTED     = 0x00,
    
    WPTS_DYNAMIC_VALIDITY_V_RECT_MIN_SUPPORTED          = 0x10,
    WPTS_DYNAMIC_VALIDITY_V_RECT_MIN_NOT_SUPPORTED      = 0x00,
    
    WPTS_DYNAMIC_VALIDITY_V_RECT_SET_SUPPORTED          = 0x08,
    WPTS_DYNAMIC_VALIDITY_V_RECT_SET_NOT_SUPPORTED      = 0x00,
    
    WPTS_DYNAMIC_VALIDITY_V_RECT_HIGH_SUPPORTED         = 0x04,
    WPTS_DYNAMIC_VALIDITY_V_RECT_HIGH_NOT_SUPPORTED     = 0x00,
};

///////////////////// PRU Dynamic Value //////////////////////

///////////////////////// PRU Alert //////////////////////////

enum
{
    WPTS_ALERT_OVERVOLTAGE              = 0x80,
    WPTS_ALERT_OVERCURRENT              = 0x40,
    WPTS_ALERT_OVERTEMPERATURE          = 0x20,
    WPTS_ALERT_SELF_PROTECT             = 0x10,
    WPTS_ALERT_CHARGE_COMPLETE          = 0x08,
    WPTS_ALERT_WIRED_CHARGET_DETECT     = 0x04,
    WPTS_ALERT_CHARGE_PORT              = 0x02,
    WPTS_ALERT_MODE_TRANS_B1            = 0x02,
    WPTS_ALERT_MODE_TRANS_B0            = 0x01,
    WPTS_ALERT_ADJ_POWER_COMPLETE_RSP   = 0x01,
    WPTS_ALERT_ADJ_POWER_FAILED_RSP     = 0x00,
    WPTS_ALERT_NONE                     = 0x00,
};

enum
{
    WPTS_ALERT_MODE_TRANSITION_0s   = 0x00,
    WPTS_ALERT_MODE_TRANSITION_2s   = 0x01,
    WPTS_ALERT_MODE_TRANSITION_3s   = 0x02,
    WPTS_ALERT_MODE_TRANSITION_6s   = 0x03,
};

///////////////////////// PRU Alert //////////////////////////

///////////////////// PTU Static Value ///////////////////////
/// Optional fields validity
enum
{
    WPTC_STATIC_MAX_IMPEDANCE   = 0x80,
    WPTC_STATIC_MAX_RESISTANCE  = 0x40,
};

/// PTU Class
enum
{
    WPTC_CLASS_1 = 0,
    WPTC_CLASS_2,
    WPTC_CLASS_3,
    WPTC_CLASS_4,
    WPTC_CLASS_5,
    WPTC_CLASS_6,
    WPTC_CLASS_END,
};

#define WPTC_STATIC_IMP_RESIS_SHIFT 3

/// PTU Max Source Impedance
enum
{
    // A4WP doc 6.5.4.5 - 3bits shift (RFU)
    WPTC_STATIC_MAX_SRC_IMPEDANCE_50        = 0,
    WPTC_STATIC_MAX_SRC_IMPEDANCE_60        = 1 << WPTC_STATIC_IMP_RESIS_SHIFT,
    WPTC_STATIC_MAX_SRC_IMPEDANCE_70        = 2 << WPTC_STATIC_IMP_RESIS_SHIFT,
    WPTC_STATIC_MAX_SRC_IMPEDANCE_80        = 3 << WPTC_STATIC_IMP_RESIS_SHIFT,
    WPTC_STATIC_MAX_SRC_IMPEDANCE_90        = 4 << WPTC_STATIC_IMP_RESIS_SHIFT,
    WPTC_STATIC_MAX_SRC_IMPEDANCE_100       = 5 << WPTC_STATIC_IMP_RESIS_SHIFT,
    WPTC_STATIC_MAX_SRC_IMPEDANCE_110       = 6 << WPTC_STATIC_IMP_RESIS_SHIFT,
    WPTC_STATIC_MAX_SRC_IMPEDANCE_120       = 7 << WPTC_STATIC_IMP_RESIS_SHIFT,
    WPTC_STATIC_MAX_SRC_IMPEDANCE_130       = 8 << WPTC_STATIC_IMP_RESIS_SHIFT,
    WPTC_STATIC_MAX_SRC_IMPEDANCE_140       = 9 << WPTC_STATIC_IMP_RESIS_SHIFT,
    WPTC_STATIC_MAX_SRC_IMPEDANCE_150       = 10 << WPTC_STATIC_IMP_RESIS_SHIFT,
    WPTC_STATIC_MAX_SRC_IMPEDANCE_175       = 11 << WPTC_STATIC_IMP_RESIS_SHIFT,
    WPTC_STATIC_MAX_SRC_IMPEDANCE_200       = 12 << WPTC_STATIC_IMP_RESIS_SHIFT,
    WPTC_STATIC_MAX_SRC_IMPEDANCE_225       = 13 << WPTC_STATIC_IMP_RESIS_SHIFT,
    WPTC_STATIC_MAX_SRC_IMPEDANCE_250       = 14 << WPTC_STATIC_IMP_RESIS_SHIFT,
    WPTC_STATIC_MAX_SRC_IMPEDANCE_275       = 15 << WPTC_STATIC_IMP_RESIS_SHIFT,
    WPTC_STATIC_MAX_SRC_IMPEDANCE_300       = 16 << WPTC_STATIC_IMP_RESIS_SHIFT,
    WPTC_STATIC_MAX_SRC_IMPEDANCE_350       = 17 << WPTC_STATIC_IMP_RESIS_SHIFT,
    WPTC_STATIC_MAX_SRC_IMPEDANCE_375       = 18 << WPTC_STATIC_IMP_RESIS_SHIFT,
    WPTC_STATIC_MAX_SRC_IMPEDANCE_RFU_19    = 19 << WPTC_STATIC_IMP_RESIS_SHIFT,
    WPTC_STATIC_MAX_SRC_IMPEDANCE_RFU_31    = 31 << WPTC_STATIC_IMP_RESIS_SHIFT,
    WPTC_STATIC_MAX_SRC_IMPEDANCE_END       = 32 << WPTC_STATIC_IMP_RESIS_SHIFT,
};

// it's recommended to use WPTC_GET_MAX_SRC_IMPEDANCE macro to get a value
extern const uint16_t ptu_max_src_imp[WPTC_STATIC_MAX_SRC_IMPEDANCE_END >> WPTC_STATIC_IMP_RESIS_SHIFT];
#define WPTC_GET_MAX_SRC_IMPEDANCE(A)       ptu_max_src_imp[A >> WPTC_STATIC_IMP_RESIS_SHIFT]

/// PTU Max Load Resistance
enum
{
    // A4WP doc 6.5.4.6 - 3bits shift (RFU)
    WPTC_STATIC_MAX_LOAD_RESISTANCE_5       = 0,
    WPTC_STATIC_MAX_LOAD_RESISTANCE_10      = 1 << WPTC_STATIC_IMP_RESIS_SHIFT,
    WPTC_STATIC_MAX_LOAD_RESISTANCE_15      = 2 << WPTC_STATIC_IMP_RESIS_SHIFT,
    WPTC_STATIC_MAX_LOAD_RESISTANCE_20      = 3 << WPTC_STATIC_IMP_RESIS_SHIFT,
    WPTC_STATIC_MAX_LOAD_RESISTANCE_25      = 4 << WPTC_STATIC_IMP_RESIS_SHIFT,
    WPTC_STATIC_MAX_LOAD_RESISTANCE_30      = 5 << WPTC_STATIC_IMP_RESIS_SHIFT,
    WPTC_STATIC_MAX_LOAD_RESISTANCE_35      = 6 << WPTC_STATIC_IMP_RESIS_SHIFT,
    WPTC_STATIC_MAX_LOAD_RESISTANCE_40      = 7 << WPTC_STATIC_IMP_RESIS_SHIFT,
    WPTC_STATIC_MAX_LOAD_RESISTANCE_45      = 8 << WPTC_STATIC_IMP_RESIS_SHIFT,
    WPTC_STATIC_MAX_LOAD_RESISTANCE_50      = 9 << WPTC_STATIC_IMP_RESIS_SHIFT,
    WPTC_STATIC_MAX_LOAD_RESISTANCE_55      = 10 << WPTC_STATIC_IMP_RESIS_SHIFT,
    WPTC_STATIC_MAX_LOAD_RESISTANCE_RFU_11  = 11 << WPTC_STATIC_IMP_RESIS_SHIFT,
    WPTC_STATIC_MAX_LOAD_RESISTANCE_RFU_31  = 31 << WPTC_STATIC_IMP_RESIS_SHIFT,
    WPTC_STATIC_MAX_LOAD_RESISTANCE_END     = 32 << WPTC_STATIC_IMP_RESIS_SHIFT,
};

// it's recommended to use WPTC_GET_MAX_LOAD_RESISTANCE macro to get a value
extern const uint8_t ptu_max_load_resistance[WPTC_STATIC_MAX_LOAD_RESISTANCE_END >> WPTC_STATIC_IMP_RESIS_SHIFT];
#define WPTC_GET_MAX_LOAD_RESISTANCE(A)     ptu_max_load_resistance[A >> WPTC_STATIC_IMP_RESIS_SHIFT]


/// PTU Number of Devices
#define WPTC_MAX_SUPPORTED_DEV 8
#define WPTC_GET_SUPPORTED_DEV_NUMBER(A)    (A < WPTC_MAX_SUPPORTED_DEV) ? A + 1 : 0

///////////////////// PTU Static Value ///////////////////////

////////////////////// PTU/PRU Common ////////////////////////
/// PTU/PRU Protocol Revision
enum
{
    WPT_PROTOCOL_A4WP_1 = 0,
    WPT_PROTOCOL_END,
};
////////////////////// PTU/PRU Common ////////////////////////

///////////////////// PRU ADV PAYLOAD ///////
enum
{
    WPT_ADV_PAYLOAD_IMP_SHIFT_NONE       = 0x00,
    WPT_ADV_PAYLOAD_TIME_SET_SUPPORTED   = 0x04,
    WPT_ADV_PAYLOAD_OVP_SUPPORTED        = 0x08,
    WPT_ADV_PAYLOAD_REBOOT_WITH_NO_RESET = 0x10,
    WPT_ADV_PAYLOAD_IMP_SHIFT_CAT1       = 0x20,
    WPT_ADV_PAYLOAD_IMP_SHIFT_CAT2       = 0x40,
    WPT_ADV_PAYLOAD_IMP_SHIFT_CAT3       = 0x60,
    WPT_ADV_PAYLOAD_IMP_SHIFT_CAT4       = 0x80,
    WPT_ADV_PAYLOAD_IMP_SHIFT_CAT5       = 0xA0,
    WPT_ADV_PAYLOAD_IMP_SHIFT_RFU1       = 0xA0,
    WPT_ADV_PAYLOAD_IMP_SHIFT_RFU2       = 0xE0,
};

enum
{
    WPT_ADV_PAYLOAD_RSSI_GAIN_MASK = 0x07,
    WPT_ADV_PAYLOAD_RSSI_PWR_MASK = 0xF8,
};

/*
 * @brief PRU Dynamic Parameter Value Parameter definition
 */
#pragma pack(push)
#pragma pack(1)
struct pru_dynamic_value {
    uint8_t opt_fields_validity;
    uint16_t v_rect;
    uint16_t i_rect;
    uint16_t v_out;
    uint16_t i_out;
    uint8_t temperature;
    uint16_t v_rect_min_dyn;
    uint16_t v_rect_set_dyn;
    uint16_t v_rect_high_dyn;
    uint8_t pru_alert;

    // 3 bytes RFU
    uint8_t rfu_1;
    uint8_t rfu_2;
    uint8_t rfu_3;
};
#pragma pack(pop)

/*
 * @brief PTU Static Parameter Value definition
 */

#pragma pack(push)
#pragma pack(1)
struct ptu_static_value {
    /// Defines which defined fields are valid
    uint8_t opt_fields_validity;
    /// Power of PTU
    uint8_t ptu_power;
    /// Maximum source impedance of the PTU
    uint8_t max_source_impedance;
    /// Maximum load resistance of the PTU
    uint8_t max_load_resistance;
    /// RFU field
    uint16_t rfu_1;
    /// PTU Class
    uint8_t ptu_class;
    /// Revision of the PRU hardware
    uint8_t hardware_rev;
    /// Revision of the PRU firmware
    uint8_t firmware_rev;
    /// A4WP supported revision
    uint8_t protocol_rev;
    /// Max number of supported devices
    uint8_t num_devices_supported;

    // 6 bytes RFU
    uint16_t rfu_2;
    uint16_t rfu_3;
    uint16_t rfu_4;
};
#pragma pack(pop)

/*
 * @brief PRU Static Parameter Value definition
 */
#pragma pack(push)
#pragma pack(1)
struct pru_static_value {
    /// Defined which optional fields are valid
    uint8_t opt_fields_validity;
    /// A4WP supported revision
    uint8_t protocol_rev;
    /// Reserved for future use
    uint8_t rfu_1;
    /// Category of PRU
    uint8_t category;
    /// Capabilities of PRU (bit field)
    uint8_t information;
    /// Revision of the PRU hardware
    uint8_t hardware_rev;
    /// Revision of the PRU firmware
    uint8_t firmware_rev;
    /// P_RECT_MAX of the PRU (mW * 100)
    uint8_t p_rect_max;
    /// V_RECT_MIN (static, first estimate) (mV)
    uint16_t v_rect_min_static;
    /// V_RECT_HIGH (static, first estimate) (mV)
    uint16_t v_rect_high_static;
    /// V_RECT_SET (mV)
    uint16_t v_rect_set;
    /// Delta R1 caused by PRU (.01 ohms)
    uint16_t v_delta_r1_value;

    /// Reserved for future use
    uint16_t rfu_2;
    uint16_t rfu_3;
};
#pragma pack(pop)

/*
 * @brief PRU Control Parameter Value definition
 */
#pragma pack(push)
#pragma pack(1)
struct pru_control_value {
    uint8_t enables; // PTU Turn on, PTU on indication etc
    uint8_t permission; // PRU is permitted in PTU
    uint8_t time_set; // PTU sets up time

    // 2 bytes RFU
    uint16_t rfu;
};
#pragma pack(pop)

/*
 * @brief PRU Alert Characteristic Value definition (7 bytes)
 */
struct pru_alert_value {
    uint8_t pru_alert; // bit field for notifications
    uint8_t device_address[WPTS_BD_ADDR_LEN]; // address used for reconnection
                                                 // after PRU reboot
};


#endif //BLE_WPTS || BLE_WPT_CLIENT

#endif // WPT_COMMON_H_
