#pragma once

#include "switchboardd/INetworkStream.h"
#include "test_CladHandlerV4.h"
#include "switchboardd/pairingMessages.h"
#include "switchboardd/keyExchange.h"
#include "switchboardd/rtsComms.h"

#include <queue>

namespace Anki {
namespace Switchboard {

class Test_INetworkStreamV4 : public INetworkStream {
public:
  explicit Test_INetworkStreamV4();
  virtual ~Test_INetworkStreamV4();

  int SendPlainText(uint8_t* bytes, int length) __attribute__((warn_unused_result));
  int SendEncrypted(uint8_t* bytes, int length) __attribute__((warn_unused_result));

  bool Connected() {
    return _connected;
  }

  void Test(RtsComms* securePairing);
  void Update();

private:
  MessageState _state;
  CladHandlerV4* _cladHandler;
  bool _handShake;
  bool _connected;
  std::queue<Anki::Vector::ExternalComms::RtsConnection_4> _queue;
  RtsComms* _securePairing;
  KeyExchange* _keyExchange;
  bool _reconnect = false;

  //
  int ClientEncrypt(uint8_t* buffer, int length, uint8_t* output, uint64_t* outputLength);
  int ClientDecrypt(uint8_t* buffer, int length, uint8_t* output, uint64_t* outputLength);

  // keys
  uint8_t _DecryptKey[crypto_kx_SESSIONKEYBYTES];
  uint8_t _EncryptKey[crypto_kx_SESSIONKEYBYTES];
  uint8_t _DecryptNonce[crypto_aead_xchacha20poly1305_ietf_NPUBBYTES];
  uint8_t _EncryptNonce[crypto_aead_xchacha20poly1305_ietf_NPUBBYTES];

  // Template Methods
  template<typename T, typename... Args>
  void SendRtsMessage(Args&&... args);

  void ReceiveMessage(const Anki::Vector::ExternalComms::RtsConnection_4& msg);
};

} // Switchboard
} // Anki