/**
 * File: BehaviorGreetAfterLongTime.h
 *
 * Author: Hamzah Khan
 * Created: 2018-06-25
 *
 * Description: This behavior causes victor to, upon seeing a face, react "loudly" if he hasn't seen the person for a certain amount of time.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorGreetAfterLongTime__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorGreetAfterLongTime__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

#include "engine/smartFaceId.h"

#include "coretech/common/engine/robotTimeStamp.h"

#include "osState/wallTime.h"

#include <string>

namespace Anki {
namespace Vector {


class BehaviorGreetAfterLongTime : public ICozmoBehavior
{
public:
  virtual ~BehaviorGreetAfterLongTime();

  using LastSeenMapPtr = std::shared_ptr<std::unordered_map<std::string, time_t>>;
  static void DebugPrintState(const char* debugLabel, const LastSeenMapPtr map);

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorGreetAfterLongTime(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.visionModesForActiveScope->insert({ VisionMode::DetectingFaces, EVisionUpdateFrequency::High });
  };
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual void InitBehavior() override;
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;

private:

  void TurnTowardsFace();  
  void PlayReactionAnimation();

  // runs checks to decide whether the behavior update should run during this tick
  bool ShouldBehaviorUpdate(time_t&);

  void DebugPrintState(const char* debugLabel) const;

  uint GetCooldownPeriod_s() const;

  struct InstanceConfig {
    InstanceConfig();
    ICozmoBehaviorPtr driveOffChargerBehavior;
  };

  struct DynamicVariables {
    DynamicVariables();
    LastSeenMapPtr lastSeenTimesPtr;

    // last time a face was seen
    RobotTimeStamp_t lastFaceCheckTime_ms;

    // should play reaction flag as long as we can play it before the given (basestation) time
    float shouldActivateBehaviorUntil_s;
    SmartFaceID targetFace;

    // for DAS
    int timeSinceSeenThisFace_s;
    int thisCooldown_s;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorGreetAfterLongTime__
