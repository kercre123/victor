//
//  BLENetworkStream.h
//  ios-server
//
//  Created by Paul Aluri on 1/16/18.
//  Copyright © 2018 Paul Aluri. All rights reserved.
//

#ifndef BLENetworkStream_h
#define BLENetworkStream_h

#include <CoreBluetooth/CoreBluetooth.h>
#include "INetworkStream.h"
#include "bleMessageProtocol.h"

namespace Anki {
namespace Switchboard {
  class BLENetworkStream : public INetworkStream {
  public:
    BLENetworkStream(CBPeripheralManager* peripheral, CBMutableCharacteristic* writeCharacteristic, CBMutableCharacteristic* encryptedWriteCharacteristic) {
      _peripheralManager = peripheral;
      _writeCharacteristic = writeCharacteristic;
      _encryptedWriteCharacteristic = encryptedWriteCharacteristic;
      
      _bleMessageProtocolEncrypted = std::make_unique<BleMessageProtocol>(kMaxPacketSize);
      _bleMessageProtocolPlainText = std::make_unique<BleMessageProtocol>(kMaxPacketSize);
      
      _onSendRawHandle = _bleMessageProtocolEncrypted->OnSendRawBufferEvent()
        .ScopedSubscribe(std::bind(&BLENetworkStream::HandleSendRawEncrypted, this, std::placeholders::_1, std::placeholders::_2));
      _onReceiveHandle = _bleMessageProtocolEncrypted->OnReceiveMessageEvent()
        .ScopedSubscribe(std::bind(&BLENetworkStream::HandleReceiveEncrypted, this, std::placeholders::_1, std::placeholders::_2));

      _onSendRawPlainHandle = _bleMessageProtocolPlainText->OnSendRawBufferEvent()
        .ScopedSubscribe(std::bind(&BLENetworkStream::HandleSendRawPlainText, this, std::placeholders::_1, std::placeholders::_2));
      _onReceivePlainHandle = _bleMessageProtocolPlainText->OnReceiveMessageEvent()
      .ScopedSubscribe(std::bind(&BLENetworkStream::HandleReceivePlainText, this, std::placeholders::_1, std::placeholders::_2));
    }
    
    using SendSignal = Signal::Signal<void (uint8_t*, int, bool)>;
    
    SendSignal& OnSendEvent() {
      return _sendSignal;
    }
    
    int SendPlainText(uint8_t* bytes, int length) __attribute__((warn_unused_result));
    int SendEncrypted(uint8_t* bytes, int length) __attribute__((warn_unused_result));
    
    void ReceivePlainText(uint8_t* bytes, int length);
    void ReceiveEncrypted(uint8_t* bytes, int length);
    
  private:
    static const uint8_t kMaxPacketSize = 20;
    
    CBPeripheralManager* _peripheralManager;
    CBMutableCharacteristic* _writeCharacteristic;
    CBMutableCharacteristic* _encryptedWriteCharacteristic;
    
    void HandleSendRawPlainText(uint8_t* buffer, size_t size);
    void HandleSendRawEncrypted(uint8_t* buffer, size_t size);
    void HandleReceivePlainText(uint8_t* buffer, size_t size);
    void HandleReceiveEncrypted(uint8_t* buffer, size_t size);
    
    std::unique_ptr<BleMessageProtocol> _bleMessageProtocolEncrypted;
    std::unique_ptr<BleMessageProtocol> _bleMessageProtocolPlainText;
    
    Signal::SmartHandle _onSendRawHandle;
    Signal::SmartHandle _onReceiveHandle;
    Signal::SmartHandle _onSendRawPlainHandle;
    Signal::SmartHandle _onReceivePlainHandle;
    
    SendSignal _sendSignal;
  };
} // Switchboard
} // Anki

#endif /* BLENetworkStream_h */
