/**
 * File: behaviorRespondPossiblyRoll.cpp
 *
 * Author: Kevin M. Karol
 * Created: 01/23/17
 *
 * Description: Behavior that turns towards a block, plays an animation
 * and then rolls it if the block is on its side
 *
 * Copyright: Anki, Inc. 2017 *
 **/

#include "anki/cozmo/basestation/behaviors/behaviorRespondPossiblyRoll.h"

#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/common/basestation/utils/timer.h"


namespace Anki {
namespace Cozmo {
  
using namespace ExternalInterface;


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorRespondPossiblyRoll::BehaviorRespondPossiblyRoll(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
, _reactionAnimation(AnimationTrigger::Count)
, _responseSuccessfull(false)
, _attemptingRoll(false)
, _completedTimestamp_s(-1)
{
  SetDefaultName("RespondPossiblyRoll");
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorRespondPossiblyRoll::~BehaviorRespondPossiblyRoll()
{  
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorRespondPossiblyRoll::IsRunnableInternal(const BehaviorPreReqNone& preReqData) const
{
  return _targetID.IsSet();
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorRespondPossiblyRoll::InitInternal(Robot& robot)
{
  _responseSuccessfull = false;
  _attemptingRoll = false;
  _completedTimestamp_s = -1;
  
  TurnAndReact(robot);
  return Result::RESULT_OK;
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorRespondPossiblyRoll::ResumeInternal(Robot& robot)
{
  _completedTimestamp_s = -1;
  return RESULT_OK;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRespondPossiblyRoll::StopInternal(Robot& robot)
{
  _completedTimestamp_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorRespondPossiblyRoll::WasResponseSuccessful(AnimationTrigger& response,
                                                        float& completedTimestamp_s,
                                                        bool& hadToRoll) const
{
  if(_responseSuccessfull){
    response = _reactionAnimation;
    completedTimestamp_s = _completedTimestamp_s;
    hadToRoll = _attemptingRoll;
  }else{
    response = AnimationTrigger::Count;
  }
  
  return _responseSuccessfull;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRespondPossiblyRoll::TurnAndReact(Robot& robot)
{
  CompoundActionSequential* turnAndReact = new CompoundActionSequential(robot);
  turnAndReact->AddAction(new TurnTowardsObjectAction(robot, _targetID, Radians(M_PI_F), true));
  turnAndReact->AddAction(new TriggerLiftSafeAnimationAction(robot, _reactionAnimation));
  
  StartActing(turnAndReact, [this, &robot](ActionResult result){
    if(result == ActionResult::SUCCESS){
      RollIfNecessary(robot);
    }
  });
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRespondPossiblyRoll::RollIfNecessary(Robot& robot)
{
  _responseSuccessfull = true;

  ObservableObject* object = robot.GetBlockWorld().GetObjectByID(_targetID);
  if (nullptr != object &&
      object->IsPoseStateKnown() &&
      object->GetPose().GetRotationMatrix().GetRotatedParentAxis<'Z'>() != AxisName::Z_POS)
  {
    _attemptingRoll = true;
    DriveToRollObjectAction* action = new DriveToRollObjectAction(robot, _targetID);
    action->RollToUpright();
    StartActing(action);
  }
}


} // namespace Cozmo
} // namespace Anki
