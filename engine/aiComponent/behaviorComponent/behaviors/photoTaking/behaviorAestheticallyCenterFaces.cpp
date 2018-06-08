/**
 * File: BehaviorAestheticallyCenterFaces.cpp
 *
 * Author: Kevin M. Karol
 * Created: 2018-06-04
 *
 * Description: Centers face/s so that they will look pleasing in an image - this may not be true center if aesthetics dictate more space should be left on the top/bottom of the frame
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/photoTaking/behaviorAestheticallyCenterFaces.h"

#include "coretech/common/engine/utils/timer.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/faceWorld.h"
#include "util/console/consoleInterface.h"

namespace Anki {
namespace Cozmo {
  

CONSOLE_VAR(int, kSearchLength_ms, "PhotoCenterFacesSearchTimeoutMS", 5000);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorAestheticallyCenterFaces::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorAestheticallyCenterFaces::DynamicVariables::DynamicVariables()
{
  state = BehaviorState::SearchForFace;
  timeFaceSearchShouldEnd = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorAestheticallyCenterFaces::BehaviorAestheticallyCenterFaces(const Json::Value& config)
: ICozmoBehavior(config)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorAestheticallyCenterFaces::~BehaviorAestheticallyCenterFaces()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorAestheticallyCenterFaces::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAestheticallyCenterFaces::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAestheticallyCenterFaces::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert(_iConfig.findFacesBehavior.get());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAestheticallyCenterFaces::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAestheticallyCenterFaces::InitBehavior()
{

  const auto& BC = GetBEI().GetBehaviorContainer();
  _iConfig.findFacesBehavior = BC.FindBehaviorByID(BEHAVIOR_ID(FindFacesPhoto));

  _iConfig.criteriaMap.insert(std::make_pair(FaceSelectionPenaltyMultiplier::RelativeBodyAngleRadians, 1000));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAestheticallyCenterFaces::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables();


  if(GetBestFaceToCenter() != nullptr){
    TransitionToCenterFace();
  }else{
    TransitionToSearchForFaces();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAestheticallyCenterFaces::BehaviorUpdate() 
{
  if(!IsActivated()){
    return;
  }

  if(_dVars.state == BehaviorState::SearchForFace){
    if(IsControlDelegated()){
      const auto currentTS = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
      if(GetBestFaceToCenter() != nullptr){
        CancelDelegates();
      }
      const bool reachedTimeout = (_dVars.timeFaceSearchShouldEnd < currentTS);
      if(reachedTimeout){
        CancelSelf();
        return;
      }
    }
    if(!IsControlDelegated()){
      TransitionToCenterFace();
    }
  }


}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAestheticallyCenterFaces::TransitionToSearchForFaces()
{
  _dVars.state = BehaviorState::SearchForFace;
  _dVars.timeFaceSearchShouldEnd = BaseStationTimer::getInstance()->GetCurrentTimeStamp() + kSearchLength_ms;
  ANKI_VERIFY(_iConfig.findFacesBehavior->WantsToBeActivated(),
              "BehaviorAestheticallyCenterFaces.TransitionToSearchForFaces.DoesNotWantToBeActivated",
              "");
  DelegateIfInControl(_iConfig.findFacesBehavior.get());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAestheticallyCenterFaces::TransitionToCenterFace()
{
  _dVars.state = BehaviorState::CenterFace;
  auto* face = GetBestFaceToCenter();
  if(face != nullptr){
    DelegateIfInControl(new TurnTowardsPoseAction(face->GetHeadPose()), [this, face](){
      if(GetBestFaceToCenter() == nullptr){
        TransitionToSearchForFaces();
      }else if(GetBestFaceToCenter() != face){
        TransitionToCenterFace();
      }
    });
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Vision::TrackedFace* BehaviorAestheticallyCenterFaces::GetBestFaceToCenter()
{
  const bool considerTrackingOnlyFaces = false;
  auto smartFaces = GetBEI().GetFaceWorld().GetSmartFaceIDsObservedSince(0, considerTrackingOnlyFaces);
  const auto& faceSelection = GetAIComp<FaceSelectionComponent>();
  auto smartFaceID = faceSelection.GetBestFaceToUse(_iConfig.criteriaMap, smartFaces);
  const Vision::TrackedFace* trackedFace = GetBEI().GetFaceWorld().GetFace(smartFaceID);
  return trackedFace;
}



}
}
