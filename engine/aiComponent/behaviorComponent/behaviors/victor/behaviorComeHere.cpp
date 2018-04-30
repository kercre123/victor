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

#include "coretech/common/engine/jsonTools.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/faceWorld.h"
#include "engine/components/mics/micComponent.h"
#include "engine/components/mics/micDirectionHistory.h"

#include <math.h>

namespace Anki {
namespace Cozmo {

namespace{
const char* kFaceSelectionPriorityKey    = "faceSelectionPriority";
const float kHighValuePriority           = 1000.0f;
const float kLowValueTieBreaker          = 1.0f;
const char* kDistToFaceAlreadyHereKey    = "distToFaceAlreadyHere_mm";
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
  JsonTools::GetValueOptional(config, kDistToFaceAlreadyHereKey, _params.distAlreadyHere_mm_sqr);
  _params.distAlreadyHere_mm_sqr = pow(_params.distAlreadyHere_mm_sqr, 2.f);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorComeHere::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kFaceSelectionPriorityKey,
    kPreferMicDirectionKey,
    kDistToFaceAlreadyHereKey,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorComeHere::OnBehaviorActivated()
{
  _currentFaceID.Reset();
  if(_params.preferMicData){
    TurnTowardsMicDirection();
  }else{
    TurnTowardsFace();
  }
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorComeHere::TurnTowardsMicDirection()
{
  DEBUG_SET_STATE(TurnTowardsMicDirection);
  // Calculate radians to turn based on mic data
  MicDirectionIndex dirIdx = GetBEI().GetMicComponent().GetMicDirectionHistory().GetRecentDirection();
  dirIdx = dirIdx < 6 ? dirIdx : -(dirIdx - 6);
  const float radiansPerIdx = (2.0f/12.0f);
  const bool isAbsolute = false;
  DelegateIfInControl(new TurnInPlaceAction(radiansPerIdx  * dirIdx, isAbsolute),
                      &BehaviorComeHere::NoFaceFound);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorComeHere::TurnTowardsFace()
{
  DEBUG_SET_STATE(TurnTowardsFace);
  // Get all faces we have locations for  
  std::vector<SmartFaceID> allFaces;
  {
    const std::set<Vision::FaceID_t> possibleFaces = GetBEI().GetFaceWorld().GetFaceIDs();
    for(auto& entry : possibleFaces){
      allFaces.emplace_back(GetBEI().GetFaceWorld().GetSmartFaceID(entry));
    }
  }

  const auto& faceSelectionComp = GetAIComp<FaceSelectionComponent>();
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
                        [this](const ActionResult& result){
                            if(result == ActionResult::SUCCESS){
                              DriveTowardsFace();
                            }else{
                              NoFaceFound();
                            }
                        });
  }else{
    // No face, so turn towards mic data
    TurnTowardsMicDirection();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorComeHere::AlreadyHere()
{
  DEBUG_SET_STATE(AlreadyHere);
  DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::AlreadyAtFace));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorComeHere::DriveTowardsFace()
{
  DEBUG_SET_STATE(DriveTowardsFace);
  // Calculate distance to face
  const Vision::TrackedFace* currentFace = GetBEI().GetFaceWorld().GetFace(_currentFaceID);
  if( nullptr == currentFace ) {
    return;
  }

  f32 distToFace;
  const Pose3d& robotPose = GetBEI().GetRobotInfo().GetPose();
  if(ComputeDistanceSQBetween(currentFace->GetHeadPose(),robotPose,distToFace)){
    if(distToFace > _params.distAlreadyHere_mm_sqr){
      const float distToDrive = sqrt(distToFace);
      DelegateIfInControl(new DriveStraightAction(distToDrive));
    }else{
      // We're close enough
      AlreadyHere();
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorComeHere::NoFaceFound()
{
  DEBUG_SET_STATE(NoFaceFound);
  DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::DEPRECATED_ComeHere_SearchForFace));
}

} // namespace Cozmo
} // namespace Anki
