#include <string.h>

extern "C"
{
  #include "nrf.h"
  #include "nrf_sdm.h"

  #include "ble_advdata.h"
  #include "ble_hci.h"
  #include "ble_conn_params.h"

  #include "softdevice_handler.h"  
}

#include "gatts.h"

#include "rtos.h"
#include "battery.h"
#include "bluetooth.h"
#include "motors.h"

// These are constants for the system
#define IS_SRVC_CHANGED_CHARACT_PRESENT 0

const char DEVICE_NAME[]                    = "Botvac D80";

const uint16_t MIN_CONN_INTERVAL            = MSEC_TO_UNITS(100, UNIT_1_25_MS);
const uint16_t MAX_CONN_INTERVAL            = MSEC_TO_UNITS(200, UNIT_1_25_MS);
const uint16_t SLAVE_LATENCY                = 0;
const uint16_t CONN_SUP_TIMEOUT             = MSEC_TO_UNITS(4000, UNIT_10_MS);

// Advertising settings
const uint16_t APP_ADV_INTERVAL             = 40;
const uint16_t APP_ADV_TIMEOUT_IN_SECONDS   = 180;

// Connection constants for RTOS
const uint32_t FIRST_CONN_PARAMS_UPDATE_DELAY = CYCLES_MS(20000);
const uint32_t NEXT_CONN_PARAMS_UPDATE_DELAY  = CYCLES_MS(5000);
const uint8_t MAX_CONN_PARAMS_UPDATE_COUNT    = 3;

// Security paramters
const uint8_t SEC_PARAM_BOND                = 1;
const uint8_t SEC_PARAM_MITM                = 0;
const uint8_t SEC_PARAM_IO_CAPABILITIES     = BLE_GAP_IO_CAPS_NONE;
const uint8_t SEC_PARAM_OOB                 = 0;
const uint8_t SEC_PARAM_MIN_KEY_SIZE        = 7;
const uint8_t SEC_PARAM_MAX_KEY_SIZE        = 16;

// Various variables for tracking junk
static uint16_t                         m_conn_handle = BLE_CONN_HANDLE_INVALID;
static ble_gap_sec_params_t             m_sec_params;
static ble_advdata_manuf_data_t         m_manuf_data;

void app_error_handler(uint32_t error_code, uint32_t line_num, const uint8_t * p_file_name)
{
  NVIC_SystemReset();
}

static void advertising_start(void)
{
  uint32_t             err_code;
  ble_gap_adv_params_t adv_params;

  // Start advertising
  memset(&adv_params, 0, sizeof(adv_params));

  adv_params.type        = BLE_GAP_ADV_TYPE_ADV_IND;
  adv_params.p_peer_addr = NULL;
  adv_params.fp          = BLE_GAP_ADV_FP_ANY;
  adv_params.interval    = APP_ADV_INTERVAL;
  adv_params.timeout     = APP_ADV_TIMEOUT_IN_SECONDS;

  err_code = sd_ble_gap_adv_start(&adv_params);
  APP_ERROR_CHECK(err_code);
}

static void on_ble_evt(ble_evt_t * p_ble_evt)
{
  uint32_t                    err_code;
  static ble_gap_master_id_t  p_master_id;
  static ble_gap_sec_keyset_t keys_exchanged;

  switch (p_ble_evt->header.evt_id)
  {
    case BLE_GAP_EVT_CONNECTED:
      m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
      break;

    case BLE_GAP_EVT_DISCONNECTED:
      m_conn_handle = BLE_CONN_HANDLE_INVALID;

      advertising_start();
      break;

    case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
      err_code = sd_ble_gap_sec_params_reply(m_conn_handle,
                                     BLE_GAP_SEC_STATUS_SUCCESS,
                                     &m_sec_params,&keys_exchanged);
      APP_ERROR_CHECK(err_code);
      break;

    case BLE_GATTS_EVT_SYS_ATTR_MISSING:
      err_code = sd_ble_gatts_sys_attr_set(m_conn_handle, NULL, 0,BLE_GATTS_SYS_ATTR_FLAG_USR_SRVCS);
      APP_ERROR_CHECK(err_code);
      break;

    case BLE_GAP_EVT_SEC_INFO_REQUEST:
      //p_enc_info = keys_exchanged.keys_central.p_enc_key

      if (p_master_id.ediv == p_ble_evt->evt.gap_evt.params.sec_info_request.master_id.ediv)
      {
        err_code = sd_ble_gap_sec_info_reply(m_conn_handle, &keys_exchanged.keys_central.p_enc_key->enc_info, &keys_exchanged.keys_central.p_id_key->id_info, NULL);
        APP_ERROR_CHECK(err_code);
        p_master_id.ediv = p_ble_evt->evt.gap_evt.params.sec_info_request.master_id.ediv;
      }
      else
      {
        // No keys found for this device
        err_code = sd_ble_gap_sec_info_reply(m_conn_handle, NULL, NULL,NULL);
        APP_ERROR_CHECK(err_code);
      }
      break;

    case BLE_GAP_EVT_TIMEOUT:
      if (p_ble_evt->evt.gap_evt.params.timeout.src == BLE_GAP_TIMEOUT_SRC_ADVERTISING)
      {
        // XXX: Go to system-off mode (this function will not return; wakeup will cause a reset)                
        
        //err_code = sd_power_system_off();
        //APP_ERROR_CHECK(err_code);
      }
      break;

    default:
      // No implementation needed.
      break;
  }
}

