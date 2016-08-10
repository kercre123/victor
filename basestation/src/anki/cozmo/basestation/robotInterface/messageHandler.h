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

#ifndef __Anki_Cozmo_Basestation_RobotInterface_MessageHandler_H__
#define __Anki_Cozmo_Basestation_RobotInterface_MessageHandler_H__

#include "anki/cozmo/basestation/events/ankiEventMgr.h"
#include "anki/common/types.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "util/signals/simpleSignal_fwd.h"
#include <memory>

namespace Json {
  class Value;
}

namespace Anki {

namespace Comms {
class IChannel;
struct IncomingPacket;
}
  
namespace Util {
  class TransportAddress;
  namespace Stats {
    class StatsAccumulator;
  }
}

namespace Cozmo {

class RobotManager;
class CozmoContext;
class RobotConnectionManager;
  
namespace ExternalInterface {
  struct ConnectToRobot;
}

namespace RobotInterface {

class MessageHandler {
public:

  MessageHandler();
  virtual ~MessageHandler();

  virtual void Init(const Json::Value& config, RobotManager* robotMgr, const CozmoContext* context);

  virtual void ProcessMessages();

  virtual Result SendMessage(const RobotID_t robotId, const RobotInterface::EngineToRobot& msg, bool reliable = true, bool hot = false);

  Signal::SmartHandle Subscribe(const uint32_t robotId, const RobotInterface::RobotToEngineTag& tagType, std::function<void(const AnkiEvent<RobotInterface::RobotToEngine>&)> messageHandler) {
    return _eventMgr.Subscribe(robotId, static_cast<uint32_t>(tagType), messageHandler);
  }
  
  // Handle various event message types
  template<typename T>
  void HandleMessage(const T& msg);
  
  Result AddRobotConnection(const ExternalInterface::ConnectToRobot& connectMsg);
  
  void Disconnect();
  
  const Util::Stats::StatsAccumulator& GetQueuedTimes_ms() const;

protected:
  void Broadcast(const uint32_t robotId, const RobotInterface::RobotToEngine& message);
  void Broadcast(const uint32_t robotId, RobotInterface::RobotToEngine&& message);
  
private:
  AnkiEventMgr<RobotInterface::RobotToEngine, MailboxSignalMap<RobotInterface::RobotToEngine> > _eventMgr;
  RobotManager* _robotManager;
  std::unique_ptr<RobotConnectionManager> _robotConnectionManager;
  bool _isInitialized;
  std::vector<Signal::SmartHandle> _signalHandles;
  
};


} // end namespace RobotInterface
} // end namespace Cozmo
} // end namespace Anki



#endif //__Anki_Cozmo_Basestation_RobotInterface_MessageHandler_H__
