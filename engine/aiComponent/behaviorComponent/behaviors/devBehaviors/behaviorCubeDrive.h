/**
 * File: BehaviorCubeDrive.h
 *
 * Author: Ron Barry
 * Created: 2019-01-07
 *
 * Description: A three-dimensional steering wheel - with 8 corners.
 *
 * Copyright: Anki, Inc. 2019
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorCubeDrive__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorCubeDrive__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

#include <chrono>
#include <memory>

namespace Anki {
namespace Vector {

namespace CubeAccelListeners {
class LowPassFilterListener;
} // namespace CubeAccelListeners

struct ActiveAccel;

class BehaviorCubeDrive : public ICozmoBehavior
{
public:
  virtual ~BehaviorCubeDrive();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorCubeDrive(const Json::Value& config);

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;

  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  virtual void BehaviorUpdate() override;

  void SetLiftState(bool up, double now);
  void RestartAnimation();

private:

  struct InstanceConfig {
    InstanceConfig();
    float                                                       triggerLiftGs;
    float                                                       deadZoneSize;
    float                                                       timeBetweenLiftActions;
    float                                                       highHeadAngle;
    float                                                       lowHeadAngle;
  };

  struct DynamicVariables {
    DynamicVariables();
    ObjectID                                                    objectId;
    std::shared_ptr<ActiveAccel>                                filteredCubeAccel;
    std::shared_ptr<CubeAccelListeners::LowPassFilterListener>  lowPassFilterListener;
    double                                                      lastLiftActionTime;
    bool                                                        liftUp;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorCubeDrive__
