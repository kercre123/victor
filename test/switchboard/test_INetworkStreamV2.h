#pragma once

#include "switchboardd/INetworkStream.h"
#include "test_CladHandler.h"
#include "switchboardd/pairingMessages.h"
#include "switchboardd/keyExchange.h"
#include "switchboardd/rtsComms.h"

#include <queue>

namespace Anki {
namespace Switchboard {

class Test_INetworkStream : public INetworkStream {
public:
  explicit Test_INetworkStream();
  virtual ~Test_INetworkStream();

  int SendPlainText(uint8_t* bytes, int length) __attribute__((warn_unused_result));
  int SendEncrypted(uint8_t* bytes, int length) __attribute__((warn_unused_result));

  bool Connected() {
    return _connected;
  }

  void Test(RtsComms* securePairing);
  void Update();

private:
  MessageState _state;
  CladHandler* _cladHandler;
  bool _handShake;
  bool _connected;
  std::queue<Anki::Vector::ExternalComms::RtsConnection_2> _queue;
  RtsComms* _securePairing;
  KeyExchange* _keyExchange;

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

  void ReceiveMessage(const Anki::Vector::ExternalComms::RtsConnection_2& msg);
};

} // Switchboard
} // Anki