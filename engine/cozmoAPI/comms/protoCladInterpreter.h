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
#include "proto/external_interface/messages.pb.h"
#include "proto/external_interface/shared.pb.h"

namespace Anki {
namespace Vector {


class ProtoCladInterpreter {
public:
  ProtoCladInterpreter(CozmoContext* cozmo_context) : _context(cozmo_context) {}

  static bool print_warnings; 

  static void warn(const char * msg);

  void HandleEvents(const AnkiEvent<external_interface::GatewayWrapper>& event);

  bool Redirect(const external_interface::GatewayWrapper& message);
  bool Redirect(const ExternalInterface::MessageEngineToGame& message);
  // DO NOT IMPLEMENT! (If you're forwarding G2E msgs -> Gateway you're probably doing something wrong.)
  // bool Redirect(const ExternalInterface::MessageGameToEngine& message);


  //
  // Events
  //

  void CladRobotObservedFaceToProto(
      const Anki::Vector::ExternalInterface::RobotObservedFace& clad_message,
      external_interface::GatewayWrapper & proto_message);


  //
  // Object Events
  //

  void CladRobotChangedObservedFaceIDToProto(
      const Anki::Vector::ExternalInterface::RobotChangedObservedFaceID& clad_message,
      external_interface::GatewayWrapper& proto_message);

  void CladRobotObservedObjectToProto(
      const Anki::Vector::ExternalInterface::RobotObservedObject& clad_message,
      external_interface::GatewayWrapper& proto_message);

  void CladObjectMovedToProto(
      const Anki::Vector::ExternalInterface::ObjectMoved& clad_message,
      external_interface::GatewayWrapper& proto_message);

  void CladObjectAvailableToProto(
      const Anki::Vector::ExternalInterface::ObjectAvailable& clad_message,
      external_interface::GatewayWrapper& proto_message);

  void CladObjectStoppedMovingToProto(
      const Anki::Vector::ExternalInterface::ObjectStoppedMoving& clad_message,
      external_interface::GatewayWrapper& proto_message);

  void CladObjectUpAxisChangedToProto(
      const Anki::Vector::ExternalInterface::ObjectUpAxisChanged& clad_message,
      external_interface::GatewayWrapper& proto_message);

  void CladObjectTappedToProto(
      const Anki::Vector::ExternalInterface::ObjectTapped& clad_message,
      external_interface::GatewayWrapper& proto_message);

  void CladObjectConnectionStateToProto(
      const Anki::Vector::ExternalInterface::ObjectConnectionState& clad_message,
      external_interface::GatewayWrapper& proto_message);


private:
  CozmoContext*                                  _context;
  std::vector<Signal::SmartHandle>               _signalHandlers;


  //
  // Proto-to-Clad interpreters
  //
  void ProtoDriveWheelsRequestToClad(
      const external_interface::GatewayWrapper& proto_message,
      ExternalInterface::MessageGameToEngine& clad_message);

  void ProtoPlayAnimationRequestToClad(
      const external_interface::GatewayWrapper& proto_message,
      ExternalInterface::MessageGameToEngine& clad_message);

  void ProtoListAnimationsRequestToClad(
      const external_interface::GatewayWrapper& proto_message,
      ExternalInterface::MessageGameToEngine& clad_message);

  void ProtoRequestEnrolledNamesRequestToClad(
      const external_interface::GatewayWrapper& proto_message,
      ExternalInterface::MessageGameToEngine& clad_message);

  void ProtoCreateFixedCustomObjectRequestToClad(
      const external_interface::GatewayWrapper& proto_message,
      ExternalInterface::MessageGameToEngine& clad_message);

  void ProtoDeleteCustomObjectsRequestToClad(
      const external_interface::GatewayWrapper& proto_message,
      ExternalInterface::MessageGameToEngine& clad_message);

