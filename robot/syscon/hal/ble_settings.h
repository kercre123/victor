#ifndef __BLE_SETTINGS_H
#define __BLE_SETTINGS_H

#include <stdint.h>

#include "ble.h"
#include "ble_advdata.h"
#include "ble_hci.h"
#include "conn_params.h"
#include "ble_stack_handler_types.h"

// These are constants for the system
#define IS_SRVC_CHANGED_CHARACT_PRESENT 0

extern const ble_uuid128_t COZMO_UUID_BASE;
extern const uint16_t COZMO_UUID_SERVICE;
extern const uint16_t COZMO_UUID_RECEIVE_CHAR;
extern const uint16_t COZMO_UUID_TRANSMIT_CHAR;

extern const uint16_t MFG_DATA_ID;

extern const uint8_t DEVICE_NAME[];
extern const int DEVICE_NAME_LENGTH;

extern const ble_gap_adv_params_t adv_params;
extern const ble_gap_conn_params_t gap_conn_params;
extern const ble_conn_params_init_t cp_init;
extern const ble_gap_sec_params_t m_sec_params;
extern const ble_advdata_t m_advdata;

#endif
