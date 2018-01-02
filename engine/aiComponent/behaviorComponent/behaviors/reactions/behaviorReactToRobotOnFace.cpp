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
bool BehaviorReactToRobotOnFace::WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const
{
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnFace::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  FlipOverIfNeeded(behaviorExternalInterface);
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnFace::FlipOverIfNeeded(BehaviorExternalInterface& behaviorExternalInterface)
{
  if( behaviorExternalInterface.GetOffTreadsState() == OffTreadsState::OnFace ) {
    auto& robotInfo = behaviorExternalInterface.GetRobotInfo();
    AnimationTrigger anim;
    if(robotInfo.GetLiftAngle() < kRobotMinLiftAngleForArmUpAnim_s){
      anim = AnimationTrigger::FacePlantRoll;
    }else{
      anim = AnimationTrigger::FacePlantRollArmUp;
    }
    
    if(behaviorExternalInterface.GetAIComponent().GetWhiteboard().HasHiccups())
    {
      anim = AnimationTrigger::HiccupRobotOnFace;
    }
    

    DelegateIfInControl(new TriggerAnimationAction(anim),
                &BehaviorReactToRobotOnFace::DelayThenCheckState);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnFace::DelayThenCheckState(BehaviorExternalInterface& behaviorExternalInterface)
{
  if( behaviorExternalInterface.GetOffTreadsState() == OffTreadsState::OnFace ) {
    DelegateIfInControl(new WaitAction(kWaitTimeBeforeRepeatAnim_s),
                &BehaviorReactToRobotOnFace::CheckFlipSuccess);
  }

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnFace::CheckFlipSuccess(BehaviorExternalInterface& behaviorExternalInterface)
{
  if(behaviorExternalInterface.GetOffTreadsState() == OffTreadsState::OnFace) {
    DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::FailedToRightFromFace),
                &BehaviorReactToRobotOnFace::FlipOverIfNeeded);
  }else{
    BehaviorObjectiveAchieved(BehaviorObjective::ReactedToRobotOnFace);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnFace::OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)
{
}

} // namespace Cozmo
} // namespace Anki
