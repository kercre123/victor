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
      
      // Message Receive Event
      ReceivedSignal& OnReceivedEvent() {
        return _receivedSignal;
      }
      
      void ClearCryptoKeys() {
        memset(_EncryptKey, 0, crypto_kx_SESSIONKEYBYTES);
        memset(_DecryptKey, 0, crypto_kx_SESSIONKEYBYTES);
      }
      
      void SetCryptoKeys(uint8_t* encryptKey, uint8_t* decryptKey) {
        memcpy(_EncryptKey, encryptKey, crypto_kx_SESSIONKEYBYTES);
        memcpy(_DecryptKey, decryptKey, crypto_kx_SESSIONKEYBYTES);
      }
      
      virtual void Receive(uint8_t* bytes, int length, bool encrypted) {
        if(length == 0) {
          return;
        }
        
        if(encrypted) {
          uint8_t* buffer = (uint8_t*)malloc(length);
          
          // insert decrypted chunk into new buffer
          Decrypt(bytes+5, length-5, buffer+5);
          
          // copy over meta info
          memcpy(buffer, bytes, 5);
          
          _receivedSignal.emit(buffer, length);
        } else {
          _receivedSignal.emit(bytes, length);
        }
      }
      
    private:
      ReceivedSignal _receivedSignal;
      std::vector<uint8_t*> _buffers;
      std::vector<int> _bufferSizes;
      int _buffersTotalSize = 0;
      uint8_t _DecryptKey[crypto_kx_SESSIONKEYBYTES];
      uint8_t _EncryptKey[crypto_kx_SESSIONKEYBYTES];
      uint8_t _Nonce[crypto_secretbox_NONCEBYTES];
      
      int Decrypt(uint8_t* buffer, int length, uint8_t* output) {
        // fixme: remove this fake nonce nonsense
        for(int i = 0; i < crypto_secretbox_NONCEBYTES; i++) {
          _Nonce[i] = 1;
        }
        
        return crypto_secretbox_open_easy(output, buffer, (uint64_t)length, _Nonce, _DecryptKey);
      }
    };
  }
}

#endif /* IAnkiNetworkStream_h */
