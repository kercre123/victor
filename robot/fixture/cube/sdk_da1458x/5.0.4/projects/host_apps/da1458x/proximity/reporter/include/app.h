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
#include "gap_task.h" 
#include "proxr_task.h" 
/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <stdint.h>            // standard integer


/*
 * TYPE DEFINITIONS
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

#define APP_DFLT_ADV_DATA        "\x07\x03\x03\x18\x02\x18\x04\x18"
#define APP_DFLT_ADV_DATA_LEN    (8)

#define APP_SCNRSP_DATA         "\x09\xFF\x00\x60\x52\x57\x2D\x42\x4C\x45"
#define APP_SCNRSP_DATA_LENGTH  (10)

#define APP_DFLT_DEVICE_NAME            ("FE_PROXR")

typedef struct 
{
    unsigned char  free;
    struct bd_addr adv_addr;
    unsigned short conhdl;
    unsigned char idx;
    unsigned char  rssi;
    unsigned char  data_len;
    unsigned char  data[ADV_DATA_LEN + 1];

}ble_dev;

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
}proxr_dev;


/// application environment structure
struct app_env_tag
{
    unsigned char state;
    unsigned char num_of_devices;
    ble_dev devices[MAX_SCAN_DEVICES];
    proxr_dev proxr_device;
    unsigned char slave_on_sleep;
    unsigned short size_tx_queue;
    unsigned short size_rx_queue;
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
}app_alert_state;


enum
{
    /// Idle state
    APP_IDLE,
    /// Scanning state
    APP_CONNECTABLE,
    /// Connected state
    APP_CONNECTED,
    /// Number of defined states.
    APP_STATE_MAX,
    /// Scanning state
    APP_SCAN,
};

enum
{
    /// Slave On Sleep
    SLAVE_UNAVAILABLE,
    /// Slave Active
    SLAVE_AVAILABLE,
};

/*
 * DEFINES
 ****************************************************************************************
 */

/// Application environment
extern struct app_env_tag app_env;
extern app_alert_state alert_state;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialize the BLE demo application.
 *
 ****************************************************************************************
 */

void app_init(void);

void app_set_mode(void);

void app_proxr_db_create(void);

void app_proxr_enable(void);

void app_inq(void);

void app_adv_start(void);

void app_connect(unsigned char indx);

void app_connect_confirm(uint8_t auth);

void app_disconnect(void);

unsigned char app_device_recorded(struct bd_addr *addr);

void app_security_enable(void);

void app_gap_bond_cfm(struct gapc_bond_req_ind *param);

void app_start_encryption(void);

void app_read_rem_fetures(void);

void app_rst_gap(void);

bool bdaddr_compare(struct bd_addr *bd_address1,
                    struct bd_addr *bd_address2);

void app_send_disconnect(uint16_t dst, uint16_t conhdl, uint8_t reason);
/// @} APP

#endif // APP_H_
