/**
 * File: behaviorOnboardingActivateCube.h
 *
 * Author: ross
 * Created: 2018-06-30
 *
 * Description: faces and possibly approaches cube, then plays an animation
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorOnboardingActivateCube__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorOnboardingActivateCube__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Vector {

class BehaviorOnboardingActivateCube : public ICozmoBehavior
{
public: 
  virtual ~BehaviorOnboardingActivateCube() = default;
  
  void SetTargetID(const ObjectID& targetID){
    _dVars.targetID = targetID;
  }
  
  // Required visual verification after turning toward object, and if the robot drives at all,
  // the "activation" animation and the behavior end prematurely if no cube is seen. This true by
  // default, but can be disabled to avoid blocking the user if the camera is borked
  void SetRequireConfirmation( bool requireConfirmation ) {
    _dVars.requireConfirmation = requireConfirmation;
  }
  
  bool WasSuccessful() const { return (_dVars.state == State::WaitingForAnimToEnd); }

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorOnboardingActivateCube(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override {}
  
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  virtual void BehaviorUpdate() override;

private:
  
  void TransitionToReactToCube();
  void TransitionToSetIdealSpacing( float howMuchToDrive_mm );
  void TransitionToActivateCube();
  
  bool ShouldApproachCube( float& howMuchToDrive_mm ) const;
  bool ShouldBackUpFromCube( float& howMuchToDrive_mm ) const;
  float GetDistToCube() const;
  
  // Time from start of anim to the cube lights turning on
  float GetCubeEventTime() const;
  

  enum class State : uint8_t {
    NotStarted=0,
    Facing,
    Reacting,
    SettingSpacing,
    Activating,
    WaitingForAnimToEnd,
  };
  
  struct InstanceConfig {
    InstanceConfig();
  };

  struct DynamicVariables {
    DynamicVariables();
    State state;
    
    ObjectID targetID;
    
    float activationStartTime_s;
    bool approached;
    float cubeActivationTime_s;
    
    bool requireConfirmation;
    std::weak_ptr<IActionRunner> verifyAction;
    std::weak_ptr<IActionRunner> activationAction;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorOnboardingActivateCube__
