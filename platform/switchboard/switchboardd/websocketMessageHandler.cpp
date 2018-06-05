/**
 * File: websocketMessageHandler.cpp
 *
 * Author: paluri
 * Created: 2/13/2018
 *
 * Description: Multipart BLE message protocol for ankiswitchboardd
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "lib/util/source/anki/util/helpers/boundedWhile.h"
#include "switchboardd/websocketMessageHandler.h"
#include "switchboardd/log.h"

namespace Anki {
namespace Switchboard {

using G2EMessageTag = Anki::Cozmo::ExternalInterface::MessageGameToEngineTag;
using G2EMessage = Anki::Cozmo::ExternalInterface::MessageGameToEngine;
using E2GMessageTag = Anki::Cozmo::ExternalInterface::MessageEngineToGameTag;
using E2GMessage = Anki::Cozmo::ExternalInterface::MessageEngineToGame;
using ExtCommsMessageTag = Anki::Cozmo::ExternalComms::ExternalCommsTag;
using ExtCommsMessage = Anki::Cozmo::ExternalComms::ExternalComms;

WebsocketMessageHandler::WebsocketMessageHandler(std::shared_ptr<EngineMessagingClient> emc) {
  // pass
  _engineMessaging = emc;
  // bind receive to engineMessagingClient
  _onEngineMessageHandle = _engineMessaging->OnReceiveEngineMessage()
    .ScopedSubscribe(std::bind(&WebsocketMessageHandler::Send, this, std::placeholders::_1));
  // bind send to websocketServer
}

void WebsocketMessageHandler::HandleMotorControl_DriveWheels( Anki::Cozmo::ExternalComms::DriveWheels sdkMessage ) {
  Anki::Cozmo::ExternalInterface::DriveWheels engineMessage;
  engineMessage.lwheel_speed_mmps = sdkMessage.lwheel_speed_mmps;
  engineMessage.rwheel_speed_mmps = sdkMessage.rwheel_speed_mmps;
  engineMessage.lwheel_accel_mmps2 = sdkMessage.lwheel_accel_mmps2;
  engineMessage.rwheel_accel_mmps2 = sdkMessage.rwheel_accel_mmps2;
  _engineMessaging->SendMessage(G2EMessage::CreateDriveWheels(std::move(engineMessage)));
}

void WebsocketMessageHandler::HandleMotorControl_DriveArc( Anki::Cozmo::ExternalComms::DriveArc sdkMessage ) {
  Anki::Cozmo::ExternalInterface::DriveArc engineMessage;
  engineMessage.speed = sdkMessage.speed;
  engineMessage.accel = sdkMessage.accel;
  engineMessage.curvatureRadius_mm = sdkMessage.curvatureRadius_mm;
  _engineMessaging->SendMessage(G2EMessage::CreateDriveArc(std::move(engineMessage)));
}

void WebsocketMessageHandler::HandleMotorControl_MoveHead( Anki::Cozmo::ExternalComms::MoveHead sdkMessage ) {
  Anki::Cozmo::ExternalInterface::MoveHead engineMessage;
  engineMessage.speed_rad_per_sec = sdkMessage.speed_rad_per_sec;
  _engineMessaging->SendMessage(G2EMessage::CreateMoveHead(std::move(engineMessage)));
}

void WebsocketMessageHandler::HandleMotorControl_MoveLift( Anki::Cozmo::ExternalComms::MoveLift sdkMessage ) {
  Anki::Cozmo::ExternalInterface::MoveLift engineMessage;
  engineMessage.speed_rad_per_sec = sdkMessage.speed_rad_per_sec;
  _engineMessaging->SendMessage(G2EMessage::CreateMoveLift(std::move(engineMessage)));
}

void WebsocketMessageHandler::HandleMotorControl(Anki::Cozmo::ExternalComms::MotorControl unionInstance) {
  switch(unionInstance.GetTag()) {
    case Anki::Cozmo::ExternalComms::MotorControlTag::DriveWheels:
        HandleMotorControl_DriveWheels( unionInstance.Get_DriveWheels() );
        break;
    case Anki::Cozmo::ExternalComms::MotorControlTag::DriveArc:
        HandleMotorControl_DriveArc( unionInstance.Get_DriveArc() );
        break;
    case Anki::Cozmo::ExternalComms::MotorControlTag::MoveHead:
        HandleMotorControl_MoveHead( unionInstance.Get_MoveHead() );
        break;
    case Anki::Cozmo::ExternalComms::MotorControlTag::MoveLift:
        HandleMotorControl_MoveLift( unionInstance.Get_MoveLift() );
        break;
    default:
        return;
  }
}

void WebsocketMessageHandler::HandleAnimations_PlayAnimation(Anki::Cozmo::ExternalComms::PlayAnimation sdkMessage) {
  Anki::Cozmo::ExternalInterface::PlayAnimation engineMessage;
  engineMessage.numLoops = sdkMessage.numLoops;
  engineMessage.animationName = sdkMessage.animationName;
  engineMessage.ignoreBodyTrack = sdkMessage.ignoreBodyTrack;
  engineMessage.ignoreHeadTrack = sdkMessage.ignoreHeadTrack;
  engineMessage.ignoreLiftTrack = sdkMessage.ignoreLiftTrack;
  _engineMessaging->SendMessage(G2EMessage::CreatePlayAnimation(std::move(engineMessage)));
}

void WebsocketMessageHandler::HandleAnimations_RequestAvailableAnimations(Anki::Cozmo::ExternalComms::RequestAvailableAnimations sdkMessage) {
  Anki::Cozmo::ExternalInterface::RequestAvailableAnimations engineMessage;
  _engineMessaging->SendMessage(G2EMessage::CreateRequestAvailableAnimations(std::move(engineMessage)));
}

void WebsocketMessageHandler::HandleAnimations_SayText(Anki::Cozmo::ExternalComms::SayText sdkMessage) {
  Anki::Cozmo::ExternalInterface::SayText engineMessage;
  engineMessage.text = sdkMessage.text;
  engineMessage.playEvent = sdkMessage.playEvent;
  engineMessage.voiceStyle = (Anki::Cozmo::SayTextVoiceStyle)sdkMessage.voiceStyle;
  engineMessage.durationScalar = sdkMessage.durationScalar;
  engineMessage.voicePitch = sdkMessage.voicePitch;
  engineMessage.fitToDuration = sdkMessage.fitToDuration;
  _engineMessaging->SendMessage(G2EMessage::CreateSayText(std::move(engineMessage)));
}

void WebsocketMessageHandler::HandleAnimations_TransferFile(Anki::Cozmo::ExternalComms::TransferFile sdkMessage) {
  Anki::Cozmo::ExternalInterface::TransferFile engineMessage;
  engineMessage.fileBytes = sdkMessage.fileBytes;
  engineMessage.filePart = sdkMessage.filePart;
  engineMessage.numFileParts = sdkMessage.numFileParts;
  engineMessage.filename = sdkMessage.filename;
  engineMessage.fileType = (Anki::Cozmo::ExternalInterface::FileType)sdkMessage.fileType;
  _engineMessaging->SendMessage(G2EMessage::CreateTransferFile(std::move(engineMessage)));
}

void WebsocketMessageHandler::HandleAnimations(Anki::Cozmo::ExternalComms::Animations unionInstance) {
  using AnimationsTag = Anki::Cozmo::ExternalComms::AnimationsTag;
  switch(unionInstance.GetTag()) {
    case AnimationsTag::PlayAnimation:
      HandleAnimations_PlayAnimation(unionInstance.Get_PlayAnimation());
      break;
    case AnimationsTag::RequestAvailableAnimations:
      HandleAnimations_RequestAvailableAnimations(unionInstance.Get_RequestAvailableAnimations());
      break;   
    case AnimationsTag::SayText:
      HandleAnimations_SayText( unionInstance.Get_SayText() );
      break;          
    case AnimationsTag::TransferFile:
      HandleAnimations_TransferFile(unionInstance.Get_TransferFile());
      break;
     default:
      return;
   }
 }

void WebsocketMessageHandler::HandleMeetVictor_AppIntent(Anki::Cozmo::ExternalComms::AppIntent sdkMessage) {
  Anki::Cozmo::ExternalInterface::AppIntent ai;
  ai.intent = sdkMessage.intent;
  ai.param = sdkMessage.param;
  _engineMessaging->SendMessage(G2EMessage::CreateAppIntent(std::move(ai)));
}

void WebsocketMessageHandler::HandleMeetVictor_CancelFaceEnrollment() {
  Anki::Cozmo::ExternalInterface::CancelFaceEnrollment cfe;
  _engineMessaging->SendMessage(G2EMessage::CreateCancelFaceEnrollment(std::move(cfe)));
}

void WebsocketMessageHandler::HandleMeetVictor_RequestEnrolledNames() {
  Anki::Cozmo::ExternalInterface::RequestEnrolledNames ref;
  _engineMessaging->SendMessage(G2EMessage::CreateRequestEnrolledNames(std::move(ref)));
}

void WebsocketMessageHandler::HandleMeetVictor_UpdateEnrolledFaceByID(Anki::Cozmo::ExternalComms::UpdateEnrolledFaceByID sdkMessage) {
  Anki::Cozmo::ExternalInterface::UpdateEnrolledFaceByID uef;
  uef.faceID = sdkMessage.faceID;
  uef.oldName = sdkMessage.oldName;
  uef.newName = sdkMessage.newName;
  _engineMessaging->SendMessage(G2EMessage::CreateUpdateEnrolledFaceByID(std::move(uef)));
}

void WebsocketMessageHandler::HandleMeetVictor_EraseEnrolledFaceByID(Anki::Cozmo::ExternalComms::EraseEnrolledFaceByID sdkMessage) {
  Anki::Cozmo::ExternalInterface::EraseEnrolledFaceByID eef;
  eef.faceID = sdkMessage.faceID;
  _engineMessaging->SendMessage(G2EMessage::CreateEraseEnrolledFaceByID(std::move(eef)));
}

void WebsocketMessageHandler::HandleMeetVictor_EraseAllEnrolledFaces(Anki::Cozmo::ExternalComms::EraseAllEnrolledFaces sdkMessage) {
  Anki::Cozmo::ExternalInterface::EraseAllEnrolledFaces eaef;
  _engineMessaging->SendMessage(G2EMessage::CreateEraseAllEnrolledFaces(std::move(eaef)));
}

void WebsocketMessageHandler::HandleMeetVictor_SetFaceToEnroll(Anki::Cozmo::ExternalComms::SetFaceToEnroll sdkMessage) {
  Anki::Cozmo::ExternalInterface::SetFaceToEnroll sfte;
  sfte.name = sdkMessage.name;
  sfte.observedID = sdkMessage.observedID;
  sfte.saveID = sdkMessage.saveID;
  sfte.saveToRobot = sdkMessage.saveToRobot;
  sfte.sayName = sdkMessage.sayName;
  sfte.useMusic = sdkMessage.useMusic;
  _engineMessaging->SendMessage(G2EMessage::CreateSetFaceToEnroll(std::move(sfte)));
}

void WebsocketMessageHandler::HandleMeetVictor(Anki::Cozmo::ExternalComms::MeetVictor unionInstance) {
  using MeetVictorTag = Anki::Cozmo::ExternalComms::MeetVictorTag;

  switch(unionInstance.GetTag()) {
    case MeetVictorTag::AppIntent:
      HandleMeetVictor_AppIntent(unionInstance.Get_AppIntent());
      break;
    case MeetVictorTag::CancelFaceEnrollment:
      HandleMeetVictor_CancelFaceEnrollment();
      break;
    case MeetVictorTag::RequestEnrolledNames:
      HandleMeetVictor_RequestEnrolledNames();
      break;
    case MeetVictorTag::UpdateEnrolledFaceByID:
      HandleMeetVictor_UpdateEnrolledFaceByID(unionInstance.Get_UpdateEnrolledFaceByID());
      break;
    case MeetVictorTag::EraseEnrolledFaceByID:
      HandleMeetVictor_EraseEnrolledFaceByID(unionInstance.Get_EraseEnrolledFaceByID());
      break;
    case MeetVictorTag::EraseAllEnrolledFaces:
      HandleMeetVictor_EraseAllEnrolledFaces(unionInstance.Get_EraseAllEnrolledFaces());
      break;
    case MeetVictorTag::SetFaceToEnroll:
      HandleMeetVictor_SetFaceToEnroll(unionInstance.Get_SetFaceToEnroll());
      break;
    default:
      return;
  }
}

void WebsocketMessageHandler::HandleMovementAction_DriveOffChargerContacts(Anki::Cozmo::ExternalComms::DriveOffChargerContacts sdkMessage) {
  Anki::Cozmo::ExternalInterface::DriveOffChargerContacts engineMessage;
  _engineMessaging->SendMessage(G2EMessage::CreateDriveOffChargerContacts(std::move(engineMessage)));
}

void WebsocketMessageHandler::HandleMovementAction_DriveStraight(Anki::Cozmo::ExternalComms::DriveStraight sdkMessage) {
  Anki::Cozmo::ExternalInterface::DriveStraight engineMessage;
  engineMessage.speed_mmps = sdkMessage.speed_mmps;
  engineMessage.dist_mm = sdkMessage.dist_mm;
  engineMessage.shouldPlayAnimation = sdkMessage.shouldPlayAnimation;
  _engineMessaging->SendMessage(G2EMessage::CreateDriveStraight(std::move(engineMessage)));
}

void WebsocketMessageHandler::HandleMovementAction_TurnInPlace(Anki::Cozmo::ExternalComms::TurnInPlace sdkMessage) {
  Anki::Cozmo::ExternalInterface::TurnInPlace engineMessage;
  engineMessage.angle_rad = sdkMessage.angle_rad;
  engineMessage.speed_rad_per_sec = sdkMessage.speed_rad_per_sec;
  engineMessage.accel_rad_per_sec2 = sdkMessage.accel_rad_per_sec2;
  engineMessage.tol_rad = sdkMessage.tol_rad;
  engineMessage.isAbsolute = sdkMessage.isAbsolute;
  _engineMessaging->SendMessage(G2EMessage::CreateTurnInPlace(std::move(engineMessage)));
}

void WebsocketMessageHandler::HandleMovementAction_SetHeadAngle(Anki::Cozmo::ExternalComms::SetHeadAngle sdkMessage) {
  Anki::Cozmo::ExternalInterface::SetHeadAngle engineMessage;
  engineMessage.angle_rad = sdkMessage.angle_rad;
  engineMessage.max_speed_rad_per_sec = sdkMessage.max_speed_rad_per_sec;
  engineMessage.accel_rad_per_sec2 = sdkMessage.accel_rad_per_sec2;
  engineMessage.duration_sec = sdkMessage.duration_sec;
  _engineMessaging->SendMessage(G2EMessage::CreateSetHeadAngle(std::move(engineMessage)));
}

void WebsocketMessageHandler::HandleMovementAction_SetLiftHeight(Anki::Cozmo::ExternalComms::SetLiftHeight sdkMessage) {
  Anki::Cozmo::ExternalInterface::SetLiftHeight engineMessage;
  engineMessage.height_mm = sdkMessage.height_mm;
  engineMessage.max_speed_rad_per_sec = sdkMessage.max_speed_rad_per_sec;
  engineMessage.accel_rad_per_sec2 = sdkMessage.accel_rad_per_sec2;
  engineMessage.duration_sec = sdkMessage.duration_sec;
  _engineMessaging->SendMessage(G2EMessage::CreateSetLiftHeight(std::move(engineMessage)));
}

void WebsocketMessageHandler::HandleMovementAction(Anki::Cozmo::ExternalComms::MovementAction unionInstance) {
  switch(unionInstance.GetTag()) {
    case Anki::Cozmo::ExternalComms::MovementActionTag::DriveOffChargerContacts:
      HandleMovementAction_DriveOffChargerContacts( unionInstance.Get_DriveOffChargerContacts() );
      break;
    case Anki::Cozmo::ExternalComms::MovementActionTag::DriveStraight:
      HandleMovementAction_DriveStraight( unionInstance.Get_DriveStraight() );
      break;
    case Anki::Cozmo::ExternalComms::MovementActionTag::TurnInPlace:
      HandleMovementAction_TurnInPlace( unionInstance.Get_TurnInPlace() );
      break;
    case Anki::Cozmo::ExternalComms::MovementActionTag::SetHeadAngle:
      HandleMovementAction_SetHeadAngle( unionInstance.Get_SetHeadAngle() );
      break;
    case Anki::Cozmo::ExternalComms::MovementActionTag::SetLiftHeight:
      HandleMovementAction_SetLiftHeight( unionInstance.Get_SetLiftHeight() );
      break;
    default:
      return;
  }
}

void WebsocketMessageHandler::HandleVictorDisplay_SetBackpackLEDs(Anki::Cozmo::ExternalComms::SetBackpackLEDs sdkMessage) {
  Anki::Cozmo::ExternalInterface::SetBackpackLEDs engineMessage;
  engineMessage.onColor = sdkMessage.onColor;
  engineMessage.offColor = sdkMessage.offColor;
  engineMessage.onPeriod_ms = sdkMessage.onPeriod_ms;
  engineMessage.offPeriod_ms = sdkMessage.offPeriod_ms;
  engineMessage.transitionOnPeriod_ms = sdkMessage.transitionOnPeriod_ms;
  engineMessage.transitionOffPeriod_ms = sdkMessage.transitionOffPeriod_ms;
  engineMessage.offset = sdkMessage.offset;
  _engineMessaging->SendMessage(G2EMessage::CreateSetBackpackLEDs(std::move(engineMessage)));
}

void WebsocketMessageHandler::HandleVictorDisplay_DisplayFaceImageRGB(Anki::Cozmo::ExternalComms::DisplayFaceImageRGB sdkMessage) {
  const unsigned int maxTransferPixels = 600;

  unsigned int pixelsRemaining = 17664;
  unsigned int chunkIndex = 0;
  unsigned int chunkCount = (pixelsRemaining + maxTransferPixels + 1) / maxTransferPixels;

  BOUNDED_WHILE(100, pixelsRemaining > 0)
  {
    unsigned int pixelCount = (pixelsRemaining > maxTransferPixels) ? maxTransferPixels : pixelsRemaining;

    Anki::Cozmo::ExternalInterface::DisplayFaceImageRGBChunk message;

    message.duration_ms = sdkMessage.duration_ms;
    message.interruptRunning = sdkMessage.interruptRunning;

    message.chunkIndex = chunkIndex;
    message.numChunks = chunkCount;

    message.numPixels = pixelCount;

    auto head = std::begin(sdkMessage.faceData) + (maxTransferPixels*chunkIndex);
    std::copy(head, head + pixelCount, std::begin(message.faceData));

    _engineMessaging->SendMessage(G2EMessage::CreateDisplayFaceImageRGBChunk(std::move(message)));

    pixelsRemaining -= pixelCount;
    ++chunkIndex;
  }
}

void WebsocketMessageHandler::HandleVictorDisplay(Anki::Cozmo::ExternalComms::VictorDisplay unionInstance) {
  switch(unionInstance.GetTag()) {
    case Anki::Cozmo::ExternalComms::VictorDisplayTag::SetBackpackLEDs:
      HandleVictorDisplay_SetBackpackLEDs( unionInstance.Get_SetBackpackLEDs() );
      break;
    case Anki::Cozmo::ExternalComms::VictorDisplayTag::DisplayFaceImageRGB:
      HandleVictorDisplay_DisplayFaceImageRGB( unionInstance.Get_DisplayFaceImageRGB() );
      break;
    default:
      return;
  }
}

void WebsocketMessageHandler::HandleCubes_SetAllActiveObjectLEDs(Anki::Cozmo::ExternalComms::SetAllActiveObjectLEDs sdkMessage) {
  Anki::Cozmo::ExternalInterface::SetAllActiveObjectLEDs engineMessage;
  engineMessage.objectID = sdkMessage.objectID;
  engineMessage.onColor = sdkMessage.onColor;
  engineMessage.offColor = sdkMessage.offColor;
  engineMessage.onPeriod_ms = sdkMessage.onPeriod_ms;
  engineMessage.offPeriod_ms = sdkMessage.offPeriod_ms;
  engineMessage.transitionOnPeriod_ms = sdkMessage.transitionOnPeriod_ms;
  engineMessage.transitionOffPeriod_ms = sdkMessage.transitionOffPeriod_ms;
  engineMessage.offset = sdkMessage.offset;
  engineMessage.rotate = 0;
  engineMessage.makeRelative = Anki::Cozmo::MakeRelativeMode::RELATIVE_LED_MODE_OFF;
  Log::Write("WebsocketMessageHandler - Sending Cube Lights Message");
  _engineMessaging->SendMessage(G2EMessage::CreateSetAllActiveObjectLEDs(std::move(engineMessage)));
}

void WebsocketMessageHandler::HandleCubes(Anki::Cozmo::ExternalComms::Cubes unionInstance) {
  switch(unionInstance.GetTag()) {
    case Anki::Cozmo::ExternalComms::CubesTag::SetAllActiveObjectLEDs:
      HandleCubes_SetAllActiveObjectLEDs( unionInstance.Get_SetAllActiveObjectLEDs() );
      break;
    default:
      return;
  }
}

void WebsocketMessageHandler::Receive(uint8_t* buffer, size_t size) {
  if(size < 1) {
    return;
  }

  ExtCommsMessage extComms;

  const size_t unpackSize = extComms.Unpack(buffer, size);
  if(unpackSize != size) {
    // bugs
    Log::Write("WebsocketMessageHandler - Somehow our bytes didn't pack to the proper size.");
  }
  switch (extComms.GetTag()) {
    case ExtCommsMessageTag::MotorControl:
       HandleMotorControl(extComms.Get_MotorControl());
       break;
    case ExtCommsMessageTag::Animations:
       HandleAnimations(extComms.Get_Animations());
       break;
    case ExtCommsMessageTag::MovementAction:
       HandleMovementAction(extComms.Get_MovementAction());
       break;
    case ExtCommsMessageTag::MeetVictor:
       HandleMeetVictor(extComms.Get_MeetVictor());
       break;
    case ExtCommsMessageTag::VictorDisplay:
       HandleVictorDisplay(extComms.Get_VictorDisplay());
       break;
    case ExtCommsMessageTag::Cubes:
       HandleCubes(extComms.Get_Cubes());
       break;
    default:
      Log::Write("Unhandled external comms message tag: %d", extComms.GetTag());
      return;
  }
}

ExtCommsMessage WebsocketMessageHandler::SendMeetVictorStarted(const Anki::Cozmo::ExternalInterface::MeetVictorStarted& msg) {
  Anki::Cozmo::ExternalComms::MeetVictorStarted mvs;
  mvs.name = msg.name;

  auto externalComms = Anki::Cozmo::ExternalComms::MeetVictor::CreateMeetVictorStarted(std::move(mvs));
  return ExtCommsMessage::CreateMeetVictor(std::move(externalComms));
}

ExtCommsMessage WebsocketMessageHandler::SendMeetVictorFaceScanStarted(const Anki::Cozmo::ExternalInterface::MeetVictorFaceScanStarted& msg) {
  Anki::Cozmo::ExternalComms::MeetVictorFaceScanStarted mvfss;

  auto externalComms = Anki::Cozmo::ExternalComms::MeetVictor::CreateMeetVictorFaceScanStarted(std::move(mvfss));
  return ExtCommsMessage::CreateMeetVictor(std::move(externalComms));
}

ExtCommsMessage WebsocketMessageHandler::SendMeetVictorFaceScanComplete(const Anki::Cozmo::ExternalInterface::MeetVictorFaceScanComplete& msg) {
  Anki::Cozmo::ExternalComms::MeetVictorFaceScanComplete mvfsc;

  auto externalComms = Anki::Cozmo::ExternalComms::MeetVictor::CreateMeetVictorFaceScanComplete(std::move(mvfsc));
  return ExtCommsMessage::CreateMeetVictor(std::move(externalComms));
}

ExtCommsMessage WebsocketMessageHandler::SendFaceEnrollmentCompleted(const Anki::Cozmo::ExternalInterface::FaceEnrollmentCompleted& msg) {
  Anki::Cozmo::ExternalComms::FaceEnrollmentCompleted fec;
  fec.result = msg.result;
  fec.faceID = msg.faceID;
  fec.name = msg.name;

  auto externalComms = Anki::Cozmo::ExternalComms::MeetVictor::CreateFaceEnrollmentCompleted(std::move(fec));
  return ExtCommsMessage::CreateMeetVictor(std::move(externalComms));
}

ExtCommsMessage WebsocketMessageHandler::SendEnrolledNamesResponse(const Anki::Cozmo::ExternalInterface::EnrolledNamesResponse& msg) {
  Anki::Cozmo::ExternalComms::EnrolledNamesResponse enr;
  enr.faces = msg.faces;
  for( const auto& faceEntry : msg.faces ) {
    Anki::Vision::LoadedKnownFace knownFace;

    knownFace.secondsSinceFirstEnrolled = faceEntry.secondsSinceFirstEnrolled;
    knownFace.secondsSinceLastUpdated = faceEntry.secondsSinceLastUpdated;
    knownFace.secondsSinceLastSeen = faceEntry.secondsSinceLastSeen;
    knownFace.faceID = faceEntry.faceID;
    knownFace.name = faceEntry.name;

    enr.faces.push_back(knownFace);
  }

  auto externalComms = Anki::Cozmo::ExternalComms::MeetVictor::CreateEnrolledNamesResponse(std::move(enr));
  return ExtCommsMessage::CreateMeetVictor(std::move(externalComms));
}

ExtCommsMessage WebsocketMessageHandler::SendAnimationAvailable(const Anki::Cozmo::ExternalInterface::AnimationAvailable& msg) {
  Anki::Cozmo::ExternalComms::AnimationAvailable anim;
  anim.animName = msg.animName;

  auto externalComms = Anki::Cozmo::ExternalComms::Animations::CreateAnimationAvailable(std::move(anim));
  return ExtCommsMessage::CreateAnimations(std::move(externalComms));
}

// Convert from EngineToGame to ExternalComms
bool WebsocketMessageHandler::Send(const E2GMessage& e2gMessage) {
  ExtCommsMessage result;

  auto tag = e2gMessage.GetTag();
  switch (tag) {
    case E2GMessageTag::MeetVictorStarted:
    {
      result = SendMeetVictorStarted(e2gMessage.Get_MeetVictorStarted());
      break;
    }
    case E2GMessageTag::MeetVictorFaceScanStarted:
    {
      result = SendMeetVictorFaceScanStarted(e2gMessage.Get_MeetVictorFaceScanStarted());
      break;
    }
    case E2GMessageTag::MeetVictorFaceScanComplete:
    {
      result = SendMeetVictorFaceScanComplete(e2gMessage.Get_MeetVictorFaceScanComplete());
      break;
    }
    case E2GMessageTag::FaceEnrollmentCompleted:
    {
      result = SendFaceEnrollmentCompleted(e2gMessage.Get_FaceEnrollmentCompleted());
      break;
    }
    case E2GMessageTag::EnrolledNamesResponse:
    {
      result = SendEnrolledNamesResponse(e2gMessage.Get_EnrolledNamesResponse());
      break;
    }
    case E2GMessageTag::AnimationAvailable:
    {
      result = SendAnimationAvailable(e2gMessage.Get_AnimationAvailable());
      break;
    }
    default:
      // Unhandled message
      return false;
  }
  size_t size = result.Size();
  uint8_t* buffer = new uint8_t[size];
  result.Pack(buffer, size);
  bool sent = _sendSignal.emit(buffer, size);
  delete[] buffer;
  return sent;
}

} // Switchboard
} // Anki