  void ProtoDefineCustomBoxToClad(
      const external_interface::GatewayWrapper& proto_message,
      ExternalInterface::MessageGameToEngine& clad_message);

  void ProtoDefineCustomCubeToClad(
      const external_interface::GatewayWrapper& proto_message,
      ExternalInterface::MessageGameToEngine& clad_message);

  void ProtoDefineCustomWallToClad(
      const external_interface::GatewayWrapper& proto_message,
      ExternalInterface::MessageGameToEngine& clad_message);

  void ProtoDefineCustomObjectRequestToClad(
      const external_interface::GatewayWrapper& proto_message,
      ExternalInterface::MessageGameToEngine& clad_message);

  void ProtoMoveHeadRequestToClad(
      const external_interface::GatewayWrapper& proto_message,
      ExternalInterface::MessageGameToEngine& clad_message);

  void ProtoMoveLiftRequestToClad(
      const external_interface::GatewayWrapper& proto_message,
      ExternalInterface::MessageGameToEngine& clad_message);

  void ProtoAppIntentRequestToClad(
      const external_interface::GatewayWrapper& proto_message,
      ExternalInterface::MessageGameToEngine& clad_message);

  void ProtoNavMapFeedRequestToClad(
      const external_interface::GatewayWrapper& proto_message,
      ExternalInterface::MessageGameToEngine& clad_message);

  void ProtoDisplayFaceImageRgbRequestToClad(
      const external_interface::GatewayWrapper& proto_message,
      ExternalInterface::MessageGameToEngine& clad_message);

  //
  // Clad-to-Proto interpreters
  //
  void CladDriveWheelsToProto(
      const ExternalInterface::MessageGameToEngine& clad_message,
      external_interface::GatewayWrapper& proto_message);

  void CladPlayAnimationToProto(
      const ExternalInterface::MessageGameToEngine& clad_message,
      external_interface::GatewayWrapper& proto_message);

  void CladAnimationAvailableToProto(
      const ExternalInterface::MessageEngineToGame& clad_message, 
      external_interface::GatewayWrapper& proto_message);

  void CladEndOfMessageToProto(
      const ExternalInterface::MessageEngineToGame& clad_message, 
      external_interface::GatewayWrapper& proto_message);

  void CladEnrolledNamesResponseToProto(
      const ExternalInterface::MessageEngineToGame& clad_message, 
      external_interface::GatewayWrapper& proto_message);

  void CladCreatedFixedCustomObjectToProto(
      const ExternalInterface::MessageEngineToGame& clad_message, 
      external_interface::GatewayWrapper& proto_message);

  void CladDeletedCustomObjectsToProto(
      const ExternalInterface::MessageEngineToGame& clad_message,
      external_interface::GatewayWrapper& proto_message);

   void CladDefinedCustomObjectToProto(
      const ExternalInterface::MessageEngineToGame& clad_message,
      external_interface::GatewayWrapper& proto_message);

  //
  // Misc Support Translators
  //

  external_interface::PoseStruct* CladPoseStructToProto(
      const Anki::PoseStruct3d& clad_message);

  void ProtoPoseStructToClad(
      const external_interface::PoseStruct& proto_message,
      Anki::PoseStruct3d& clad_message);

  external_interface::CladRect* CladCladRectToProto(
      const Anki::CladRect& clad_message);

  external_interface::FacialExpression CladFacialExpressionToProto(
      const Anki::Vision::FacialExpression& clad_message);

  external_interface::ObjectFamily CladObjectFamilyToProto(
      const Anki::Vector::ObjectFamily& clad_message);

  external_interface::ObjectType CladObjectTypeToProto(
      const Anki::Vector::ObjectType& clad_message);

  external_interface::UpAxis CladUpAxisToProto(
    const Anki::Vector::UpAxis& clad_message);

};

} // namespace Vector
} // namespace Anki

#endif // _ENGINE_COMMS_PROTOCLADINTERPRETER_H__