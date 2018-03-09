#include "signals/simpleSignal.hpp"
#include "clad/externalInterface/messageExternalComms.h"

namespace Anki {
namespace Switchboard {
  class ExternalCommsCladHandler {
    public:
    using RtsConnectionSignal = Signal::Signal<void (const Anki::Victor::ExternalComms::RtsConnection& msg)>;
    
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

    Anki::Victor::ExternalComms::ExternalComms ReceiveExternalCommsMsg(uint8_t* buffer, size_t length) {
      Anki::Victor::ExternalComms::ExternalComms extComms;

      const size_t unpackSize = extComms.Unpack(buffer, length);
      if(unpackSize != length) {
        // bugs
      }
      
      if(extComms.GetTag() == Anki::Victor::ExternalComms::ExternalCommsTag::RtsConnection) {
        Anki::Victor::ExternalComms::RtsConnection rtsMsg = extComms.Get_RtsConnection();
        
        switch(rtsMsg.GetTag()) {
          case Anki::Victor::ExternalComms::RtsConnectionTag::Error:
            //
            break;
          case Anki::Victor::ExternalComms::RtsConnectionTag::RtsConnResponse: {
            _receiveRtsConnResponse.emit(rtsMsg);          
            break;
          }
          case Anki::Victor::ExternalComms::RtsConnectionTag::RtsChallengeMessage: {
            _receiveRtsChallengeMessage.emit(rtsMsg);
            break;
          }
          case Anki::Victor::ExternalComms::RtsConnectionTag::RtsWifiConnectRequest: {
            _receiveRtsWifiConnectRequest.emit(rtsMsg);
            break;
          }
          case Anki::Victor::ExternalComms::RtsConnectionTag::RtsWifiIpRequest: {
            _receiveRtsWifiIpRequest.emit(rtsMsg);
            break;
          }
          case Anki::Victor::ExternalComms::RtsConnectionTag::RtsStatusRequest: {
            _receiveRtsStatusRequest.emit(rtsMsg);
            break;
          }
          case Anki::Victor::ExternalComms::RtsConnectionTag::RtsWifiScanRequest: {
            _receiveRtsWifiScanRequest.emit(rtsMsg);
            break;
          }
          case Anki::Victor::ExternalComms::RtsConnectionTag::RtsOtaUpdateRequest: {
            _receiveRtsOtaUpdateRequest.emit(rtsMsg);
            break;
          }
          case Anki::Victor::ExternalComms::RtsConnectionTag::RtsWifiAccessPointRequest: {
            _receiveRtsWifiAccessPointRequest.emit(rtsMsg);
            break;
          }
          case Anki::Victor::ExternalComms::RtsConnectionTag::RtsCancelPairing: {
            _receiveRtsCancelPairing.emit(rtsMsg);
            break;
          }
          case Anki::Victor::ExternalComms::RtsConnectionTag::RtsAck: {
            _receiveRtsAck.emit(rtsMsg);
            break;
          }
          // RtsSsh
          case Anki::Victor::ExternalComms::RtsConnectionTag::RtsSshRequest: {
            _DEV_ReceiveSshKey.emit(rtsMsg);
            break;
          }
          default:
            // unhandled messages
            break;
        }
      }

      return extComms;
    }

    static std::vector<uint8_t> SendExternalCommsMsg(Anki::Victor::ExternalComms::ExternalComms msg) {
      std::vector<uint8_t> messageData(msg.Size());

      const size_t packedSize = msg.Pack(messageData.data(), msg.Size());

      if(packedSize != msg.Size()) {
        // bugs
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