/**
* File: behaviorComeHere.h
*
* Author: Kevin M. Karol
* Created: 2017-12-05
*
* Description: Behavior that turns towards a face, confirms it through a search, and then drives towards it
*
* Copyright: Anki, Inc. 2017
*
**/


#include "engine/aiComponent/behaviorComponent/behaviors/victor/behaviorComeHere.h"

#include "anki/common/basestation/jsonTools.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/faceWorld.h"
#include "engine/micDirectionHistory.h"

#include <math.h>

namespace Anki {
namespace Cozmo {

namespace{
const char* kFaceSelectionPriorityKey    = "faceSelectionPriority";
const float kHighValuePriority           = 1000.0f;
const float kLowValueTieBreaker          = 1.0f;
const char* kDistToFaceAlreadyHere_mmKey = "distToFaceAlreadyHere_mm";
const char* kPreferMicDirectionKey       = "preferMicDirection";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorComeHere::BehaviorComeHere(const Json::Value& config)
: ICozmoBehavior(config)
{
  for(auto& entry: config[kFaceSelectionPriorityKey]){
    _facePriorities.push_back(FaceSelectionComponent::FaceSelectionPenaltyFromString(entry.asString()));
  }

  JsonTools::GetValueOptional(config, kPreferMicDirectionKey, _params.preferMicData);

  // get dist in mm and then square it
  JsonTools::GetValueOptional(config, kDistToFaceAlreadyHere_mmKey, _params.distAlreadyHere_mm_sqr);
  _params.distAlreadyHere_mm_sqr = pow(_params.distAlreadyHere_mm_sqr, 2.f);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorComeHere::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  _currentFaceID.Reset();
  if(_params.preferMicData){
    TurnTowardsMicDirection(behaviorExternalInterface);
  }else{
    TurnTowardsFace(behaviorExternalInterface);
  }
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorComeHere::TurnTowardsMicDirection(BehaviorExternalInterface& behaviorExternalInterface)
{
  DEBUG_SET_STATE(TurnTowardsMicDirection);
  // Calculate radians to turn based on mic data
  MicDirectionHistory::DirectionIndex dirIdx = behaviorExternalInterface.GetMicDirectionHistory().GetRecentDirection();
  dirIdx = dirIdx < 6 ? dirIdx : -(dirIdx - 6);
  const float radiansPerIdx = (2.0f/12.0f);
  const bool isAbsolute = false;
  DelegateIfInControl(new TurnInPlaceAction(radiansPerIdx  * dirIdx, isAbsolute),
                      &BehaviorComeHere::NoFaceFound);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorComeHere::TurnTowardsFace(BehaviorExternalInterface& behaviorExternalInterface)
{
  DEBUG_SET_STATE(TurnTowardsFace);
  // Get all faces we have locations for  
  std::set<SmartFaceID> allFaces; 
  {
    const std::set<Vision::FaceID_t> possibleFaces = behaviorExternalInterface.GetFaceWorld().GetFaceIDs();
    for(auto& entry : possibleFaces){
      allFaces.insert(behaviorExternalInterface.GetFaceWorld().GetSmartFaceID(entry));
    }
  }

  const auto& faceSelectionComp = behaviorExternalInterface.GetAIComponent().GetFaceSelectionComponent();
  // Select face based on priority penalties
  FaceSelectionComponent::FaceSelectionFactorMap map;
  for(auto& priority: _facePriorities){
    map.clear();
    map.insert(std::make_pair(priority, kHighValuePriority));
    // Add in a low value tie breaker
    if(priority == FaceSelectionPenaltyMultiplier::RelativeBodyAngleRadians){
      map.insert(std::make_pair(FaceSelectionPenaltyMultiplier::RelativeHeadAngleRadians, kLowValueTieBreaker));
    }else{
      map.insert(std::make_pair(FaceSelectionPenaltyMultiplier::RelativeBodyAngleRadians, kLowValueTieBreaker));
    }

    _currentFaceID = faceSelectionComp.GetBestFaceToUse(map, allFaces);
    if(_currentFaceID.IsValid()){
      break;
    }
  }

  // If we found a face we like turn towards that
  // otherwise turn in the direction of the mic data
  if(_currentFaceID.IsValid()){
    const bool sayName = true;
    auto action = new TurnTowardsFaceAction(_currentFaceID, M_PI_F, sayName);
    action->SetRequireFaceConfirmation(true);
    DelegateIfInControl(action,
                        [this](const ActionResult& result, BehaviorExternalInterface& bei){
                            if(result == ActionResult::SUCCESS){
                              DriveTowardsFace(bei);
                            }else{
                              NoFaceFound(bei);
                            }
                        });
  }else{
    // No face, so turn towards mic data
    TurnTowardsMicDirection(behaviorExternalInterface);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorComeHere::AlreadyHere(BehaviorExternalInterface& behaviorExternalInterface)
{
  DEBUG_SET_STATE(AlreadyHere);
  DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::ComeHere_AlreadyHere));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorComeHere::DriveTowardsFace(BehaviorExternalInterface& behaviorExternalInterface)
{
  DEBUG_SET_STATE(DriveTowardsFace);
  // Calculate distance to face
  const Vision::TrackedFace* currentFace = behaviorExternalInterface.GetFaceWorld().GetFace(_currentFaceID);
  if( nullptr == currentFace ) {
    return;
  }

  f32 distToFace;
  const Pose3d& robotPose = behaviorExternalInterface.GetRobotInfo().GetPose();
  if(ComputeDistanceSQBetween(currentFace->GetHeadPose(),robotPose,distToFace)){
    if(distToFace > _params.distAlreadyHere_mm_sqr){
      const float distToDrive = sqrt(distToFace);
      DelegateIfInControl(new DriveStraightAction(distToDrive));
    }else{
      // We're close enough
      AlreadyHere(behaviorExternalInterface);
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorComeHere::NoFaceFound(BehaviorExternalInterface& behaviorExternalInterface)
{
  DEBUG_SET_STATE(NoFaceFound);
  DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::ComeHere_SearchForFace));
}

} // namespace Cozmo
} // namespace Anki
