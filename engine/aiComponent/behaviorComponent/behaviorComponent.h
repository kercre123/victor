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
#include "engine/dependencyManagedComponent.h"
#include "engine/dependencyManagedEntity.h"
#include "engine/entity.h"

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
class BehaviorComponentCloudReceiver;
class BehaviorContainer;
class BehaviorEventAnimResponseDirector;
class BehaviorExternalInterface;
class BehaviorHelperComponent;
class BehaviorManager;
class BehaviorSystemManager;
class BlockWorld;
class DelegationComponent;
class DevBaseBehavior;
class DevBehaviorComponentMessageHandler;
class FaceWorld;
class IBehavior;
class Robot;
class BehaviorEventComponent;
  
namespace Audio {
class BehaviorAudioComponent;
}


class BaseBehaviorWrapper : public IDependencyManagedComponent<BCComponentID>
{
public:
  BaseBehaviorWrapper(IBehavior* baseBehavior)
  : IDependencyManagedComponent(BCComponentID::BaseBehaviorWrapper)
  , _baseBehavior(baseBehavior){}


  virtual void InitDependent(Robot* robot, const BCCompMap& dependentComponents) override {};
  virtual void UpdateDependent(const BCCompMap& dependentComponents) override {};
  virtual void GetInitDependencies(BCCompIDSet& dependencies) const override {};
  virtual void GetUpdateDependencies(BCCompIDSet& dependencies) const override {};

  IBehavior* _baseBehavior;

};

class BehaviorComponent : public IBehaviorMessageSubscriber, public ManageableComponent, private Util::noncopyable
{
public:
  BehaviorComponent();
  ~BehaviorComponent();
  
  struct BCComponentWrapper{
      // pass in a fully enumerated entity
      BCComponentWrapper(DependencyManagedEntity<BCComponentID,BCComponentID::Count>&& entity);
      using RequiredComponents = DependencyManagedEntityFullEnumeration<BCComponentID, BCComponentID::Count>;
      RequiredComponents _array;
  };

  using UniqueComponents   = std::unique_ptr<BCComponentWrapper>;

  // Pass in nullptr for any components you want managed internally
  // Pass in a pointer to use it as a reference for the behavior component
  static UniqueComponents GenerateManagedComponents(Robot& robot,
                                                    AIComponent&               aiComponent,
                                                    IBehavior*                 baseBehavior,
                                                    BehaviorSystemManager*     behaviorSysMgrPtr = nullptr,
                                                    BehaviorExternalInterface* behaviorExternalInterfacePtr = nullptr,
                                                    BehaviorContainer*         behaviorContainerPtr = nullptr, 
                                                    BehaviorEventComponent*    behaviorEventComponentPtr = nullptr,
                                                    AsyncMessageGateComponent* asyncMessageComponentPtr = nullptr,
                                                    DelegationComponent*       delegationComponentPtr = nullptr);
  
  void Init(Robot* robot, UniqueComponents&& components);

  void Update(Robot& robot,
              std::string& currentActivityName,
              std::string& behaviorDebugStr);
    
  template<typename T>
  T& GetComponent(BCComponentID componentID) const {assert(_components); return _components->_array.GetComponent(componentID).GetValue<T>();}

  virtual void SubscribeToTags(IBehavior* subscriber, std::set<ExternalInterface::MessageGameToEngineTag>&& tags) const override;
  virtual void SubscribeToTags(IBehavior* subscriber, std::set<ExternalInterface::MessageEngineToGameTag>&& tags) const override;
  virtual void SubscribeToTags(IBehavior* subscriber, std::set<RobotInterface::RobotToEngineTag>&& tags) const override;
  
  inline const BehaviorEventAnimResponseDirector& GetBehaviorEventAnimResponseDirector() const
    { return GetComponent<BehaviorEventAnimResponseDirector>(BCComponentID::BehaviorEventAnimResponseDirector);}

  inline BehaviorComponentCloudReceiver& GetCloudReceiver() const 
    { return GetComponent<BehaviorComponentCloudReceiver>(BCComponentID::BehaviorComponentCloudReceiver); }
           
  
protected:
  // Support legacy cozmo code
  friend class Robot;
  friend class AIComponent;
  friend class DevBehaviorComponentMessageHandler;
  friend class TestBehaviorFramework; // for testing access to internals
  



  inline const BehaviorHelperComponent& GetBehaviorHelperComponent() const 
    { return GetComponent<BehaviorHelperComponent>(BCComponentID::BehaviorHelperComponent); }
  inline BehaviorHelperComponent&       GetBehaviorHelperComponent()
    { return GetComponent<BehaviorHelperComponent>(BCComponentID::BehaviorHelperComponent); }
  
  // For test only
  BehaviorContainer& GetBehaviorContainer();
  
private:
   UniqueComponents _components;
};

} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorComponent_H__

