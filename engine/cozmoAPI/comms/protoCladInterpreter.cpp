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
    case external_interface::GatewayWrapper::kPlayAnimationRequest:
      ProtoPlayAnimationRequestToClad(proto_message, clad_message);
    break;
    case external_interface::GatewayWrapper::kListAnimationsRequest:
      ProtoListAnimationsRequestToClad(proto_message, clad_message);
    break;
    default:
      converted_to_clad = false;
    break;
  }

  if(converted_to_clad) {
    cozmo_context->GetExternalInterface()->Broadcast(clad_message);
  } 
  if (converted_to_clad or true) { //DO NOT SUBMIT (make this part of the previous block)
    auto od = proto_message.GetMetadata().descriptor->FindOneofByName("oneof_message_type");
    LOG_WARNING("ron_proto_to_clad_testing", "In p2c (request) Redirect(): message.descriptor: %d %s %s  -->  %s",
      proto_message.oneof_message_type_case(),
      proto_message.GetMetadata().reflection->GetOneofFieldDescriptor(proto_message, od)->name().c_str(),
      proto_message.GetMetadata().descriptor->full_name().c_str(),
      MessageGameToEngineTagToString(clad_message.GetTag()));
  }

  return converted_to_clad;
}

bool ProtoCladInterpreter::Redirect(const ExternalInterface::MessageEngineToGame & message, CozmoContext * cozmo_context) {
  external_interface::GatewayWrapper proto_message;
  bool converted_to_proto = true;
  ::google::protobuf::Message * wrapped_message = nullptr;

  LOG_WARNING("ron_proto_to_clad_testing", "MARK %d", __LINE__);
  LOG_WARNING("ron_proto_to_clad_testing", "In ME2G->proto (response) Redirect(). %d %s ", 
      (int)message.GetTag(),
      MessageEngineToGameTagToString(message.GetTag()));
  switch(message.GetTag()) {
    case ExternalInterface::MessageEngineToGameTag::AnimationAvailable:
      LOG_WARNING("ron_proto_to_clad_testing", "MARK %d", __LINE__);
      wrapped_message = CladAnimationAvailableToProto(message, proto_message);
      LOG_WARNING("ron_proto_to_clad_testing", "MARK %d", __LINE__);
    break;
    case ExternalInterface::MessageEngineToGameTag::EndOfMessage:
      LOG_WARNING("ron_proto_to_clad_testing", "MARK %d", __LINE__);
      wrapped_message = CladEndOfMessageToProto(message, proto_message);
      LOG_WARNING("ron_proto_to_clad_testing", "MARK %d", __LINE__);
    break;
    default:
      converted_to_proto = false;
    break;
  }

  LOG_WARNING("ron_proto_to_clad_testing", "MARK %d", __LINE__);
  if(converted_to_proto) {
    LOG_WARNING("ron_proto_to_clad_testing", "MARK %d", __LINE__);
    LOG_WARNING("ron_proto_to_clad_testing", "In ME2G->proto (response) Redirect(). converted_to_proto(%s): %d", 
        MessageEngineToGameTagToString(message.GetTag()),
        converted_to_proto);

    cozmo_context->GetGatewayInterface()->Broadcast(proto_message);   

    //delete wrapped_message;  //DO NOT SUBMIT until you're sure this is being deleted in the proto_message destructor.
  }

  return converted_to_proto;
}

bool ProtoCladInterpreter::Redirect(const ExternalInterface::MessageGameToEngine & message, CozmoContext * cozmo_context) {
  external_interface::GatewayWrapper proto_message;
  bool converted_to_proto = true;
  ::google::protobuf::Message * wrapped_message = nullptr;

  LOG_WARNING("ron_proto_to_clad_testing", "In MG2E->proto (response) Redirect(). Tag: %d", (int)message.GetTag());
  switch(message.GetTag()) {
    case ExternalInterface::MessageGameToEngineTag::DriveWheels:
      wrapped_message = CladDriveWheelsToProto(message, proto_message);
    break;
    case ExternalInterface::MessageGameToEngineTag::PlayAnimation:
      wrapped_message = CladPlayAnimationToProto(message, proto_message);
    break;
    default:
      converted_to_proto = false;
    break;
  }

  LOG_WARNING("ron_proto_to_clad_testing", "In MG2E->proto (response) Redirect(). converted_to_proto: %d", converted_to_proto);
  if(converted_to_proto) {
    LOG_WARNING("ron_proto_to_clad_testing", "In MG2E->proto (response) Redirect(): %s", MessageGameToEngineTagToString(message.GetTag()));

    cozmo_context->GetGatewayInterface()->Broadcast(proto_message);   

    delete wrapped_message;
  }


  return converted_to_proto;
}

