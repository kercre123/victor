#include "test.h"
#include "ev++.h"
#include "test_INetworkStreamV3.h"

namespace Anki {
namespace Switchboard {

Test_INetworkStreamV3::Test_INetworkStreamV3() :
_state(MessageState::TestRaw),
_handShake(false),
_connected(false),
_securePairing(nullptr),
_keyExchange(nullptr),
_reconnect(false) {
  _cladHandler = new CladHandlerV3();
  _cladHandler->OnReceiveRtsMessage().SubscribeForever(std::bind(&Test_INetworkStreamV3::ReceiveMessage, this, std::placeholders::_1));
}

Test_INetworkStreamV3::~Test_INetworkStreamV3() {
  delete _cladHandler;

  if(_keyExchange != nullptr) {
    delete _keyExchange;
  }
}

void Test_INetworkStreamV3::ReceiveMessage(const Anki::Vector::ExternalComms::RtsConnection_3& msg) {
  _queue.push(msg);
}

int Test_INetworkStreamV3::SendPlainText(uint8_t* bytes, int length) {
  // Received a message
  if(_state == MessageState::TestRaw) {
    if((SetupMessage)bytes[0] == SetupMessage::MSG_HANDSHAKE) {
      ASSERT(length == (sizeof(uint32_t) + 1), "Handshake message length bad.")
        
      uint32_t hostVersion = *(uint32_t*)(bytes + 1);
      ASSERT((hostVersion == SB_PAIRING_PROTOCOL_VERSION), "Host version wrong.");

      _handShake = true;
    }
  } else if(_state == MessageState::TestClad) {
    _cladHandler->ReceiveExternalCommsMsg(bytes, length);
  }

  return 0;
}

int Test_INetworkStreamV3::SendEncrypted(uint8_t* bytes, int length) {
  uint8_t outBytes[length + 16];
  uint64_t outLength = 0;
  Encrypt(bytes, length, outBytes, &outLength);

  uint8_t decryptedBytes[outLength];
  uint64_t decryptedLength = 0;

  int status = ClientDecrypt(outBytes, (int)outLength, decryptedBytes, &decryptedLength);

  ASSERT(status==0, "Client could not decrypt message.");

  // Received a secure message
  _cladHandler->ReceiveExternalCommsMsg(decryptedBytes, (size_t)decryptedLength);

  return 0;
}

void Test_INetworkStreamV3::Update() {
  // instance tick

  if(_handShake) {
    _state = MessageState::TestClad;
    _handShake = false;
    
    // send handshake
    uint8_t bytes[5] = {0};
    bytes[0] = SetupMessage::MSG_HANDSHAKE;
    *(uint32_t*)(bytes + 1) = V3;

    // Receive Handshake Message
    ReceivePlainText(bytes, sizeof(bytes));
    return;
  }

  while(_queue.size() > 0) {
    Anki::Vector::ExternalComms::RtsConnection_3 msg = _queue.front();
    _queue.pop();

    switch(msg.GetTag()) {
      case Anki::Vector::ExternalComms::RtsConnection_3Tag::RtsConnRequest: {
        Anki::Vector::ExternalComms::RtsConnRequest m = msg.Get_RtsConnRequest();
        const uint8_t pinDigits = 6;
        if(!_reconnect) {
          _keyExchange = new KeyExchange(pinDigits);
        }
        _keyExchange->SetRemotePublicKey((const uint8_t*)m.publicKey.begin());
        uint8_t* publicKeyPtr;
        
        if(_reconnect) {
          publicKeyPtr = _keyExchange->GetPublicKey();
        } else {
          publicKeyPtr = _keyExchange->GenerateKeys();
        }
      
        std::array<uint8_t, crypto_kx_PUBLICKEYBYTES> publicKey;
        std::copy(publicKeyPtr, 
                  publicKeyPtr + crypto_kx_PUBLICKEYBYTES, 
                  publicKey.begin());

        if(_reconnect) {
          SendRtsMessage<Vector::ExternalComms::RtsConnResponse>
            (Vector::ExternalComms::RtsConnType::Reconnection, publicKey);
        } else {
          SendRtsMessage<Vector::ExternalComms::RtsConnResponse>
            (Vector::ExternalComms::RtsConnType::FirstTimePair, publicKey);
        }
        break;
      }
      case Anki::Vector::ExternalComms::RtsConnection_3Tag::RtsNonceMessage: {
        Anki::Vector::ExternalComms::RtsNonceMessage m = msg.Get_RtsNonceMessage();

        _keyExchange->CalculateSharedKeysClient((const uint8_t*)_securePairing->GetPin().c_str());

        std::copy(m.toRobotNonce.begin(), 
                  m.toRobotNonce.end(), 
                  _EncryptNonce);

        std::copy(m.toDeviceNonce.begin(), 
                  m.toDeviceNonce.end(), 
                  _DecryptNonce);

        if(!_reconnect) {
          std::copy(_keyExchange->GetEncryptKey(), 
                    _keyExchange->GetEncryptKey() + crypto_kx_SESSIONKEYBYTES, 
                    _EncryptKey);

          std::copy(_keyExchange->GetDecryptKey(), 
                    _keyExchange->GetDecryptKey() + crypto_kx_SESSIONKEYBYTES, 
                    _DecryptKey);
        }

        SendRtsMessage<Vector::ExternalComms::RtsAck>
          ((uint8_t)Anki::Vector::ExternalComms::RtsConnection_3Tag::RtsNonceMessage);

          _state = MessageState::TestSecure;
        break;
      }
      case Anki::Vector::ExternalComms::RtsConnection_3Tag::RtsChallengeMessage: {
        Anki::Vector::ExternalComms::RtsChallengeMessage m = msg.Get_RtsChallengeMessage();

        uint32_t challenge = m.number + 1;
        SendRtsMessage<Vector::ExternalComms::RtsChallengeMessage>(challenge);
        break;
      }
      case Anki::Vector::ExternalComms::RtsConnection_3Tag::RtsChallengeSuccessMessage: {
        if(_reconnect) {
          Log::Write("Reconnection worked!");
        }
        _connected = true;
        break;
      }
      default:
        break;
    }
  }

  if(_connected && !_reconnect) {
    _state = MessageState::TestRaw;
    _handShake = true;
    _reconnect = true;
    delete _securePairing;
    std::shared_ptr<TaskExecutor> taskExecutor = std::make_shared<Anki::TaskExecutor>(ev_default_loop(0));
    Anki::Wifi::Initialize(taskExecutor);
    std::shared_ptr<WifiWatcher> wifiWatcher = std::make_shared<WifiWatcher>(ev_default_loop(0));

    _securePairing = new RtsComms(
      this,                 // 
      ev_default_loop(0),   // ev loop
      nullptr,              // engineClient (don't need--only for updating face)
      nullptr,
      nullptr,              // tokenClient
      nullptr,
      wifiWatcher,
      taskExecutor,
      false,                // is pairing
      false,                // is ota-ing
      false);               // has cloud owner
    _securePairing->BeginPairing();
  }
}

//
//
//
int Test_INetworkStreamV3::ClientEncrypt(uint8_t* buffer, int length, uint8_t* output, uint64_t* outputLength) {  
  // encrypt message
  int result = crypto_aead_xchacha20poly1305_ietf_encrypt(output, outputLength, buffer, length, nullptr, 0, nullptr, _EncryptNonce, _EncryptKey);
  
  if(result == 0) {
    // increment nonce if successful (don't let attacker derail our comms)
    sodium_increment(_EncryptNonce, crypto_aead_xchacha20poly1305_ietf_NPUBBYTES);
  }
  
  return result;
}

int Test_INetworkStreamV3::ClientDecrypt(uint8_t* buffer, int length, uint8_t* output, uint64_t* outputLength) {
  // decrypt message
  int result = crypto_aead_xchacha20poly1305_ietf_decrypt(output, outputLength, nullptr, buffer, length, nullptr, 0, _DecryptNonce, _DecryptKey);
  
  if(result == 0) {
    // increment nonce if successful (don't let attacker derail our comms)
    sodium_increment(_DecryptNonce, crypto_aead_xchacha20poly1305_ietf_NPUBBYTES);
  }
  
  return result;
}

//
//
//

void TickV3(struct ev_loop* loop, struct ev_timer* w, int revents) {  
  Test_INetworkStreamV3* stream = (Test_INetworkStreamV3*)w->data;

  if(stream) {
    stream->Update();   
  }
  
  // break out condition
  if(false) ev_break(ev_default_loop(0), EVBREAK_ONE);
}

void TimeoutV3(struct ev_loop* loop, struct ev_timer* w, int revents) {  
  Test_INetworkStreamV3* stream = (Test_INetworkStreamV3*)w->data;
  
  // break out condition
  if(!stream->Connected()) {
    ASSERT(false, "Could not make successful encrypted connection.");
  } 
  
  ev_break(ev_default_loop(0), EVBREAK_ONE);
}

void Test_INetworkStreamV3::Test(RtsComms* securePairing) {
  ev_timer t;
  t.data = this;
  ev_timer_init(&t, TickV3, 0.1f, 0.1f);
  ev_timer_start(ev_default_loop(0), &t);

  _securePairing = securePairing;
  _securePairing->BeginPairing();
  _securePairing->SetIsPairing(true);

  ev_timer timeout;
  timeout.data = this;
  ev_timer_init(&timeout, TimeoutV3, 3, 0);
  ev_timer_start(ev_default_loop(0), &timeout);

  // start loop
  ev_loop(ev_default_loop(0), 0);
}

template<typename T, typename... Args>
void Test_INetworkStreamV3::SendRtsMessage(Args&&... args) {
  Anki::Vector::ExternalComms::ExternalComms msg = Anki::Vector::ExternalComms::ExternalComms(
    Anki::Vector::ExternalComms::RtsConnection(Anki::Vector::ExternalComms::RtsConnection_3(T(std::forward<Args>(args)...))));
  std::vector<uint8_t> messageData(msg.Size());
  const size_t packedSize = msg.Pack(messageData.data(), msg.Size());

  if(_state == MessageState::TestClad) {
    return ReceivePlainText(messageData.data(), packedSize);
  } else if(_state == MessageState::TestSecure) {
    uint8_t encryptedBuffer[packedSize + 16];
    uint64_t encryptedSize = 0;

    ClientEncrypt(messageData.data(), packedSize, encryptedBuffer, &encryptedSize);
      
    return ReceiveEncrypted(encryptedBuffer, (int)encryptedSize);
  } else {
    Log::Write("Tried to send clad message when state was already set back to RAW.");
  }
}

} // Switchboard
} // Anki