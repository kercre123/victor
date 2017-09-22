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

#include "engine/aiComponent/behaviorSystem/behaviors/basicWorldInteractions/behaviorSearchForFace.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/behaviorSystem/behaviorExternalInterface.h"
#include "engine/faceWorld.h"
#include "engine/robot.h"

namespace Anki {
namespace Cozmo {

namespace {
#define SET_STATE(s) SetState_internal(State::s, #s)

}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorSearchForFace::BehaviorSearchForFace(const Json::Value& config)
: IBehavior(config)
{
  
}
 
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorSearchForFace::WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const
{
  // For the time being this behavior is designed under the assumption that
  // no faces are known and we're searching for a new face
  return !behaviorExternalInterface.GetFaceWorld().HasAnyFaces();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorSearchForFace::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  if(behaviorExternalInterface.GetFaceWorld().HasAnyFaces()){
    return Result::RESULT_FAIL;
  }
  
  TransitionToSearchingAnimation(behaviorExternalInterface);
  return Result::RESULT_OK;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior::Status BehaviorSearchForFace::UpdateInternal_WhileRunning(BehaviorExternalInterface& behaviorExternalInterface)
{
  if((_behaviorState == State::SearchingForFace) &&
     (behaviorExternalInterface.GetFaceWorld().HasAnyFaces())){
    StopActing(false);
    TransitionToFoundFace(behaviorExternalInterface);
  }
  
  return base::UpdateInternal_WhileRunning(behaviorExternalInterface);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSearchForFace::TransitionToSearchingAnimation(BehaviorExternalInterface& behaviorExternalInterface)
{
  SET_STATE(SearchingForFace);
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = behaviorExternalInterface.GetRobot();
  StartActing(new TriggerAnimationAction(robot, AnimationTrigger::ComeHere_SearchForFace));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSearchForFace::TransitionToFoundFace(BehaviorExternalInterface& behaviorExternalInterface)
{
  SET_STATE(FoundFace);
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = behaviorExternalInterface.GetRobot();
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
