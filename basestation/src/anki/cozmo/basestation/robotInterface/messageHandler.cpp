/**
* File: messageHandler
*
* Author: damjan stulic
* Created: 9/8/15
*
* Description: 
*
* Copyright: Anki, inc. 2015
*
*/


#include "anki/cozmo/basestation/robotInterface/messageHandler.h"
#include "anki/cozmo/basestation/robotManager.h"
#include "anki/messaging/basestation/IChannel.h"
#include "anki/messaging/basestation/IComms.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageEngineToRobot.h"

namespace Anki {
namespace Cozmo {
namespace RobotInterface {

MessageHandler::MessageHandler()
: _channel(nullptr)
, _robotManager(nullptr)
, _isInitialized(false)
{
}

void MessageHandler::Init(Comms::IChannel* channel, RobotManager* robotMgr)
{
  _channel = channel;
  _robotManager = robotMgr;
  _isInitialized = true;
}

void MessageHandler::ProcessMessages()
{
  if(_isInitialized) {
    Comms::IncomingPacket packet;
    while (_channel->PopIncomingPacket(packet)) {
      ProcessPacket(packet);
    }
  }
}

Result MessageHandler::SendMessage(const RobotID_t robotId, const RobotInterface::EngineToRobot& msg, bool reliable, bool hot)
{
  Comms::OutgoingPacket p;
  p.bufferSize = (uint32_t)msg.Pack(p.buffer, p.MAX_SIZE);
  p.destId = robotId;
  p.reliable = reliable;
  p.hot = hot;

  return _channel->Send(p) > 0 ? RESULT_OK : RESULT_FAIL;
}

void MessageHandler::Broadcast(const uint32_t robotId, const RobotInterface::RobotToEngine& message)
{
  u32 type = static_cast<u32>(message.GetTag());
  _eventMgr.Broadcast(robotId, AnkiEvent<RobotInterface::RobotToEngine>(type, message));
}

void MessageHandler::Broadcast(const uint32_t robotId, RobotInterface::RobotToEngine&& message)
{
  u32 type = static_cast<u32>(message.GetTag());
  _eventMgr.Broadcast(robotId, AnkiEvent<RobotInterface::RobotToEngine>(type, std::move(message)));
}

void MessageHandler::ProcessPacket(const Comms::IncomingPacket& packet)
{

  switch (packet.tag) {
    case Comms::IncomingPacket::Tag::Connected:
      if (!_robotManager->DoesRobotExist(static_cast<RobotID_t>(packet.sourceId))) {
        PRINT_STREAM_ERROR("RobotMessageHandler.Connected",
          "Incoming connection not found for robot id " << packet.sourceId << ", address " << packet.sourceAddress << ". Disconnecting.");
        return;
      }

      PRINT_STREAM_DEBUG("RobotMessageHandler.Connected",
        "Connection accepted from connection id " << packet.sourceId << ", address " << packet.sourceAddress << ".");
      return;
    case Comms::IncomingPacket::Tag::Disconnected:
      PRINT_STREAM_INFO("RobotMessageHandler.Disconnected",
        "Disconnected from connection id " << packet.sourceId << ", address " << packet.sourceAddress << ".");
      _robotManager->RemoveRobot(static_cast<RobotID_t>(packet.sourceId));
      return;

    case Comms::IncomingPacket::Tag::NormalMessage:
      break;

      // should be handled internally by MultiClientChannel, but in case it's a different IChannel
    case Comms::IncomingPacket::Tag::ConnectionRequest:
      _channel->RefuseIncomingConnection(packet.sourceAddress);
      return;

    default:
      return;
  }

  if (packet.bufferSize == 0) {
    PRINT_STREAM_ERROR("RobotMessageHandler.ZeroSizeNormalPacket",
      "Got a normal packet of zero length from " << packet.sourceAddress);
    return;
  }

  const u8 msgID = packet.buffer[0];
  const RobotID_t robotID = static_cast<RobotID_t>(packet.sourceId);
  Robot* robot = _robotManager->GetRobotByID(robotID);
  if(robot == NULL) {
    PRINT_NAMED_ERROR("MessageFromInvalidRobotSource", "Message %d received from invalid robot source ID %d.", msgID, robotID);
    return;
  }

  RobotInterface::RobotToEngine message;
  if (message.Unpack(packet.buffer, packet.bufferSize) != packet.bufferSize) {
    PRINT_NAMED_ERROR("RobotMessageHandler.MessageUnpack", "Message unpack error");
    return;
  }

  Broadcast(robotID, std::move(message));
}
} // end namespace RobotInterface
} // end namespace Cozmo
} // end namespace Anki
