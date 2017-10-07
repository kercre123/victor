/**
* File: behaviorComponent.h
*
* Author: Kevin M. Karol
* Created: 9/22/17
*
* Description: Component responsible for maintaining all aspects of the AI system
* relating to behaviors
*
* Copyright: Anki, Inc. 2017
*
**/


#ifndef __Cozmo_Basestation_BehaviorSystem_BehaviorComponent_H__
#define __Cozmo_Basestation_BehaviorSystem_BehaviorComponent_H__

#include "clad/externalInterface/messageEngineToGameTag.h"
#include "clad/externalInterface/messageGameToEngineTag.h"
#include "clad/robotInterface/messageRobotToEngineTag.h"

#include "util/helpers/noncopyable.h"

#include <assert.h>
#include <memory>
#include <set>

#define USE_BSM 1


namespace Anki {
namespace Cozmo {

// Forward declarations
class AsyncMessageGateComponent;
class BehaviorContainer;
class BehaviorEventAnimResponseDirector;
class BehaviorExternalInterface;
class BehaviorHelperComponent;
class BehaviorManager;
class BehaviorSystemManager;
class DelegationComponent;
class IBehavior;
class Robot;
class StateChangeComponent;
  
namespace Audio {
class BehaviorAudioComponent;
}

class BehaviorComponent : private Util::noncopyable
{
public:
  BehaviorComponent(Robot& robot);
  ~BehaviorComponent();
  
  void Init(Robot& robot);
  
  void Update(Robot& robot,
              std::string& currentActivityName,
              std::string& behaviorDebugStr);
  
  void OnRobotDelocalized();
  
  void SubscribeToTags(IBehavior* subscriber, std::set<ExternalInterface::MessageGameToEngineTag>&& tags) const;
  void SubscribeToTags(IBehavior* subscriber, std::set<ExternalInterface::MessageEngineToGameTag>&& tags) const;
  void SubscribeToTags(IBehavior* subscriber, std::set<RobotInterface::RobotToEngineTag>&& tags) const;
  
  inline const BehaviorEventAnimResponseDirector& GetBehaviorEventAnimResponseDirector() const
           { assert(_behaviorEventAnimResponseDirector); return *_behaviorEventAnimResponseDirector; }
  
protected:
  // Support legacy cozmo code
  friend class Robot;
  friend class AIComponent;
  inline const BehaviorManager& GetBehaviorManager() const { return *_behaviorMgr; }
  inline BehaviorManager&       GetBehaviorManager()       { return *_behaviorMgr; }
  
  inline const BehaviorHelperComponent& GetBehaviorHelperComponent() const { assert(_behaviorHelperComponent); return *_behaviorHelperComponent; }
  inline BehaviorHelperComponent&       GetBehaviorHelperComponent()       { assert(_behaviorHelperComponent); return *_behaviorHelperComponent; }
  
  // For test only
  inline BehaviorContainer& GetBehaviorContainer() { return *_behaviorContainer; }
  
private:
  // Component which behaviors and helpers can query to find out the appropriate animation
  // to play in response to a user facing action result
  std::unique_ptr<BehaviorEventAnimResponseDirector> _behaviorEventAnimResponseDirector;
  
  // component which behaviors can delegate to for automatic action error handling
  std::unique_ptr<BehaviorHelperComponent> _behaviorHelperComponent;
  
  // Factory creates and tracks data-driven behaviors etc
  std::unique_ptr<BehaviorContainer> _behaviorContainer;
  
  // Interface that the behavior system can use to communicate with the rest of engine
  std::unique_ptr<BehaviorExternalInterface> _behaviorExternalInterface;
  
  std::unique_ptr<StateChangeComponent> _stateChangeComponent;
  std::unique_ptr<AsyncMessageGateComponent> _asyncMessageComponent;

  // components which manage the behavior system
  std::unique_ptr<BehaviorManager>       _behaviorMgr;
  std::unique_ptr<BehaviorSystemManager> _behaviorSysMgr;
  
  // Behavior audio client is used to update the audio engine with the current sparked state (a.k.a. "round")
  std::unique_ptr<Audio::BehaviorAudioComponent> _audioClient;
  
  std::unique_ptr<DelegationComponent>   _delegationComponent;
  
};

} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorComponent_H__

