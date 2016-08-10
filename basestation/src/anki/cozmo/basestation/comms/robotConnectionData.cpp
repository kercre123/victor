/**
* File: RobotConnectionData
*
* Author: Lee Crippen
* Created: 7/6/2016
*
* Description: Has data related to a robot connection
*
* Copyright: Anki, inc. 2016
*
*/
#include "anki/cozmo/basestation/comms/robotConnectionData.h"
#include "util/logging/logging.h"


namespace Anki {
namespace Cozmo {
  
bool RobotConnectionData::HasMessages()
{
  std::lock_guard<std::mutex> lockGuard(_messageMutex);
  return !_arrivedMessages.empty();
}
  
RobotConnectionMessageData RobotConnectionData::PopNextMessage()
{
  std::lock_guard<std::mutex> lockGuard(_messageMutex);
  ASSERT_NAMED(!_arrivedMessages.empty(), "RobotConnectionData.PopNextMessage.NoMessages");
  if (_arrivedMessages.empty())
  {
    return RobotConnectionMessageData();
  }
  
  RobotConnectionMessageData nextMessage = std::move(_arrivedMessages.front());
  _arrivedMessages.pop_front();
  return nextMessage;
}

void RobotConnectionData::PushArrivedMessage(const uint8_t* buffer, uint32_t numBytes, const Util::TransportAddress& address)
{
  RobotConnectionMessageType messageType = RobotConnectionMessageType::Data;
  if (Util::INetTransportDataReceiver::OnConnectRequest == buffer)
  {
    messageType = RobotConnectionMessageType::ConnectionRequest;
  }
  else if (Util::INetTransportDataReceiver::OnConnected == buffer)
  {
    messageType = RobotConnectionMessageType::ConnectionResponse;
  }
  else if (Util::INetTransportDataReceiver::OnDisconnected == buffer)
  {
    messageType = RobotConnectionMessageType::Disconnect;
  }
  
  std::lock_guard<std::mutex> lockGuard(_messageMutex);
  if (messageType == RobotConnectionMessageType::Data)
  {
    // Note we don't bother passing the MessageType::Data into this constructor because that's the default
    _arrivedMessages.emplace_back(buffer, numBytes, address, TRACK_INCOMING_PACKET_LATENCY_TIMESTAMP_MS());
  }
  else
  {
    _arrivedMessages.emplace_back(messageType, address, TRACK_INCOMING_PACKET_LATENCY_TIMESTAMP_MS());
  }
}
  
void RobotConnectionData::ReceiveData(const uint8_t* buffer, unsigned int size, const Util::TransportAddress& sourceAddress)
{
  const bool isConnectionRequest = INetTransportDataReceiver::OnConnectRequest == buffer;
  ASSERT_NAMED(!isConnectionRequest, "RobotConnectionManager.ReceiveData.ConnectionRequest.NotHandled");
  if (isConnectionRequest)
  {
    // We don't accept requests for connection!
    return;
  }
  
  // Otherwise we hold onto the message
  PushArrivedMessage(buffer, size, sourceAddress);
}
  
void RobotConnectionData::Clear()
{
  _currentState = State::Disconnected;
  _address = {};
  {
    std::lock_guard<std::mutex> lockGuard(_messageMutex);
    _arrivedMessages.clear();
  }
}
  
void RobotConnectionData::QueueConnectionDisconnect()
{
  PushArrivedMessage(Util::INetTransportDataReceiver::OnDisconnected, 0, _address);
}
  
} // end namespace Cozmo
} // end namespace Anki
