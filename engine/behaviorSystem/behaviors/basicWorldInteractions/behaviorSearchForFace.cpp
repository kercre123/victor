/**
* File: behaviorSearchForFace.cpp
*
* Author: Kevin M. Karol
* Created: 7/21/17
*
* Description: A behavior which uses an animation in order to search around for a face
*
* Copyright: Anki, Inc. 2017
*
**/

#include "engine/behaviorSystem/behaviors/basicWorldInteractions/behaviorSearchForFace.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/behaviorSystem/behaviorPreReqs/behaviorPreReqRobot.h"
#include "engine/faceWorld.h"
#include "engine/robot.h"

namespace Anki {
namespace Cozmo {

namespace {
#define SET_STATE(s) SetState_internal(State::s, #s)

}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorSearchForFace::BehaviorSearchForFace(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
{
  
}
 
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorSearchForFace::IsRunnableInternal(const BehaviorPreReqRobot& preReqData) const
{
  // For the time being this behavior is designed under the assumption that
  // no faces are known and we're searching for a new face
  return !preReqData.GetRobot().GetFaceWorld().HasAnyFaces();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorSearchForFace::InitInternal(Robot& robot)
{
  if(robot.GetFaceWorld().HasAnyFaces()){
    return Result::RESULT_FAIL;
  }
  
  TransitionToSearchingAnimation(robot);
  return Result::RESULT_OK;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior::Status BehaviorSearchForFace::UpdateInternal(Robot& robot)
{
  if((_behaviorState == State::SearchingForFace) &&
     (robot.GetFaceWorld().HasAnyFaces())){
    StopActing(false);
    TransitionToFoundFace(robot);
  }
  
  return base::UpdateInternal(robot);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSearchForFace::TransitionToSearchingAnimation(Robot& robot)
{
  SET_STATE(SearchingForFace);
  StartActing(new TriggerAnimationAction(robot, AnimationTrigger::ComeHere_SearchForFace));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSearchForFace::TransitionToFoundFace(Robot& robot)
{
  SET_STATE(FoundFace);
  StartActing(new TriggerAnimationAction(robot, AnimationTrigger::ComeHere_SearchForFace_FoundFace));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSearchForFace::SetState_internal(State state, const std::string& stateName)
{
  _behaviorState = state;
  SetDebugStateName(stateName);
}

} // namespace Cozmo
} // namespace Anki
