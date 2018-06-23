/**
* File: behaviorEventComponent.h
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

#ifndef __Cozmo_Basestation_BehaviorComponent_BehaviorEventComponent_H__
#define __Cozmo_Basestation_BehaviorComponent_BehaviorEventComponent_H__

#include "clad/externalInterface/messageEngineToGameTag.h"
#include "clad/externalInterface/messageGameToEngineTag.h"
#include "clad/robotInterface/messageRobotToEngineTag.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior_fwd.h"
#include "util/entityComponent/entity.h"

#include <set>

namespace Anki {
namespace Cozmo {

// Forward Declaration
class BehaviorSystemManager;
class IBehavior;

class BehaviorEventComponent : public IBehaviorMessageSubscriber, 
                               public IDependencyManagedComponent<BCComponentID>, 
                               private Util::noncopyable {
public:
  BehaviorEventComponent();
  virtual ~BehaviorEventComponent(){};
  
  //////
  // IDependencyManagedComponent functions
  //////
  virtual void InitDependent(Robot* robot, const BCCompMap& dependentComps) override;
  virtual void GetInitDependencies(BCCompIDSet& dependencies) const override {}
  virtual void GetUpdateDependencies(BCCompIDSet& dependencies) const override {};
  //////
  // end IDependencyManagedComponent functions
  //////


  void Init(IBehaviorMessageSubscriber& messageSubscriber);

  
  virtual void SubscribeToTags(IBehavior* subscriber, std::set<ExternalInterface::MessageGameToEngineTag>&& tags) const override;
  virtual void SubscribeToTags(IBehavior* subscriber, std::set<ExternalInterface::MessageEngineToGameTag>&& tags) const override;
  virtual void SubscribeToTags(IBehavior* subscriber, std::set<RobotInterface::RobotToEngineTag>&& tags) const override;
  virtual void SubscribeToTags(IBehavior* subscriber, std::set<AppToEngineTag>&& tags) const override;

  const std::vector<const GameToEngineEvent>& GetGameToEngineEvents() const   { return _gameToEngineEvents;}
  const std::vector<const EngineToGameEvent>& GetEngineToGameEvents() const   { return _engineToGameEvents;}
  const std::vector<const RobotToEngineEvent>& GetRobotToEngineEvents() const { return _robotToEngineEvents;}
  const std::vector<const AppToEngineEvent>& GetAppToEngineEvents() const { return _appToEngineEvents;}

  const std::vector<ExternalInterface::RobotCompletedAction>& GetActionsCompletedThisTick() const { return _actionsCompletedThisTick;}
  
protected:
  friend class BehaviorManager;
  friend class BehaviorSystemManager;
  friend class BehaviorStack;
  std::vector<const GameToEngineEvent>  _gameToEngineEvents;
  std::vector<const EngineToGameEvent>  _engineToGameEvents;
  std::vector<const RobotToEngineEvent> _robotToEngineEvents;
  std::vector<const AppToEngineEvent>   _appToEngineEvents;
  
  std::vector<ExternalInterface::RobotCompletedAction> _actionsCompletedThisTick;
  
private:
  struct SubscriberWrapper{
    SubscriberWrapper(IBehaviorMessageSubscriber& ref)
    :_ref(ref){}
    IBehaviorMessageSubscriber& _ref;
  };
  std::unique_ptr<SubscriberWrapper> _messageSubscriber;


  
};
  
  


} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorComponent_BehaviorEventComponent_H__
