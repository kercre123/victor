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
#include "anki/cozmo/basestation/actions/actionContainers.h"
#include "anki/cozmo/basestation/ankiEventUtil.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/debug/devLoggingSystem.h"
#include "anki/cozmo/basestation/multiClientChannel.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotManager.h"
#include "anki/cozmo/basestation/utils/parsingConstants/parsingConstants.h"
#include "anki/common/basestation/utils/timer.h"
#include "anki/messaging/basestation/IChannel.h"
#include "anki/messaging/basestation/IComms.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "json/json.h"
#include "util/global/globalDefinitions.h"

namespace Anki {
namespace Cozmo {
namespace RobotInterface {

MessageHandler::MessageHandler()
: _channel(new MultiClientChannel())
, _robotManager(nullptr)
, _isInitialized(false)
{
}
  
MessageHandler::~MessageHandler()
{
  _channel->Stop();
}

void MessageHandler::Init(const Json::Value& config, RobotManager* robotMgr, const CozmoContext* context)
{
  const char *ipString = config[AnkiUtil::kP_ADVERTISING_HOST_IP].asCString();
  int port = config[AnkiUtil::kP_ROBOT_ADVERTISING_PORT].asInt();
  if (port < 0 || port >= 0x10000) {
    PRINT_NAMED_ERROR("RobotInterface.MessageHandler.Init", "Failed to initialize RobotComms; bad port %d", port);
    return;
  }
  
  Anki::Util::TransportAddress address(ipString, static_cast<uint16_t>(port));
  PRINT_STREAM_DEBUG("RobotInterface.MessageHandler.Init", "Initializing on address: " << address << ": " << address.GetIPAddress() << ":" << address.GetIPPort() << "; originals: " << ipString << ":" << port);
  
  _channel->Start(address);
  
  _robotManager = robotMgr;
  _isInitialized = true;

  if (context->GetExternalInterface() != nullptr) {
    auto helper = MakeAnkiEventUtil(*context->GetExternalInterface(), *this, _signalHandles);
    using namespace ExternalInterface;
    helper.SubscribeGameToEngine<MessageGameToEngineTag::ReliableTransportRunMode>();
  }
}

void MessageHandler::ProcessMessages()
{
  VerifyRobotConnection();
  
  if(_isInitialized) {
    _channel->Update();
    
    Comms::IncomingPacket packet;
    while (_channel->PopIncomingPacket(packet)) {
      ProcessPacket(packet);
    }
  }
}
  
void MessageHandler::VerifyRobotConnection()
{
  auto robotID = 0;
  if (!_robotManager)
  {
    return;
  }
  
  Robot* robot = _robotManager->GetFirstRobot();
  if (!robot)
  {
    return;
  }
  
  robotID = robot->GetID();
  
  static auto lastTime = std::chrono::system_clock::now();
  auto currentTime = std::chrono::system_clock::now();
  auto timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastTime).count();
  
