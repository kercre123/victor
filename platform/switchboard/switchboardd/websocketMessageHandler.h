/**
 * File: websocketMessageHandler.h
 *
 * Author: shawnblakesley
 * Created: 4/05/2018
 *
 * Description: WiFi message protocol for ankiswitchboardd
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#pragma once

#include <cstdio>
#include <stdlib.h>
#include "signals/simpleSignal.hpp"
#include "switchboardd/engineMessagingClient.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "clad/externalInterface/messageExternalComms.h"
#include "clad/types/ledTypes.h"

namespace Anki {
namespace Switchboard {
class WebsocketMessageHandler {
public:
  // Constructor
  explicit WebsocketMessageHandler(std::shared_ptr<EngineMessagingClient> emc);
  
  // Signals
  using SendToWebsocketSignal = Signal::Signal<bool (uint8_t*, size_t)>;
  
  void Receive(uint8_t* buffer, size_t size);
  bool Send(const Anki::Cozmo::ExternalInterface::MessageEngineToGame& msg);
  
  SendToWebsocketSignal& OnSendEvent() {
    return _sendSignal;
  }
  
private:
  void HandleMotorControl(Anki::Cozmo::ExternalComms::MotorControl unionInstance);
  void HandleMotorControl_DriveWheels(Anki::Cozmo::ExternalComms::DriveWheels sdkMessage);
  void HandleMotorControl_DriveArc(Anki::Cozmo::ExternalComms::DriveArc sdkMessage);
  void HandleMotorControl_MoveHead(Anki::Cozmo::ExternalComms::MoveHead sdkMessage);
  void HandleMotorControl_MoveLift(Anki::Cozmo::ExternalComms::MoveLift sdkMessage);

  void HandleAnimations(Anki::Cozmo::ExternalComms::Animations unionInstance);
  void HandleAnimations_PlayAnimation(Anki::Cozmo::ExternalComms::PlayAnimation sdkMessage);
  void HandleAnimations_RequestAvailableAnimations(Anki::Cozmo::ExternalComms::RequestAvailableAnimations sdkMessage);
  void HandleAnimations_SayText(Anki::Cozmo::ExternalComms::SayText sdkMessage);
  void HandleAnimations_TransferFile(Anki::Cozmo::ExternalComms::TransferFile sdkMessage); 

  void HandleMovementAction(Anki::Cozmo::ExternalComms::MovementAction unionInstance);
  void HandleMovementAction_DriveOffChargerContacts(Anki::Cozmo::ExternalComms::DriveOffChargerContacts sdkMessage);
  void HandleMovementAction_DriveStraight(Anki::Cozmo::ExternalComms::DriveStraight sdkMessage);
  void HandleMovementAction_TurnInPlace(Anki::Cozmo::ExternalComms::TurnInPlace sdkMessage);
  void HandleMovementAction_SetHeadAngle(Anki::Cozmo::ExternalComms::SetHeadAngle sdkMessage);
  void HandleMovementAction_SetLiftHeight(Anki::Cozmo::ExternalComms::SetLiftHeight sdkMessage);
  
  void HandleMeetVictor(Anki::Cozmo::ExternalComms::MeetVictor meetVictor);
  void HandleMeetVictor_AppIntent(Anki::Cozmo::ExternalComms::AppIntent sdkMessage);
  void HandleMeetVictor_CancelFaceEnrollment();
  void HandleMeetVictor_RequestEnrolledNames();
  void HandleMeetVictor_UpdateEnrolledFaceByID(Anki::Cozmo::ExternalComms::UpdateEnrolledFaceByID sdkMessage);
  void HandleMeetVictor_EraseEnrolledFaceByID(Anki::Cozmo::ExternalComms::EraseEnrolledFaceByID sdkMessage);
  void HandleMeetVictor_EraseAllEnrolledFaces(Anki::Cozmo::ExternalComms::EraseAllEnrolledFaces sdkMessage);
  void HandleMeetVictor_SetFaceToEnroll(Anki::Cozmo::ExternalComms::SetFaceToEnroll sdkMessage);

  void HandleVictorDisplay(Anki::Cozmo::ExternalComms::VictorDisplay unionInstance);
  void HandleVictorDisplay_SetBackpackLEDs(Anki::Cozmo::ExternalComms::SetBackpackLEDs sdkMessage);

  void HandleCubes(Anki::Cozmo::ExternalComms::Cubes unionInstance);
  void HandleCubes_SetAllActiveObjectLEDs(Anki::Cozmo::ExternalComms::SetAllActiveObjectLEDs sdkMessage);

  Anki::Cozmo::ExternalComms::ExternalComms SendMeetVictorStarted(const Anki::Cozmo::ExternalInterface::MeetVictorStarted& msg);
  Anki::Cozmo::ExternalComms::ExternalComms SendMeetVictorFaceScanStarted(const Anki::Cozmo::ExternalInterface::MeetVictorFaceScanStarted& msg);
  Anki::Cozmo::ExternalComms::ExternalComms SendMeetVictorFaceScanComplete(const Anki::Cozmo::ExternalInterface::MeetVictorFaceScanComplete& msg);
  Anki::Cozmo::ExternalComms::ExternalComms SendFaceEnrollmentCompleted(const Anki::Cozmo::ExternalInterface::FaceEnrollmentCompleted& msg);
  Anki::Cozmo::ExternalComms::ExternalComms SendEnrolledNamesResponse(const Anki::Cozmo::ExternalInterface::EnrolledNamesResponse& msg);
  
  Anki::Cozmo::ExternalComms::ExternalComms SendAnimationAvailable(const Anki::Cozmo::ExternalInterface::AnimationAvailable& msg);

  Signal::SmartHandle _onEngineMessageHandle;
  SendToWebsocketSignal _sendSignal;
  std::shared_ptr<EngineMessagingClient> _engineMessaging;
};
} // Switchboard
} // Anki
