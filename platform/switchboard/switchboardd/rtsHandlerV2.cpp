/**
 * File: rtsHandlerV2.cpp
 *
 * Author: paluri
 * Created: 6/27/2018
 *
 * Description: V2 of BLE Protocol
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "switchboardd/rtsHandlerV2.h"

namespace Anki {
namespace Switchboard {

bool RtsHandlerV2::StartRts() {
  return true;
}

void RtsHandlerV2::StopPairing() {

}

void RtsHandlerV2::SendOtaProgress(int status, uint64_t progress, uint64_t expectedTotal) {

}

void RtsHandlerV2::HandleTimeout() {
  
}

} // Switchboard
} // Anki