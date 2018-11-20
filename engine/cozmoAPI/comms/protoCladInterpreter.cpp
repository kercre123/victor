/**
 * File: protoCladInterpreter.cpp
 *
 * Author: Ron Barry
 * Date:   11/16/2018
 *
 * Description: Determine which proto messages need to be converted to
 *              Clad before being dispatched to their final destinations.
 *              (The gateway no longer does this work.)
 *
 * Copyright: Anki, Inc. 2018
 **/

#include "protoCladInterpreter.h"

#include "util/logging/logging.h"
#include "proto/external_interface/messages.pb.h"

#include "engine/cozmoEngine.h"
#include "engine/cozmoAPI/comms/uiMessageHandler.h"
#include "engine/externalInterface/externalMessageRouter.h"


namespace Anki {
namespace Vector {

/**
 * Given a message reference, checks to see if that Oneof type used to be
 * translated (to Clad) by the gateway. If it is, it's now the engine's
 * responsibility to do the translation, so we do that, then re-broadcast
 * the Clad version of the message to uiMessageHandler - where it would
 * have arrived, had gateway left it as Clad.
 * 
 * @param message The Proto message to check-translate-rebroadcast
 * @return
*/
bool ProtoCladInterpreter::Redirect(const external_interface::GatewayWrapper & proto_message, CozmoContext * cozmo_context) {
  
  ExternalInterface::MessageGameToEngine clad_message;
  bool converted_to_clad = true;

  switch(proto_message.oneof_message_type_case()) {
    case external_interface::GatewayWrapper::kDriveWheelsRequest:
      ProtoDriveWheelsRequestToClad(proto_message, clad_message);
    break;
    /*
    case external_interface::GatewayWrapperTag::kPlayAnimationRequest:
      ProtoPlayAnimationToClad(clad_message, proto_message);
      converted_to_clad = true;
    break;
    
    case external_interface::GatewayWrapperTag::kListAnimationsRequest:
      ListAnimationsRequestToClad(clad_message, proto_message)
    */
    default:
      converted_to_clad = false;
    break;
  }

  if(converted_to_clad) {
    cozmo_context->GetExternalInterface()->Broadcast(clad_message);

    auto od = proto_message.GetMetadata().descriptor->FindOneofByName("oneof_message_type");
    LOG_WARNING("ron_proto_to_clad_testing", "message.descriptor: %d %s %s",
      proto_message.oneof_message_type_case(),
      proto_message.GetMetadata().reflection->GetOneofFieldDescriptor(proto_message, od)->name().c_str(),
      proto_message.GetMetadata().descriptor->full_name().c_str());
  }

  return converted_to_clad;
}

bool ProtoCladInterpreter::Redirect(const ExternalInterface::MessageGameToEngine & message, CozmoContext * cozmo_context) {
  external_interface::GatewayWrapper proto_message;
  bool converted_to_proto = true;

  switch(message.GetTag()) {
    case ExternalInterface::MessageGameToEngineTag::DriveWheels:
      
    break;
    default:
      converted_to_proto = false;
    break;
  }

  return converted_to_proto;
}


void ProtoCladInterpreter::ProtoDriveWheelsRequestToClad(
      const external_interface::GatewayWrapper & proto_message,
      ExternalInterface::MessageGameToEngine & clad_message) {
  Anki::Vector::ExternalInterface::DriveWheels drive_wheels;
  drive_wheels.lwheel_speed_mmps = proto_message.drive_wheels_request().left_wheel_mmps();
  drive_wheels.rwheel_speed_mmps = proto_message.drive_wheels_request().right_wheel_mmps();
  drive_wheels.lwheel_accel_mmps2 = proto_message.drive_wheels_request().right_wheel_mmps2();
  drive_wheels.rwheel_accel_mmps2 = proto_message.drive_wheels_request().left_wheel_mmps2();
  clad_message.Set_DriveWheels(drive_wheels);
}

/*
void ProtoCladInterpreter::ProtoPlayAnimationToClad(
      ExternalInterface::MessageGameToEngine & clad_message
      const external_interface::GatewayWrapper & proto_message) {
  Anki::Vector::ExternalInterface::PlayAnimation play_animation;
  play_animation.animationName = proto_message.play_animation_request().animation()
  clad_message.Set_PlayAnimation(play_animation);
}
*/

void ProtoCladInterpreter::CladDriveWheelsToProto(
      const ExternalInterface::MessageGameToEngine & clad_message,
      external_interface::GatewayWrapper & proto_message) {
  
  external_interface::DriveWheelsResponse drive_wheels_response;
  proto_message = ExternalMessageRouter::WrapResponse(&drive_wheels_response);
}

} // namespace Vector
} // namespace Anki
