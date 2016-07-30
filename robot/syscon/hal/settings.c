/**
 ** These are the constant settings for configuring the soft device for BLE
 ** They are placed here because struct initalizers are nicer in C than C++
 ** Sorry bout it.
 **/

#include "ble_advdata.h"
#include "ble_hci.h"
#include "conn_params.h"
#include "ble_stack_handler_types.h"
#include "timer.h"

#include "ble_settings.h"

// These are hooks we use for BLE callback
void on_conn_params_evt(ble_conn_params_evt_t * p_evt);
void conn_params_error_handler(uint32_t nrf_error);

const ble_uuid128_t COZMO_UUID_BASE         = {{ 0xB3, 0xBA, 0xE5, 0x2B, 0x57, 0x51, 0xAC, 0x8A, 0x5E, 0x40, 0xF1, 0x5D, 0x00, 0x00, 0x3D, 0x76 }};
const uint16_t COZMO_UUID_SERVICE           = 0xbeef;
const uint16_t COZMO_UUID_RECEIVE_CHAR      = 0xbee1;
const uint16_t COZMO_UUID_TRANSMIT_CHAR     = 0xbee0;

#define MFG_DATA_ID 0xefbe

// Gap connection parameters
const ble_gap_conn_params_t gap_conn_params = {
  .min_conn_interval = MSEC_TO_UNITS(100, UNIT_1_25_MS),
  .max_conn_interval = MSEC_TO_UNITS(200, UNIT_1_25_MS),
  .slave_latency     = 0,
  .conn_sup_timeout  = MSEC_TO_UNITS(4000, UNIT_10_MS)
};

// Advertising parameters
const ble_gap_adv_params_t adv_params = {
  .type        = BLE_GAP_ADV_TYPE_ADV_IND,
  .p_peer_addr = NULL,
  .fp          = BLE_GAP_ADV_FP_ANY,
  .interval    = BLE_GAP_ADV_INTERVAL_MIN,
  .timeout     = 0
};

// Connection constants for RTOS
const ble_conn_params_init_t cp_init = {
  .p_conn_params                  = NULL,
  .first_conn_params_update_delay = CYCLES_MS(20000),
  .next_conn_params_update_delay  = CYCLES_MS(5000),
  .max_conn_params_update_count   = 3,
  .start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID,
  .disconnect_on_fail             = false,
  .evt_handler                    = on_conn_params_evt,
  .error_handler                  = conn_params_error_handler
};

// Security paramters
const ble_gap_sec_params_t m_sec_params = {
  .bond         = 1,
  .mitm         = 0,
  .io_caps      = BLE_GAP_IO_CAPS_NONE,
  .oob          = 0,
  .min_key_size = 7,
  .max_key_size = 16
};

const uint8_t DEVICE_NAME[] = "Cozmo";
const int DEVICE_NAME_LENGTH = 5;

// Advertising settings
// NOTE: THIS IS NOT CONSTANT SO I CAN COPY SOME REGISTER VALUES IN AT RUNTIME
ManufacturerData  manif_data = {
  .revision = 0 /// TODO plumb something in here
};


const ble_advdata_manuf_data_t m_manuf_data = {
  .company_identifier = MFG_DATA_ID,
  .data.size = sizeof(manif_data),
  .data.p_data = (uint8_t*)&manif_data
};

const ble_advdata_t m_advdata = {
  .name_type               = BLE_ADVDATA_FULL_NAME,
  .include_appearance      = true,
  .flags                   = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE,
  .p_manuf_specific_data   = (ble_advdata_manuf_data_t*)&m_manuf_data
};
