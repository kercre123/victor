#include "switchboardd/rtsHandlerV3.h"

bool RtsHandlerV3::StartRts() {
  if(!AssertState(CommsState::Clad)) {
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
  
  return true;
}
