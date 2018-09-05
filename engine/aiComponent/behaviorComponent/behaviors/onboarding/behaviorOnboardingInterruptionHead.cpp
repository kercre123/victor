/**
 * File: behaviorOnboardingInterruptionHead.cpp
 *
 * Author: ross
 * Created: 2018-06-03
 *
 * Description: Interruption to match the current animation when picked up or on charger. This
 *              has special casing around the first stage, but isn't part of that stage so that all
 *              picked up/on charger logic can be centralized
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/onboarding/behaviorOnboardingInterruptionHead.h"

#include "clad/types/onboardingStages.h"
#include "engine/aiComponent/aiWhiteboard.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "util/cladHelpers/cladFromJSONHelpers.h"

namespace Anki {
namespace Vector {
  
namespace {
  const char* const kAnimWhenGroggyKey = "animWhenGroggy";
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboardingInterruptionHead::InstanceConfig::InstanceConfig()
{
  animWhenGroggy = AnimationTrigger::Count;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboardingInterruptionHead::DynamicVariables::DynamicVariables()
{
  isGroggy = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboardingInterruptionHead::BehaviorOnboardingInterruptionHead(const Json::Value& config)
 : ICozmoBehavior(config)
{
  JsonTools::GetCladEnumFromJSON(config, kAnimWhenGroggyKey, _iConfig.animWhenGroggy, GetDebugLabel(), false);
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
  modifiers.wantsToBeActivatedWhenOnCharger = true;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingInterruptionHead::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  expectedKeys.insert( kAnimWhenGroggyKey );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingInterruptionHead::OnBehaviorActivated() 
{
  auto* action = new CompoundActionSequential;
  // make sure lift is down
  action->AddAction( new MoveLiftToHeightAction{ MoveLiftToHeightAction::Preset::LOW_DOCK } );
  
  const OnboardingStages currentStage = GetAIComp<AIWhiteboard>().GetCurrentOnboardingStage();
  if( static_cast<u8>(currentStage) < static_cast<u8>(OnboardingStages::FinishedComeHere) ) {
    // head should still be down
    auto* parallelAction = new CompoundActionParallel;
    parallelAction->AddAction( new MoveHeadToAngleAction( DEG_TO_RAD(-22.0f) ) );
    if( _dVars.isGroggy && (_iConfig.animWhenGroggy != AnimationTrigger::Count) ) {
      const u8 tracks = (u8)AnimTrackFlag::LIFT_TRACK | (u8)AnimTrackFlag::BODY_TRACK;
      auto* animAction = new TriggerLiftSafeAnimationAction{ _iConfig.animWhenGroggy, 0, true, tracks };
      parallelAction->AddAction( animAction );
    }
    action->AddAction( parallelAction );
  }
  DelegateIfInControl(action);
  
  // reset dynamic variables
  _dVars = DynamicVariables();
}

}
}
