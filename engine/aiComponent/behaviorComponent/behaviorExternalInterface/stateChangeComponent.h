/**
* File: stateChangeComponent.h
*
* Author: Kevin M. Karol
* Created: 10/6/17
*
* Description: Component which contains information about changes
* and events that behaviors care about which have come in during the last tick
*
* Copyright: Anki, Inc. 2017
*
**/

#ifndef __Cozmo_Basestation_BehaviorComponent_StateChangeComponent_H__
#define __Cozmo_Basestation_BehaviorComponent_StateChangeComponent_H__

#include "clad/externalInterface/messageEngineToGameTag.h"
#include "clad/externalInterface/messageGameToEngineTag.h"
#include "clad/robotInterface/messageRobotToEngineTag.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior_fwd.h"

#include <set>

namespace Anki {
namespace Cozmo {

// Forward Declaration
class BehaviorSystemManager;
class IBehavior;

class StateChangeComponent {
public:
  StateChangeComponent(){}; // for tests
  StateChangeComponent(BehaviorComponent& behaviorComponent);
  virtual ~StateChangeComponent(){};
  
  void SubscribeToTags(IBehavior* subscriber, std::set<ExternalInterface::MessageGameToEngineTag>&& tags) const;
  void SubscribeToTags(IBehavior* subscriber, std::set<ExternalInterface::MessageEngineToGameTag>&& tags) const;
  void SubscribeToTags(IBehavior* subscriber, std::set<RobotInterface::RobotToEngineTag>&& tags) const;

  const std::vector<const GameToEngineEvent>& GetGameToEngineEvents() const   { return _gameToEngineEvents;}
  const std::vector<const EngineToGameEvent>& GetEngineToGameEvents() const   { return _engineToGameEvents;}
  const std::vector<const RobotToEngineEvent>& GetRobotToEngineEvents() const { return _robotToEngineEvents;}

  const std::vector<ExternalInterface::RobotCompletedAction>& GetActionsCompletedThisTick() const { return _actionsCompletedThisTick;}
  
protected:
  friend class BehaviorSystemManager;
  friend class RunnableStack;
  std::vector<const GameToEngineEvent>  _gameToEngineEvents;
  std::vector<const EngineToGameEvent>  _engineToGameEvents;
  std::vector<const RobotToEngineEvent> _robotToEngineEvents;
  
  std::vector<ExternalInterface::RobotCompletedAction> _actionsCompletedThisTick;
  
private:
  std::unique_ptr<BehaviorComponent> _behaviorComponent;
  
};
  
  


} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorComponent_StateChangeComponent_H__
