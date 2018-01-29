//
//  IAnkiNetworkStream.h
//  ios-server
//
//  Created by Paul Aluri on 1/16/18.
//  Copyright © 2018 Paul Aluri. All rights reserved.
//

#ifndef IAnkiNetworkStream_h
#define IAnkiNetworkStream_h

#include "../signals/simpleSignal.hpp"
#include "../sodium/sodium.h"

namespace Anki {
  namespace Networking {
    class INetworkStream {
      
    public:
      virtual void SendPlainText(uint8_t* bytes, int length) = 0;
      virtual void SendEncrypted(uint8_t* bytes, int length) = 0;
      
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
        Decrypt(bytes, length, buffer);
                
        _receivedEncryptedSignal.emit(buffer, length);
        
        free(buffer);
      }
      
    private:
      ReceivedSignal _receivedPlainTextSignal;
      ReceivedSignal _receivedEncryptedSignal;
      NotificationSignal _failedDecryptionSignal;
      
      std::vector<uint8_t*> _buffers;
      std::vector<int> _bufferSizes;
      
      int _buffersTotalSize = 0;
      uint8_t _DecryptKey[crypto_kx_SESSIONKEYBYTES];
      uint8_t _EncryptKey[crypto_kx_SESSIONKEYBYTES];
      uint8_t _DecryptNonce[crypto_aead_xchacha20poly1305_ietf_NPUBBYTES];
      uint8_t _EncryptNonce[crypto_aead_xchacha20poly1305_ietf_NPUBBYTES];
      
      int Decrypt(uint8_t* buffer, int length, uint8_t* output) {
        uint64_t msgLength = 0;
        
        // decrypt message
        int result = crypto_aead_xchacha20poly1305_ietf_decrypt(output, &msgLength, nullptr, buffer, length, nullptr, 0, _DecryptNonce, _DecryptKey);
        
        if(result == 0) {
          // increment nonce if successful (don't let attacker derail our comms)
          sodium_increment(_DecryptNonce, crypto_aead_xchacha20poly1305_ietf_NPUBBYTES);
        } else {
          _failedDecryptionSignal.emit();
        }
        
        return result;
      }
      
    protected:
      int Encrypt(uint8_t* buffer, int length, uint8_t* output, uint64_t* outputLength) {
        // encrypt message
        int result = crypto_aead_xchacha20poly1305_ietf_encrypt(output, outputLength, buffer, length, nullptr, 0, nullptr, _EncryptNonce, _EncryptKey);
        
        if(result == 0) {
          // increment nonce if successful (don't let attacker derail our comms)
          sodium_increment(_EncryptNonce, crypto_aead_xchacha20poly1305_ietf_NPUBBYTES);
        }
        
        return result;
      }
    };
  }
}

#endif /* IAnkiNetworkStream_h */
