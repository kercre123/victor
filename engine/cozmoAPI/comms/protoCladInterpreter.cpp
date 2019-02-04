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

#include <thread>
#include <sstream>

#include "protoCladInterpreter.h"

#include "util/logging/logging.h"
#include "proto/external_interface/messages.pb.h"

#include "engine/cozmoEngine.h"
#include "engine/cozmoAPI/comms/uiMessageHandler.h"
#include "engine/cozmoAPI/comms/protoMessageHandler.h"
#include "engine/externalInterface/externalMessageRouter.h"


namespace Anki {
namespace Vector {

bool ProtoCladInterpreter::Redirect(const external_interface::GatewayWrapper & proto_message) {
  
  ExternalInterface::MessageGameToEngine clad_message;
  /*
  auto od = proto_message.GetMetadata().descriptor->FindOneofByName("oneof_message_type");
  LOG_WARNING("proto_clad_interpreter", "ProtoCladInterpreter::Redirect((%d, %s, %s, %s)=>clad)", 
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
    case external_interface::GatewayWrapper::kRequestEnrolledNamesRequest:
    {
      ProtoRequestEnrolledNamesRequestToClad(proto_message, clad_message);
      break;
    }
    case external_interface::GatewayWrapper::kCreateFixedCustomObjectRequest:
    {
      ProtoCreateFixedCustomObjectRequestToClad(proto_message, clad_message);
      break;
    }
    case external_interface::GatewayWrapper::kDeleteCustomObjectsRequest:
    {
      ProtoDeleteCustomObjectsRequestToClad(proto_message, clad_message);
      break;
    }
    case external_interface::GatewayWrapper::kDefineCustomObjectRequest:
    {
      ProtoDefineCustomObjectRequestToClad(proto_message, clad_message);
      break;
    }
    case external_interface::GatewayWrapper::kMoveHeadRequest:
    {
      ProtoMoveHeadRequestToClad(proto_message, clad_message);
      break;
    }
    case external_interface::GatewayWrapper::kMoveLiftRequest:
    {
      ProtoMoveLiftRequestToClad(proto_message, clad_message);
      break;
    }
    case external_interface::GatewayWrapper::kAppIntentRequest:
    {
      ProtoAppIntentRequestToClad(proto_message, clad_message);
      break;
    }
    case external_interface::GatewayWrapper::kNavMapFeedRequest:
    {
      ProtoNavMapFeedRequestToClad(proto_message, clad_message);
      break;
    }
    case external_interface::GatewayWrapper::kDisplayFaceImageRgbRequest:
    {
      ProtoDisplayFaceImageRgbRequestToClad(proto_message, clad_message);
      break;
    }
    default:
    {
      return false;
    }
  }

  _context->GetExternalInterface()->Broadcast(std::move(clad_message));

  return true; 
}

bool ProtoCladInterpreter::Redirect(const ExternalInterface::MessageEngineToGame& message) {
  
  external_interface::GatewayWrapper proto_message;

  /*
  LOG_WARNING("proto_clad_interpreter", "Redirect(ME2G(%d, %s)=>proto): %s:%d", 
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
    case ExternalInterface::MessageEngineToGameTag::EnrolledNamesResponse:
    {
      CladEnrolledNamesResponseToProto(message, proto_message);
      break;
    }
    case ExternalInterface::MessageEngineToGameTag::CreatedFixedCustomObject:
    {
      CladCreatedFixedCustomObjectToProto(message, proto_message);
      break;
    }
    case ExternalInterface::MessageEngineToGameTag::RobotDeletedFixedCustomObjects:   // Fallthrough intended
    case ExternalInterface::MessageEngineToGameTag::RobotDeletedCustomMarkerObjects:  // Fallthrough intended
    {
      CladDeletedCustomObjectsToProto(message, proto_message);
      break;
    }
    case ExternalInterface::MessageEngineToGameTag::DefinedCustomObject:
    {
      CladDefinedCustomObjectToProto(message, proto_message);
      break;
    }
    default:
    {
      return false;
    }
  }

  _context->GetGatewayInterface()->Broadcast(std::move(proto_message));
  return true;
}

//
// Proto-to-Clad interpreters
//

void ProtoCladInterpreter::ProtoDriveWheelsRequestToClad(
    const external_interface::GatewayWrapper& proto_message,
    ExternalInterface::MessageGameToEngine& clad_message) {

  ExternalInterface::DriveWheels drive_wheels;
  drive_wheels.lwheel_speed_mmps =  proto_message.drive_wheels_request().left_wheel_mmps();
  drive_wheels.rwheel_speed_mmps =  proto_message.drive_wheels_request().right_wheel_mmps();
  drive_wheels.lwheel_accel_mmps2 = proto_message.drive_wheels_request().right_wheel_mmps2();
  drive_wheels.rwheel_accel_mmps2 = proto_message.drive_wheels_request().left_wheel_mmps2();
  clad_message.Set_DriveWheels(drive_wheels);
}

void ProtoCladInterpreter::ProtoPlayAnimationRequestToClad(
    const external_interface::GatewayWrapper& proto_message,
    ExternalInterface::MessageGameToEngine& clad_message) {

  ExternalInterface::PlayAnimation play_animation;
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

  ExternalInterface::RequestAvailableAnimations request_available_animations;
  clad_message.Set_RequestAvailableAnimations(request_available_animations);
}

void ProtoCladInterpreter::ProtoRequestEnrolledNamesRequestToClad(
    const external_interface::GatewayWrapper& proto_message,
    ExternalInterface::MessageGameToEngine& clad_message) {

  ExternalInterface::RequestEnrolledNames request_enrolled_names;
  clad_message.Set_RequestEnrolledNames(request_enrolled_names);
}

void ProtoCladInterpreter::ProtoCreateFixedCustomObjectRequestToClad(
    const external_interface::GatewayWrapper& proto_message,
    ExternalInterface::MessageGameToEngine& clad_message) {

  ExternalInterface::CreateFixedCustomObject create_fixed_custom_object;
  create_fixed_custom_object.xSize_mm = proto_message.create_fixed_custom_object_request().x_size_mm();
  create_fixed_custom_object.ySize_mm = proto_message.create_fixed_custom_object_request().y_size_mm();
  create_fixed_custom_object.zSize_mm = proto_message.create_fixed_custom_object_request().z_size_mm();
  ProtoPoseStructToClad(proto_message.create_fixed_custom_object_request().pose(), create_fixed_custom_object.pose);
  clad_message.Set_CreateFixedCustomObject(create_fixed_custom_object);
}

void ProtoCladInterpreter::ProtoDeleteCustomObjectsRequestToClad(
    const external_interface::GatewayWrapper& proto_message,
    ExternalInterface::MessageGameToEngine& clad_message) {

  switch(proto_message.delete_custom_objects_request().mode()) {
    case external_interface::CustomObjectDeletionMode::DELETION_MASK_FIXED_CUSTOM_OBJECTS:
    {
      ExternalInterface::DeleteFixedCustomObjects delete_fixed_custom_objects;
      clad_message.Set_DeleteFixedCustomObjects(delete_fixed_custom_objects);
      break;
    }
    case external_interface::CustomObjectDeletionMode::DELETION_MASK_CUSTOM_MARKER_OBJECTS:
    {
      ExternalInterface::DeleteCustomMarkerObjects delete_custom_marker_objects;
      clad_message.Set_DeleteCustomMarkerObjects(delete_custom_marker_objects);
      break;
    }
    case external_interface::CustomObjectDeletionMode::DELETION_MASK_ARCHETYPES:
    {
      ExternalInterface::UndefineAllCustomMarkerObjects undefine_all_custom_marker_objects;
      clad_message.Set_UndefineAllCustomMarkerObjects(undefine_all_custom_marker_objects);
      break;
    }
    default:
    {
      break;
    }
  }

}

void ProtoCladInterpreter::ProtoDefineCustomBoxToClad(
    const external_interface::GatewayWrapper& proto_message,
    ExternalInterface::MessageGameToEngine& clad_message) {

  ExternalInterface::DefineCustomBox define_custom_box;
  const external_interface::CustomBoxDefinition& proto_custom_box = 
      proto_message.define_custom_object_request().custom_box();
  
  define_custom_box.customType = ObjectType(
      proto_message.define_custom_object_request().custom_type() -
      external_interface::CUSTOM_TYPE_00 +
      int(ObjectType::CustomType00));

  define_custom_box.markerFront = CustomObjectMarker(proto_custom_box.marker_front() - 1);
  define_custom_box.markerBack = CustomObjectMarker(proto_custom_box.marker_back() - 1);
  define_custom_box.markerTop = CustomObjectMarker(proto_custom_box.marker_top() - 1);
  define_custom_box.markerBottom = CustomObjectMarker(proto_custom_box.marker_bottom() - 1);
  define_custom_box.markerLeft = CustomObjectMarker(proto_custom_box.marker_left() - 1);
  define_custom_box.markerRight = CustomObjectMarker(proto_custom_box.marker_right() - 1);
  define_custom_box.xSize_mm = proto_custom_box.x_size_mm();
  define_custom_box.ySize_mm = proto_custom_box.y_size_mm();
  define_custom_box.zSize_mm = proto_custom_box.z_size_mm();
  define_custom_box.markerWidth_mm = proto_custom_box.marker_width_mm();
  define_custom_box.markerHeight_mm = proto_custom_box.marker_height_mm();
  define_custom_box.isUnique = proto_message.define_custom_object_request().is_unique();

  clad_message.Set_DefineCustomBox(define_custom_box);
}

void ProtoCladInterpreter::ProtoDefineCustomCubeToClad(
    const external_interface::GatewayWrapper& proto_message,
    ExternalInterface::MessageGameToEngine& clad_message) {

  ExternalInterface::DefineCustomCube define_custom_cube;
  const external_interface::CustomCubeDefinition& proto_custom_cube = 
      proto_message.define_custom_object_request().custom_cube();

  define_custom_cube.customType = ObjectType(
      proto_message.define_custom_object_request().custom_type() -
      external_interface::CUSTOM_TYPE_00 +
      int(ObjectType::CustomType00));
  define_custom_cube.marker = CustomObjectMarker(proto_custom_cube.marker() - 1);
  define_custom_cube.size_mm = proto_custom_cube.size_mm();
  define_custom_cube.markerWidth_mm = proto_custom_cube.marker_width_mm();
  define_custom_cube.markerHeight_mm = proto_custom_cube.marker_height_mm();
  define_custom_cube.isUnique = proto_message.define_custom_object_request().is_unique();

  clad_message.Set_DefineCustomCube(define_custom_cube);
}

void ProtoCladInterpreter::ProtoDefineCustomWallToClad(
    const external_interface::GatewayWrapper& proto_message,
    ExternalInterface::MessageGameToEngine& clad_message) {

  ExternalInterface::DefineCustomWall define_custom_wall;
  const external_interface::CustomWallDefinition& proto_custom_wall = 
      proto_message.define_custom_object_request().custom_wall();

  define_custom_wall.customType = ObjectType(
      proto_message.define_custom_object_request().custom_type() -
      external_interface::CUSTOM_TYPE_00 +
      int(ObjectType::CustomType00));
  define_custom_wall.marker = CustomObjectMarker(proto_custom_wall.marker() - 1);
  define_custom_wall.width_mm = proto_custom_wall.width_mm();
  define_custom_wall.height_mm = proto_custom_wall.height_mm();
  define_custom_wall.markerWidth_mm = proto_custom_wall.marker_width_mm();
  define_custom_wall.markerHeight_mm = proto_custom_wall.marker_height_mm();
  define_custom_wall.isUnique = proto_message.define_custom_object_request().is_unique();

  clad_message.Set_DefineCustomWall(define_custom_wall);
}

void ProtoCladInterpreter::ProtoDefineCustomObjectRequestToClad(
    const external_interface::GatewayWrapper& proto_message,
    ExternalInterface::MessageGameToEngine& clad_message) {

  if(proto_message.define_custom_object_request().has_custom_box()) {
    ProtoDefineCustomBoxToClad(proto_message, clad_message);
  } else if (proto_message.define_custom_object_request().has_custom_cube()) {
    ProtoDefineCustomCubeToClad(proto_message, clad_message);
  } else if (proto_message.define_custom_object_request().has_custom_wall()) {
    ProtoDefineCustomWallToClad(proto_message, clad_message);
  }
}

void ProtoCladInterpreter::ProtoMoveHeadRequestToClad(
    const external_interface::GatewayWrapper& proto_message,
    ExternalInterface::MessageGameToEngine& clad_message) {
  ExternalInterface::MoveHead move_head;

  move_head.speed_rad_per_sec = proto_message.move_head_request().speed_rad_per_sec();

  clad_message.Set_MoveHead(move_head);
}

void ProtoCladInterpreter::ProtoMoveLiftRequestToClad(
    const external_interface::GatewayWrapper& proto_message,
    ExternalInterface::MessageGameToEngine& clad_message) {
  ExternalInterface::MoveLift move_lift;

  move_lift.speed_rad_per_sec = proto_message.move_lift_request().speed_rad_per_sec();

  clad_message.Set_MoveLift(move_lift);
}

void ProtoCladInterpreter::ProtoAppIntentRequestToClad(
    const external_interface::GatewayWrapper& proto_message,
    ExternalInterface::MessageGameToEngine& clad_message) {
  ExternalInterface::AppIntent app_intent;

  app_intent.intent = proto_message.app_intent_request().intent();
  app_intent.param = proto_message.app_intent_request().param();

  clad_message.Set_AppIntent(app_intent);
}

void ProtoCladInterpreter::ProtoNavMapFeedRequestToClad(
    const external_interface::GatewayWrapper& proto_message,
    ExternalInterface::MessageGameToEngine& clad_message) {
  ExternalInterface::SetMemoryMapBroadcastFrequency_sec memory_map_broadcast_frequency_sec;

  memory_map_broadcast_frequency_sec.frequency = proto_message.nav_map_feed_request().frequency();

  clad_message.Set_SetMemoryMapBroadcastFrequency_sec(memory_map_broadcast_frequency_sec);
}

void ProtoCladInterpreter::ProtoDisplayFaceImageRgbRequestToClad(
    const external_interface::GatewayWrapper& proto_message,
    ExternalInterface::MessageGameToEngine& clad_message) {
  
  ExternalInterface::DisplayFaceImageRGBChunk display_face_image_rgb_chunk;

  display_face_image_rgb_chunk.chunkIndex = proto_message.display_face_image_rgb_request().chunk_index();
  display_face_image_rgb_chunk.numChunks = proto_message.display_face_image_rgb_request().num_chunks();
  display_face_image_rgb_chunk.interruptRunning = proto_message.display_face_image_rgb_request().interrupt_running();
  display_face_image_rgb_chunk.duration_ms = proto_message.display_face_image_rgb_request().duration_ms();
  display_face_image_rgb_chunk.numPixels = proto_message.display_face_image_rgb_request().num_pixels();

  const unsigned short* raw_face_data = 
      (unsigned short*)(proto_message.display_face_image_rgb_request().face_data().c_str());
  for (int i=0; i<display_face_image_rgb_chunk.numPixels; i++) {
    unsigned short s = raw_face_data[i];
    //Endian swap:
    display_face_image_rgb_chunk.faceData[i] = ((s&0xff00)>>8)|((s&0x00ff)<<8);
  }

  clad_message.Set_DisplayFaceImageRGBChunk(display_face_image_rgb_chunk);
}

//
// Clad-to-Proto interpreters
//

void ProtoCladInterpreter::CladDriveWheelsToProto(
    const ExternalInterface::MessageGameToEngine& clad_message,
    external_interface::GatewayWrapper& proto_message) {

  auto* drive_wheels_response = new external_interface::DriveWheelsResponse;

  proto_message = ExternalMessageRouter::WrapResponse(drive_wheels_response);
}

void ProtoCladInterpreter::CladPlayAnimationToProto(
    const ExternalInterface::MessageGameToEngine& clad_message,
    external_interface::GatewayWrapper& proto_message) {
      
  auto* play_animation_response = new external_interface::PlayAnimationResponse;

  proto_message = ExternalMessageRouter::WrapResponse(play_animation_response);
}

void ProtoCladInterpreter::CladAnimationAvailableToProto(
    const ExternalInterface::MessageEngineToGame& clad_message, 
    external_interface::GatewayWrapper& proto_message) {
      
  auto* list_animations_response = new external_interface::ListAnimationsResponse;

  std::string animName = clad_message.Get_AnimationAvailable().animName;
  list_animations_response->add_animation_names()->set_name(animName);
  proto_message = ExternalMessageRouter::WrapResponse(list_animations_response);
}

void ProtoCladInterpreter::CladEndOfMessageToProto(
    const ExternalInterface::MessageEngineToGame& clad_message, 
    external_interface::GatewayWrapper& proto_message) {

  auto* end_of_list_animations_response = new external_interface::ListAnimationsResponse;

  // Don't change "EndOfListAnimationsResponses" - The .go recipient depends upon it.
  end_of_list_animations_response->add_animation_names()->set_name("EndOfListAnimationsResponses");
  proto_message = ExternalMessageRouter::WrapResponse(end_of_list_animations_response);
}

void ProtoCladInterpreter::CladEnrolledNamesResponseToProto(
    const ExternalInterface::MessageEngineToGame& clad_message,
    external_interface::GatewayWrapper& proto_message) {

  auto* enrolled_names_response = new external_interface::RequestEnrolledNamesResponse;

  enrolled_names_response->clear_faces();
  for(auto face=clad_message.Get_EnrolledNamesResponse().faces.begin();
      face!=clad_message.Get_EnrolledNamesResponse().faces.end();
      face++) {
    external_interface::LoadedKnownFace* loaded_known_face = enrolled_names_response->add_faces();
    loaded_known_face->set_face_id(face->faceID);
    loaded_known_face->set_seconds_since_first_enrolled(face->secondsSinceFirstEnrolled);
    loaded_known_face->set_seconds_since_last_updated(face->secondsSinceLastUpdated);
    loaded_known_face->set_seconds_since_last_seen(face->secondsSinceLastSeen);
    loaded_known_face->set_last_seen_seconds_since_epoch(face->lastSeenSecondsSinceEpoch);
    loaded_known_face->set_name(face->name);
  }
  
  proto_message = ExternalMessageRouter::WrapResponse(enrolled_names_response);
}

void ProtoCladInterpreter::CladCreatedFixedCustomObjectToProto(
    const ExternalInterface::MessageEngineToGame& clad_message,
    external_interface::GatewayWrapper& proto_message) {

  auto* create_fixed_custom_object_response = new external_interface::CreateFixedCustomObjectResponse;

  create_fixed_custom_object_response->set_object_id(clad_message.Get_CreatedFixedCustomObject().objectID);

  proto_message = ExternalMessageRouter::WrapResponse(create_fixed_custom_object_response);
}

void ProtoCladInterpreter::CladDeletedCustomObjectsToProto(
    const ExternalInterface::MessageEngineToGame& clad_message,
    external_interface::GatewayWrapper& proto_message) {
  
  auto * deleted_custom_objects_response = new external_interface::DeleteCustomObjectsResponse;

  proto_message = ExternalMessageRouter::WrapResponse(deleted_custom_objects_response);
}

void ProtoCladInterpreter::CladDefinedCustomObjectToProto(
    const ExternalInterface::MessageEngineToGame& clad_message,
    external_interface::GatewayWrapper& proto_message) {

  auto* define_custom_object_response = new external_interface::DefineCustomObjectResponse;

  define_custom_object_response->set_success(clad_message.Get_DefinedCustomObject().success);

  proto_message = ExternalMessageRouter::WrapResponse(define_custom_object_response);
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


void ProtoCladInterpreter::ProtoPoseStructToClad(
    const external_interface::PoseStruct& proto_message,
    Anki::PoseStruct3d& clad_message) {
  clad_message.x = proto_message.x();
  clad_message.y = proto_message.y();
  clad_message.z = proto_message.z();
  clad_message.q0 = proto_message.q0();
  clad_message.q1 = proto_message.q1();
  clad_message.q2 = proto_message.q2();
  clad_message.q3 = proto_message.q3();
  clad_message.originID = proto_message.origin_id();
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
external_interface::ObjectFamily ProtoCladInterpreter::CladObjectFamilyToProto(const ObjectFamily& clad_message) {

  // Admittedly, this is nasty, but 1) it's the way that it's being done on the gateway, already, and
  // 2) it ensures that adding a new expression doesn't require that you alter this interpreter.
  return external_interface::ObjectFamily((int)clad_message + 1);
}

//TODO: templatize the ENUM converters do DRY out the code:
external_interface::ObjectType ProtoCladInterpreter::CladObjectTypeToProto(const ObjectType& clad_message) {

  // Admittedly, this is nasty, but 1) it's the way that it's being done on the gateway, already, and
  // 2) it ensures that adding a new expression doesn't require that you alter this interpreter.
  return external_interface::ObjectType((int)clad_message + 1);
}

//TODO: templatize the ENUM converters do DRY out the code:
external_interface::UpAxis ProtoCladInterpreter::CladUpAxisToProto(const UpAxis& clad_message) {

  // Admittedly, this is nasty, but 1) it's the way that it's being done on the gateway, already, and
  // 2) it ensures that adding a new expression doesn't require that you alter this interpreter.
  return external_interface::UpAxis((int)clad_message + 1);
}


//
// Events
//
void ProtoCladInterpreter::CladRobotObservedFaceToProto(
    const ExternalInterface::RobotObservedFace& clad_message,
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
    const ExternalInterface::RobotChangedObservedFaceID& clad_message,
    external_interface::GatewayWrapper& proto_message) {
  
  external_interface::RobotChangedObservedFaceID* changed_observed_face_id = 
    new external_interface::RobotChangedObservedFaceID;
  
  changed_observed_face_id->set_new_id(clad_message.newID);
  changed_observed_face_id->set_old_id(clad_message.oldID);

  proto_message = ExternalMessageRouter::Wrap(changed_observed_face_id);
}

void ProtoCladInterpreter::CladRobotObservedObjectToProto(
    const ExternalInterface::RobotObservedObject& clad_message,
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
    const ExternalInterface::ObjectMoved& clad_message,
    external_interface::GatewayWrapper & proto_message) {

  external_interface::ObjectMoved* object_moved = new external_interface::ObjectMoved;

  object_moved->set_timestamp(clad_message.timestamp);
  object_moved->set_object_id(clad_message.objectID);

  external_interface::ObjectEvent* object_event = new external_interface::ObjectEvent{ object_moved };

  proto_message = ExternalMessageRouter::Wrap(object_event);
}

void ProtoCladInterpreter::CladObjectAvailableToProto(
    const ExternalInterface::ObjectAvailable& clad_message,
    external_interface::GatewayWrapper & proto_message) {

  external_interface::ObjectAvailable* object_available = new external_interface::ObjectAvailable;

  object_available->set_factory_id(clad_message.factory_id);

  external_interface::ObjectEvent* object_event = new external_interface::ObjectEvent{ object_available };

  proto_message = ExternalMessageRouter::Wrap(object_event);
}

void ProtoCladInterpreter::CladObjectStoppedMovingToProto(
    const ExternalInterface::ObjectStoppedMoving & clad_message,
    external_interface::GatewayWrapper & proto_message) {

  external_interface::ObjectStoppedMoving* object_stopped_moving = new external_interface::ObjectStoppedMoving;

  object_stopped_moving->set_timestamp(clad_message.timestamp);
  object_stopped_moving->set_object_id(clad_message.objectID);

  external_interface::ObjectEvent* object_event = new external_interface::ObjectEvent{ object_stopped_moving };

  proto_message = ExternalMessageRouter::Wrap(object_event);
}

void ProtoCladInterpreter::CladObjectUpAxisChangedToProto(
    const ExternalInterface::ObjectUpAxisChanged& clad_message,
    external_interface::GatewayWrapper& proto_message) {

  external_interface::ObjectUpAxisChanged* object_up_axis_changed = new external_interface::ObjectUpAxisChanged;

  object_up_axis_changed->set_timestamp(clad_message.timestamp); 
  object_up_axis_changed->set_object_id(clad_message.objectID);
  object_up_axis_changed->set_up_axis(CladUpAxisToProto(clad_message.upAxis));
  
  external_interface::ObjectEvent* object_event = new external_interface::ObjectEvent{ object_up_axis_changed };

  proto_message = ExternalMessageRouter::Wrap(object_event);
}

void ProtoCladInterpreter::CladObjectTappedToProto(
    const ExternalInterface::ObjectTapped& clad_message,
    external_interface::GatewayWrapper& proto_message) {

  external_interface::ObjectTapped* object_tapped = new external_interface::ObjectTapped;

  object_tapped->set_timestamp(clad_message.timestamp);
  object_tapped->set_object_id(clad_message.objectID);

  external_interface::ObjectEvent* object_event = new external_interface::ObjectEvent{ object_tapped };

  proto_message = ExternalMessageRouter::Wrap(object_event);
}

void ProtoCladInterpreter::CladObjectConnectionStateToProto(
    const ExternalInterface::ObjectConnectionState& clad_message,
    external_interface::GatewayWrapper& proto_message) {

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
