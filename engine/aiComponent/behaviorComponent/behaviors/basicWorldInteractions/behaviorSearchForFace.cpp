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

#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorSearchForFace.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/faceWorld.h"

namespace Anki {
namespace Cozmo {

namespace {
#define SET_STATE(s) SetState_internal(State::s, #s)

}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorSearchForFace::InstanceConfig::InstanceConfig()
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorSearchForFace::DynamicVariables::DynamicVariables()
{
  behaviorState = State::SearchingForFace;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorSearchForFace::BehaviorSearchForFace(const Json::Value& config)
: ICozmoBehavior(config)
{
  
}
 
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorSearchForFace::WantsToBeActivatedBehavior() const
{
  // For the time being this behavior is designed under the assumption that
  // no faces are known and we're searching for a new face
  return !GetBEI().GetFaceWorld().HasAnyFaces();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSearchForFace::OnBehaviorActivated()
{
  if(GetBEI().GetFaceWorld().HasAnyFaces()){
    return;
  }
  
  TransitionToSearchingAnimation();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSearchForFace::BehaviorUpdate()
{
  if(!IsActivated()){
    return;
  }

  if((_dVars.behaviorState == State::SearchingForFace) &&
     (GetBEI().GetFaceWorld().HasAnyFaces())){
    CancelDelegates(false);
    TransitionToFoundFace();
  }  
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSearchForFace::TransitionToSearchingAnimation()
{
  SET_STATE(SearchingForFace);
  DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::DEPRECATED_SearchForFace_Search));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSearchForFace::TransitionToFoundFace()
{
  SET_STATE(FoundFace);
  DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::DEPRECATED_SearchForFace_FoundFace));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSearchForFace::SetState_internal(State state, const std::string& stateName)
{
  _dVars.behaviorState = state;
  SetDebugStateName(stateName);
}

} // namespace Cozmo
} // namespace Anki
