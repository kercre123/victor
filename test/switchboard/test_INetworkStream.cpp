#include "test.h"
#include "ev++.h"
#include "test_INetworkStream.h"

namespace Anki {
namespace Switchboard {

Test_INetworkStream::Test_INetworkStream() :
_state(MessageState::TestRaw),
_handShake(false),
_connected(false),
_securePairing(nullptr),
_keyExchange(nullptr) {
  _cladHandler = new CladHandler();
  _cladHandler->OnReceiveRtsMessage().SubscribeForever(std::bind(&Test_INetworkStream::ReceiveMessage, this, std::placeholders::_1));
}

Test_INetworkStream::~Test_INetworkStream() {
  delete _cladHandler;

  if(_keyExchange != nullptr) {
    delete _keyExchange;
  }
}

void Test_INetworkStream::ReceiveMessage(const Anki::Cozmo::ExternalComms::RtsConnection_2& msg) {
  _queue.push(msg);
}

int Test_INetworkStream::SendPlainText(uint8_t* bytes, int length) {
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

int Test_INetworkStream::SendEncrypted(uint8_t* bytes, int length) {
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

void Test_INetworkStream::Update() {
  // instance tick

  if(_handShake) {
    _state = MessageState::TestClad;
    _handShake = false;
    
    // send handshake
    uint8_t bytes[5] = {0};
    bytes[0] = SetupMessage::MSG_HANDSHAKE;
    *(uint32_t*)(bytes + 1) = SB_PAIRING_PROTOCOL_VERSION;

    // Receive Handshake Message
    ReceivePlainText(bytes, sizeof(bytes));
    return;
  }

  while(_queue.size() > 0) {
    Anki::Cozmo::ExternalComms::RtsConnection_2 msg = _queue.front();
    _queue.pop();

    switch(msg.GetTag()) {
      case Anki::Cozmo::ExternalComms::RtsConnection_2Tag::RtsConnRequest: {
        Anki::Cozmo::ExternalComms::RtsConnRequest m = msg.Get_RtsConnRequest();
        const uint8_t pinDigits = 6;
        _keyExchange = new KeyExchange(pinDigits);
        _keyExchange->SetRemotePublicKey((const uint8_t*)m.publicKey.begin());
        uint8_t* publicKeyPtr = _keyExchange->GenerateKeys();
        std::array<uint8_t, crypto_kx_PUBLICKEYBYTES> publicKey;
        std::copy(publicKeyPtr, 
                  publicKeyPtr + crypto_kx_PUBLICKEYBYTES, 
                  publicKey.begin());

        SendRtsMessage<Cozmo::ExternalComms::RtsConnResponse>
          (Cozmo::ExternalComms::RtsConnType::FirstTimePair, publicKey);
        break;
      }
      case Anki::Cozmo::ExternalComms::RtsConnection_2Tag::RtsNonceMessage: {
        Anki::Cozmo::ExternalComms::RtsNonceMessage m = msg.Get_RtsNonceMessage();

        _keyExchange->CalculateSharedKeysClient((const uint8_t*)_securePairing->GetPin().c_str());

        std::copy(m.toRobotNonce.begin(), 
                  m.toRobotNonce.end(), 
                  _EncryptNonce);

        std::copy(m.toDeviceNonce.begin(), 
                  m.toDeviceNonce.end(), 
                  _DecryptNonce);

        std::copy(_keyExchange->GetEncryptKey(), 
                  _keyExchange->GetEncryptKey() + crypto_kx_SESSIONKEYBYTES, 
                  _EncryptKey);

        std::copy(_keyExchange->GetDecryptKey(), 
                  _keyExchange->GetDecryptKey() + crypto_kx_SESSIONKEYBYTES, 
                  _DecryptKey);

        SendRtsMessage<Cozmo::ExternalComms::RtsAck>
          ((uint8_t)Anki::Cozmo::ExternalComms::RtsConnection_2Tag::RtsNonceMessage);

          _state = MessageState::TestSecure;
        break;
      }
      case Anki::Cozmo::ExternalComms::RtsConnection_2Tag::RtsChallengeMessage: {
        Anki::Cozmo::ExternalComms::RtsChallengeMessage m = msg.Get_RtsChallengeMessage();

        uint32_t challenge = m.number + 1;
        SendRtsMessage<Cozmo::ExternalComms::RtsChallengeMessage>(challenge);
        break;
      }
      case Anki::Cozmo::ExternalComms::RtsConnection_2Tag::RtsChallengeSuccessMessage: {
        _connected = true;
        break;
      }
      default:
        break;
    }
  }
}

//
//
//
int Test_INetworkStream::ClientEncrypt(uint8_t* buffer, int length, uint8_t* output, uint64_t* outputLength) {  
  // encrypt message
  int result = crypto_aead_xchacha20poly1305_ietf_encrypt(output, outputLength, buffer, length, nullptr, 0, nullptr, _EncryptNonce, _EncryptKey);
  
  if(result == 0) {
    // increment nonce if successful (don't let attacker derail our comms)
    sodium_increment(_EncryptNonce, crypto_aead_xchacha20poly1305_ietf_NPUBBYTES);
  }
  
  return result;
}

int Test_INetworkStream::ClientDecrypt(uint8_t* buffer, int length, uint8_t* output, uint64_t* outputLength) {
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

void Tick(struct ev_loop* loop, struct ev_timer* w, int revents) {  
  Test_INetworkStream* stream = (Test_INetworkStream*)w->data;

  if(stream) {
    stream->Update();   
  }
  
  // break out condition
  if(false) ev_break(ev_default_loop(0), EVBREAK_ONE);
}

void Timeout(struct ev_loop* loop, struct ev_timer* w, int revents) {  
  Test_INetworkStream* stream = (Test_INetworkStream*)w->data;
  
  // break out condition
  if(!stream->Connected()) {
    ASSERT(false, "Could not make successful encrypted connection.");
  } 
  
  ev_break(ev_default_loop(0), EVBREAK_ONE);
}

void Test_INetworkStream::Test(SecurePairing* securePairing) {
  ev_timer t;
  t.data = this;
  ev_timer_init(&t, Tick, 0.1f, 0.1f);
  ev_timer_start(ev_default_loop(0), &t);

  _securePairing = securePairing;
  _securePairing->BeginPairing();
  _securePairing->SetIsPairing(true);

  ev_timer timeout;
  timeout.data = this;
  ev_timer_init(&timeout, Timeout, 3, 0);
  ev_timer_start(ev_default_loop(0), &timeout);

  // start loop
  ev_loop(ev_default_loop(0), 0);
}

template<typename T, typename... Args>
void Test_INetworkStream::SendRtsMessage(Args&&... args) {
  Anki::Cozmo::ExternalComms::ExternalComms msg = Anki::Cozmo::ExternalComms::ExternalComms(
    Anki::Cozmo::ExternalComms::RtsConnection(Anki::Cozmo::ExternalComms::RtsConnection_2(T(std::forward<Args>(args)...))));
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