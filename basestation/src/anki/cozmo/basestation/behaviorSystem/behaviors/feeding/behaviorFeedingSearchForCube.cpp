/**
* File: behaviorFeedingSearchForCube.h
*
* Author: Kevin M. Karol
* Created: 2017-3-28
*
* Description: Behavior for cozmo to anticipate energy being loaded
* into a cube as the user prepares it
*
* Copyright: Anki, Inc. 2017
*
**/

#include "anki/cozmo/basestation/behaviorSystem/behaviors/feeding/behaviorFeedingSearchForCube.h"

#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/compoundActions.h"
#include "anki/cozmo/basestation/aiComponent/AIWhiteboard.h"
#include "anki/cozmo/basestation/aiComponent/aiComponent.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/faceWorld.h"
#include "anki/cozmo/basestation/needsSystem/needsManager.h"
#include "anki/cozmo/basestation/needsSystem/needsState.h"
#include "anki/cozmo/basestation/robot.h"

#include "anki/common/basestation/utils/timer.h"
#include "util/console/consoleInterface.h"

namespace Anki {
namespace Cozmo {
  
namespace{
#define SET_STATE(s) SetState_internal(State::s, #s)

CONSOLE_VAR(float, kFirstSearchLength_s, "Feeding.SearchForCube", 5.0f);
CONSOLE_VAR(float, kSecondSearchLength_s, "Feeding.SearchForCube", 5.0f);

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorFeedingSearchForCube::BehaviorFeedingSearchForCube(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
, _currentState(State::FirstSearchForCube)
, _timeEndSearch_s(0)
{
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorFeedingSearchForCube::IsRunnableInternal(const BehaviorPreReqNone& preReqData ) const
{
  return true;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorFeedingSearchForCube::InitInternal(Robot& robot)
{
  TransitionToFirstSearchForFood(robot);
  return Result::RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior::Status BehaviorFeedingSearchForCube::UpdateInternal(Robot& robot)
{
  if((_currentState == State::FirstSearchForCube) ||
     (_currentState == State::SecondSearchForCube)){
    const float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    if(currentTime_s > _timeEndSearch_s){
      StopActing(false);
      if(_currentState == State::FirstSearchForCube){
        TransitionToMakeFoodRequest(robot);
      }else{
        TransitionToFailedToFindCubeReaction(robot);
      }
    }
  }
  
  return Base::UpdateInternal(robot);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingSearchForCube::TransitionToFirstSearchForFood(Robot& robot)
{
  SET_STATE(FirstSearchForCube);
  _timeEndSearch_s = kFirstSearchLength_s + BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  TransitionToSearchForFoodBase(robot);
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingSearchForCube::TransitionToSecondSearchForFood(Robot& robot)
{
  SET_STATE(SecondSearchForCube);
  _timeEndSearch_s = kSecondSearchLength_s + BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  TransitionToSearchForFoodBase(robot);
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingSearchForCube::TransitionToSearchForFoodBase(Robot& robot)
{
  AnimationTrigger searchIdle = AnimationTrigger::Count;
  NeedsState& currNeedState = robot.GetContext()->GetNeedsManager()->GetCurNeedsStateMutable();
  if(currNeedState.IsNeedAtBracket(NeedId::Energy, NeedBracketId::Critical)){
    searchIdle = AnimationTrigger::FeedingIdleSearch_Severe;
  }else{
    searchIdle = AnimationTrigger::FeedingIdleSearch_Normal;
  }
  
  IActionRunner* searchAction = new CompoundActionParallel(robot, {
    new SearchForNearbyObjectAction(robot),
    new TriggerAnimationAction(robot, searchIdle)
  });
  
  StartActing(searchAction,
              &BehaviorFeedingSearchForCube::TransitionToSearchForFoodBase);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingSearchForCube::TransitionToMakeFoodRequest(Robot& robot)
{
  SET_STATE(MakeFoodRequest);
  
  AnimationTrigger reactionAnimation = AnimationTrigger::FeedingSearchRequest ;
  if(robot.GetAIComponent().GetWhiteboard().GetSevereNeedExpression() == NeedId::Energy){
    reactionAnimation = AnimationTrigger::FeedingSearchRequest_Severe;
  }
  
  IActionRunner* playAnimAction = new TriggerAnimationAction(robot, reactionAnimation);
  IActionRunner* turnToFaceAction = new TurnTowardsFaceWrapperAction(robot, playAnimAction);

  StartActing(turnToFaceAction,
              &BehaviorFeedingSearchForCube::TransitionToSecondSearchForFood);
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingSearchForCube::TransitionToFailedToFindCubeReaction(Robot& robot)
{
  SET_STATE(FailedToFindCubeReaction);
  
  AnimationTrigger reactionAnimation = AnimationTrigger::FeedingSearchFailure;
  if(robot.GetAIComponent().GetWhiteboard().GetSevereNeedExpression() == NeedId::Energy){
    reactionAnimation = AnimationTrigger::FeedingSearchFailure_Severe;
  }
  
  StartActing(new TriggerAnimationAction(robot, reactionAnimation));
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingSearchForCube::SetState_internal(State state,
                                                     const std::string& stateName)
{
  _currentState = state;
  SetDebugStateName(stateName);
}



} // namespace Cozmo
} // namespace Anki