  constexpr auto kMinTimeBetweenConnectAttempts_ms = 100;
  if (_cachedAddress && timeDiff > kMinTimeBetweenConnectAttempts_ms)
  {
    if (!_channel->IsAddressConnected(*_cachedAddress.get()))
    {
      _channel->RemoveConnection(robotID);
      _channel->AddConnection(robotID, *_cachedAddress.get());
    }
    else
    {
      _cachedAddress.reset();
      robot->GetActionList().Clear();
      //TODO:(lc) maybe also clear the animation streamer state here?
    }
    lastTime = currentTime;
  }
}

Result MessageHandler::SendMessage(const RobotID_t robotId, const RobotInterface::EngineToRobot& msg, bool reliable, bool hot)
{
  if (_isInitialized)
  {
#if ANKI_DEV_CHEATS
    if (nullptr != DevLoggingSystem::GetInstance())
    {
      DevLoggingSystem::GetInstance()->LogMessage(msg);
    }
#endif
    
    Comms::OutgoingPacket p;
    p.bufferSize = (uint32_t)msg.Pack(p.buffer, p.MAX_SIZE);
    p.destId = robotId;
    p.reliable = reliable;
    p.hot = hot;

    return _channel->Send(p) > 0 ? RESULT_OK : RESULT_FAIL;
  }
  return RESULT_FAIL;
}

void MessageHandler::Broadcast(const uint32_t robotId, const RobotInterface::RobotToEngine& message)
{
  u32 type = static_cast<u32>(message.GetTag());
  _eventMgr.Broadcast(robotId, AnkiEvent<RobotInterface::RobotToEngine>(BaseStationTimer::getInstance()->GetCurrentTimeInSeconds(), type, message));
}

void MessageHandler::Broadcast(const uint32_t robotId, RobotInterface::RobotToEngine&& message)
{
  u32 type = static_cast<u32>(message.GetTag());
  _eventMgr.Broadcast(robotId, AnkiEvent<RobotInterface::RobotToEngine>(BaseStationTimer::getInstance()->GetCurrentTimeInSeconds(), type, std::move(message)));
}

void MessageHandler::ProcessPacket(const Comms::IncomingPacket& packet)
{
  switch (packet.tag) {
    case Comms::IncomingPacket::Tag::Connected:
    {
      if (!_robotManager->DoesRobotExist(static_cast<RobotID_t>(packet.sourceId))) {
        PRINT_STREAM_ERROR("RobotMessageHandler.Connected",
          "Incoming connection not found for robot id " << packet.sourceId << ", address " << packet.sourceAddress << ". Disconnecting.");
        return;
      }

      PRINT_STREAM_DEBUG("RobotMessageHandler.Connected",
        "Connection accepted from connection id " << packet.sourceId << ", address " << packet.sourceAddress << ".");
      return;
    }
    case Comms::IncomingPacket::Tag::Disconnected:
    {
      PRINT_STREAM_INFO("RobotMessageHandler.Disconnected",
        "Disconnected from connection id " << packet.sourceId << ", address " << packet.sourceAddress << ".");
      
      // Make a local copy of the address we'll want to reconnect to
      _cachedAddress.reset(new Util::TransportAddress(packet.sourceAddress));
      // Remove the connection from the channel
      _channel->RemoveConnection(packet.sourceId);
      // Cancel/clear all actions in the robot queue
      _robotManager->GetFirstRobot()->GetActionList().Clear();
      return;
    }
    case Comms::IncomingPacket::Tag::NormalMessage:
    {
      break;
    }
      // should be handled internally by MultiClientChannel, but in case it's a different IChannel
    case Comms::IncomingPacket::Tag::ConnectionRequest:
    {
      _channel->RefuseIncomingConnection(packet.sourceAddress);
      return;
    }

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
  const size_t unpackSize = message.Unpack(packet.buffer, packet.bufferSize);
  if (unpackSize != packet.bufferSize) {
    PRINT_NAMED_ERROR("RobotMessageHandler.MessageUnpack", "Message unpack error, tag %02x expecting %d but have %d",
                      msgID, static_cast<int>(unpackSize), packet.bufferSize);
    return;
  }
  
#if ANKI_DEV_CHEATS
  if (nullptr != DevLoggingSystem::GetInstance())
  {
    DevLoggingSystem::GetInstance()->LogMessage(message);
  }
#endif

  Broadcast(robotID, std::move(message));
}
  
Result MessageHandler::AddRobotConnection(const ExternalInterface::ConnectToRobot& connectMsg)
{
  Anki::Comms::ConnectionId id = static_cast<Anki::Comms::ConnectionId>(connectMsg.robotID);
  int port = !connectMsg.isSimulated ? Anki::Cozmo::ROBOT_RADIO_BASE_PORT : Anki::Cozmo::ROBOT_RADIO_BASE_PORT + connectMsg.robotID;
  Anki::Util::TransportAddress address((const char*)connectMsg.ipAddress.data(), static_cast<uint16_t>(port));
  _channel->AddConnection(id, address);
  
  return RESULT_OK;
}
 
const Util::Stats::StatsAccumulator& MessageHandler::GetQueuedTimes_ms() const
{
  return _channel->GetQueuedTimes_ms();
}
  
template<>
void MessageHandler::HandleMessage(const ExternalInterface::ReliableTransportRunMode& msg)
{
  _channel->SetReliableTransportRunMode(msg.isSync);
}
  
} // end namespace RobotInterface
} // end namespace Cozmo
} // end namespace Anki
