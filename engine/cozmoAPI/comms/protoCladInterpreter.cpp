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
#include "engine/cozmoAPI/comms/protoMessageHandler.h"
#include "engine/externalInterface/externalMessageRouter.h"


namespace Anki {
namespace Vector {

// Given a message reference, checks to see if that Oneof type used to be
// translated (to Clad) by the gateway. If it is, it's now the engine's
// responsibility to do the translation, so we do that, then re-broadcast
// the Clad version of the message to uiMessageHandler - where it would
// have arrived, had gateway left it as Clad.
// 
// @param message The Proto message to check-translate-rebroadcast
// @return true, if a conversion-and-Broadcast was done, false otherwise.

bool ProtoCladInterpreter::Redirect(
    const external_interface::GatewayWrapper& proto_message, CozmoContext* cozmo_context) {
  
  ExternalInterface::MessageGameToEngine clad_message;

  switch(proto_message.oneof_message_type_case()) {
    case external_interface::GatewayWrapper::kDriveWheelsRequest:
    {
      ProtoDriveWheelsRequestToClad(proto_message, clad_message);
      break;
    }
    case external_interface::GatewayWrapper::kPlayAnimationRequest:
    {
      ProtoPlayAnimationRequestToClad(proto_message, clad_message);
      break;
    }
    case external_interface::GatewayWrapper::kListAnimationsRequest:
    {
      ProtoListAnimationsRequestToClad(proto_message, clad_message);
      break;
    }
    default:
    {
      return false;
    }
  }

  cozmo_context->GetExternalInterface()->Broadcast(std::move(clad_message));

  return true; 
}

bool ProtoCladInterpreter::Redirect(
    const ExternalInterface::MessageEngineToGame& message, CozmoContext* cozmo_context) {
  
  external_interface::GatewayWrapper proto_message;

  switch(message.GetTag()) {
    case ExternalInterface::MessageEngineToGameTag::AnimationAvailable:
    {
      CladAnimationAvailableToProto(message, proto_message);
      break;
    }
    case ExternalInterface::MessageEngineToGameTag::EndOfMessage:
    {
      CladEndOfMessageToProto(message, proto_message);
      break;
    }
    default:
    {
      return false;
    }
  }

  cozmo_context->GetGatewayInterface()->Broadcast(std::move(proto_message));
  return true;
}

bool ProtoCladInterpreter::Redirect(
    const ExternalInterface::MessageGameToEngine& message, CozmoContext* cozmo_context) {
  
  external_interface::GatewayWrapper proto_message;

  switch(message.GetTag()) {
    case ExternalInterface::MessageGameToEngineTag::DriveWheels:
    {
      CladDriveWheelsToProto(message, proto_message);
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::PlayAnimation:
    {
      CladPlayAnimationToProto(message, proto_message);
      break;
    }
    default:
    {
      return false;
    }
  }

  cozmo_context->GetGatewayInterface()->Broadcast(std::move(proto_message));
  return true;
}

void ProtoCladInterpreter::ProtoDriveWheelsRequestToClad(
    const external_interface::GatewayWrapper& proto_message,
    ExternalInterface::MessageGameToEngine& clad_message) {
  Anki::Vector::ExternalInterface::DriveWheels drive_wheels;
  drive_wheels.lwheel_speed_mmps =  proto_message.drive_wheels_request().left_wheel_mmps();
  drive_wheels.rwheel_speed_mmps =  proto_message.drive_wheels_request().right_wheel_mmps();
  drive_wheels.lwheel_accel_mmps2 = proto_message.drive_wheels_request().right_wheel_mmps2();
  drive_wheels.rwheel_accel_mmps2 = proto_message.drive_wheels_request().left_wheel_mmps2();
  clad_message.Set_DriveWheels(drive_wheels);
}

void ProtoCladInterpreter::ProtoPlayAnimationRequestToClad(
    const external_interface::GatewayWrapper& proto_message,
    ExternalInterface::MessageGameToEngine& clad_message) {
  Anki::Vector::ExternalInterface::PlayAnimation play_animation;
  play_animation.animationName =   proto_message.play_animation_request().animation().name();
  play_animation.ignoreBodyTrack = proto_message.play_animation_request().ignore_body_track();
  play_animation.ignoreHeadTrack = proto_message.play_animation_request().ignore_head_track();
  play_animation.ignoreLiftTrack = proto_message.play_animation_request().ignore_lift_track();
  play_animation.numLoops =        proto_message.play_animation_request().loops();
  clad_message.Set_PlayAnimation(play_animation);
}

void ProtoCladInterpreter::ProtoListAnimationsRequestToClad(
    const external_interface::GatewayWrapper& proto_message,
    ExternalInterface::MessageGameToEngine& clad_message) {
  Anki::Vector::ExternalInterface::RequestAvailableAnimations request_available_animations;
  clad_message.Set_RequestAvailableAnimations(request_available_animations);
}


void ProtoCladInterpreter::CladDriveWheelsToProto(
    const ExternalInterface::MessageGameToEngine& clad_message,
    external_interface::GatewayWrapper& proto_message) { 
  external_interface::DriveWheelsResponse* drive_wheels_response = new external_interface::DriveWheelsResponse;
  proto_message = ExternalMessageRouter::WrapResponse(drive_wheels_response);
}

void ProtoCladInterpreter::CladPlayAnimationToProto(
    const ExternalInterface::MessageGameToEngine& clad_message,
    external_interface::GatewayWrapper& proto_message) { 
  external_interface::PlayAnimationResponse* play_animation_response = new external_interface::PlayAnimationResponse;
  proto_message = ExternalMessageRouter::WrapResponse(play_animation_response);
}

void ProtoCladInterpreter::CladAnimationAvailableToProto(
    const ExternalInterface::MessageEngineToGame& clad_message, 
    external_interface::GatewayWrapper& proto_message) {
  external_interface::ListAnimationsResponse* list_animations_response = new external_interface::ListAnimationsResponse;
  std::string animName = clad_message.Get_AnimationAvailable().animName;
  list_animations_response->add_animation_names()->set_name(animName);
  proto_message = ExternalMessageRouter::WrapResponse(list_animations_response);
}

void ProtoCladInterpreter::CladEndOfMessageToProto(
    const ExternalInterface::MessageEngineToGame& clad_message, 
    external_interface::GatewayWrapper& proto_message) {
  external_interface::ListAnimationsResponse* end_of_list_animations_response = 
      new external_interface::ListAnimationsResponse;
  // Don't change "EndOfListAnimationsResponses" - The .go recipient depends upon it.
  end_of_list_animations_response->add_animation_names()->set_name("EndOfListAnimationsResponses");
  proto_message = ExternalMessageRouter::WrapResponse(end_of_list_animations_response);
}


} // namespace Vector
} // namespace Anki
