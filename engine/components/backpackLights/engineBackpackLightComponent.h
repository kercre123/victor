/**
 * File: backpackLightComponent.h
 *
 * Author: Al Chaussee
 * Created: 1/23/2017
 *
 * Description: Manages various lights on Cozmo's body. 
 *              Currently this includes the backpack lights and headlight.
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Anki_Cozmo_Basestation_Components_BackpackLightComponent_H__
#define __Anki_Cozmo_Basestation_Components_BackpackLightComponent_H__

#include "coretech/common/shared/types.h"
#include "clad/types/ledTypes.h"
#include "clad/types/backpackAnimationTriggers.h"

#include "engine/ankiEventUtil.h"
#include "engine/robotComponents_fwd.h"
#include "engine/components/backpackLights/engineBackpackLightAnimation.h"
#include "engine/components/backpackLights/engineBackpackLightComponentTypes.h"
#include "engine/robotComponents_fwd.h"

#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/helpers/noncopyable.h"
#include "util/signals/simpleSignal_fwd.h"

#include <list>
#include <map>
#include <memory>

namespace Anki {
namespace Vector {
  
class Robot;
class BackpackLightAnimationContainer;

class BackpackLightComponent : public IDependencyManagedComponent<RobotComponentID>, private Util::noncopyable
{
public:
  BackpackLightComponent();

  //////
  // IDependencyManagedComponent functions
  //////
  virtual void InitDependent(Vector::Robot* robot, const RobotCompMap& dependentComps) override;
  virtual void GetInitDependencies(RobotCompIDSet& dependencies) const override {
    dependencies.insert(RobotComponentID::CozmoContextWrapper);
  };
  virtual void GetUpdateDependencies(RobotCompIDSet& dependencies) const override {
    dependencies.insert(RobotComponentID::AIComponent);
    dependencies.insert(RobotComponentID::Battery);
  };
    
  virtual void UpdateDependent(const RobotCompMap& dependentComps) override;
  //////
  // end IDependencyManagedComponent functions
  //////
  
  template<typename T>
  void HandleMessage(const T& msg);
    
  void ClearAllBackpackLightConfigs() { _backpackLightList.clear(); }
  
  // Allows the caller to start a looping light pattern on Vector's backpack and get a handle to cancel it later.
  void StartLoopingBackpackAnimation(const BackpackLightAnimation::BackpackAnimation& lights,
                                     BackpackLightDataLocator& lightLocator_out);
  
  // Related to the above start looping call, allows the caller to cancel a currently looping light pattern
  bool StopLoopingBackpackAnimation(const BackpackLightDataLocator& lightDataLocator);
  
  // General purpose call to set backpack lights. The light pattern will persist until this function is called again.
  void SetBackpackAnimation(const BackpackLightAnimation::BackpackAnimation& lights);
  
  // Start the backpack lights associated with a trigger
  void SetBackpackAnimation(const BackpackAnimationTrigger& trigger);

private:
    
  Result SetBackpackAnimationInternal(const BackpackLightAnimation::BackpackAnimation& lights);
  
  void StartLoopingBackpackAnimationInternal(const BackpackLightAnimation::BackpackAnimation& lights,
                                             BackpackLightDataLocator& lightLocator_out);
  
  void SetBackpackAnimationInternal(const BackpackAnimationTrigger& trigger);

  Robot* _robot = nullptr;
  std::list<Signal::SmartHandle> _eventHandles;

  // Contains a list of light configurations
  BackpackLightList _backpackLightList;
  
  // Reference to the most recently used light configuration
  BackpackLightDataRefWeak _curBackpackLightConfig;
  
  // Locator handle for the shared light configuration associated with SetBackpackLightAnimation above
  BackpackLightDataLocator _sharedLightConfig{};
  
};

}
}

#endif

