/**
 * File: aiComponent.h
 *
 * Author: Brad Neuman
 * Created: 2016-12-07
 *
 * Description: Component for all aspects of the higher level AI
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_AiComponent_H__
#define __Cozmo_Basestation_Behaviors_AiComponent_H__

#include "coretech/common/shared/types.h"
#include "engine/aiComponent/aiComponents_fwd.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponents_fwd.h"
#include "engine/robotComponents_fwd.h"
#include "util/entityComponent/componentWrapper.h"
#include "util/entityComponent/dependencyManagedEntity.h"
#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/helpers/noncopyable.h"

#include <assert.h>
#include <string>

namespace Anki {
namespace Cozmo {

class BehaviorContainer;

// AIComponent is updated at the robot component level, same as BehaviorComponent
// Therefore BCComponents (which are managed by BehaviorComponent) can't declare dependencies on AIComponent
// since when it's Init/Update relative to BehaviorComponent must be declared by BehaviorComponent explicitly,
// not by individual components within BehaviorComponent
class AIComponent :  public UnreliableComponent<BCComponentID>,
                     public IDependencyManagedComponent<RobotComponentID>,
                     private Util::noncopyable
{
public:
  explicit AIComponent();
  virtual ~AIComponent();

  //////
  // IDependencyManagedComponent functions
  //////
  virtual void InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComps) override final;
  virtual void GetInitDependencies(RobotCompIDSet& dependencies) const override {
    dependencies.insert(RobotComponentID::CozmoContextWrapper);
    dependencies.insert(RobotComponentID::DataAccessor);
    dependencies.insert(RobotComponentID::FaceWorld);
    dependencies.insert(RobotComponentID::MicComponent);
    dependencies.insert(RobotComponentID::MoodManager);
    dependencies.insert(RobotComponentID::NVStorage);
    dependencies.insert(RobotComponentID::Vision);
  };
  virtual void UpdateDependent(const RobotCompMap& dependentComps) override;
  virtual void GetUpdateDependencies(RobotCompIDSet& dependencies) const override {
    dependencies.insert(RobotComponentID::BlockTapFilter);
    dependencies.insert(RobotComponentID::BlockWorld);
    dependencies.insert(RobotComponentID::CliffSensor);
    dependencies.insert(RobotComponentID::FaceWorld);
    dependencies.insert(RobotComponentID::Inventory);
    dependencies.insert(RobotComponentID::Map);
    dependencies.insert(RobotComponentID::Movement);
    dependencies.insert(RobotComponentID::MoodManager);
    dependencies.insert(RobotComponentID::PetWorld);
    dependencies.insert(RobotComponentID::ProxSensor);
    dependencies.insert(RobotComponentID::RobotIdleTimeout);
    dependencies.insert(RobotComponentID::TouchSensor);
    dependencies.insert(RobotComponentID::Vision);
    dependencies.insert(RobotComponentID::VisionScheduleMediator);
  };

  // Prevent hiding function warnings by exposing the (valid) unreliable component methods
  using UnreliableComponent<BCComponentID>::InitDependent;
  using UnreliableComponent<BCComponentID>::GetInitDependencies;
  using UnreliableComponent<BCComponentID>::UpdateDependent;
  using UnreliableComponent<BCComponentID>::GetUpdateDependencies;
  //////
  // end IDependencyManagedComponent functions
  //////


  ////////////////////////////////////////////////////////////////////////////////
  // Components
  ////////////////////////////////////////////////////////////////////////////////
  template<typename T>
  T& GetComponent() const {assert(_aiComponents); return _aiComponents->GetComponent<T>();}

  template<typename T>
  T* GetComponentPtr() const {assert(_aiComponents); return _aiComponents->GetComponentPtr<T>();}

  #if ANKI_DEV_CHEATS
  // For test only
  BehaviorContainer& GetBehaviorContainer();
  #endif

  ////////////////////////////////////////////////////////////////////////////////
  // Message handling / dispatch
  ////////////////////////////////////////////////////////////////////////////////

  void OnRobotDelocalized();
  void OnRobotRelocalized();

  ////////////////////////////////////////////////////////////////////////////////
  // Accessors
  ////////////////////////////////////////////////////////////////////////////////

  inline bool IsSuddenObstacleDetected() const { return _suddenObstacleDetected; }

private:
  Robot* _robot = nullptr;
  using EntityType = DependencyManagedEntity<AIComponentID>;
  using ComponentPtr = std::unique_ptr<EntityType>;

  ComponentPtr _aiComponents;
  bool   _suddenObstacleDetected;

  void CheckForSuddenObstacle(Robot& robot);
};

}
}

#endif
