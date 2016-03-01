#include <string.h>
#include <stdint.h>

extern "C" {
  #include "ble.h"
}

#include "gatts.h"

uint16_t                    GATTS::service_handle;
ble_gatts_char_handles_t    GATTS::receive_handles;
ble_gatts_char_handles_t    GATTS::transmit_handles;
uint8_t                     GATTS::uuid_type;
uint16_t                    GATTS::conn_handle;

static void on_write(drive_receive* receive)
{
  // TODO: HANDLE DATA RECEIVE HERE
}

uint32_t GATTS::transmit(drive_send* send)
{
  if (conn_handle == BLE_CONN_HANDLE_INVALID) {
    return NRF_ERROR_INVALID_STATE;
  }

  ble_gatts_hvx_params_t params;
  uint16_t len = sizeof(drive_send);
  
  memset(&params, 0, sizeof(params));
  params.type = BLE_GATT_HVX_NOTIFICATION;
  params.handle = transmit_handles.value_handle;
  params.p_data = (uint8_t*) send;
  params.p_len = &len;
  
  return sd_ble_gatts_hvx(conn_handle, &params);
}

void GATTS::on_ble_evt(ble_evt_t * p_ble_evt)
{
  switch (p_ble_evt->header.evt_id)
  {
    case BLE_GAP_EVT_CONNECTED:
      conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
      break;
      
    case BLE_GAP_EVT_DISCONNECTED:
      conn_handle = BLE_CONN_HANDLE_INVALID;
      break;
      
    case BLE_GATTS_EVT_WRITE:
    {
      ble_gatts_evt_write_t * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;

      if ((p_evt_write->handle == GATTS::receive_handles.value_handle) &&
          (p_evt_write->len == sizeof(drive_receive)))
      {
        on_write((drive_receive*) p_evt_write->data);
      }      
      break;
    }
    
    default:
      // No implementation needed.
      break;
  }
}

static uint32_t receive_char_add()
{
  ble_gatts_char_md_t char_md;
  ble_gatts_attr_t    attr_char_value;
  ble_uuid_t          ble_uuid;
  ble_gatts_attr_md_t attr_md;

  memset(&char_md, 0, sizeof(char_md));
  
  char_md.char_props.read   = 1;
  char_md.char_props.write  = 1;
  char_md.p_char_user_desc  = NULL;
  char_md.p_char_pf         = NULL;
  char_md.p_user_desc_md    = NULL;
  char_md.p_cccd_md         = NULL;
  char_md.p_sccd_md         = NULL;
  
  ble_uuid.type = GATTS::uuid_type;
  ble_uuid.uuid = DRIVE_UUID_RECEIVE_CHAR;
  
  memset(&attr_md, 0, sizeof(attr_md));

  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);
  attr_md.vloc       = BLE_GATTS_VLOC_STACK;
  attr_md.rd_auth    = 0;
  attr_md.wr_auth    = 0;
  attr_md.vlen       = 0;
  
  memset(&attr_char_value, 0, sizeof(attr_char_value));

  attr_char_value.p_uuid       = &ble_uuid;
  attr_char_value.p_attr_md    = &attr_md;
  attr_char_value.init_len     = sizeof(drive_receive);
  attr_char_value.init_offs    = 0;
  attr_char_value.max_len      = sizeof(drive_receive);
  attr_char_value.p_value      = NULL;
  
  return sd_ble_gatts_characteristic_add(GATTS::service_handle, &char_md,
                                            &attr_char_value,
                                            &GATTS::receive_handles);
}

static uint32_t transmit_char_add()
{
  ble_gatts_char_md_t char_md;
  ble_gatts_attr_md_t cccd_md;
  ble_gatts_attr_t    attr_char_value;
  ble_uuid_t          ble_uuid;
  ble_gatts_attr_md_t attr_md;

  memset(&cccd_md, 0, sizeof(cccd_md));

  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);
  cccd_md.vloc = BLE_GATTS_VLOC_STACK;
  
  memset(&char_md, 0, sizeof(char_md));
  
  char_md.char_props.read   = 1;
  char_md.char_props.notify = 1;
  char_md.p_char_user_desc  = NULL;
  char_md.p_char_pf         = NULL;
  char_md.p_user_desc_md    = NULL;
  char_md.p_cccd_md         = &cccd_md;
  char_md.p_sccd_md         = NULL;
  
  ble_uuid.type = GATTS::uuid_type;
  ble_uuid.uuid = DRIVE_UUID_TRANSMIT_CHAR;
  
  memset(&attr_md, 0, sizeof(attr_md));

  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
  BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&attr_md.write_perm);
  attr_md.vloc       = BLE_GATTS_VLOC_STACK;
  attr_md.rd_auth    = 0;
  attr_md.wr_auth    = 0;
  attr_md.vlen       = 0;
  
  memset(&attr_char_value, 0, sizeof(attr_char_value));

  attr_char_value.p_uuid       = &ble_uuid;
  attr_char_value.p_attr_md    = &attr_md;
  attr_char_value.init_len     = sizeof(drive_send);
  attr_char_value.init_offs    = 0;
  attr_char_value.max_len      = sizeof(drive_send);
  attr_char_value.p_value      = (uint8_t*)&"Cozmo says hello... ";//NULL;
  
  return sd_ble_gatts_characteristic_add(GATTS::service_handle, &char_md,
                                          &attr_char_value,
                                          &GATTS::transmit_handles);
}

uint32_t GATTS::init()
{
  uint32_t   err_code;
  ble_uuid_t ble_uuid;

  // Initialize service structure
  conn_handle       = BLE_CONN_HANDLE_INVALID;
  
  // Add service
  err_code = sd_ble_uuid_vs_add(&DRIVE_UUID_BASE, &uuid_type);
  if (err_code != NRF_SUCCESS)
  {
    return err_code;
  }
  
  ble_uuid.type = uuid_type;
  ble_uuid.uuid = DRIVE_UUID_SERVICE;

  err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &service_handle);
  if (err_code != NRF_SUCCESS)
  {
    return err_code;
  }
  
  err_code = receive_char_add();
  if (err_code != NRF_SUCCESS)
  {
    return err_code;
  }
  
  err_code = transmit_char_add();
  if (err_code != NRF_SUCCESS)
  {
    return err_code;
  }
  
  return NRF_SUCCESS;
}
