/**
 * File: behaviorReactToUnclaimedIntent.cpp
 *
 * Author: ross
 * Created: 2018 feb 21
 *
 * Description: WantsToActivate when the UserIntentComponent received an intent that nobody claimed
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/victor/behaviorReactToUnclaimedIntent.h"
#include "engine/actions/animActions.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponent.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "util/cladHelpers/cladFromJSONHelpers.h"

#include "util/logging/logging.h"

namespace Anki {
namespace Vector {

namespace {
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToUnclaimedIntent::BehaviorReactToUnclaimedIntent( const Json::Value& config )
  : ICozmoBehavior( config )
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToUnclaimedIntent::GetBehaviorOperationModifiers( BehaviorOperationModifiers& modifiers ) const
{
  modifiers.wantsToBeActivatedWhenCarryingObject  = true;
  modifiers.behaviorAlwaysDelegates               = true;
  modifiers.wantsToBeActivatedWhenOnCharger       = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToUnclaimedIntent::WantsToBeActivatedBehavior() const
{
  const auto& uic = GetBehaviorComp<UserIntentComponent>();
  
  return uic.WasUserIntentUnclaimed();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToUnclaimedIntent::OnBehaviorActivated()
{
  auto& uic = GetBehaviorComp<UserIntentComponent>();
  uic.ResetUserIntentUnclaimed();
  
  const AnimationTrigger animTrigger = AnimationTrigger::ReactToUnclaimedIntent;
  const bool interruptRunning = true;
  const u32 numLoops = 1;
  
  const auto& robotInfo = GetBEI().GetRobotInfo();
  const auto tracksToLock = robotInfo.IsOnChargerPlatform()
                            ? (u8)AnimTrackFlag::BODY_TRACK
                            : (u8)AnimTrackFlag::NO_TRACKS;
  
  IAction* playAnim = new TriggerLiftSafeAnimationAction(animTrigger,
                                                         numLoops,
                                                         interruptRunning,
                                                         tracksToLock);
  
  DelegateIfInControl(playAnim, [](const ActionResult& result) {
    ANKI_VERIFY( result == ActionResult::SUCCESS,
                 "BehaviorReactToUnclaimedIntent.OnBehaviorActivated.AnimFail",
                 "Could not play animation" );
  });
}

}
}

