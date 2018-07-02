/**
 * File: rtsHandlerV2.h
 *
 * Author: paluri
 * Created: 6/27/2018
 *
 * Description: V2 of BLE Protocol
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#pragma once

#include "switchboardd/IRtsHandler.h"

namespace Anki {
namespace Switchboard {

class RtsHandlerV2 : public IRtsHandler {
public:
  bool StartRts();
  void StopPairing();
  void SendOtaProgress(int status, uint64_t progress, uint64_t expectedTotal);
  void HandleTimeout();
};

} // Switchboard
} // Anki