/**
 * File: internalInterfaceCladHandler.h
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

#include "signals/simpleSignal.hpp"
#include "clad/internalInterface/messageInternalInterface.h"

namespace Anki {
namespace Switchboard {
  class InternalInterfaceCladHandler {
    public:
    using RtsConnectionSignal = Signal::Signal<void (const Anki::Victor::InternalInterface::RtsConnection& msg)>;
    
    RtsConnectionSignal& OnReceiveRtsConnResponse() {
      return _receiveRtsConnResponse;
    }

    RtsConnectionSignal& OnReceiveRtsChallengeMessage() {
      return _receiveRtsChallengeMessage;
    }

    RtsConnectionSignal& OnReceiveRtsWifiConnectRequest() {
      return _receiveRtsWifiConnectRequest;
    }

    RtsConnectionSignal& OnReceiveRtsWifiIpRequest() {
      return _receiveRtsWifiIpRequest;
    }

    RtsConnectionSignal& OnReceiveRtsStatusRequest() {
      return _receiveRtsStatusRequest;
    }

    RtsConnectionSignal& OnReceiveRtsWifiScanRequest() {
      return _receiveRtsWifiScanRequest;
    }

    RtsConnectionSignal& OnReceiveRtsOtaUpdateRequest() {
      return _receiveRtsOtaUpdateRequest;
    }

    RtsConnectionSignal& OnReceiveRtsWifiAccessPointRequest() {
      return _receiveRtsWifiAccessPointRequest;
    }
    
    RtsConnectionSignal& OnReceiveCancelPairingRequest() {
      return _receiveRtsCancelPairing;
    }

    RtsConnectionSignal& OnReceiveRtsAck() {
      return _receiveRtsAck;
    }

    // RtsSsh
    RtsConnectionSignal& OnReceiveRtsSsh() {
      return _DEV_ReceiveSshKey;
    }

    Anki::Victor::InternalInterface::InternalInterface ReceiveInternalInterfaceMsg(uint8_t* buffer, size_t length) {
      Anki::Victor::InternalInterface::InternalInterface internalInterface;

      const size_t unpackSize = internalInterface.Unpack(buffer, length);
      if(unpackSize != length) {
        // bugs
        Log::Write("internalInterfaceCladHandler - Somehow our bytes didn't unpack to the proper size.");
      }
      
      if(internalInterface.GetTag() == Anki::Victor::InternalInterface::InternalInterfaceTag::RtsConnection) {
        Anki::Victor::InternalInterface::RtsConnection rtsMsg = internalInterface.Get_RtsConnection();
        
        switch(rtsMsg.GetTag()) {
          case Anki::Victor::InternalInterface::RtsConnectionTag::Error:
            //
            break;
          case Anki::Victor::InternalInterface::RtsConnectionTag::RtsConnResponse: {
            _receiveRtsConnResponse.emit(rtsMsg);          
            break;
          }
          case Anki::Victor::InternalInterface::RtsConnectionTag::RtsChallengeMessage: {
            _receiveRtsChallengeMessage.emit(rtsMsg);
            break;
          }
          case Anki::Victor::InternalInterface::RtsConnectionTag::RtsWifiConnectRequest: {
            _receiveRtsWifiConnectRequest.emit(rtsMsg);
            break;
          }
          case Anki::Victor::InternalInterface::RtsConnectionTag::RtsWifiIpRequest: {
            _receiveRtsWifiIpRequest.emit(rtsMsg);
            break;
          }
          case Anki::Victor::InternalInterface::RtsConnectionTag::RtsStatusRequest: {
            _receiveRtsStatusRequest.emit(rtsMsg);
            break;
          }
          case Anki::Victor::InternalInterface::RtsConnectionTag::RtsWifiScanRequest: {
            _receiveRtsWifiScanRequest.emit(rtsMsg);
            break;
          }
          case Anki::Victor::InternalInterface::RtsConnectionTag::RtsOtaUpdateRequest: {
            _receiveRtsOtaUpdateRequest.emit(rtsMsg);
            break;
          }
          case Anki::Victor::InternalInterface::RtsConnectionTag::RtsWifiAccessPointRequest: {
            _receiveRtsWifiAccessPointRequest.emit(rtsMsg);
            break;
          }
          case Anki::Victor::InternalInterface::RtsConnectionTag::RtsCancelPairing: {
            _receiveRtsCancelPairing.emit(rtsMsg);
            break;
          }
          case Anki::Victor::InternalInterface::RtsConnectionTag::RtsAck: {
            _receiveRtsAck.emit(rtsMsg);
            break;
          }
          // RtsSsh
          case Anki::Victor::InternalInterface::RtsConnectionTag::RtsSshRequest: {
            _DEV_ReceiveSshKey.emit(rtsMsg);
            break;
          }
          default:
            // unhandled messages
            break;
        }
      }

      return internalInterface;
    }

    static std::vector<uint8_t> SendInternalInterfaceMsg(Anki::Victor::InternalInterface::InternalInterface msg) {
      std::vector<uint8_t> messageData(msg.Size());

      const size_t packedSize = msg.Pack(messageData.data(), msg.Size());

      if(packedSize != msg.Size()) {
        // bugs
        Log::Write("internalInterfaceCladHandler - Somehow our bytes didn't pack to the proper size.");
      }

      return messageData;
    }

    private:
      RtsConnectionSignal _receiveRtsConnResponse;
      RtsConnectionSignal _receiveRtsChallengeMessage;
      RtsConnectionSignal _receiveRtsWifiConnectRequest;
      RtsConnectionSignal _receiveRtsWifiIpRequest;
      RtsConnectionSignal _receiveRtsStatusRequest;
      RtsConnectionSignal _receiveRtsWifiScanRequest;
      RtsConnectionSignal _receiveRtsOtaUpdateRequest;
      RtsConnectionSignal _receiveRtsWifiAccessPointRequest;
      RtsConnectionSignal _receiveRtsCancelPairing;
      RtsConnectionSignal _receiveRtsAck;

      // RtsSsh 
      RtsConnectionSignal _DEV_ReceiveSshKey;
  };
} // Switchboard
} // Anki