/**
 ****************************************************************************************
 *
 * @file app.h
 *
 * @brief Header file Main application.
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

#ifndef APP_H_
#define APP_H_

#include "gapc_task.h"
#include "co_bt.h"
#include "smpc_task.h"
#include "ble_580_sw_version.h"

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <stdint.h>            // standard integer


/*
 * DEFINES
 ****************************************************************************************
 */

#define MAX_SCAN_DEVICES 9   

/// Local address type
#define APP_ADDR_TYPE     0
/// Advertising channel map
#define APP_ADV_CHMAP     0x07
/// Advertising filter policy
#define APP_ADV_POL       0
/// Advertising minimum interval
#define APP_ADV_INT_MIN   0x800
/// Advertising maximum interval
#define APP_ADV_INT_MAX   0x800
/// Advertising data maximal length
#define APP_ADV_DATA_MAX_SIZE           (ADV_DATA_LEN - 3)
/// Scan Response data maximal length
#define APP_SCAN_RESP_DATA_MAX_SIZE     (SCAN_RSP_DATA_LEN)
#if (BLE_SPOTA_RECEIVER)
    #define APP_DFLT_ADV_DATA        "\x09\x03\x03\x18\x02\x18\x04\x18\xF5\xFE"
    #define APP_DFLT_ADV_DATA_LEN    (8+2)
#else
    #define APP_DFLT_ADV_DATA        "\x07\x03\x03\x18\x02\x18\x04\x18"
    #define APP_DFLT_ADV_DATA_LEN    (8)
#endif
#define APP_SCNRSP_DATA         "\x09\xFF\x00\x60\x52\x57\x2D\x42\x4C\x45"
#define APP_SCNRSP_DATA_LENGTH  (10)
#define APP_DFLT_DEVICE_NAME    ("FE_PROXR")

/*
 * DISS DEFINITIONS
 ****************************************************************************************
 */
/// Manufacturer Name (up to 18 chars)
#define APP_DIS_MANUFACTURER_NAME       ("Dialog Semi")
#define APP_DIS_MANUFACTURER_NAME_LEN   (11)
/// Model Number String (up to 18 chars)
#define APP_DIS_MODEL_NB_STR            ("DA14580")
#define APP_DIS_MODEL_NB_STR_LEN        (7)
/// System ID - LSB -> MSB (FIXME)
#define APP_DIS_SYSTEM_ID               ("\x12\x34\x56\xFF\xFE\x9A\xBC\xDE")
#define APP_DIS_SYSTEM_ID_LEN           (8)

#define APP_DIS_SW_REV                  ""
#define APP_DIS_FIRM_REV                DA14580_SW_VERSION
#define APP_DIS_FEATURES                (DIS_MANUFACTURER_NAME_CHAR_SUP | DIS_MODEL_NB_STR_CHAR_SUP | DIS_SYSTEM_ID_CHAR_SUP | DIS_SW_REV_STR_CHAR_SUP | DIS_FIRM_REV_STR_CHAR_SUP | DIS_PNP_ID_CHAR_SUP)

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

typedef struct 
{
    unsigned char  free;
    struct bd_addr adv_addr;
    unsigned short conidx;
    unsigned short conhdl;
    unsigned char idx;
    unsigned char  rssi;
    unsigned char  data_len;
    unsigned char  data[ADV_DATA_LEN + 1];

} ble_dev;

//Proximity Reporter connected device
typedef struct 
{
    ble_dev device;
    unsigned char bonded;
    struct gapc_ltk ltk;
    struct gapc_irk irk;
    struct gap_sec_key csrk;
    unsigned char llv;
    char txp;
    unsigned char rssi;
    unsigned char alert;

} proxr_dev;

/// application environment structure
struct app_env_tag
{
    unsigned char state;
    unsigned char num_of_devices;
    ble_dev devices[MAX_SCAN_DEVICES];
    proxr_dev proxr_device;
};

struct app_env_tag_adv
{
    /// Connection handle
    uint16_t conhdl;

    ///  Advertising data
    uint8_t app_adv_data[32];
    ///  Advertising data length
    uint8_t app_adv_data_length;
    /// Scan response data
    uint8_t app_scanrsp_data[32];
    /// Scan response data length
    uint8_t app_scanrsp_data_length;
};
extern struct app_env_tag_adv app_env_adv;

typedef struct
{
    uint32_t blink_timeout;
    uint8_t blink_toggle;
    uint8_t lvl;
    uint8_t ll_alert_lvl;
    int8_t  txp_lvl;
    uint8_t adv_toggle; 

} app_alert_state;


/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

/// Application environment
extern struct app_env_tag app_env;
extern app_alert_state alert_state;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/*
 ****************************************************************************************
 * @brief Exit application.
 *
 * @return void.
 ****************************************************************************************
*/
void app_exit(void);
/**
 ****************************************************************************************
 * @brief Set Bondabe mode.
 *
 * @return void.
 ****************************************************************************************
 */
