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
    const external_interface::GatewayWrapper & proto_message, CozmoContext * cozmo_context) {
  
  ExternalInterface::MessageGameToEngine clad_message;

  /*
  auto od = proto_message.GetMetadata().descriptor->FindOneofByName("oneof_message_type");
  LOG_WARNING("ron_proto", "ProtoCladInterpreter::Redirect((%d, %s, %s, %s)=>clad)", 
      proto_message.oneof_message_type_case(),
      proto_message.GetMetadata().reflection->GetOneofFieldDescriptor(proto_message, od)->name().c_str(),
      proto_message.GetMetadata().descriptor->full_name().c_str(),
      MessageGameToEngineTagToString(clad_message.GetTag()));
  */

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

  /*
  LOG_WARNING("ron_proto", "Redirect(ME2G(%d, %s)=>proto): %s:%d", 
      (int)message.GetTag(),
      MessageEngineToGameTagToString(message.GetTag()),
      __FILE__, __LINE__
      );
  */

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

  LOG_WARNING("ron_proto", "Redirect(MG2E(%d, %s))=>proto", 
      (int)message.GetTag(),
      MessageGameToEngineTagToString(message.GetTag()));

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


//
// Misc Support Translators
//
external_interface::PoseStruct* ProtoCladInterpreter::CladPoseStructToProto(
    const Anki::PoseStruct3d& clad_message) {
  external_interface::PoseStruct* pose_struct = new external_interface::PoseStruct;
  pose_struct->set_x(clad_message.x);
  pose_struct->set_y(clad_message.y);
  pose_struct->set_z(clad_message.z);
  pose_struct->set_q0(clad_message.q0);
  pose_struct->set_q1(clad_message.q1);
  pose_struct->set_q2(clad_message.q2);
  pose_struct->set_q3(clad_message.q3);
  pose_struct->set_origin_id(clad_message.originID);

  return pose_struct;
}

external_interface::CladRect* ProtoCladInterpreter::CladCladRectToProto(
    const Anki::CladRect& clad_message) {
  
  external_interface::CladRect* clad_rect = new external_interface::CladRect;
  clad_rect->set_x_top_left(clad_message.x_topLeft);
  clad_rect->set_y_top_left(clad_message.y_topLeft);
  clad_rect->set_width(clad_message.width);
  clad_rect->set_height(clad_message.height);
  
  return clad_rect;
}

//TODO: templatize the ENUM converters do DRY out the code:
external_interface::FacialExpression ProtoCladInterpreter::CladFacialExpressionToProto(
    const Anki::Vision::FacialExpression& clad_message) {

  // Admittedly, this is nasty, but 1) it's the way that it's being done on the gateway, already, and
  // 2) it ensures that adding a new expression doesn't require that you alter this interpreter.
  return external_interface::FacialExpression((int)clad_message + 1);
}

//TODO: templatize the ENUM converters do DRY out the code:
external_interface::ObjectFamily ProtoCladInterpreter::CladObjectFamilyToProto(
    const Anki::Vector::ObjectFamily& clad_message) {

  // Admittedly, this is nasty, but 1) it's the way that it's being done on the gateway, already, and
  // 2) it ensures that adding a new expression doesn't require that you alter this interpreter.
  return external_interface::ObjectFamily((int)clad_message + 1);
}

//TODO: templatize the ENUM converters do DRY out the code:
external_interface::ObjectType ProtoCladInterpreter::CladObjectTypeToProto(
    const Anki::Vector::ObjectType& clad_message) {

  // Admittedly, this is nasty, but 1) it's the way that it's being done on the gateway, already, and
  // 2) it ensures that adding a new expression doesn't require that you alter this interpreter.
  return external_interface::ObjectType((int)clad_message + 1);
}

//TODO: templatize the ENUM converters do DRY out the code:
external_interface::UpAxis ProtoCladInterpreter::CladUpAxisToProto(
    const Anki::Vector::UpAxis& clad_message) {

  // Admittedly, this is nasty, but 1) it's the way that it's being done on the gateway, already, and
  // 2) it ensures that adding a new expression doesn't require that you alter this interpreter.
  return external_interface::UpAxis((int)clad_message + 1);
}


//
// Events
//
void ProtoCladInterpreter::CladRobotObservedFaceToProto(
    const Anki::Vector::ExternalInterface::RobotObservedFace& clad_message,
    external_interface::GatewayWrapper& proto_message) {
  
  external_interface::RobotObservedFace* robot_observed_face = new external_interface::RobotObservedFace;
  
  robot_observed_face->set_face_id(clad_message.faceID);
  robot_observed_face->set_timestamp(clad_message.timestamp);
  robot_observed_face->set_allocated_pose(CladPoseStructToProto(clad_message.pose));
  robot_observed_face->set_allocated_img_rect(CladCladRectToProto(clad_message.img_rect));
  robot_observed_face->set_name(clad_message.name);
  robot_observed_face->set_expression(CladFacialExpressionToProto(clad_message.expression));
  
  robot_observed_face->clear_expression_values();
  for (auto expression_value = clad_message.expressionValues.begin();
       expression_value != clad_message.expressionValues.end();
       expression_value++) {
    robot_observed_face->add_expression_values(*expression_value);
  }
  
  robot_observed_face->clear_left_eye();
  for (auto point = clad_message.leftEye.begin();
       point != clad_message.leftEye.end();
       point++) {
    auto eye = robot_observed_face->add_left_eye();
    eye->set_x(point->x);
    eye->set_y(point->y);
  }

  robot_observed_face->clear_right_eye();
  for (auto point = clad_message.rightEye.begin();
       point != clad_message.rightEye.end();
       point++) {
    auto eye = robot_observed_face->add_right_eye();
    eye->set_x(point->x);
    eye->set_y(point->y);
  }

  robot_observed_face->clear_nose();
  for (auto point = clad_message.nose.begin();
       point != clad_message.nose.end();
       point++) {
    auto nose = robot_observed_face->add_nose();
    nose->set_x(point->x);
    nose->set_y(point->y);
  }

  robot_observed_face->clear_mouth();
  for (auto point = clad_message.mouth.begin();
       point != clad_message.mouth.end();
       point++) {
    auto mouth = robot_observed_face->add_mouth();
    mouth->set_x(point->x);
    mouth->set_y(point->y);
  }
  proto_message = ExternalMessageRouter::Wrap(robot_observed_face);
}

void ProtoCladInterpreter::CladRobotChangedObservedFaceIDToProto(
    const Anki::Vector::ExternalInterface::RobotChangedObservedFaceID& clad_message,
    external_interface::GatewayWrapper& proto_message) {
  
  external_interface::RobotChangedObservedFaceID* changed_observed_face_id = 
    new external_interface::RobotChangedObservedFaceID;
  
  changed_observed_face_id->set_new_id(clad_message.newID);
  changed_observed_face_id->set_old_id(clad_message.oldID);

  proto_message = ExternalMessageRouter::Wrap(changed_observed_face_id);
}

void ProtoCladInterpreter::CladRobotObservedObjectToProto(
    const Anki::Vector::ExternalInterface::RobotObservedObject& clad_message,
    external_interface::GatewayWrapper& proto_message) {
  
  external_interface::RobotObservedObject* robot_observed_object = new external_interface::RobotObservedObject;

  robot_observed_object->set_timestamp(clad_message.timestamp);
  robot_observed_object->set_object_family(CladObjectFamilyToProto(clad_message.objectFamily));
  robot_observed_object->set_object_type(CladObjectTypeToProto(clad_message.objectType));
  robot_observed_object->set_object_id(clad_message.objectID);
  robot_observed_object->set_allocated_img_rect(CladCladRectToProto(clad_message.img_rect));
  robot_observed_object->set_allocated_pose(CladPoseStructToProto(clad_message.pose));
  robot_observed_object->set_top_face_orientation_rad(clad_message.topFaceOrientation_rad);
  robot_observed_object->set_is_active(clad_message.isActive);

  external_interface::ObjectEvent* object_event = new external_interface::ObjectEvent{ robot_observed_object };

  proto_message = ExternalMessageRouter::Wrap(object_event);
}

void ProtoCladInterpreter::CladObjectMovedToProto(
    const Anki::Vector::ExternalInterface::ObjectMoved& clad_message,
    external_interface::GatewayWrapper & proto_message) {

  external_interface::ObjectMoved* object_moved = new external_interface::ObjectMoved;

  object_moved->set_timestamp(clad_message.timestamp);
  object_moved->set_object_id(clad_message.objectID);

  external_interface::ObjectEvent* object_event = new external_interface::ObjectEvent{ object_moved };

  proto_message = ExternalMessageRouter::Wrap(object_event);
}

void ProtoCladInterpreter::CladObjectAvailableToProto(
    const Anki::Vector::ExternalInterface::ObjectAvailable& clad_message,
    external_interface::GatewayWrapper & proto_message) {

  external_interface::ObjectAvailable* object_available = new external_interface::ObjectAvailable;

  object_available->set_factory_id(clad_message.factory_id);
  //there is no objectType or factory_id in the proto format.

  external_interface::ObjectEvent* object_event = new external_interface::ObjectEvent{ object_available };

  proto_message = ExternalMessageRouter::Wrap(object_event);
}

void ProtoCladInterpreter::CladObjectStoppedMovingToProto(
    const Anki::Vector::ExternalInterface::ObjectStoppedMoving & clad_message,
    external_interface::GatewayWrapper & proto_message) {

  external_interface::ObjectStoppedMoving* object_stopped_moving = new external_interface::ObjectStoppedMoving;

  object_stopped_moving->set_timestamp(clad_message.timestamp);
  object_stopped_moving->set_object_id(clad_message.objectID);

  external_interface::ObjectEvent* object_event = new external_interface::ObjectEvent{ object_stopped_moving };

  proto_message = ExternalMessageRouter::Wrap(object_event);
}

void ProtoCladInterpreter::CladObjectUpAxisChangedToProto(
    const Anki::Vector::ExternalInterface::ObjectUpAxisChanged& clad_message,
    external_interface::GatewayWrapper& proto_message) {

  external_interface::ObjectUpAxisChanged* object_up_axis_changed = new external_interface::ObjectUpAxisChanged;

  object_up_axis_changed->set_timestamp(clad_message.timestamp); 
  object_up_axis_changed->set_object_id(clad_message.objectID);
  object_up_axis_changed->set_up_axis(CladUpAxisToProto(clad_message.upAxis));
  
  external_interface::ObjectEvent* object_event = new external_interface::ObjectEvent{ object_up_axis_changed };

  proto_message = ExternalMessageRouter::Wrap(object_event);
}

void ProtoCladInterpreter::CladObjectTappedToProto(
    const Anki::Vector::ExternalInterface::ObjectTapped& clad_message,
    external_interface::GatewayWrapper& proto_message) {

  external_interface::ObjectTapped* object_tapped = new external_interface::ObjectTapped;

  object_tapped->set_timestamp(clad_message.timestamp);
  object_tapped->set_object_id(clad_message.objectID);

  external_interface::ObjectEvent* object_event = new external_interface::ObjectEvent{ object_tapped };

  proto_message = ExternalMessageRouter::Wrap(object_event);
}

void ProtoCladInterpreter::CladObjectConnectionStateToProto(
    const Anki::Vector::ExternalInterface::ObjectConnectionState& clad_message,
    external_interface::GatewayWrapper& proto_message) {

  LOG_WARNING("ron_proto", "CladObjectConnectionStateToProto()");
  external_interface::ObjectConnectionState* object_connection_state = new external_interface::ObjectConnectionState;

  object_connection_state->set_object_id(clad_message.objectID);
  object_connection_state->set_factory_id(clad_message.factoryID);
  object_connection_state->set_object_type(CladObjectTypeToProto(clad_message.object_type));
  object_connection_state->set_connected(clad_message.connected);

  external_interface::ObjectEvent* object_event = new external_interface::ObjectEvent{ object_connection_state };

  proto_message = ExternalMessageRouter::Wrap(object_event);
}


} // namespace Vector
} // namespace Anki
