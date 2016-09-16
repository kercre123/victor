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
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "clad/externalInterface/messageActions.h"
#include "util/signals/simpleSignal_fwd.h"
#include "util/helpers/noncopyable.h"

#include <map>
#include <vector>

namespace Anki {
namespace Cozmo {

// Forward declarations
class ActionList;
class IActionRunner;
class IPathPlanner;
class IExternalInterface;
class RobotManager;
class Robot;
class CozmoContext;
enum class QueueActionPosition : uint8_t;

template <typename Type>
class AnkiEvent;


class RobotEventHandler : private Util::noncopyable
{
public:
  RobotEventHandler(const CozmoContext* context);
  
  void SetupGainsHandlers(IExternalInterface& externalInterface);
  
  template<typename T>
  void HandleMessage(const T& msg);
  
  // Whether or not to ignore all incoming external messages that create/queue actions
  // Use with care: Make sure a call to ignore is eventually followed by a call to unignore
  void IgnoreExternalActions(bool ignore) { _ignoreExternalActions = ignore; }
  
  using ActionUnionFcn  = IActionRunner* (*)(Robot& robot, const ExternalInterface::RobotActionUnion& actionUnion);
  using GameToEngineFcn = IActionRunner* (*)(Robot& robot, const ExternalInterface::MessageGameToEngine& msg);
  
protected:
  const CozmoContext* _context;
  std::vector<Signal::SmartHandle> _signalHandles;
  using GameToEngineEvent = AnkiEvent<ExternalInterface::MessageGameToEngine>;
  using EngineToGameEvent = AnkiEvent<ExternalInterface::MessageEngineToGame>;
  
  void HandleActionEvents(const GameToEngineEvent& event);
  
private:
  // Whether or not to ignore all external action messages
  bool _ignoreExternalActions = false;
  
  std::map<ExternalInterface::RobotActionUnionTag,    ActionUnionFcn>                   _actionUnionHandlerLUT;
  std::map<ExternalInterface::MessageGameToEngineTag, std::pair<GameToEngineFcn,s32> >  _gameToEngineHandlerLUT;
  
};

  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_RobotEventHandler_H__
