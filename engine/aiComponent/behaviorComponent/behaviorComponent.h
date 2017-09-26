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

#include "util/helpers/noncopyable.h"

#include <assert.h>
#include <memory>

#define USE_BSM 0


namespace Anki {
namespace Cozmo {

// Forward declarations
class BehaviorContainer;
class BehaviorEventAnimResponseDirector;
class BehaviorExternalInterface;
class BehaviorHelperComponent;
class BehaviorManager;
class BehaviorSystemManager;
class DelegationComponent;
class Robot;
  
namespace Audio {
class BehaviorAudioClient;
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
  
  // components which manage the behavior system
  std::unique_ptr<BehaviorManager>       _behaviorMgr;
  std::unique_ptr<BehaviorSystemManager> _behaviorSysMgr;
  
  // Behavior audio client is used to update the audio engine with the current sparked state (a.k.a. "round")
  std::unique_ptr<Audio::BehaviorAudioClient> _audioClient;
  
  std::unique_ptr<DelegationComponent>   _delegationComponent;
  
};

} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorComponent_H__

