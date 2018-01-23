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
      virtual void Send(char* bytes, int length) = 0;
      
      // Message Receive Signal
      using ReceivedSignal = Signal::Signal<void (char*, int)>;
      
      // Message Receive Event
      ReceivedSignal& OnReceivedEvent() {
        return _receivedSignal;
      }
      
      void ClearCryptoKeys() {
        memset(_EncryptKey, 0, crypto_kx_SESSIONKEYBYTES);
        memset(_DecryptKey, 0, crypto_kx_SESSIONKEYBYTES);
      }
      
      void SetCryptoKeys(char* encryptKey, char* decryptKey) {
        memcpy(_EncryptKey, encryptKey, crypto_kx_SESSIONKEYBYTES);
        memcpy(_DecryptKey, decryptKey, crypto_kx_SESSIONKEYBYTES);
      }
      
      virtual void Receive(char* bytes, int length) {
        if(length == 0) {
          return;
        }
        
        if(IsEncrypted(bytes[0])) {
          char* buffer = (char*)malloc(length - 1);
          
          // insert decrypted chunk into new buffer
          Decrypt((unsigned char*)(bytes+6), length-6, (unsigned char*)buffer+5);
          
          // copy over meta info
          memcpy(buffer, bytes + 1, 5);
          
          _receivedSignal.emit(buffer, length - 1);
        } else {
          _receivedSignal.emit(bytes + 1, length - 1);
        }
      }
      
    private:
      ReceivedSignal _receivedSignal;
      std::vector<char*> _buffers;
      std::vector<int> _bufferSizes;
      int _buffersTotalSize = 0;
      const unsigned char MASK_ENCRYPTED = 0x80;
      const unsigned char MASK_STATE = 0x60;
      unsigned char _DecryptKey[crypto_kx_SESSIONKEYBYTES];
      unsigned char _EncryptKey[crypto_kx_SESSIONKEYBYTES];
      unsigned char _Nonce[crypto_secretbox_NONCEBYTES];
      
      void Decrypt(unsigned char* buffer, int length, unsigned char* output) {
        for(int i = 0; i < crypto_secretbox_NONCEBYTES; i++) {
          _Nonce[i] = 1;
        }
        
        crypto_secretbox_open_easy(output, buffer, (unsigned long long)length, _Nonce, _DecryptKey);
      }
      
      bool IsEncrypted(char byte) {
        return byte == 1;//((byte & MASK_ENCRYPTED) >> 7) != 0;
      }
      
      char GetState(char byte) {
        return ((byte & MASK_STATE) >> 5);
      }
    };
  }
}

#endif /* IAnkiNetworkStream_h */
