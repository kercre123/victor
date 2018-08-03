/**
 * File: BehaviorCoordinateInHabitat.h
 *
 * Author: Arjun Menon
 * Created: 2018-07-30
 *
 * Description: A coordinating behavior that ensures certain behaviors
 * and voice commands are suppressed or allowed when the robot knows
 * it is inside the Habitat tray
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorCoordinateInHabitat__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorCoordinateInHabitat__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/dispatch/behaviorDispatcherPassThrough.h"
#include "engine/aiComponent/behaviorComponent/behaviorTreeStateHelpers.h"

namespace Anki {
namespace Cozmo {

class BehaviorCoordinateInHabitat : public BehaviorDispatcherPassThrough
{
public: 
  virtual ~BehaviorCoordinateInHabitat();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorCoordinateInHabitat(const Json::Value& config);
  
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;

  virtual void InitPassThrough() override;
  virtual void OnPassThroughActivated() override;
  virtual void PassThroughUpdate() override;
  virtual void OnPassThroughDeactivated() override;

private:

  struct InstanceConfig {
    InstanceConfig();
    
    std::vector<std::string> suppressedBehaviorNames;
    std::vector<ICozmoBehaviorPtr> behaviorsToSuppressInHabitat;
  };

  InstanceConfig _iConfig;
};

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorCoordinateInHabitat__
