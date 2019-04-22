/**
 * File: externalCommsCladHandlerV6.h
 *
 * Author: Paul Aluri (@paluri)
 * Created: 04/17/20198
 *
 * Description: File for handling clad messages from
 * external devices and processing them into listenable
 * events.
 *
 * Copyright: Anki, Inc. 2019
 *
 **/

#pragma once

#include "signals/simpleSignal.hpp"
#include "clad/externalInterface/messageExternalComms.h"

namespace Anki {
namespace Switchboard {
  class ExternalCommsCladHandlerV6 {
    public:
    using RtsConnectionSignal = Signal::Signal<void (const Anki::Vector::ExternalComms::RtsConnection_6& msg)>;
    
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

    RtsConnectionSignal& OnReceiveRtsWifiForgetRequest() {
      return _receiveRtsWifiForgetRequest;
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

    RtsConnectionSignal& OnReceiveRtsLogRequest() {
      return _receiveRtsLogRequest;
    }

    RtsConnectionSignal& OnReceiveRtsForceDisconnect() {
      return _receiveRtsForceDisconnect;
    }

    RtsConnectionSignal& OnReceiveRtsCloudSessionRequest() {
      return _receiveRtsCloudSessionRequest;
    }

    RtsConnectionSignal& OnReceiveRtsAppConnectionIdRequest() {
      return _receiveRtsAppConnectionIdRequest;
    }

    RtsConnectionSignal& OnReceiveRtsSdkRequest() {
      return _receiveRtsSdkProxyRequest;
    }

    RtsConnectionSignal& OnReceiveRtsVersionListRequest() {
      return _receiveRtsVersionListRequest;
    }

    RtsConnectionSignal& OnReceiveRtsBleshConnectRequest() {
      return _receiveRtsBleshConnectRequest;
    }

    RtsConnectionSignal& OnReceiveRtsBleshDisconnectRequest() {
      return _receiveRtsBleshDisconnectRequest;
    }

    RtsConnectionSignal& OnReceiveRtsBleshToServerRequest() {
      return _receiveRtsBleshToServerRequest;
    }

    // RtsSsh
    RtsConnectionSignal& OnReceiveRtsSsh() {
      return _DEV_ReceiveSshKey;
    }

    // RtsSsh
    RtsConnectionSignal& OnReceiveRtsOtaCancelRequest() {
      return _receiveRtsOtaCancelRequest;
    }

    Anki::Vector::ExternalComms::ExternalComms ReceiveExternalCommsMsg(uint8_t* buffer, size_t length) {
      Anki::Vector::ExternalComms::ExternalComms extComms;

      if(length == 5 && buffer[0] == 1) {
        // for now, just ignore handshake
        return extComms;
      }

      const size_t unpackSize = extComms.Unpack(buffer, length);
      if(unpackSize != length) {
        // bugs
        Log::Write("externalCommsCladHandler - Somehow our bytes didn't unpack to the proper size.");
      }
      
      if(extComms.GetTag() == Anki::Vector::ExternalComms::ExternalCommsTag::RtsConnection) {
        Anki::Vector::ExternalComms::RtsConnection rstContainer = extComms.Get_RtsConnection();
        Anki::Vector::ExternalComms::RtsConnection_6 rtsMsg = rstContainer.Get_RtsConnection_6();
        
        switch(rtsMsg.GetTag()) {
          case Anki::Vector::ExternalComms::RtsConnection_6Tag::Error:
            //
            break;
          case Anki::Vector::ExternalComms::RtsConnection_6Tag::RtsConnResponse: {
            _receiveRtsConnResponse.emit(rtsMsg);          
            break;
          }
          case Anki::Vector::ExternalComms::RtsConnection_6Tag::RtsChallengeMessage: {
            _receiveRtsChallengeMessage.emit(rtsMsg);
            break;
          }
          case Anki::Vector::ExternalComms::RtsConnection_6Tag::RtsWifiConnectRequest: {
            _receiveRtsWifiConnectRequest.emit(rtsMsg);
            break;
          }
          case Anki::Vector::ExternalComms::RtsConnection_6Tag::RtsWifiIpRequest: {
            _receiveRtsWifiIpRequest.emit(rtsMsg);
            break;
          }
          case Anki::Vector::ExternalComms::RtsConnection_6Tag::RtsStatusRequest: {
            _receiveRtsStatusRequest.emit(rtsMsg);
            break;
          }
          case Anki::Vector::ExternalComms::RtsConnection_6Tag::RtsWifiScanRequest: {
            _receiveRtsWifiScanRequest.emit(rtsMsg);
            break;
          }
          case Anki::Vector::ExternalComms::RtsConnection_6Tag::RtsWifiForgetRequest: {
            _receiveRtsWifiForgetRequest.emit(rtsMsg);
            break;
          }
          case Anki::Vector::ExternalComms::RtsConnection_6Tag::RtsOtaUpdateRequest: {
            _receiveRtsOtaUpdateRequest.emit(rtsMsg);
            break;
          }
          case Anki::Vector::ExternalComms::RtsConnection_6Tag::RtsOtaCancelRequest: {
            _receiveRtsOtaCancelRequest.emit(rtsMsg);
            break;
          }
          case Anki::Vector::ExternalComms::RtsConnection_6Tag::RtsWifiAccessPointRequest: {
            _receiveRtsWifiAccessPointRequest.emit(rtsMsg);
            break;
          }
          case Anki::Vector::ExternalComms::RtsConnection_6Tag::RtsCancelPairing: {
            _receiveRtsCancelPairing.emit(rtsMsg);
            break;
          }
          case Anki::Vector::ExternalComms::RtsConnection_6Tag::RtsAck: {
            _receiveRtsAck.emit(rtsMsg);
            break;
          }
          case Anki::Vector::ExternalComms::RtsConnection_6Tag::RtsLogRequest: {
            _receiveRtsLogRequest.emit(rtsMsg);
            break;
          }
          case Anki::Vector::ExternalComms::RtsConnection_6Tag::RtsForceDisconnect: {
            _receiveRtsForceDisconnect.emit(rtsMsg);
            break;
          }
          case Anki::Vector::ExternalComms::RtsConnection_6Tag::RtsCloudSessionRequest_5: {
            _receiveRtsCloudSessionRequest.emit(rtsMsg);
            break;
          }
          case Anki::Vector::ExternalComms::RtsConnection_6Tag::RtsAppConnectionIdRequest: {
            _receiveRtsAppConnectionIdRequest.emit(rtsMsg);
            break;
          }
          case Anki::Vector::ExternalComms::RtsConnection_6Tag::RtsSdkProxyRequest: {
            _receiveRtsSdkProxyRequest.emit(rtsMsg);
            break;
          }
          case Anki::Vector::ExternalComms::RtsConnection_6Tag::RtsVersionListRequest: {
            _receiveRtsVersionListRequest.emit(rtsMsg);
            break;
          }
          case Anki::Vector::ExternalComms::RtsConnection_6Tag::RtsBleshConnectRequest: {
            _receiveRtsBleshConnectRequest.emit(rtsMsg);
            break;
          }
          case Anki::Vector::ExternalComms::RtsConnection_6Tag::RtsBleshDisconnectRequest: {
            _receiveRtsBleshDisconnectRequest.emit(rtsMsg);
            break;
          }
          case Anki::Vector::ExternalComms::RtsConnection_6Tag::RtsBleshToServerRequest: {
            _receiveRtsBleshToServerRequest.emit(rtsMsg);
            break;
          }
          // RtsSsh
          case Anki::Vector::ExternalComms::RtsConnection_6Tag::RtsSshRequest: {
            // only handle ssh message in debug build
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
      RtsConnectionSignal _receiveRtsConnResponse;
      RtsConnectionSignal _receiveRtsChallengeMessage;
      RtsConnectionSignal _receiveRtsWifiConnectRequest;
      RtsConnectionSignal _receiveRtsWifiIpRequest;
      RtsConnectionSignal _receiveRtsStatusRequest;
      RtsConnectionSignal _receiveRtsWifiScanRequest;
      RtsConnectionSignal _receiveRtsWifiForgetRequest;
      RtsConnectionSignal _receiveRtsOtaUpdateRequest;
      RtsConnectionSignal _receiveRtsWifiAccessPointRequest;
      RtsConnectionSignal _receiveRtsCancelPairing;
      RtsConnectionSignal _receiveRtsAck;
      RtsConnectionSignal _receiveRtsOtaCancelRequest;
      RtsConnectionSignal _receiveRtsLogRequest;
      RtsConnectionSignal _receiveRtsForceDisconnect;
      RtsConnectionSignal _receiveRtsCloudSessionRequest;
      RtsConnectionSignal _receiveRtsAppConnectionIdRequest;
      RtsConnectionSignal _receiveRtsSdkProxyRequest;
      RtsConnectionSignal _receiveRtsVersionListRequest;
      RtsConnectionSignal _receiveRtsBleshConnectRequest;
      RtsConnectionSignal _receiveRtsBleshDisconnectRequest;
      RtsConnectionSignal _receiveRtsBleshToServerRequest;

      // RtsSsh 
      RtsConnectionSignal _DEV_ReceiveSshKey;
  };
} // Switchboard
} // Anki
