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

#include "anki/types.h"
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
class CozmoContext;
  
enum class QueueActionPosition : uint8_t;

template <typename Type>
class AnkiEvent;

namespace ExternalInterface {
  class MessageGameToEngine;
}


class RobotEventHandler : private Util::noncopyable
{
public:
  RobotEventHandler(const CozmoContext* context);
  
protected:
  const CozmoContext* _context;
  std::vector<Signal::SmartHandle> _signalHandles;
    
  void HandleActionEvents(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event);
  void HandleQueueSingleAction(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event);
  void HandleQueueCompoundAction(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event);
  void HandleSetLiftHeight(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event);
  void HandleEnableLiftPower(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event);
  void HandleDisplayProceduralFace(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event);
  void HandleForceDelocalizeRobot(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event);
  void HandleMoodEvent(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event);
  void HandleProgressionEvent(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event);
  void HandleBehaviorManagerEvent(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event);
  void HandleConnectToBlocks(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event);
};

  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_RobotEventHandler_H__