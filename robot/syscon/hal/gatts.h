#ifndef __GATTS_H
#define __GATTS_H

#include <stdint.h>
#include "ble.h"

static const ble_uuid128_t DRIVE_UUID_BASE = {{0xF4, 0x8D, 0x4D, 0x9C, 0xD8, 0x0B, 0x81, 0x83, 0x7E, 0x40, 0x86, 0x61, 0x00, 0x00, 0x15, 0xBE}};
static const uint16_t DRIVE_UUID_SERVICE = 0xbeef;
static const uint16_t DRIVE_UUID_RECEIVE_CHAR = 0xbee1;
static const uint16_t DRIVE_UUID_TRANSMIT_CHAR = 0xbee0;

static const uint16_t MFG_DATA_ID        = 0xefbe;
static const uint8_t  MFG_DATA[]          = {0xff, 0xff, 0x00, 0x01, 0x00, 0x01, 0xFF, 0xFF}; // This is actually garbage

struct drive_receive {
  uint8_t data[20];
};

struct drive_send {
  uint8_t data[20];
};

namespace GATTS {
  extern uint16_t                    service_handle;
  extern ble_gatts_char_handles_t    receive_handles;
  extern ble_gatts_char_handles_t    transmit_handles;
  extern uint8_t                     uuid_type;
  extern uint16_t                    conn_handle;
  
  uint32_t init();
  void on_ble_evt(ble_evt_t * p_ble_evt);
  uint32_t transmit(drive_send* send);
}

#endif
