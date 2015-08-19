/**
 * File: robotEventHandler.h
 *
 * Author: Lee
 * Created: 08/11/15
 *
 * Description: Class for subscribing to and handling events going to robots.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __Cozmo_Basestation_RobotEventHandler_H__
#define __Cozmo_Basestation_RobotEventHandler_H__

#include "anki/cozmo/shared/cozmoTypes.h"
#include "util/signals/simpleSignal_fwd.h"
#include "util/helpers/noncopyable.h"

#include <vector>

namespace Anki {
namespace Cozmo {

// Forward declarations
class ActionList;
class IActionRunner;
class IExternalInterface;
class RobotManager;
  
enum class QueueActionPosition : uint8_t;

template <typename Type>
class AnkiEvent;

namespace ExternalInterface {
  class MessageGameToEngine;
}


class RobotEventHandler : private Util::noncopyable
{
public:
  RobotEventHandler(RobotManager& manager, IExternalInterface* interface);
  
protected:
  RobotManager& _robotManager;
  IExternalInterface* _externalInterface;
  std::vector<Signal::SmartHandle> _signalHandles;
  
  void QueueActionHelper(const QueueActionPosition position, const u32 idTag, const u32 inSlot,
                         ActionList& actionList, IActionRunner* action, const u8 numRetries = 0);
  
  // Note this version does not include the idTag parameter
  void QueueActionHelper(const QueueActionPosition position, const u32 inSlot,
                         ActionList& actionList, IActionRunner* action, const u8 numRetries = 0);
  
  void HandleActionEvents(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event);
  void HandleQueueSingleAction(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event);
  void HandleQueueCompoundAction(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event);
  void HandleSetLiftHeight(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event);
};

  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_RobotEventHandler_H__