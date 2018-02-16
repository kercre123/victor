/**
 * File: INetworkStream.h
 *
 * Author: paluri
 * Created: 1/16/2018
 *
 * Description: Interface for network streams.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef IAnkiNetworkStream_h
#define IAnkiNetworkStream_h

#include "../signals/simpleSignal.hpp"
#include "../sodium/sodium.h"
#include "log.h"

namespace Anki {
namespace Switchboard {
  enum NetworkResult : int8_t {
    MsgSuccess = 0,
    MsgFailure = -1,
  };
  
  class INetworkStream {
  public:
    virtual int SendPlainText(uint8_t* bytes, int length) __attribute__((warn_unused_result)) = 0;
    virtual int SendEncrypted(uint8_t* bytes, int length) __attribute__((warn_unused_result)) = 0;
    
    // Message Receive Signal
    using ReceivedSignal = Signal::Signal<void (uint8_t*, int)>;
    using NotificationSignal = Signal::Signal<void ()>;
    
    // Message Receive Event
    ReceivedSignal& OnReceivedPlainTextEvent() {
      return _receivedPlainTextSignal;
    }
    
    ReceivedSignal& OnReceivedEncryptedEvent() {
      return _receivedEncryptedSignal;
    }
    
    NotificationSignal& OnFailedDecryptionEvent() {
      return _failedDecryptionSignal;
    }
    
    void ClearCryptoKeys() {
      memset(_EncryptKey, 0, crypto_kx_SESSIONKEYBYTES);
      memset(_DecryptKey, 0, crypto_kx_SESSIONKEYBYTES);
    }
    
    void SetCryptoKeys(uint8_t* encryptKey, uint8_t* decryptKey) {
      memcpy(_EncryptKey, encryptKey, crypto_kx_SESSIONKEYBYTES);
      memcpy(_DecryptKey, decryptKey, crypto_kx_SESSIONKEYBYTES);
    }
    
    void SetNonce(uint8_t* nonce) {
      memcpy(_EncryptNonce, nonce, crypto_aead_xchacha20poly1305_ietf_NPUBBYTES);
      memcpy(_DecryptNonce, nonce, crypto_aead_xchacha20poly1305_ietf_NPUBBYTES);
    }
    
    void SetEncryptedChannelEstablished(bool canCommunicateSecurely) {
      _encryptedChannelEstablished = canCommunicateSecurely;
    }
    
    virtual void ReceivePlainText(uint8_t* bytes, int length)
    {
      if(length == 0) {
        return;
      }
      
      _receivedPlainTextSignal.emit(bytes, length);
    }
    
    virtual void ReceiveEncrypted(uint8_t* bytes, int length)
    {
      if(length == 0) {
        return;
      }
      
      uint8_t* buffer = (uint8_t*)malloc(length);
      
      // insert decrypted chunk into new buffer
      uint64_t msgLength = 0;
      int decryptStatus = Decrypt(bytes, length, buffer, &msgLength);
      
      if(decryptStatus == ENCRYPTION_SUCCESS) {
        _receivedEncryptedSignal.emit(buffer, (int)msgLength);
      }
      
      free(buffer);
    }
    
  protected:
    int Encrypt(uint8_t* buffer, int length, uint8_t* output, uint64_t* outputLength) {
      // check if state is good
      if(!_encryptedChannelEstablished) {
        Log::Write("Tried to send message before ready.");
        return -1;
      }
      
      // encrypt message
      int result = crypto_aead_xchacha20poly1305_ietf_encrypt(output, outputLength, buffer, length, nullptr, 0, nullptr, _EncryptNonce, _EncryptKey);
      
      if(result == ENCRYPTION_SUCCESS) {
        // increment nonce if successful (don't let attacker derail our comms)
        sodium_increment(_EncryptNonce, crypto_aead_xchacha20poly1305_ietf_NPUBBYTES);
      }
      
      return result;
    }
    
    const char ENCRYPTION_SUCCESS = 0;
    
  private:
    ReceivedSignal _receivedPlainTextSignal;
    ReceivedSignal _receivedEncryptedSignal;
    NotificationSignal _failedDecryptionSignal;
    
    std::vector<uint8_t*> _buffers;
    std::vector<int> _bufferSizes;
  
    bool _encryptedChannelEstablished = false;
    int _buffersTotalSize = 0;
    uint8_t _DecryptKey[crypto_kx_SESSIONKEYBYTES];
    uint8_t _EncryptKey[crypto_kx_SESSIONKEYBYTES];
    uint8_t _DecryptNonce[crypto_aead_xchacha20poly1305_ietf_NPUBBYTES];
    uint8_t _EncryptNonce[crypto_aead_xchacha20poly1305_ietf_NPUBBYTES];
    
    int Decrypt(uint8_t* buffer, int length, uint8_t* output, uint64_t* outputLength) {
      // check if state is good
      if(!_encryptedChannelEstablished) {
        Log::Write("Tried to receive message before ready.");
        return -1;
      }
      
      // decrypt message
      int result = crypto_aead_xchacha20poly1305_ietf_decrypt(output, outputLength, nullptr, buffer, length, nullptr, 0, _DecryptNonce, _DecryptKey);
      
      if(result == ENCRYPTION_SUCCESS) {
        // increment nonce if successful (don't let attacker derail our comms)
        sodium_increment(_DecryptNonce, crypto_aead_xchacha20poly1305_ietf_NPUBBYTES);
      } else {
        _failedDecryptionSignal.emit();
      }
      
      return result;
    }
  };
} // Switchboard
} // Anki

#endif /* IAnkiNetworkStream_h */
