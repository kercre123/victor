/**
 * File: protoCladInterpreter.h
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

#ifndef _ENGINE_COMMS_PROTOCLADINTERPRETER_H__
#define _ENGINE_COMMS_PROTOCLADINTERPRETER_H__

#include "engine/cozmoContext.h"
#include "engine/externalInterface/externalInterface.h"

#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "clad/gateway/messageRobotToExternalTag.h"
#include "proto/external_interface/messages.pb.h"
#include "proto/external_interface/shared.pb.h"

namespace Anki {
namespace Vector {


class ProtoCladInterpreter {
public:
  static bool Redirect(const external_interface::GatewayWrapper& message, CozmoContext* cozmo_context);
  static bool Redirect(const ExternalInterface::MessageGameToEngine& message, CozmoContext* cozmo_context);
  static bool Redirect(const ExternalInterface::MessageEngineToGame& message, CozmoContext* cozmo_context);

  //
  // Events
  //

  static void CladRobotObservedFaceToProto(
      const Anki::Vector::ExternalInterface::RobotObservedFace& clad_message,
      external_interface::GatewayWrapper & proto_message);


  //
  // Object Events
  //

  static void CladRobotChangedObservedFaceIDToProto(
      const Anki::Vector::ExternalInterface::RobotChangedObservedFaceID& clad_message,
      external_interface::GatewayWrapper& proto_message);

  static void CladRobotObservedObjectToProto(
      const Anki::Vector::ExternalInterface::RobotObservedObject& clad_message,
      external_interface::GatewayWrapper& proto_message);

  static void CladObjectMovedToProto(
      const Anki::Vector::ExternalInterface::ObjectMoved& clad_message,
      external_interface::GatewayWrapper& proto_message);

  static void CladObjectAvailableToProto(
      const Anki::Vector::ExternalInterface::ObjectAvailable& clad_message,
      external_interface::GatewayWrapper& proto_message);

  static void CladObjectStoppedMovingToProto(
      const Anki::Vector::ExternalInterface::ObjectStoppedMoving& clad_message,
      external_interface::GatewayWrapper& proto_message);

  static void CladObjectUpAxisChangedToProto(
      const Anki::Vector::ExternalInterface::ObjectUpAxisChanged& clad_message,
      external_interface::GatewayWrapper& proto_message);

  static void CladObjectTappedToProto(
      const Anki::Vector::ExternalInterface::ObjectTapped& clad_message,
      external_interface::GatewayWrapper& proto_message);

private:
  ProtoCladInterpreter() {}

  //
  // Proto-to-Clad interpreters
  //
  static void ProtoDriveWheelsRequestToClad(
      const external_interface::GatewayWrapper& proto_message,
      ExternalInterface::MessageGameToEngine& clad_message);

  static void ProtoPlayAnimationRequestToClad(
      const external_interface::GatewayWrapper& proto_message,
      ExternalInterface::MessageGameToEngine& clad_message);

  static void ProtoListAnimationsRequestToClad(
      const external_interface::GatewayWrapper& proto_message,
      ExternalInterface::MessageGameToEngine& clad_message);

  //
  // Clad-to-Proto interpreters
  //
  static void CladDriveWheelsToProto(
      const ExternalInterface::MessageGameToEngine& clad_message,
      external_interface::GatewayWrapper& proto_message);

  static void CladPlayAnimationToProto(
      const ExternalInterface::MessageGameToEngine& clad_message,
      external_interface::GatewayWrapper& proto_message);

  static void CladAnimationAvailableToProto(
      const ExternalInterface::MessageEngineToGame& clad_message, 
      external_interface::GatewayWrapper& proto_message);

  static void CladEndOfMessageToProto(
      const ExternalInterface::MessageEngineToGame& clad_message, 
      external_interface::GatewayWrapper& proto_message);


  //
  // Misc Support Translators
  //
  static external_interface::PoseStruct* CladPoseStructToProto(
      const Anki::PoseStruct3d& clad_message);

  static external_interface::CladRect* CladCladRectToProto(
      const Anki::CladRect& clad_message);

  static external_interface::FacialExpression CladFacialExpressionToProto(
      const Anki::Vision::FacialExpression& clad_message);

  static external_interface::ObjectFamily CladObjectFamilyToProto(
      const Anki::Vector::ObjectFamily& clad_message);

  static external_interface::ObjectType CladObjectTypeToProto(
      const Anki::Vector::ObjectType& clad_message);

  static external_interface::UpAxis CladUpAxisToProto(
    const Anki::Vector::UpAxis& clad_message);

};

} // namespace Vector
} // namespace Anki

#endif // _ENGINE_COMMS_PROTOCLADINTERPRETER_H__
