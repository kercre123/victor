/**
 * File: behaviorFindFaces.h
 *
 * Author: Lee Crippen / Brad Neuman / Matt Michini
 * Created: 2016-08-31
 *
 * Description: Delegates to the configured search behavior, terminating based on
 *              configurable stopping conditions.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorFindFaces_H__
#define __Cozmo_Basestation_Behaviors_BehaviorFindFaces_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Cozmo {
  
class BehaviorFindFaces : public ICozmoBehavior
{
protected:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorFindFaces(const Json::Value& config);
  
public:
  virtual ~BehaviorFindFaces() override {}
  virtual bool WantsToBeActivatedBehavior() const override;
  
  void InitBehavior() override;
  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.visionModesForActiveScope->insert({ VisionMode::DetectingFaces, EVisionUpdateFrequency::Med });
  }
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;

  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  virtual void BehaviorUpdate() override;
  
private:
  enum class StoppingCondition {
    Invalid,
    None,    // Simply let the search behavior finish on its own
    AnyFace, // Stop searching as soon as we know about any face
    NewFace, // Stop searching as soon as a _new_ face is seen
    Timeout, // Stop searching after a configurable timeout
  };

  struct InstanceConfig {
    InstanceConfig();   
    std::string searchBehaviorStr;
    ICozmoBehaviorPtr searchBehavior;
    
    u32 maxFaceAgeToLook_ms;
    StoppingCondition stoppingCondition;

    // Behavior timeout (used only for StoppingCondition::Timeout)
    float timeout_sec;
  };

  struct DynamicVariables {
    DynamicVariables();
    bool searchingForFaces;
    // The robot's image timestamp at the time the behavior was activated
    // (used to determine if new faces have been observed since the behavior started)
    TimeStamp_t imageTimestampWhenActivated;
    std::set<Vision::FaceID_t> startingFaces;
  };

  InstanceConfig   _iConfig;
  DynamicVariables _dVars;
  
  StoppingCondition StoppingConditionFromString(const std::string&) const;
  const char* StoppingConditionToString(StoppingCondition) const;
};
  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorFindFaces_H__
