/**
 * File: behaviorTrackCube.h
 *
 * Author: ross
 * Created: 2018-03-15
 *
 * Description: tracks a cube with eyes
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorTrackCube__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorTrackCube__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Cozmo {

class BehaviorTrackCube : public ICozmoBehavior
{
public: 
  virtual ~BehaviorTrackCube();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorTrackCube(const Json::Value& config);

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;

private:
  
  ObjectID GetVisibleCube() const;
  void StartAction();

  struct InstanceConfig {
    InstanceConfig();
    int maxTimeSinceObserved_ms;
    int maxDistance_mm;
  };

  struct DynamicVariables {
    DynamicVariables();
    ObjectID cube;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
};

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorTrackCube__
