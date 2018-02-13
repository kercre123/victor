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

#include "engine/aiComponent/behaviorComponent/behaviors/feeding/behaviorFeedingSearchForCube.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/compoundActions.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/severeNeedsComponent.h"
#include "engine/cozmoContext.h"
#include "engine/faceWorld.h"
#include "engine/needsSystem/needsManager.h"
#include "engine/needsSystem/needsState.h"

#include "coretech/common/engine/utils/timer.h"
#include "util/console/consoleInterface.h"

namespace Anki {
namespace Cozmo {
  
namespace{
#define SET_STATE(s) SetState_internal(State::s, #s)

CONSOLE_VAR(float, kFirstSearchLength_s, "Feeding.SearchForCube", 5.0f);
CONSOLE_VAR(float, kSecondSearchLength_s, "Feeding.SearchForCube", 5.0f);

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorFeedingSearchForCube::BehaviorFeedingSearchForCube(const Json::Value& config)
: ICozmoBehavior(config)
, _currentState(State::FirstSearchForCube)
, _timeEndSearch_s(0)
{
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorFeedingSearchForCube::WantsToBeActivatedBehavior() const
{
  return true;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingSearchForCube::OnBehaviorActivated()
{
  TransitionToFirstSearchForFood();
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingSearchForCube::BehaviorUpdate()
{
  if(!IsActivated()){
    return;
  }

  if((_currentState == State::FirstSearchForCube) ||
     (_currentState == State::SecondSearchForCube)){
    const float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    if(currentTime_s > _timeEndSearch_s){
      CancelDelegates(false);
      if(_currentState == State::FirstSearchForCube){
        TransitionToMakeFoodRequest();
      }else{
        TransitionToFailedToFindCubeReaction();
      }
    }
  }  
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingSearchForCube::TransitionToFirstSearchForFood()
{
  SET_STATE(FirstSearchForCube);
  _timeEndSearch_s = kFirstSearchLength_s + BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  TransitionToSearchForFoodBase();
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingSearchForCube::TransitionToSecondSearchForFood()
{
  SET_STATE(SecondSearchForCube);
  _timeEndSearch_s = kSecondSearchLength_s + BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  TransitionToSearchForFoodBase();
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingSearchForCube::TransitionToSearchForFoodBase()
{
  AnimationTrigger searchIdle = AnimationTrigger::FeedingIdleSearch_Normal;
  if(GetBEI().GetAIComponent().GetSevereNeedsComponent().GetSevereNeedExpression() == NeedId::Energy){
    searchIdle = AnimationTrigger::FeedingIdleSearch_Severe;
  }
  
  IActionRunner* searchAction = new CompoundActionParallel({
    new SearchForNearbyObjectAction(),
    new TriggerAnimationAction(searchIdle)
  });
  
  DelegateIfInControl(searchAction,
              &BehaviorFeedingSearchForCube::TransitionToSearchForFoodBase);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingSearchForCube::TransitionToMakeFoodRequest()
{
  SET_STATE(MakeFoodRequest);
  
  AnimationTrigger reactionAnimation = AnimationTrigger::FeedingSearchRequest ;
  if(GetBEI().GetAIComponent().GetSevereNeedsComponent().GetSevereNeedExpression() == NeedId::Energy){
    reactionAnimation = AnimationTrigger::FeedingSearchRequest_Severe;
  }

  IActionRunner* playAnimAction = new TriggerAnimationAction(reactionAnimation);
  IActionRunner* turnToFaceAction = new TurnTowardsFaceWrapperAction(playAnimAction);

  DelegateIfInControl(turnToFaceAction,
              &BehaviorFeedingSearchForCube::TransitionToSecondSearchForFood);
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingSearchForCube::TransitionToFailedToFindCubeReaction()
{
  SET_STATE(FailedToFindCubeReaction);
  
  AnimationTrigger reactionAnimation = AnimationTrigger::FeedingSearchFailure;
  if(GetBEI().GetAIComponent().GetSevereNeedsComponent().GetSevereNeedExpression() == NeedId::Energy){
    reactionAnimation = AnimationTrigger::FeedingSearchFailure_Severe;
  }
  
  DelegateIfInControl(new TriggerAnimationAction(reactionAnimation));
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





