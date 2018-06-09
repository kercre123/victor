/**
 * File: behaviorOnboardingInterruptionHead.cpp
 *
 * Author: ross
 * Created: 2018-06-03
 *
 * Description: Stays on charger with the head in the correct position based on onboarding stage
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/onboarding/behaviorOnboardingInterruptionHead.h"
#include "engine/aiComponent/aiWhiteboard.h"
#include "engine/actions/basicActions.h"

namespace Anki {
namespace Cozmo {
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboardingInterruptionHead::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboardingInterruptionHead::DynamicVariables::DynamicVariables()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboardingInterruptionHead::BehaviorOnboardingInterruptionHead(const Json::Value& config)
 : ICozmoBehavior(config)
{
  // TODO: read config into _iConfig
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboardingInterruptionHead::~BehaviorOnboardingInterruptionHead()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingInterruptionHead::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.behaviorAlwaysDelegates = false;
  modifiers.wantsToBeActivatedWhenOffTreads = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingInterruptionHead::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables();
  
  const OnboardingStages currentStage = GetAIComp<AIWhiteboard>().GetCurrentOnboardingStage();
  if( static_cast<u8>(currentStage) < static_cast<u8>(OnboardingStages::FinishedWelcomeHome) ) {
    // head should still be down
    auto* action = new MoveHeadToAngleAction( DEG_TO_RAD(-22.0f) );
    DelegateIfInControl(action);
  }
  
  // there will eventually be more logic here for animations
}

}
}
