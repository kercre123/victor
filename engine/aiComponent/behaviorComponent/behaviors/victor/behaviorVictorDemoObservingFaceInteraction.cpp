/**
 * File: behaviorVictorDemoObservingFaceInteraction.cpp
 *
 * Author: Brad Neuman
 * Created: 2017-10-24
 *
 * Description: Behavior to search for and interact with faces in a "quiet" way during the observing state
 *
 * Copyright: Anki, Inc. 2017
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/victor/behaviorVictorDemoObservingFaceInteraction.h"

#include "engine/actions/basicActions.h"
#include "engine/actions/compoundActions.h"
#include "engine/actions/trackFaceAction.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "engine/faceWorld.h"
#include "engine/robot.h"

#define SET_STATE(s) SetState_internal(State::s, #s)

namespace Anki {
namespace Cozmo {

namespace {
static const u32 kMaxTimeSinceSeenFaceToLook_ms = 5000;
static const float kStaringTime_s = 6.0f; // TODO:(bn) randomize
static const float kMinTrackingTiltAngle_deg = 4.0f;
static const float kMinTrackingPanAngle_deg = 4.0f;
static const float kMinTrackingClampPeriod_s = 0.2f;
static const float kMaxTrackingClampPeriod_s = 0.7f;
}

BehaviorVictorDemoObservingFaceInteraction::BehaviorVictorDemoObservingFaceInteraction(const Json::Value& config)
  : ICozmoBehavior(config)
{
}

void BehaviorVictorDemoObservingFaceInteraction::InitBehavior(BehaviorExternalInterface& behaviorExternalInterface)
{
  const auto& BC = behaviorExternalInterface.GetBehaviorContainer();

  BC.FindBehaviorByIDAndDowncast(BEHAVIOR_ID(VictorDemoObservingFindFaces),
                                 BEHAVIOR_CLASS(FindFaces),
                                 _searchBehavior);

}

void BehaviorVictorDemoObservingFaceInteraction::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert(_searchBehavior.get());
}

Result BehaviorVictorDemoObservingFaceInteraction::OnBehaviorActivated(
  BehaviorExternalInterface& behaviorExternalInterface)
{
  _faceIdsLookedAt.clear();

  // if we have any faces, stare at one. Otherwise, search
  const auto& faceWorld = behaviorExternalInterface.GetFaceWorld();

  if( faceWorld.HasAnyFaces( GetRecentFaceTime( behaviorExternalInterface ) ) ) {
    TransitionToTurnTowardsAFace(behaviorExternalInterface);
  }
  else {
    TransitionToFindFaces(behaviorExternalInterface);
  }

  return Result::RESULT_OK;
}

bool BehaviorVictorDemoObservingFaceInteraction::CanBeGentlyInterruptedNow(BehaviorExternalInterface& behaviorExternalInterface) const
{
  switch (_state ) {
    case State::FindFaces: {
      return true;
    }
    case State::TurnTowardsFace: {
      return !IsControlDelegated();
    }
    case State::StareAtFace: {
      return true;
    }
  }
}


ICozmoBehavior::Status BehaviorVictorDemoObservingFaceInteraction::UpdateInternal_WhileRunning(
  BehaviorExternalInterface& behaviorExternalInterface)
{

  if( _state == State::FindFaces ) {
    // check if we have found a new face during the search
    // TODO:(bn) make a HasFaceToStareAt function?
    if( GetFaceToStareAt(behaviorExternalInterface).IsValid() ) {
      const bool allowCallback = false;
      CancelDelegates(allowCallback);
      TransitionToTurnTowardsAFace(behaviorExternalInterface);
    }
  }

  return ICozmoBehavior::UpdateInternal_WhileRunning(behaviorExternalInterface);
}


void BehaviorVictorDemoObservingFaceInteraction::TransitionToFindFaces(
  BehaviorExternalInterface& behaviorExternalInterface)
{
  SET_STATE(FindFaces);
  
  if( _searchBehavior->WantsToBeActivated(behaviorExternalInterface) ) {
    DelegateIfInControl( behaviorExternalInterface,
                         _searchBehavior.get() );
  }
  else {
    PRINT_NAMED_WARNING("BehaviorVictorDemoObservingFaceInteraction.FindFaces.DoesntWantToActivate",
                        "Behavior %s doesn't want to activate, so face interaction will end",
                        _searchBehavior->GetIDStr().c_str());
  }
}

void BehaviorVictorDemoObservingFaceInteraction::TransitionToTurnTowardsAFace(
  BehaviorExternalInterface& behaviorExternalInterface)
{
  SET_STATE(TurnTowardsFace);
  
  const SmartFaceID face = GetFaceToStareAt(behaviorExternalInterface);
  if( face.IsValid() ) {
    
    TurnTowardsFaceAction* action = new TurnTowardsFaceAction( behaviorExternalInterface.GetRobot(),
                                                               face );

    DelegateIfInControl(action, [this, face](ActionResult res,
                                             BehaviorExternalInterface& behaviorExternalInterface) {
        if( res == ActionResult::SUCCESS &&
            face.IsValid() ) {
          // turned towards a good face, let's stare at it for a bit
          TransitionToStareAtFace(behaviorExternalInterface, face);
        }
        else {
          // either action failed, or the face is gone now. In either case, try to find a new face
          TransitionToTurnTowardsAFace(behaviorExternalInterface);
        }
      });
          
  }
  else {
    // there's no face to look at, so search instead
    TransitionToFindFaces(behaviorExternalInterface);
  }
}


void BehaviorVictorDemoObservingFaceInteraction::TransitionToStareAtFace(
  BehaviorExternalInterface& behaviorExternalInterface,
  SmartFaceID face)
{
  SET_STATE(StareAtFace);
  
  if( face.IsValid() &&
      ShouldStareAtFace(behaviorExternalInterface, face) ) {
    // if the face is still valid and one we want to look at, go ahead and stare for a bit
    
    PRINT_CH_INFO("Behaviors", "BehaviorVictorDemoObservingFaceInteraction.StareAtFace",
                  "Adding %s as a 'stared at' face",
                  face.GetDebugStr().c_str());
    
    _faceIdsLookedAt.push_back( face );

    WaitAction* waitAction = new WaitAction(behaviorExternalInterface.GetRobot(), kStaringTime_s);
    TrackFaceAction* trackAction = new TrackFaceAction(behaviorExternalInterface.GetRobot(),
                                                       face);
    trackAction->StopTrackingWhenOtherActionCompleted( waitAction->GetTag() );
    trackAction->SetTiltTolerance(DEG_TO_RAD(kMinTrackingTiltAngle_deg));
    trackAction->SetPanTolerance(DEG_TO_RAD(kMinTrackingPanAngle_deg));
    trackAction->SetClampSmallAnglesToTolerances(true);
    trackAction->SetClampSmallAnglesPeriod(kMinTrackingClampPeriod_s, kMaxTrackingClampPeriod_s);

    // stare for a while, then turn towards another face
    DelegateIfInControl( new CompoundActionParallel(behaviorExternalInterface.GetRobot(),
                                                    {waitAction, trackAction}),
                         &BehaviorVictorDemoObservingFaceInteraction::TransitionToTurnTowardsAFace );
  }
  else {
    PRINT_CH_INFO("Behaviors", "BehaviorVictorDemoObservingFaceInteraction.StareAtFace.Invalid",
                  "After turning, we see that face %s is not valid for staring",
                  face.GetDebugStr().c_str());
    
    // otherwise, pick another face to look at (or fall back to searching)
    TransitionToTurnTowardsAFace(behaviorExternalInterface);
  }
}

SmartFaceID BehaviorVictorDemoObservingFaceInteraction::GetFaceToStareAt(
  BehaviorExternalInterface& behaviorExternalInterface)
{
  // NOTE: because face ids can start out different and then become equal, _faceIdsLookedAt may have multiple
  // entries that are now equal

  const auto& faceWorld = behaviorExternalInterface.GetFaceWorld();
  const auto& faces = faceWorld.GetFaceIDsObservedSince( GetRecentFaceTime( behaviorExternalInterface ) );

  for( const auto& rawFaceID : faces ) {
    const SmartFaceID faceID = faceWorld.GetSmartFaceID(rawFaceID);
    if( ShouldStareAtFace(behaviorExternalInterface, faceID) ) {
      PRINT_CH_INFO("Behaviors", "BehaviorVictorDemoObservingFaceInteraction.GetFaceToStareAt",
                    "Face %s is a valid one (didn't match any of the %zu we've already seen)",
                    faceID.GetDebugStr().c_str(),
                    _faceIdsLookedAt.size());
      
      // TODO:(bn) track the nearest face? Or the furthest? For now make it arbitrary
      return faceID;
    }
    // else try the next face
  }

  // no faces were found that we want to track
  return SmartFaceID{};
}

bool BehaviorVictorDemoObservingFaceInteraction::ShouldStareAtFace(
  BehaviorExternalInterface& behaviorExternalInterface,
  const SmartFaceID& face) const
{
  for( const auto& boringFace : _faceIdsLookedAt ) {
    if( face == boringFace ) {
      // already looked at this face during this run, so don't do it again
      return false;
    }
  }

  // otherwise, this is a new face, so it's a good one to look at
  return true;
}

TimeStamp_t BehaviorVictorDemoObservingFaceInteraction::GetRecentFaceTime(
  BehaviorExternalInterface& behaviorExternalInterface)
{

  const TimeStamp_t lastImgTime = behaviorExternalInterface.GetRobot().GetLastImageTimeStamp();
  const TimeStamp_t recentTime = lastImgTime > kMaxTimeSinceSeenFaceToLook_ms ?
                                 ( lastImgTime - kMaxTimeSinceSeenFaceToLook_ms ) :
                                 0;
  return recentTime;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVictorDemoObservingFaceInteraction::SetState_internal(
  BehaviorVictorDemoObservingFaceInteraction::State state,
  const std::string& stateName)
{
  _state = state;
  SetDebugStateName(stateName);
}  


}
}

