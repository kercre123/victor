#ifndef __BLE_H
#define __BLE_H

extern "C"
{
  #include "nrf.h"
  #include "nrf_sdm.h"
  #include "app_error.h"
  #include "ble_settings.h"
}

#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/robotInterface/bleMessages.h"

namespace Bluetooth {
  extern uint16_t                   conn_handle;
  extern uint16_t                   service_handle;
  extern ble_gatts_char_handles_t   receive_handles;
  extern ble_gatts_char_handles_t   transmit_handles;
  extern uint8_t                    uuid_type;

  uint32_t init();
  void shutdown(void);
  void advertise(void);
  void disconnect(uint32_t reason);
  bool enabled(void);
  void manage(void);
  
  void sendFrame(const Anki::Cozmo::BLE::Frame*);
};

#endif
