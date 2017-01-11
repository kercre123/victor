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

#include "anki/cozmo/basestation/behaviors/reactionary/behaviorReactToRobotOnFace.h"

#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"
#include "clad/externalInterface/messageEngineToGame.h"

namespace Anki {
namespace Cozmo {
  
using namespace ExternalInterface;

static const float kWaitTimeBeforeRepeatAnim_s = 0.5f;
static const float kRobotMinLiftAngleForArmUpAnim_s = 45.f;

  
BehaviorReactToRobotOnFace::BehaviorReactToRobotOnFace(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
{
  SetDefaultName("ReactToRobotOnFace");
}


bool BehaviorReactToRobotOnFace::IsRunnableInternal(const BehaviorPreReqNone& preReqData) const
{
  return true;
}

Result BehaviorReactToRobotOnFace::InitInternal(Robot& robot)
{
  FlipOverIfNeeded(robot);
  return Result::RESULT_OK;
}

void BehaviorReactToRobotOnFace::FlipOverIfNeeded(Robot& robot)
{
  if( robot.GetOffTreadsState() == OffTreadsState::OnFace ) {
    AnimationTrigger anim;
    if(robot.GetLiftAngle() < kRobotMinLiftAngleForArmUpAnim_s){
      anim = AnimationTrigger::FacePlantRoll;
    }else{
      anim = AnimationTrigger::FacePlantRollArmUp;
    }
    
    StartActing(new TriggerAnimationAction(robot, anim),
                &BehaviorReactToRobotOnFace::DelayThenCheckState);
  }
}

void BehaviorReactToRobotOnFace::DelayThenCheckState(Robot& robot)
{
  if( robot.GetOffTreadsState() == OffTreadsState::OnFace ) {
    StartActing(new WaitAction(robot, kWaitTimeBeforeRepeatAnim_s),
                &BehaviorReactToRobotOnFace::CheckFlipSuccess);
  }

}
  
void BehaviorReactToRobotOnFace::CheckFlipSuccess(Robot& robot)
{
  if(robot.GetOffTreadsState() == OffTreadsState::OnFace) {
    StartActing(new TriggerAnimationAction(robot, AnimationTrigger::FailedToRightFromFace),
                &BehaviorReactToRobotOnFace::FlipOverIfNeeded);
  }else{
    BehaviorObjectiveAchieved(BehaviorObjective::ReactedToRobotOnFace);
  }
}

void BehaviorReactToRobotOnFace::StopInternal(Robot& robot)
{
}

} // namespace Cozmo
} // namespace Anki
