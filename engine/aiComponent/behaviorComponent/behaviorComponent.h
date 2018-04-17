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

#include "engine/aiComponent/behaviorComponent/behaviorComponents_fwd.h"
#include "engine/aiComponent/behaviorComponent/iBehaviorMessageSubscriber.h"
#include "engine/aiComponent/aiComponents_fwd.h"
#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/entityComponent/dependencyManagedEntity.h"
#include "util/entityComponent/iManageableComponent.h"

#include "clad/externalInterface/messageEngineToGameTag.h"
#include "clad/externalInterface/messageGameToEngineTag.h"
#include "clad/robotInterface/messageRobotToEngineTag.h"


#include "util/helpers/noncopyable.h"

#include <assert.h>
#include <memory>
#include <set>

namespace Anki {
namespace Cozmo {

// Forward declarations
class AIComponent;
class AsyncMessageGateComponent;
class BEIRobotInfo;
class BehaviorComponent;
class BehaviorContainer;
class BehaviorEventAnimResponseDirector;
class BehaviorExternalInterface;
class BehaviorManager;
class BehaviorSystemManager;
class BehaviorTimers;
class BlockWorld;
class DelegationComponent;
class DevBaseBehavior;
class DevBehaviorComponentMessageHandler;
class FaceWorld;
class IBehavior;
class Robot;
class UserIntentComponent;
class BehaviorEventComponent;

namespace Audio {
class BehaviorAudioComponent;
}


class BaseBehaviorWrapper : public IDependencyManagedComponent<BCComponentID>
{
public:
  BaseBehaviorWrapper(IBehavior* baseBehavior)
  : IDependencyManagedComponent(this, BCComponentID::BaseBehaviorWrapper)
  , _baseBehavior(baseBehavior){}

  virtual ~BaseBehaviorWrapper(){}


  virtual void InitDependent(Robot* robot, const BCCompMap& dependentComponents) override {};
  virtual void UpdateDependent(const BCCompMap& dependentComponents) override {};
  virtual void GetInitDependencies(BCCompIDSet& dependencies) const override {};
  virtual void GetUpdateDependencies(BCCompIDSet& dependencies) const override {};

  IBehavior* _baseBehavior;

};

class BehaviorComponent : public IBehaviorMessageSubscriber,
                          public IDependencyManagedComponent<AIComponentID>,
                          private Util::noncopyable
{
public:
  BehaviorComponent();
  ~BehaviorComponent();

  using EntityType = DependencyManagedEntity<BCComponentID>;
  using ComponentPtr = std::unique_ptr<EntityType>;

  // IDependencyManagedComponent<AIComponentID> functions
  virtual void GetInitDependencies(AICompIDSet& dependencies) const override {
    dependencies.insert(AIComponentID::BEIConditionFactory);
  }
  
  virtual void InitDependent(Robot* robot, const AICompMap& dependentComponents) override;

  virtual void GetUpdateDependencies(AICompIDSet& dependencies) const override {
    dependencies.insert(AIComponentID::InformationAnalyzer);
    dependencies.insert(AIComponentID::ContinuityComponent);
    dependencies.insert(AIComponentID::Whiteboard);
  };
  
  virtual void AdditionalUpdateAccessibleComponents(AICompIDSet& components) const override {
    components.insert(AIComponentID::BEIConditionFactory);
  }

  virtual void UpdateDependent(const AICompMap& dependentComps) override;


  // end IDependencyManagedComponent<AIComponentID> functions

  // Pass in any components that have already been initialized as part of the entity
  // all other required components will be automatically generated
  static void GenerateManagedComponents(Robot& robot,
                                        ComponentPtr& entity);

  // NOTE: BehaviorComponent
  void SetComponents(ComponentPtr&& components);


  template<typename T>
  T& GetComponent() const {return _comps->GetValue<T>();}

  virtual void SubscribeToTags(IBehavior* subscriber, std::set<ExternalInterface::MessageGameToEngineTag>&& tags) const override;
  virtual void SubscribeToTags(IBehavior* subscriber, std::set<ExternalInterface::MessageEngineToGameTag>&& tags) const override;
  virtual void SubscribeToTags(IBehavior* subscriber, std::set<RobotInterface::RobotToEngineTag>&& tags) const override;

protected:
  // Support legacy cozmo code
  friend class Robot;
  friend class AIComponent;
  friend class DevBehaviorComponentMessageHandler;
  friend class TestBehaviorFramework; // for testing access to internals

  // For test only
  BehaviorContainer& GetBehaviorContainer();

private:
  ComponentPtr _comps;
};

} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorComponent_H__
