/**
 * File: TcpSocketComms
 *
 * Author: Mark Wesley
 * Created: 05/14/16
 *
 * Description: TCP implementation for socket-based communications from e.g. Game/SDK to Engine
 *
 * Copyright: Anki, Inc. 2016
 *
 **/


#include "engine/cozmoAPI/comms/tcpSocketComms.h"
#include "engine/utils/parsingConstants/parsingConstants.h"
#include "coretech/messaging/engine/IComms.h"
#include "coretech/messaging/shared/TcpServer.h"
#include "json/json.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/helpers/templateHelpers.h"
#include "util/logging/logging.h"


namespace Anki {
namespace Cozmo {

  
using MessageSizeType = uint16_t; // Must match on Engine and Python SDK side


TcpSocketComms::TcpSocketComms(bool isEnabled)
  : ISocketComms(isEnabled)
  , _tcpServer( new TcpServer() )
  , _connectedId( kDeviceIdInvalid )
  , _port(0)
  , _hasClient(false)
{
  _receivedBuffer.reserve(4096); // Big enough to hold several messages without reallocating
}


TcpSocketComms::~TcpSocketComms()
{
  Util::SafeDelete(_tcpServer);
}

  
bool TcpSocketComms::Init(UiConnectionType connectionType, const Json::Value& config)
{
  // assert(connectionType == UiConnectionType::SdkOverTcp);
  
  const Json::Value& portValue = config[AnkiUtil::kP_SDK_ON_DEVICE_TCP_PORT];
  
  if (portValue.isNumeric())
  {
    const uint32_t port = portValue.asUInt();
    _port = port;
    
    if ((uint32_t)_port != port)
    {
      PRINT_NAMED_ERROR("TcpSocketComms.Init",
                        "Bad '%s' entry in Json config file should fit in u16 (%u != %u)",
                        AnkiUtil::kP_SDK_ON_DEVICE_TCP_PORT, (uint32_t)_port, port);
      return false;
    }
    
    _tcpServer->DisconnectClient();
    _tcpServer->StopListening();
    if (IsConnectionEnabled())
    {
      PRINT_CH_INFO("UiComms", "TcpSocketComms.StartListening", "Start Listening on port %u", _port);
      _tcpServer->StartListening(_port);
    }
  }
  else
  {
    PRINT_NAMED_ERROR("TcpSocketComms.Init",
                      "Missing/Invalid '%s' entry in Json config file.", AnkiUtil::kP_SDK_ON_DEVICE_TCP_PORT);
    return false;
  }
  
  return true;
}
  
  
void TcpSocketComms::OnEnableConnection(bool wasEnabled, bool isEnabled)
{
  if (isEnabled)
  {
    _tcpServer->StartListening(_port);
  }
  else
  {
    _tcpServer->DisconnectClient();
    _tcpServer->StopListening();
  }
}


void TcpSocketComms::HandleDisconnect()
{
  _receivedBuffer.clear();
  _connectedId = kDeviceIdInvalid;
  _hasClient   = false;
}
  
  
void TcpSocketComms::UpdateInternal()
{
  ANKI_CPU_PROFILE("TcpSocketComms::Update");
  
  // See if we lost the client since last upate
  if (_hasClient && !_tcpServer->HasClient())
  {
    PRINT_CH_INFO("UiComms", "TcpSocketComms.Update.ClientLost", "Client Connection to Device %d lost", _connectedId);
    HandleDisconnect();
  }
  
  if (!_hasClient && IsConnectionEnabled())
  {
    if (_tcpServer->Accept())
    {
      _hasClient = _tcpServer->HasClient();
      if (_hasClient)
      {
        PRINT_CH_INFO("UiComms", "TcpSocketComms.Update.ClientAccepted", "Client Connected to server");
      }
    }
  }
}


bool TcpSocketComms::SendMessageInternal(const Comms::MsgPacket& msgPacket)
{
  ANKI_CPU_PROFILE("TcpSocketComms::SendMessage");
  
  if (IsConnected())
  {
    // Send the size of the message, followed by the message itself
    // so that messages can be re-assembled on the other side:
    // Send as 2 consecutive sends rather than copy into a new larger packet
    // TCP should stream it all together anyway, and both sides handle receiving partial data anyway
    
    static_assert(sizeof(msgPacket.dataLen) == 2, "size mismatch");
    
    // Data directly follows the dataLen in msgPatcket, so we can do 1 contiguous send
    
    const int res = _tcpServer->Send((const char*)&msgPacket.dataLen, sizeof(msgPacket.dataLen) + msgPacket.dataLen);
    
    if (res < 0)
    {
      return false;
    }
  }

  return false;
}


bool TcpSocketComms::ReadFromSocket()
{
  // Resize _receivedBuffer big enough to read into, then resize back to fit the size of bytes actually read
  // Buffer is reserved initially so the resize shouldn't allocate and therefore be fast
  
  const size_t kMaxReadSize = 2048;
  const size_t oldBufferSize  = _receivedBuffer.size();
  _receivedBuffer.resize( oldBufferSize + kMaxReadSize );
  
  const int bytesRecv = _tcpServer->Recv((char*)&_receivedBuffer[oldBufferSize], kMaxReadSize);
  
  if (bytesRecv > 0)
  {
    const size_t newBufferSize = oldBufferSize + bytesRecv;
    _receivedBuffer.resize(newBufferSize);
    return true;
  }
  else
  {
    _receivedBuffer.resize(oldBufferSize);
    return false;
  }
}

  
bool TcpSocketComms::ExtractNextMessage(std::vector<uint8_t>& outBuffer)
{
  if (_receivedBuffer.size() >= sizeof(MessageSizeType))
  {
    MessageSizeType sizeofMessage;
    memcpy(&sizeofMessage, &_receivedBuffer[0], sizeof(MessageSizeType));
    MessageSizeType sizeofMessageAndHeader = sizeofMessage + sizeof(MessageSizeType);
    
    if (sizeofMessageAndHeader <= _receivedBuffer.size())
    {
      if( sizeofMessage == 0 )
      {
        PRINT_NAMED_WARNING("TcpSocketComms.ExtractNextMessage", "Ignoring recieved message with zero size");
        outBuffer.clear();
      }
      else
      {
        const uint8_t* sourceBuffer = &_receivedBuffer[sizeof(MessageSizeType)];
        outBuffer = {sourceBuffer, sourceBuffer+sizeofMessage};
      }
      _receivedBuffer.erase(_receivedBuffer.begin(), _receivedBuffer.begin() + sizeofMessageAndHeader);
      return true;
    }
  }
  
  return false;
}
  
  
bool TcpSocketComms::RecvMessageInternal(std::vector<uint8_t>& outBuffer)
{
  if (IsConnected())
  {
    // Try to extract a message from already received bytes first (to avoid overfilling the recv buffer)
    if (ExtractNextMessage(outBuffer))
    {
      return true;
    }
    
    // See if there's anything else in the socket, and if that is enough to extract the next message
    
    if (ReadFromSocket())
    {
      return ExtractNextMessage(outBuffer);
    }
  }
  
  return false;
}


bool TcpSocketComms::ConnectToDeviceByID(DeviceId deviceId)
{
  assert(deviceId != kDeviceIdInvalid);
  
  if (_connectedId == kDeviceIdInvalid)
  {
    _connectedId = deviceId;
    return true;
  }
  else
  {
    PRINT_NAMED_WARNING("TcpSocketComms.ConnectToDeviceByID.Failed",
                        "Cannot connect to device %d, already connected to %d", deviceId, _connectedId);
    return false;
  }
}


bool TcpSocketComms::DisconnectDeviceByID(DeviceId deviceId)
{
  assert(deviceId != kDeviceIdInvalid);
  
  if ((_connectedId != kDeviceIdInvalid) && (_connectedId == deviceId))
  {
    _tcpServer->DisconnectClient();
    HandleDisconnect();
    return true;
  }
  else
  {
    return false;
  }
}

bool TcpSocketComms::DisconnectAllDevices() 
{
  return DisconnectDeviceByID(_connectedId);
}


void TcpSocketComms::GetAdvertisingDeviceIDs(std::vector<ISocketComms::DeviceId>& outDeviceIds)
{
  if (_tcpServer->HasClient())
  {
    if (!IsConnected())
    {
      // Advertising doesn't really make sense for TCP, just pretend we have Id 1 whenever a client connection is made
      outDeviceIds.push_back(1);
    }
  }
}


bool TcpSocketComms::IsConnected() const
{
  if ((kDeviceIdInvalid != _connectedId) && _tcpServer->HasClient())
  {
    return true;
  }
  
  return false;
}


uint32_t TcpSocketComms::GetNumConnectedDevices() const
{
  if (IsConnected())
  {
    return 1;
  }
  
  return 0;
}


} // namespace Cozmo
} // namespace Anki

