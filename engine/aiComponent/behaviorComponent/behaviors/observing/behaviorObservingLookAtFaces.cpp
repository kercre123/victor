/**
 * File: behaviorObservingLookAtFaces.cpp
 *
 * Author: Brad Neuman
 * Created: 2017-10-24
 *
 * Description: Behavior to search for and interact with faces in a "quiet" way during the observing state
 *
 * Copyright: Anki, Inc. 2017
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/observing/behaviorObservingLookAtFaces.h"

#include "engine/actions/basicActions.h"
#include "engine/actions/compoundActions.h"
#include "engine/actions/trackFaceAction.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "engine/faceWorld.h"

#include "coretech/common/engine/utils/timer.h"

#define LOG_CHANNEL "Behaviors"

#define SET_STATE(s) SetState_internal(State::s, #s)

namespace Anki {
namespace Vector {

namespace {
static const u32 kMaxTimeSinceSeenFaceToLook_ms = 5000;
static const float kDefaultStaringTime_s = 6.0f; // TODO:(bn) randomize
static const float kMinTrackingTiltAngle_deg = 4.0f;
static const float kMinTrackingPanAngle_deg = 4.0f;
static const float kMinTrackingClampPeriod_s = 0.2f;
static const float kMaxTrackingClampPeriod_s = 0.7f;
static const float kDefaultSearchTime_s = -1.f; // Negative values disable the timeout.
static const float kTrackingTimeout_s = 2.5f;
const char* const kSearchBehaviorKey = "searchBehavior";
const char* const kSearchTimeoutKey = "searchTimeout_sec";
const char* const kStaringTimeKey = "staringTime_sec";
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorObservingLookAtFaces::InstanceConfig::InstanceConfig()
{
  searchBehavior = nullptr;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorObservingLookAtFaces::DynamicVariables::DynamicVariables()
{
  latestFaceSearchStartTime_sec = 0.f;
  persistent.state = State::FindFaces;
}

BehaviorObservingLookAtFaces::BehaviorObservingLookAtFaces(const Json::Value& config)
  : ICozmoBehavior(config)
{
  const std::string& debugName = "Behavior" + GetDebugLabel() + ".LoadConfig";
  _iConfig.searchBehaviorStr = JsonTools::ParseString(config, kSearchBehaviorKey, debugName);

  // Parse the maximum allowed face-search time from the config if possible, otherwise
  // assign to the default value.
  if (JsonTools::GetValueOptional(config, kSearchTimeoutKey, _iConfig.searchTimeout_sec)) {
    const std::string& errorStr = debugName + ".InvalidSearchTimeout";
    DEV_ASSERT(Util::IsFltGTZero(_iConfig.searchTimeout_sec),
               errorStr.c_str());
    LOG_DEBUG(debugName.c_str(), "Search timeout set to %f seconds", _iConfig.searchTimeout_sec);
  } else {
    _iConfig.searchTimeout_sec = kDefaultSearchTime_s;
  }

  // Parse the staring time from the config if possible, otherwise assign to the default value
  if (JsonTools::GetValueOptional(config, kStaringTimeKey, _iConfig.staringTime_sec)) {
    const std::string& errorStr = debugName + ".InvalidStaringTime";
    DEV_ASSERT(Util::IsFltGTZero(_iConfig.staringTime_sec),
               errorStr.c_str());
    LOG_DEBUG(debugName.c_str(), "Staring time set to %f seconds", _iConfig.staringTime_sec);
  } else {
    _iConfig.staringTime_sec = kDefaultStaringTime_s;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorObservingLookAtFaces::GetBehaviorJsonKeys(std::set<const char *> &expectedKeys) const
{
  const char* list[] = {
    kSearchBehaviorKey,
    kSearchTimeoutKey,
    kStaringTimeKey,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

void BehaviorObservingLookAtFaces::InitBehavior()
{
  _iConfig.searchBehavior = FindBehavior(_iConfig.searchBehaviorStr);
  DEV_ASSERT(_iConfig.searchBehavior != nullptr,
             "BehaviorObservingLookAtFaces.InitBehavior.NullSearchBehavior");
}

void BehaviorObservingLookAtFaces::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert(_iConfig.searchBehavior.get());
}

void BehaviorObservingLookAtFaces::OnBehaviorActivated()
{
  const auto persistent = _dVars.persistent;
  _dVars = DynamicVariables();
  _dVars.persistent = persistent;

  // if we have any faces, stare at one. Otherwise, search
  const auto& faceWorld = GetBEI().GetFaceWorld();

  if(faceWorld.HasAnyFaces(GetRecentFaceTime())) {
    TransitionToTurnTowardsAFace();
  }
  else {
    TransitionToFindFaces();
  }

  
}

bool BehaviorObservingLookAtFaces::CanBeGentlyInterruptedNow() const
{
  switch (_dVars.persistent.state ) {
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


void BehaviorObservingLookAtFaces::BehaviorUpdate()
{
  if(!IsActivated()){
    return;
  }

  if( _dVars.persistent.state == State::FindFaces ) {
    // check if we have found a new face during the search
    // TODO:(bn) make a HasFaceToStareAt function?
    if( GetFaceToStareAt().IsValid() ) {
      const bool allowCallback = false;
      CancelDelegates(allowCallback);
      TransitionToTurnTowardsAFace();
    } else if (_iConfig.searchTimeout_sec >= 0.f) {
      const float faceSearchTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds()
                                   - _dVars.latestFaceSearchStartTime_sec;
      if (faceSearchTime >= _iConfig.searchTimeout_sec) {
        LOG_INFO("BehaviorObservingLookAtFaces.Update.FaceSearchTimeout",
                 "Behavior %s cancelling self after search for faces timed out",
                 GetDebugLabel().c_str());
        const bool allowCallback = false;
        CancelDelegates(allowCallback);
        CancelSelf();
      }
    }
  }
}


void BehaviorObservingLookAtFaces::TransitionToFindFaces()
{
  SET_STATE(FindFaces);
  
  if( _iConfig.searchBehavior->WantsToBeActivated() ) {
    _dVars.latestFaceSearchStartTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    DelegateIfInControl(_iConfig.searchBehavior.get());
  }
  else {
    LOG_WARNING("BehaviorObservingLookAtFaces.FindFaces.DoesntWantToActivate",
                "Behavior %s doesn't want to activate, so face interaction will end",
                _iConfig.searchBehavior->GetDebugLabel().c_str());
  }
}

void BehaviorObservingLookAtFaces::TransitionToTurnTowardsAFace()
{
  SET_STATE(TurnTowardsFace);
  
  const SmartFaceID face = GetFaceToStareAt();
  if( face.IsValid() ) {
    
    auto & sayNameTable = GetAIComp<AIWhiteboard>().GetSayNameProbabilityTable();
    TurnTowardsFaceAction* action = new TurnTowardsFaceAction(face, M_PI_F, sayNameTable);
    action->SetNoNameAnimationTrigger(AnimationTrigger::InteractWithFacesInitialUnnamed);
    action->SetSayNameAnimationTrigger(AnimationTrigger::InteractWithFacesInitialNamed);
    
    DelegateIfInControl(action, [this, face](ActionResult res) {
        if( res == ActionResult::SUCCESS &&
            face.IsValid() ) {
          // turned towards a good face, let's stare at it for a bit
          TransitionToStareAtFace(face);
        }
        else {
          // either action failed, or the face is gone now. In either case, try to find a new face
          TransitionToTurnTowardsAFace();
        }
      });
          
  }
  else {
    // there's no face to look at, so search instead
    TransitionToFindFaces();
  }
}


void BehaviorObservingLookAtFaces::TransitionToStareAtFace(SmartFaceID face)
{
  SET_STATE(StareAtFace);
  
  if( face.IsValid() &&
      ShouldStareAtFace(face) ) {
    // if the face is still valid and one we want to look at, go ahead and stare for a bit
    
    LOG_INFO("BehaviorObservingLookAtFaces.StareAtFace",
             "Adding %s as a 'stared at' face",
             face.GetDebugStr().c_str());
    
    _dVars.faceIdsLookedAt.push_back( face );

    WaitAction* waitAction = new WaitAction(_iConfig.staringTime_sec);
    TrackFaceAction* trackAction = new TrackFaceAction(face);
    trackAction->StopTrackingWhenOtherActionCompleted( waitAction->GetTag() );
    trackAction->SetTiltTolerance(DEG_TO_RAD(kMinTrackingTiltAngle_deg));
    trackAction->SetPanTolerance(DEG_TO_RAD(kMinTrackingPanAngle_deg));
    trackAction->SetClampSmallAnglesToTolerances(true);
    trackAction->SetClampSmallAnglesPeriod(kMinTrackingClampPeriod_s, kMaxTrackingClampPeriod_s);
    trackAction->SetUpdateTimeout(kTrackingTimeout_s);
    // Some variations of this behavior run in the user's palm, so we need to allow the tracking
    // action to run in the InAir tread state as well.
    trackAction->SetValidOffTreadsStates({OffTreadsState::OnTreads, OffTreadsState::InAir});

    // If the track action component fails, the entire action should end. Otherwise the robot
    // will just sit there staring at a point in space that may not correspond to the face's
    // location any more until the wait action expires.
    CompoundActionParallel* waitAndTrackAction = new CompoundActionParallel({waitAction, trackAction});
    waitAndTrackAction->SetShouldEndWhenFirstActionCompletes(true);
    
    // stare for a while, then turn towards another face
    DelegateIfInControl( waitAndTrackAction,
                         &BehaviorObservingLookAtFaces::TransitionToTurnTowardsAFace );
  }
  else {
    LOG_INFO("BehaviorObservingLookAtFaces.StareAtFace.Invalid",
             "After turning, we see that face %s is not valid for staring",
             face.GetDebugStr().c_str());
    
    // otherwise, pick another face to look at (or fall back to searching)
    TransitionToTurnTowardsAFace();
  }
}

SmartFaceID BehaviorObservingLookAtFaces::GetFaceToStareAt()
{
  // NOTE: because face ids can start out different and then become equal, faceIdsLookedAt may have multiple
  // entries that are now equal

  const auto& faceWorld = GetBEI().GetFaceWorld();
  const auto& faces = faceWorld.GetFaceIDs( GetRecentFaceTime() );

  for( const auto& rawFaceID : faces ) {
    const SmartFaceID faceID = faceWorld.GetSmartFaceID(rawFaceID);
    if( ShouldStareAtFace(faceID) ) {
      LOG_INFO("BehaviorObservingLookAtFaces.GetFaceToStareAt",
               "Face %s is a valid one (didn't match any of the %zu we've already seen)",
               faceID.GetDebugStr().c_str(),
               _dVars.faceIdsLookedAt.size());
      
      // TODO:(bn) track the nearest face? Or the furthest? For now make it arbitrary
      return faceID;
    }
    // else try the next face
  }

  // no faces were found that we want to track
  return SmartFaceID{};
}

bool BehaviorObservingLookAtFaces::ShouldStareAtFace(const SmartFaceID& face) const
{
  for( const auto& boringFace : _dVars.faceIdsLookedAt ) {
    if( face == boringFace ) {
      // already looked at this face during this run, so don't do it again
      return false;
    }
  }

  // otherwise, this is a new face, so it's a good one to look at
  return true;
}

RobotTimeStamp_t BehaviorObservingLookAtFaces::GetRecentFaceTime()
{

  const RobotTimeStamp_t lastImgTime = GetBEI().GetRobotInfo().GetLastImageTimeStamp();
  const RobotTimeStamp_t recentTime = lastImgTime > kMaxTimeSinceSeenFaceToLook_ms ?
                                      ( lastImgTime - kMaxTimeSinceSeenFaceToLook_ms ) :
                                      0;
  return recentTime;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorObservingLookAtFaces::SetState_internal(
  BehaviorObservingLookAtFaces::State state,
  const std::string& stateName)
{
  _dVars.persistent.state = state;
  SetDebugStateName(stateName);
}  


}
}

