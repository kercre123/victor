/**
 * File: behaviorReactToRobotOnFace.h
 *
 * Author: Kevin M. Karol
 * Created: 2016-07-18
 *
 * Description: Allows Cozmo to right himself when placed on his face
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToRobotOnFace.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/aiWhiteboard.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/externalInterface/externalInterface.h"
#include "clad/externalInterface/messageEngineToGame.h"

namespace Anki {
namespace Cozmo {
  
using namespace ExternalInterface;

static const float kWaitTimeBeforeRepeatAnim_s = 0.5f;
static const float kRobotMinLiftAngleForArmUpAnim_s = 45.f;


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToRobotOnFace::BehaviorReactToRobotOnFace(const Json::Value& config)
: ICozmoBehavior(config)
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToRobotOnFace::WantsToBeActivatedBehavior() const
{
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnFace::OnBehaviorActivated()
{
  FlipOverIfNeeded();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnFace::FlipOverIfNeeded()
{
  if( GetBEI().GetOffTreadsState() == OffTreadsState::OnFace ) {
    auto& robotInfo = GetBEI().GetRobotInfo();
    AnimationTrigger anim;
    if(robotInfo.GetLiftAngle() < kRobotMinLiftAngleForArmUpAnim_s){
      anim = AnimationTrigger::DEPRECATED_FacePlantRoll;
    }else{
      anim = AnimationTrigger::DEPRECATED_FacePlantRollArmUp;
    }
    
    if(GetAIComp<AIWhiteboard>().HasHiccups())
    {
      anim = AnimationTrigger::DEPRECATED_HiccupRobotOnFace;
    }
    

    DelegateIfInControl(new TriggerAnimationAction(anim),
                &BehaviorReactToRobotOnFace::DelayThenCheckState);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnFace::DelayThenCheckState()
{
  if( GetBEI().GetOffTreadsState() == OffTreadsState::OnFace ) {
    DelegateIfInControl(new WaitAction(kWaitTimeBeforeRepeatAnim_s),
                &BehaviorReactToRobotOnFace::CheckFlipSuccess);
  }

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnFace::CheckFlipSuccess()
{
  if(GetBEI().GetOffTreadsState() == OffTreadsState::OnFace) {
    DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::DEPRECATED_FailedToRightFromFace),
                &BehaviorReactToRobotOnFace::FlipOverIfNeeded);
  }else{
    BehaviorObjectiveAchieved(BehaviorObjective::ReactedToRobotOnFace);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnFace::OnBehaviorDeactivated()
{
}

} // namespace Cozmo
} // namespace Anki
