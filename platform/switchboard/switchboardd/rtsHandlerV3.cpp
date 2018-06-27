/**
 * File: rtsHandlerV3.cpp
 *
 * Author: paluri
 * Created: 6/27/2018
 *
 * Description: V3 of BLE Protocol
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "switchboardd/rtsHandlerV3.h"

namespace Anki {
namespace Switchboard {

using namespace Anki::Cozmo::ExternalComms;

RtsHandlerV3::RtsHandlerV3(INetworkStream* stream, 
    struct ev_loop* evloop,
    std::shared_ptr<EngineMessagingClient> engineClient,
    bool isPairing,
    bool isOtaUpdating) :
_stream(stream),
_loop(evloop),
_engineClient(engineClient),
_isPairing(isPairing),
_isOtaUpdating(isOtaUpdating)
{
  (void)_stream;
  (void)_loop;
  (void)_isPairing;
  (void)_isOtaUpdating;

  // Initialize the key exchange object
  _keyExchange = std::make_unique<KeyExchange>(kNumPinDigits);
}

bool RtsHandlerV3::StartRts() {
  SendPublicKey();
  
  return true;
}

void RtsHandlerV3::SendPublicKey() {
  if(!AssertState(RtsCommsType::Unencrypted)) {
    return;
  }

  // Generate public, private key
  (void)LoadKeys();

  std::array<uint8_t, crypto_kx_PUBLICKEYBYTES> publicKeyArray;
  std::copy((uint8_t*)&_rtsKeys.keys.id.publicKey,
            (uint8_t*)&_rtsKeys.keys.id.publicKey + crypto_kx_PUBLICKEYBYTES,
            publicKeyArray.begin());

  SendRtsMessage<RtsConnRequest>(publicKeyArray);

  // Save public key to file
  Log::Write("Sending public key to client.");
}


} // Switchboard
} // Anki
