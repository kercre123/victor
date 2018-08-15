/**
 * File: externalCommsCladHandler.h
 *
 * Author: Paul Aluri (@paluri)
 * Created: 3/15/2018
 *
 * Description: File for handling clad messages from
 * external devices and processing them into listenable
 * events.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#pragma once
#include "signals/simpleSignal.hpp"
#include "clad/externalInterface/messageExternalComms.h"

namespace Anki {
namespace Switchboard {
  class CladHandler {
    public:
    using RtsConnectionSignal = Signal::Signal<void (const Anki::Vector::ExternalComms::RtsConnection_2& msg)>;
    
    RtsConnectionSignal& OnReceiveRtsMessage() {
      return _receiveRtsMessage;
    }

    Anki::Vector::ExternalComms::ExternalComms ReceiveExternalCommsMsg(uint8_t* buffer, size_t length) {
      Anki::Vector::ExternalComms::ExternalComms extComms;

      const size_t unpackSize = extComms.Unpack(buffer, length);
      if(unpackSize != length) {
        // bugs
        Log::Write("externalCommsCladHandler - Somehow our bytes didn't pack to the proper size.");
      }
      
      if(extComms.GetTag() == Anki::Vector::ExternalComms::ExternalCommsTag::RtsConnection) {
        Anki::Vector::ExternalComms::RtsConnection rstContainer = extComms.Get_RtsConnection();
        Anki::Vector::ExternalComms::RtsConnection_2 rtsMsg = rstContainer.Get_RtsConnection_2();
        
        _receiveRtsMessage.emit(rtsMsg);          
      }

      return extComms;
    }

    static std::vector<uint8_t> SendExternalCommsMsg(Anki::Vector::ExternalComms::ExternalComms msg) {
      std::vector<uint8_t> messageData(msg.Size());

      const size_t packedSize = msg.Pack(messageData.data(), msg.Size());

      if(packedSize != msg.Size()) {
        // bugs
        Log::Write("externalCommsCladHandler - Somehow our bytes didn't pack to the proper size.");
      }

      return messageData;
    }

    private:
      RtsConnectionSignal _receiveRtsMessage;

  };
} // Switchboard
} // Anki