void app_set_mode(void);
/**
 ****************************************************************************************
 * @brief Send Reset request to GAPM task.
 *
 * @return void.
 ****************************************************************************************
 */
void app_rst_gap(void);

/**
 ****************************************************************************************
 * @brief Send Inquiry (devices discovery) request to GAPM task.
 *
 * @return void.
 ****************************************************************************************
 */
void app_inq(void);
/**
 ****************************************************************************************
 * @brief Send Start Advertising command to GAPM task.
 *
 * @return void.
 ****************************************************************************************
 */
void app_adv_start(void);
/**
 ****************************************************************************************
 * @brief Send Connect request to GAPM task.
 *
 * @param[in] indx  Peer device's index in discovered devices list.
 *
 * @return void.
 ****************************************************************************************
 */
void app_connect(unsigned char indx);

/**
 ****************************************************************************************
 * @brief Send the GAPC_DISCONNECT_IND message to a task.
 *
 * @param[in] dst     Task id of the destination task.
 * @param[in] conhdl  The conhdl parameter of the GAPC_DISCONNECT_IND message.
 * @param[in] reason  The reason parameter of the GAPC_DISCONNECT_IND message.
 *
 * @return void.
 ****************************************************************************************
 */
void app_send_disconnect(uint16_t dst, uint16_t conhdl, uint8_t reason);

/**
 ****************************************************************************************
 * @brief Send Read rssi request to GAPC task instance for the current connection.
 *
 * @return void.
 ****************************************************************************************
 */
void app_read_rssi(void);
/**
 ****************************************************************************************
 * @brief Send disconnect request to GAPC task instance for the current connection.
 *
 * @return void.
 ****************************************************************************************
 */
void app_disconnect(void);
/**
 ****************************************************************************************
 * @brief Send pairing request.
 *
 * @return void.
 ****************************************************************************************
 */
void app_security_enable(void);
/**
 ****************************************************************************************
 * @brief Send connection confirmation.
 *
 * param[in] auth  Authentication requirements.
 *
 * @return void.
 ****************************************************************************************
 */
void app_connect_confirm(uint8_t auth);
/**
 ****************************************************************************************
 * @brief generate csrk key.
 *
 * @return void.
 ****************************************************************************************
 */
void app_gen_csrk(void);
/**
 ****************************************************************************************
 * @brief generate ltk key.
 *
 * @return void.
 ****************************************************************************************
 */
void app_sec_gen_ltk(uint8_t key_size);

/**
 ****************************************************************************************
 * @brief Generate Temporary Key.
 *
 * @return temporary key value
 ****************************************************************************************
 */
uint32_t app_gen_tk(void);
/**
 ****************************************************************************************
 * @brief Confirm bonding.
 *
 *  param[in] param  The parameters of GAPC_BOND_REQ_IND to be confirmed.
 *.
 * @return void.
 ****************************************************************************************
 */
void app_gap_bond_cfm(struct gapc_bond_req_ind *param);
/**
 ****************************************************************************************
 * @brief Start Encryption with preeagreed key.
 *
 * @return void.
 ****************************************************************************************
 */
void app_start_encryption(void);
/**
 ****************************************************************************************
 * @brief Send enable request to proximity reporter profile task.
 *
 * @return void.
 ****************************************************************************************
 */
void app_proxr_enable(void);
/**
 ****************************************************************************************
 * @brief Send enable request to DISS profile task.
 *
 * @return void.
 ****************************************************************************************
 */
void app_dis_enable(void);
/**
 ****************************************************************************************
 * @brief bd adresses compare.
 *
 *  @param[in] bd_address1  Pointer to bd_address 1.
 *  @param[in] bd_address2  Pointer to bd_address 2.
 *
 * @return true if adresses are equal / false if not.
 ****************************************************************************************
 */
bool bdaddr_compare(struct bd_addr *bd_address1,
                    struct bd_addr *bd_address2);
/**
 ****************************************************************************************
 * @brief Check if device is in application's descovered devices list.
 *
 *  @param[in] padv_addr  Pointer to devices bd_addr.
 *
 * @return Index in list. if return value equals MAX_SCAN_DEVICES device is not listed.
 ****************************************************************************************
 */
unsigned char app_device_recorded(struct bd_addr *padv_addr);

/**
 ****************************************************************************************
 * @brief Send enable request to proximity monitor profile task.
 *
 * @return void.
 ****************************************************************************************
 */
void app_proxr_db_create(void);
/**
 ****************************************************************************************
 * @brief Send enable request to DISS profile task.
 *
 * @return void.
 ****************************************************************************************
 */
void app_diss_db_create(void);
/**
 ****************************************************************************************
 * @brief Handles commands sent from console interface.
 *
 *
 * @return void.
 ****************************************************************************************
 */
void ConsoleEvent(void);
/**
 ****************************************************************************************
 * @brief Initialization of application environment
 *
 ****************************************************************************************
 */
void app_env_init(void);

/// @} APP

#endif // APP_H_
