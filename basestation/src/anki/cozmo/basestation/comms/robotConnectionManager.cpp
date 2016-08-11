/**
* File: RobotConnectionManager
*
* Author: Lee Crippen
* Created: 7/6/2016
*
* Description: Holds onto current RobotConnections
*
* Copyright: Anki, inc. 2016
*
*/

#include "anki/cozmo/basestation/actions/actionContainers.h"
#include "anki/cozmo/basestation/comms/robotConnectionData.h"
#include "anki/cozmo/basestation/comms/robotConnectionManager.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotManager.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/logging/logging.h"
#include "util/transport/udpTransport.h"
#include "util/transport/reliableConnection.h"
#include "util/transport/reliableTransport.h"

// Match embedded reliableTransport.h
#define RELIABLE_PACKET_HEADER_PREFIX "COZ\x03"
#define RELIABLE_PACKET_HEADER_PREFIX_LENGTH 4

namespace Anki {
namespace Cozmo {
  
RobotConnectionManager::RobotConnectionManager(RobotManager* robotManager)
: _currentConnectionData(new RobotConnectionData())
, _udpTransport(new Util::UDPTransport())
, _reliableTransport(new Util::ReliableTransport(_udpTransport.get(), _currentConnectionData.get()))
, _robotManager(robotManager)
{
  
}

RobotConnectionManager::~RobotConnectionManager()
{
  DisconnectCurrent();
  _reliableTransport->StopClient();
}

void RobotConnectionManager::Init()
{
  _udpTransport->SetHeaderPrefix((uint8_t*)(RELIABLE_PACKET_HEADER_PREFIX), RELIABLE_PACKET_HEADER_PREFIX_LENGTH);
  _udpTransport->SetDoesHeaderHaveCRC(false);
  _udpTransport->SetMaxNetMessageSize(1420);
  _udpTransport->SetPort(0);
  
  ConfigureReliableTransport();
  
  _reliableTransport->StartClient();
}

void RobotConnectionManager::ConfigureReliableTransport()
{
  // Set parameters for all reliable transport instances
  Util::ReliableTransport::SetSendAckOnReceipt(false);
  Util::ReliableTransport::SetSendUnreliableMessagesImmediately(false);
  Util::ReliableTransport::SetMaxPacketsToReSendOnAck(0);
  Util::ReliableTransport::SetMaxPacketsToSendOnSendMessage(1);
  Util::ReliableTransport::SetTrackAckLatency(true);
  // Set parameters for all reliable connections
  Util::ReliableConnection::SetTimeBetweenPingsInMS(33.3); // Heartbeat interval
  Util::ReliableConnection::SetTimeBetweenResendsInMS(33.3);
  Util::ReliableConnection::SetMaxTimeSinceLastSend( Util::ReliableConnection::GetTimeBetweenResendsInMS() - 1.0 );
  Util::ReliableConnection::SetConnectionTimeoutInMS(5000.0);
  Util::ReliableConnection::SetPacketSeparationIntervalInMS(2.0);
  Util::ReliableConnection::SetMaxPingRoundTripsToTrack(10);
  Util::ReliableConnection::SetSendSeparatePingMessages(false);
  Util::ReliableConnection::SetSendPacketsImmediately(false);
  Util::ReliableConnection::SetMaxPacketsToReSendOnUpdate(1);
}
  
void RobotConnectionManager::Update()
{
  ANKI_CPU_PROFILE("RobotConnectionManager::Update");
  
  // If we're running reliableTransport sync we need to update it
  if(_reliableTransport->IsSynchronous())
  {
    _reliableTransport->Update();
  }
  
  ProcessArrivedMessages();
}
  
void RobotConnectionManager::Connect(const Util::TransportAddress& address)
{
  _currentConnectionData->Clear();
  
  _currentConnectionData->SetAddress(address);
  _reliableTransport->Connect(address);
}
  
void RobotConnectionManager::DisconnectCurrent()
{
  _reliableTransport->Disconnect(_currentConnectionData->GetAddress());
  _currentConnectionData->QueueConnectionDisconnect();
}
  
bool RobotConnectionManager::SendData(const uint8_t* buffer, unsigned int size)
{
  const bool validState = IsValidConnection();
  ASSERT_NAMED(validState, "RobotConnectionManager.SendData.NotValidState");
  if (!validState)
  {
    return false;
  }
  
  _reliableTransport->SendData(true, _currentConnectionData->GetAddress(), buffer, size);
  
  return true;
}

void RobotConnectionManager::ProcessArrivedMessages()
{
  while (_currentConnectionData->HasMessages())
  {
    RobotConnectionMessageData nextMessage = _currentConnectionData->PopNextMessage();
    
#if TRACK_INCOMING_PACKET_LATENCY
    const auto& timeReceived = nextMessage.GetTimeReceived();
    if (timeReceived != Util::kNetTimeStampZero)
    {
      const double timeQueued_ms = Util::GetCurrentNetTimeStamp() - timeReceived;
      _queuedTimes_ms.AddStat(timeQueued_ms);
    }
#endif
    
    if (RobotConnectionMessageType::Data == nextMessage.GetType())
    {
      HandleDataMessage(nextMessage);
    }
    else if (RobotConnectionMessageType::ConnectionResponse == nextMessage.GetType())
    {
      HandleConnectionResponseMessage(nextMessage);
    }
    else if (RobotConnectionMessageType::Disconnect == nextMessage.GetType())
    {
      HandleDisconnectMessage(nextMessage);
    }
    else if (RobotConnectionMessageType::ConnectionRequest == nextMessage.GetType())
    {
      HandleConnectionRequestMessage(nextMessage);
    }
    else
    {
      PRINT_NAMED_ERROR("RobotConnectionManager.ProcessArrivedMessages.UnhandledMessageType",
                        "Unhandled message type %d. Ignoring", nextMessage.GetType());
    }
  }
}
  
void RobotConnectionManager::HandleDataMessage(RobotConnectionMessageData& nextMessage)
{
  const bool validState = IsValidConnection();
  if (!validState)
  {
    PRINT_NAMED_ERROR("RobotConnectionManager.HandleDataMessage.NotValidState", "Connection not yet valid, dropping message");
    return;
  }
  
  const bool correctAddress = _currentConnectionData->GetAddress() == nextMessage.GetAddress();
  if (!correctAddress)
  {
    PRINT_NAMED_ERROR("RobotConnectionManager.HandleDataMessage.IncorrectAddress",
                      "Expected messages from %s but arrived from %s. Dropping message.",
                      _currentConnectionData->GetAddress().ToString().c_str(),
                      nextMessage.GetAddress().ToString().c_str());
    return;
  }
  
  _readyData.push_back(std::move(nextMessage.GetData()));
}

void RobotConnectionManager::HandleConnectionResponseMessage(RobotConnectionMessageData& nextMessage)
{
  const bool isWaitingState = _currentConnectionData->GetState() == RobotConnectionData::State::Disconnected;
  if (!isWaitingState)
  {
    PRINT_NAMED_ERROR("RobotConnectionManager.HandleConnectionResponseMessage.NotWaitingForConnection", "Got connection response at unexpected time");
    return;
  }
  
  _currentConnectionData->SetState(RobotConnectionData::State::Connected);
}

void RobotConnectionManager::HandleDisconnectMessage(RobotConnectionMessageData& nextMessage)
{
  // This connection is no longer valid.
  // Note not calling DisconnectCurrent because this message means reliableTransport is already deleting this connection data
  _currentConnectionData->Clear();
  
  // This robot is gone.
  Robot* robot =  _robotManager->GetFirstRobot();
  if (nullptr != robot)
  {
    _robotManager->RemoveRobot(robot->GetID());
  }
}

void RobotConnectionManager::HandleConnectionRequestMessage(RobotConnectionMessageData& nextMessage)
{
  PRINT_NAMED_WARNING("RobotConnectionManager.HandleConnectionRequestMessage",
                      "Received connection request from %s. Ignoring",
                      nextMessage.GetAddress().ToString().c_str());
}
  
bool RobotConnectionManager::IsValidConnection() const
{
  return _currentConnectionData->GetState() == RobotConnectionData::State::Connected;
}
  
bool RobotConnectionManager::PopData(std::vector<uint8_t>& data_out)
{
  const bool hasData = !_readyData.empty();
  if (!hasData)
  {
    return false;
  }

  data_out = std::move(_readyData.front());
  _readyData.pop_front();
  return true;
}
  
const Anki::Util::Stats::StatsAccumulator& RobotConnectionManager::GetQueuedTimes_ms() const
{
#if TRACK_INCOMING_PACKET_LATENCY
  return _queuedTimes_ms.GetPrimaryAccumulator();
#else
  static Anki::Util::Stats::StatsAccumulator sNullStats;
  return sNullStats;
#endif // TRACK_INCOMING_PACKET_LATENCY
}
  
void RobotConnectionManager::SetReliableTransportRunMode(bool isSync)
{
  _reliableTransport->ChangeSyncMode(isSync);
}

} // end namespace Cozmo
} // end namespace Anki
