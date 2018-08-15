/**
 * File: behaviorLookAtMe.cpp
 *
 * Author: ross
 * Created: 2018-06-22
 *
 * Description: Behavior for tracking a face
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/simpleFaceBehaviors/behaviorLookAtMe.h"

#include "engine/actions/animActions.h"
#include "engine/actions/trackFaceAction.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/faceWorld.h"
#include "util/cladHelpers/cladFromJSONHelpers.h"

namespace Anki {
namespace Vector {
  
namespace{
const char* const kPanToleranceKey = "panTolerance_deg";
const char* const kAnimGetInKey = "animGetIn";
const char* const kAnimLoopKey = "animLoop";
const char* const kAnimGetOutKey = "animGetOut";
const char* const kNumLoopsKey = "numLoops";
const char* const kPlayEmergencyGetOutKey = "playEmergencyGetOut";
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorLookAtMe::InstanceConfig::InstanceConfig()
{
  animGetIn = AnimationTrigger::Count;
  animLoop = AnimationTrigger::Count;
  animGetOut = AnimationTrigger::Count;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorLookAtMe::DynamicVariables::DynamicVariables()
{
  startedGetOut = false;
  trackFaceAction.reset();
  loopingAnimAction.reset();
  getInAnimAction.reset();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorLookAtMe::BehaviorLookAtMe(const Json::Value& config)
  : ISimpleFaceBehavior(config)
{
  _iConfig.panTolerance_deg = config.get(kPanToleranceKey, -1.0f).asFloat();
  
  JsonTools::GetCladEnumFromJSON(config, kAnimGetInKey, _iConfig.animGetIn, GetDebugLabel(), false);
  JsonTools::GetCladEnumFromJSON(config, kAnimLoopKey, _iConfig.animLoop, GetDebugLabel(), false);
  JsonTools::GetCladEnumFromJSON(config, kAnimGetOutKey, _iConfig.animGetOut, GetDebugLabel(), false);
  
  if( _iConfig.animLoop != AnimationTrigger::Count ) {
    _iConfig.numLoops = config.get(kNumLoopsKey, 0).asUInt();
  }
  
  if( _iConfig.animGetOut != AnimationTrigger::Count ) {
    _iConfig.playEmergencyGetOut = config.get( kPlayEmergencyGetOutKey, false ).asBool();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookAtMe::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.visionModesForActiveScope->insert( {VisionMode::DetectingFaces, EVisionUpdateFrequency::High} );
  modifiers.behaviorAlwaysDelegates = true;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookAtMe::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kPanToleranceKey,
    kAnimGetInKey,
    kAnimLoopKey,
    kAnimGetOutKey,
    kNumLoopsKey,
    kPlayEmergencyGetOutKey,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorLookAtMe::WantsToBeActivatedBehavior() const
{
  return GetTargetFace().IsValid();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookAtMe::OnBehaviorActivated()
{
  _dVars = DynamicVariables();
  
  const Vision::TrackedFace* facePtr = GetBEI().GetFaceWorld().GetFace( GetTargetFace() );
  if(facePtr != nullptr){
    
    auto* parallelAction = new CompoundActionParallel();
    
    Vision::FaceID_t faceID = facePtr->GetID();
    TrackFaceAction* trackAction = new TrackFaceAction( faceID );
    if( _iConfig.panTolerance_deg >= 0.0f ) {
      trackAction->SetPanTolerance( DEG_TO_RAD(_iConfig.panTolerance_deg) );
    }
    const bool onCharger = GetBEI().GetRobotInfo().IsOnChargerContacts();
    if( onCharger ) {
      trackAction->SetMode( ITrackAction::Mode::HeadOnly );
    }
    
    _dVars.trackFaceAction = parallelAction->AddAction( trackAction );
    
    // If getin/loop/getout animation actions are provided, then they will be cut short when the track action ends.
    // For example, if the track action ends in the middle of the looping animation, then at the end of the
    // current loop, it will play the getout. However, if the getin/loop/getout end before the track action,
    // the behavior continues until tracking is complete
    CompoundActionSequential* animActions = nullptr;
    if( (_iConfig.animGetIn != AnimationTrigger::Count)
        || (_iConfig.animLoop != AnimationTrigger::Count)
        || (_iConfig.animGetOut != AnimationTrigger::Count))
    {
      animActions = new CompoundActionSequential;
    }
    
    if( _iConfig.animGetIn != AnimationTrigger::Count ) {
      _dVars.getInAnimAction = animActions->AddAction( new TriggerLiftSafeAnimationAction{ _iConfig.animGetIn } );
    }
    if( _iConfig.animLoop != AnimationTrigger::Count ) {
      _dVars.loopingAnimAction = animActions->AddAction( new ReselectingLoopAnimationAction{ _iConfig.animLoop, _iConfig.numLoops } );
    }
    if( _iConfig.animGetOut != AnimationTrigger::Count ) {
      animActions->AddAction( new TriggerLiftSafeAnimationAction{ _iConfig.animGetOut } );
    }
    
    if( animActions != nullptr ) {
      parallelAction->AddAction( animActions );
    }
    
    DelegateIfInControl( parallelAction );
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookAtMe::BehaviorUpdate()
{
  if( !IsActivated() ) {
    return;
  }
  // if the track face actions ends, tell the looping animation to end if it is still running
  if( _dVars.trackFaceAction.expired() ) {
    // track action ended
    if( IsControlDelegated() && _dVars.getInAnimAction.expired() ) {
      // animation action is still running and the getin either didn't exist or finished
      auto loopingAnimation = _dVars.loopingAnimAction.lock();
      if( loopingAnimation ) {
        // get-in ended (or didnt exist) and looping animation hasn't started or is currently looping.
        // make it end after the current loop
        auto castAnim = std::dynamic_pointer_cast<ReselectingLoopAnimationAction>(loopingAnimation);
        if( castAnim != nullptr ) {
          // has no effect if already called
          castAnim->StopAfterNextLoop();
        }
      } else {
        // looping animation ended or didn't exist, so the getout must have started, if it exists.
        // this will mean the emergency getout is skipped.
        _dVars.startedGetOut = true;
      }
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookAtMe::OnBehaviorDeactivated()
{
   if( (_iConfig.animGetOut != AnimationTrigger::Count) && !_dVars.startedGetOut && _iConfig.playEmergencyGetOut ) {
     PlayEmergencyGetOut( _iConfig.animGetOut );
     _dVars.startedGetOut = true;
   }
}
  

}
}