void ProtoCladInterpreter::ProtoDriveWheelsRequestToClad(
    const external_interface::GatewayWrapper & proto_message,
    ExternalInterface::MessageGameToEngine & clad_message) {
  Anki::Vector::ExternalInterface::DriveWheels drive_wheels;
  drive_wheels.lwheel_speed_mmps =  proto_message.drive_wheels_request().left_wheel_mmps();
  drive_wheels.rwheel_speed_mmps =  proto_message.drive_wheels_request().right_wheel_mmps();
  drive_wheels.lwheel_accel_mmps2 = proto_message.drive_wheels_request().right_wheel_mmps2();
  drive_wheels.rwheel_accel_mmps2 = proto_message.drive_wheels_request().left_wheel_mmps2();
  clad_message.Set_DriveWheels(drive_wheels);
}

void ProtoCladInterpreter::ProtoPlayAnimationRequestToClad(
    const external_interface::GatewayWrapper & proto_message,
    ExternalInterface::MessageGameToEngine & clad_message) {
  Anki::Vector::ExternalInterface::PlayAnimation play_animation;
  play_animation.animationName =   proto_message.play_animation_request().animation().name();
  play_animation.ignoreBodyTrack = proto_message.play_animation_request().ignore_body_track();
  play_animation.ignoreHeadTrack = proto_message.play_animation_request().ignore_head_track();
  play_animation.ignoreLiftTrack = proto_message.play_animation_request().ignore_lift_track();
  play_animation.numLoops =        proto_message.play_animation_request().loops();
  clad_message.Set_PlayAnimation(play_animation);
}

void ProtoCladInterpreter::ProtoListAnimationsRequestToClad(
    const external_interface::GatewayWrapper & proto_message,
    ExternalInterface::MessageGameToEngine & clad_message) {
  LOG_WARNING("ron_proto_to_clad_testing", "ProtoCladInterpreter::ProtoListAnimationsRequestToClad()");
  Anki::Vector::ExternalInterface::RequestAvailableAnimations request_available_animations;
  clad_message.Set_RequestAvailableAnimations(request_available_animations);
}


::google::protobuf::Message * ProtoCladInterpreter::CladDriveWheelsToProto(
    const ExternalInterface::MessageGameToEngine & clad_message,
    external_interface::GatewayWrapper & proto_message) { 
  external_interface::DriveWheelsResponse * drive_wheels_response = new external_interface::DriveWheelsResponse;
  proto_message = ExternalMessageRouter::WrapResponse(drive_wheels_response);
  return drive_wheels_response;
}

::google::protobuf::Message * ProtoCladInterpreter::CladPlayAnimationToProto(
    const ExternalInterface::MessageGameToEngine & clad_message,
    external_interface::GatewayWrapper & proto_message) { 
  external_interface::PlayAnimationResponse * play_animation_response = new external_interface::PlayAnimationResponse;
  //DO NOT SUBMIT until you're sure you have the response correctly formatted. There are two values there.
  proto_message = ExternalMessageRouter::WrapResponse(play_animation_response);
  return play_animation_response;
}

::google::protobuf::Message * ProtoCladInterpreter::CladAnimationAvailableToProto(
    const ExternalInterface::MessageEngineToGame & clad_message, 
    external_interface::GatewayWrapper & proto_message) {
  external_interface::ListAnimationsResponse * list_animations_response = new external_interface::ListAnimationsResponse;
  std::string animName = clad_message.Get_AnimationAvailable().animName;
  LOG_WARNING("ron_proto_to_clad_testing", "animation name: %s", animName.c_str());
  list_animations_response->add_animation_names()->set_name(*(new std::string(animName))); //DO NOT SUBMIT (GET RID OF THE LEAK)
  proto_message = ExternalMessageRouter::WrapResponse(list_animations_response);
  return list_animations_response;
}

::google::protobuf::Message * ProtoCladInterpreter::CladEndOfMessageToProto(
    const ExternalInterface::MessageEngineToGame & clad_message, 
    external_interface::GatewayWrapper & proto_message) {
  external_interface::ListAnimationsResponse * end_of_list_animations_response = new external_interface::ListAnimationsResponse;
  // Don't change "EndOfListAnimationsResponses" - The .go recipient depends upon it.
  end_of_list_animations_response->add_animation_names()->set_name("EndOfListAnimationsResponses");
  proto_message = ExternalMessageRouter::WrapResponse(end_of_list_animations_response);
  return end_of_list_animations_response;
}


} // namespace Vector
} // namespace Anki
