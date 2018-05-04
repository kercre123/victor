/**
 * File: websocketMessageProtocol.cpp
 *
 * Author: paluri
 * Created: 2/13/2018
 *
 * Description: Multipart BLE message protocol for ankiswitchboardd
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

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

void WebsocketMessageHandler::Receive(uint8_t* buffer, size_t size) {
  if(size < 1) {
    return;
  }
  // TODO: Unpack to ExternalComms then convert that to GameToEngine
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
    case ExtCommsMessageTag::MovementAction:
       HandleMovementAction(extComms.Get_MovementAction());
       break;
    default:
      Log::Write("Unhandled external comms message tag: %d", extComms.GetTag());
      return;
  }
}

bool WebsocketMessageHandler::Send(const E2GMessage& msg) {
  // TODO: convert from EngineToGame to ExternalComms
  ExtCommsMessage result;
  switch (msg.GetTag()) {
    // Handle conversion from EngineToGame to ExternalComms here
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