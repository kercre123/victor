/**
 * File: behaviorHighLevelAI.h
 *
 * Author: Brad Neuman
 * Created: 2017-10-16
 *
 * Description: Root behavior to handle the state machine for the high level AI of victor (similar to Cozmo's
 *              freeplay activities)
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorHighLevelAI_H__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorHighLevelAI_H__

#include "engine/aiComponent/behaviorComponent/behaviors/internalStatesBehavior.h"
#include "clad/types/behaviorComponent/postBehaviorSuggestions.h"

#include <unordered_map>

namespace Anki {
namespace Vector {

class BehaviorHighLevelAI : public InternalStatesBehavior
{
protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;  
  BehaviorHighLevelAI(const Json::Value& config);
  
public:

  virtual ~BehaviorHighLevelAI();
  
  virtual bool WantsToBeActivatedBehavior() const override { return true; }

protected:
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override{
    modifiers.wantsToBeActivatedWhenCarryingObject = true;
    modifiers.wantsToBeActivatedWhenOffTreads = true;
    modifiers.wantsToBeActivatedWhenOnCharger = true;
    modifiers.behaviorAlwaysDelegates = false;
  }
  
  virtual void BehaviorUpdate() override;
  
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual void OverrideResumeState( StateID& resumeState ) override;
  
  virtual void OnStateNameChange( const std::string& oldStateName, const std::string& newStateName ) const override;
  
  bool ShouldTransitionIntoExploring() const;

private:
  
  bool IsBehaviorActive( BehaviorID behaviorID ) const;

  void UpdateExploringTransitionCooldown();
  
  struct {
    float socializeKnownFaceCooldown_s;
    float playWithCubeCooldown_s;
    float playWithCubeOnChargerCooldown_s;
    float maxFaceDistanceToSocialize_mm;
    
    std::unordered_map< PostBehaviorSuggestions, StateID > pbsResumeOverrides;
  } _params;

  float _specialExploringTransitionCooldownBase_s;

  CustomBEIConditionHandleList CreateCustomConditions();
  
};

}
}

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorHighLevelAI_H__
