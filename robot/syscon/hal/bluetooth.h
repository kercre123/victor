#ifndef __BLE_H
#define __BLE_H

extern "C"
{
  #include "nrf.h"
  #include "nrf_sdm.h"
  #include "app_error.h"
  #include "ble_settings.h"
}

#include "tasks.h"

#include "clad/robotInterface/messageEngineToRobot.h"

enum BLEError {
  BLE_ERROR_NONE,
  BLE_ERROR_BUFFER_UNDERFLOW,
  BLE_ERROR_MESSAGE_ENCRYPTION_WRONG,
  BLE_ERROR_BAD_FRAMING,
  BLE_ERROR_AUTHENTICATED_FAILED,
  BLE_ERROR_NOT_AUTHENTICATED,
  BLE_ERROR_BUFFER_OVERFLOW
};

static const int COZMO_FRAME_DATA_LENGTH = AES_KEY_LENGTH;

enum CozmoFrameFlags {
  START_OF_MESSAGE = 0x01,
  END_OF_MESSAGE = 0x02,
  MESSAGE_ENCRYPTED = 0x04
};

struct CozmoFrame {
  uint8_t message[COZMO_FRAME_DATA_LENGTH];
  uint8_t flags;
};

namespace Bluetooth {
  extern uint16_t                   conn_handle;
  extern uint16_t                   service_handle;
  extern ble_gatts_char_handles_t   receive_handles;
  extern ble_gatts_char_handles_t   transmit_handles;
  extern uint8_t                    uuid_type;

  uint32_t init();
  void shutdown(void);
  void advertise(void);
  bool enabled(void);
  void manage(void);
  
  bool transmit(const uint8_t* data, int length, uint8_t id);

  // These are message handlers
  void enterPairing(const Anki::Cozmo::EnterPairing& msg);
  void diffieHellmanResults(const Anki::Cozmo::DiffieHellmanResults& msg);
};

#endif
