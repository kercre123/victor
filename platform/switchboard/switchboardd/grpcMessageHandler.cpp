/**
 * File: grpcMessageProtocol.cpp
 *
 * Author: paluri
 * Created: 2/13/2018
 *
 * Description: Multipart BLE message protocol for ankiswitchboardd
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "switchboardd/grpcMessageHandler.h"
#include "switchboardd/log.h"

namespace Anki {
namespace Switchboard {

using G2EMessageTag = Anki::Cozmo::ExternalInterface::MessageGameToEngineTag;
using G2EMessage = Anki::Cozmo::ExternalInterface::MessageGameToEngine;
using E2GMessageTag = Anki::Cozmo::ExternalInterface::MessageEngineToGameTag;
using E2GMessage = Anki::Cozmo::ExternalInterface::MessageEngineToGame;

GrpcMessageHandler::GrpcMessageHandler(std::shared_ptr<EngineMessagingClient> emc) {
  // pass
  _engineMessaging = emc;
  // bind receive to engineMessagingClient
  _onEngineMessageHandle = _engineMessaging->OnReceiveEngineMessage()
    .ScopedSubscribe(std::bind(&GrpcMessageHandler::Send, this, std::placeholders::_1));
  // bind send to grpcServer
}

// // Note: example message_sender
// void GrpcMessageHandler::HandleMeetVictor_MeetVictorRequest(float lwheel_speed_mmps, float rwheel_speed_mmps) {
//       Anki::Cozmo::ExternalInterface::DriveWheels dw;
//       dw.lwheel_speed_mmps = lwheel_speed_mmps;
//       dw.rwheel_speed_mmps = rwheel_speed_mmps;
//       _engineMessaging->SendMessage(G2EMessage::CreateDriveWheels(std::move(dw)));
// }

// // Note: example handler
// void GrpcMessageHandler::HandleMeetVictor(Anki::Cozmo::ExternalComms::MeetVictor meetVictor) {
//   switch(meetVictor.GetTag()) {
//     case Anki::Cozmo::ExternalComms::MeetVictorTag::MeetVictorRequest:
//       HandleMeetVictor_MeetVictorRequest(100, -100);
//       break;
//     default:
//       return;
//   }
// }

void GrpcMessageHandler::Receive(uint8_t* buffer, size_t size) {
  // if(size < 1) {
  //   return;
  // }

  // if(unpackSize != size) {
  //   // bugs
  //   Log::Write("GrpcMessageHandler - Somehow our bytes didn't pack to the proper size.");
  // }
  // switch (extComms.GetTag()) {
  //   // // Note: example case
  //   // case ExtCommsMessageTag::MeetVictor:
  //   //   HandleMeetVictor(extComms.Get_MeetVictor());
  //   //   break;
  //   default:
  //     Log::Write("Unhandled external comms message tag: %d", extComms.GetTag());
  //     return;
  // }
}

bool GrpcMessageHandler::Send(const E2GMessage& msg) {
  // // TODO: convert from EngineToGame to ExternalComms
  // ExtCommsMessage result;
  // switch (msg.GetTag()) {
  //   // Handle conversion from EngineToGame to ExternalComms here
  //   default:
  //     // Unhandled message
  //     return false;
  // }
  // size_t size = result.Size();
  // uint8_t* buffer = new uint8_t[size];
  // result.Pack(buffer, size);
  // return _sendSignal.emit(buffer, size);
  return false;
}

} // Switchboard
} // Anki