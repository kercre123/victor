/**
 * File: protoMessageHandler.h
 *
 * Author: Shawn Blakesley
 * Date:   6/15/2018
 *
 * Description: Handles messages between gateway and engine just as 
 *              MessageHandler handles messages between basestation and robot.
 *
 * Copyright: Anki, Inc. 2018
 **/

#ifndef COZMO_PROTO_MESSAGEHANDLER_H
#define COZMO_PROTO_MESSAGEHANDLER_H

#include "engine/cozmoAPI/comms/iSocketComms.h"
#include "engine/events/ankiEventMgr.h"
#include "engine/externalInterface/gatewayInterface.h"
#include "proto/external_interface/shared.pb.h"
#include "util/signals/simpleSignal_fwd.h"

#include <memory>

// Forward declarations
namespace Anki {

namespace Util {
namespace Stats {
class StatsAccumulator;
}
}
}

namespace Anki {
namespace Cozmo {

class CozmoContext;
class Robot;
class RobotManager;
class GameMessagePort;
namespace external_interface {
  class Ping;
  class Bing;
}

class ProtoMessageHandler : public IGatewayInterface
{
public:
  ProtoMessageHandler(GameMessagePort* messagePipe); // Force construction with stuff in Init()?
  virtual ~ProtoMessageHandler();

  Result Init(CozmoContext* context, const Json::Value& config);
  Result Update();

  virtual void Broadcast(const external_interface::GatewayWrapper& message) override;
  virtual void Broadcast(external_interface::GatewayWrapper&& message) override;

  virtual Signal::SmartHandle Subscribe(const external_interface::GatewayWrapperTag& tagType, std::function<void(const AnkiEvent<external_interface::GatewayWrapper>&)> messageHandler) override;

  AnkiEventMgr<external_interface::GatewayWrapper>& GetEventMgr() { return _eventMgr; }

  const Util::Stats::StatsAccumulator& GetLatencyStats() const;

  virtual uint32_t GetMessageCountOutgoing() const override { return _messageCountOutgoing; }
  virtual uint32_t GetMessageCountIncoming() const override { return _messageCountIncoming; }
  virtual void     ResetMessageCounts() override { _messageCountOutgoing = 0; _messageCountIncoming = 0; }

private:
  // ============================== Private Member Functions ==============================

  void HandleEvents(const AnkiEvent<external_interface::GatewayWrapper>& event);
  void PingPong(const external_interface::Ping& ping); // TODO: remove these once enough example use cases exist
  void BingBong(const external_interface::Bing& bing);

  // As long as there are messages available from the comms object,
  // process them and pass them along.
  Result ProcessMessages();

  // Process a raw byte buffer as a GameToEngine CLAD message and broadcast it
  Result ProcessMessageBytes(const uint8_t* packetBytes, size_t packetSize, bool isSingleMessage);

  // Send a message to a specified ID
  virtual void DeliverToExternal(const external_interface::GatewayWrapper& message) override;

  bool ConnectToProtoDevice(ISocketComms::DeviceId deviceId);

  // ============================== Private Member Vars ==============================

  std::unique_ptr<ISocketComms>                           _socketComms;

  std::vector<Signal::SmartHandle>                        _signalHandles;

  AnkiEventMgr<external_interface::GatewayWrapper>        _eventMgr;

  std::vector<external_interface::GatewayWrapper>         _threadedMsgs;
  std::mutex                                              _mutex;

  uint32_t                                                _hostProtoDeviceID = 0;

  uint32_t                                                _updateCount = 0;

  double                                                  _lastPingTime_ms = 0.f;

  bool                                                    _isInitialized = false;

  CozmoContext*                                           _context = nullptr;

  uint32_t                                                _messageCountOutgoing = 0;
  uint32_t                                                _messageCountIncoming = 0;
}; // class MessageHandler
  
  
#undef MESSAGE_BASECLASS_NAME
  
} // namespace Cozmo
} // namespace Anki


#endif // COZMO_PROTO_MESSAGEHANDLER_H
