/**
 * File: BehaviorEarnedSparks
 *
 * Author: Paul Terry
 * Created: 07/18/2017
 *
 * Description: Simple behavior for Cozmo playing an animaton when he has earned sparks
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/behaviorEarnedSparks.h"

#include "engine/actions/animActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/cozmoContext.h"
#include "engine/events/animationTriggerHelpers.h"
#include "engine/needsSystem/needsManager.h"

namespace Anki {
namespace Cozmo {

using namespace ExternalInterface;


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorEarnedSparks::BehaviorEarnedSparks(const Json::Value& config)
: ICozmoBehavior(config)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorEarnedSparks::~BehaviorEarnedSparks()
{  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorEarnedSparks::WantsToBeActivatedBehavior() const
{
  if(GetBEI().HasNeedsManager()){
    auto& needsManager = GetBEI().GetNeedsManager();
    return needsManager.IsPendingSparksRewardMsg();
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEarnedSparks::OnBehaviorActivated()
{
  const AnimationTrigger animTrigger = AnimationTrigger::EarnedSparks;
  DelegateIfInControl(new TriggerLiftSafeAnimationAction(animTrigger));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEarnedSparks::OnBehaviorDeactivated()
{
  if(GetBEI().HasNeedsManager()){
    auto& needsManager = GetBEI().GetNeedsManager();
    if(needsManager.IsPendingSparksRewardMsg()){
      needsManager.SparksRewardCommunicatedToUser();
    }
  }
}


} // namespace Cozmo
} // namespace Anki
