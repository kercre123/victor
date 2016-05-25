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
  class MessageEngineToGame;
}


class RobotEventHandler : private Util::noncopyable
{
public:
  RobotEventHandler(const CozmoContext* context);
  
  template<typename T>
  void HandleMessage(const T& msg);
  
protected:
  const CozmoContext* _context;
  std::vector<Signal::SmartHandle> _signalHandles;
  
  using GameToEngineEvent = AnkiEvent<ExternalInterface::MessageGameToEngine>;
  using EngineToGameEvent = AnkiEvent<ExternalInterface::MessageEngineToGame>;
  
  void HandleActionEvents(const GameToEngineEvent& event);
};

  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_RobotEventHandler_H__