static void on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
  uint32_t err_code;

  if(p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
  {
    err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
    APP_ERROR_CHECK(err_code);
  }
}

static void ble_evt_dispatch(ble_evt_t * p_ble_evt)
{
  on_ble_evt(p_ble_evt);
  ble_conn_params_on_ble_evt(p_ble_evt);
  GATTS::on_ble_evt(p_ble_evt);
}

static void sys_evt_dispatch(uint32_t sys_evt)
{
  // TODO: SYSTEM EVENTS
}

static void conn_params_error_handler(uint32_t nrf_error)
{
  APP_ERROR_HANDLER(nrf_error);
}

static void ble_stack_init(void)
{
  uint32_t err_code;
  
  // Enable BLE stack 
  ble_enable_params_t ble_enable_params;
  memset(&ble_enable_params, 0, sizeof(ble_enable_params));
  ble_enable_params.gatts_enable_params.service_changed = IS_SRVC_CHANGED_CHARACT_PRESENT;
  err_code = sd_ble_enable(&ble_enable_params);
  APP_ERROR_CHECK(err_code);

  ble_gap_addr_t addr;
  
  err_code = sd_ble_gap_address_get(&addr);
  APP_ERROR_CHECK(err_code);
  sd_ble_gap_address_set(BLE_GAP_ADDR_CYCLE_MODE_NONE, &addr);
  APP_ERROR_CHECK(err_code);
  // Subscribe for BLE events.
  err_code = softdevice_ble_evt_handler_set(ble_evt_dispatch);
  APP_ERROR_CHECK(err_code);
  
  // Register with the SoftDevice handler module for BLE events.
  err_code = softdevice_sys_evt_handler_set(sys_evt_dispatch);
  APP_ERROR_CHECK(err_code);
}

static void gap_params_init(void)
{
  uint32_t                err_code;
  ble_gap_conn_params_t   gap_conn_params;
  ble_gap_conn_sec_mode_t sec_mode;

  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

  err_code = sd_ble_gap_device_name_set(&sec_mode,
                                        (const uint8_t *)DEVICE_NAME,
                                        sizeof(DEVICE_NAME) - 1);
  APP_ERROR_CHECK(err_code);

  memset(&gap_conn_params, 0, sizeof(gap_conn_params));

  gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
  gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
  gap_conn_params.slave_latency     = SLAVE_LATENCY;
  gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

  err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
  APP_ERROR_CHECK(err_code);
}

static void services_init(void)
{
  uint32_t err_code;

  err_code = GATTS::init();
  APP_ERROR_CHECK(err_code);
}

static void advertising_init(void)
{
  uint32_t      err_code;
  ble_advdata_t advdata;
  ble_advdata_t scanrsp;
  
  ble_uuid_t adv_uuids[] = {
    {DRIVE_UUID_SERVICE, GATTS::uuid_type}
  };

  m_manuf_data.company_identifier = MFG_DATA_ID;
  m_manuf_data.data.size = sizeof(MFG_DATA);
  m_manuf_data.data.p_data = (uint8_t*)MFG_DATA;

  // Build and set advertising data
  memset(&advdata, 0, sizeof(advdata));

  advdata.name_type               = BLE_ADVDATA_FULL_NAME;
  advdata.include_appearance      = true;
  advdata.flags                   = BLE_GAP_ADV_FLAGS_LE_ONLY_LIMITED_DISC_MODE;
  advdata.p_manuf_specific_data   = &m_manuf_data;

  memset(&scanrsp, 0, sizeof(scanrsp));
  scanrsp.uuids_complete.uuid_cnt = sizeof(adv_uuids) / sizeof(ble_uuid_t);
  scanrsp.uuids_complete.p_uuids  = adv_uuids;
  
  err_code = ble_advdata_set(&advdata, &scanrsp);
  APP_ERROR_CHECK(err_code);
}

static void conn_params_init(void)
{
  uint32_t               err_code;
  ble_conn_params_init_t cp_init;

  memset(&cp_init, 0, sizeof(cp_init));

  cp_init.p_conn_params                  = NULL;
  cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
  cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
  cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
  cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
  cp_init.disconnect_on_fail             = false;
  cp_init.evt_handler                    = on_conn_params_evt;
  cp_init.error_handler                  = conn_params_error_handler;

  err_code = ble_conn_params_init(&cp_init);
  APP_ERROR_CHECK(err_code);
}

static void sec_params_init(void)
{
  m_sec_params.bond         = SEC_PARAM_BOND;
  m_sec_params.mitm         = SEC_PARAM_MITM;
  m_sec_params.io_caps      = SEC_PARAM_IO_CAPABILITIES;
  m_sec_params.oob          = SEC_PARAM_OOB;
  m_sec_params.min_key_size = SEC_PARAM_MIN_KEY_SIZE;
  m_sec_params.max_key_size = SEC_PARAM_MAX_KEY_SIZE;
}

void Bluetooth::init(void) {
}

void Bluetooth::advertise(void) {
  ble_stack_init();
  gap_params_init();
  services_init();
  advertising_init();
  conn_params_init();
  sec_params_init();

  advertising_start();
}

void Bluetooth::shutdown(void) {
  // Make sure the soft device is disabled so we can do things
  sd_softdevice_disable();
}
