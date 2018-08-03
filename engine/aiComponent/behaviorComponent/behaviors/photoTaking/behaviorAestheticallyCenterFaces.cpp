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
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/faceWorld.h"
#include "util/console/consoleInterface.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorAestheticallyCenterFaces::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorAestheticallyCenterFaces::DynamicVariables::DynamicVariables()
{
  state = BehaviorState::SearchForFace;
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
  modifiers.behaviorAlwaysDelegates = true;
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
  
  _dVars.imageTimestampWhenActivated = GetBEI().GetRobotInfo().GetLastImageTimeStamp();

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
    if(GetBestFaceToCenter() != nullptr){
      if(IsControlDelegated()){
        CancelDelegates(false);
      }
      TransitionToCenterFace();
    }
  }


}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAestheticallyCenterFaces::TransitionToSearchForFaces()
{
  _dVars.state = BehaviorState::SearchForFace;
  ANKI_VERIFY(_iConfig.findFacesBehavior->WantsToBeActivated(),
              "BehaviorAestheticallyCenterFaces.TransitionToSearchForFaces.DoesNotWantToBeActivated",
              "");
  // Delegate to the find faces behavior. It will exit if a face is seen, so in that case skip
  // directly to TransitionToCenterFace. It will also dynamically adjust its timeout based on
  // whether a body is seen
  DelegateIfInControl(_iConfig.findFacesBehavior.get(),
                      &BehaviorAestheticallyCenterFaces::TransitionToCenterFace);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAestheticallyCenterFaces::TransitionToCenterFace()
{
  _dVars.state = BehaviorState::CenterFace;
  auto* face = GetBestFaceToCenter();
  if(face != nullptr){
    const auto& smartFaceId = GetBEI().GetFaceWorld().GetSmartFaceID(face->GetID());
    DelegateIfInControl(new TurnTowardsFaceAction(smartFaceId), [this, face](){
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
  auto smartFaces = GetBEI().GetFaceWorld().GetSmartFaceIDs();
  
  // Only allow faces that have been observed since the behavior started
  auto haveNotSeenSinceStarted = [this](const SmartFaceID& smartId) {
    const auto* face = GetBEI().GetFaceWorld().GetFace(smartId);
    return (face == nullptr) || (face->GetTimeStamp() < _dVars.imageTimestampWhenActivated);
  };
  smartFaces.erase(std::remove_if(smartFaces.begin(),
                                  smartFaces.end(),
                                  haveNotSeenSinceStarted),
                   smartFaces.end());
  
  const auto& faceSelection = GetAIComp<FaceSelectionComponent>();
  auto smartFaceID = faceSelection.GetBestFaceToUse(_iConfig.criteriaMap, smartFaces);
  const Vision::TrackedFace* trackedFace = GetBEI().GetFaceWorld().GetFace(smartFaceID);
  return trackedFace;
}



}
}
