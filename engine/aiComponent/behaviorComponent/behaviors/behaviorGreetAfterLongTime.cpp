/**
 * File: BehaviorGreetAfterLongTime.cpp
 *
 * Author: Hamzah Khan
 * Created: 2018-06-25
 *
 * Description: This behavior causes victor to, upon seeing a face, react "loudly" if he hasn't seen the person for a certain amount of time.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/behaviorGreetAfterLongTime.h"

#include "engine/actions/animActions.h"
#include "engine/components/variableSnapshot/variableSnapshotComponent.h"
#include "engine/faceWorld.h"
#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"


#include "clad/types/variableSnapshotIds.h"
#include "clad/types/animationTrigger.h"

namespace Anki {
namespace Cozmo {
  
namespace {
const char* kCooldownKey = "cooldown_s";

// faceWorld constants
const bool kRecognizableFacesOnly = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorGreetAfterLongTime::InstanceConfig::InstanceConfig()
  : cooldownPeriod_s(0)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorGreetAfterLongTime::DynamicVariables::DynamicVariables()
  : lastSeenTimesPtr(std::make_shared<std::unordered_map<std::string, time_t>>()),
    lastFaceCheckTime_ms(0),
    shouldActivateBehavior(false)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorGreetAfterLongTime::BehaviorGreetAfterLongTime(const Json::Value& config)
 : ICozmoBehavior(config)
{
  // get cooldown period in seconds
  _iConfig.cooldownPeriod_s = JsonTools::ParseUInt32(config, kCooldownKey, "Could not parse cooldown value");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorGreetAfterLongTime::~BehaviorGreetAfterLongTime()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorGreetAfterLongTime::InitBehavior()
{
  auto& variableSnapshotComp = GetBEI().GetVariableSnapshotComponent();
  variableSnapshotComp.InitVariable<std::unordered_map<std::string, time_t>>(VariableSnapshotId::BehaviorGreetAfterLongTime_LastSeenTimes,
                                                                        _dVars.lastSeenTimesPtr);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorGreetAfterLongTime::WantsToBeActivatedBehavior() const
{
  return _dVars.shouldActivateBehavior;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorGreetAfterLongTime::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kCooldownKey
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorGreetAfterLongTime::OnBehaviorActivated() 
{
  _dVars.shouldActivateBehavior = false;
  PlayReactionAnimation();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorGreetAfterLongTime::BehaviorUpdate() 
{
  // get the walltime as a time_t
  time_t unixUtcTime_s;
  if( !ShouldBehaviorUpdate(unixUtcTime_s) ) {
    return;
  }

  const int kDontCheckRelativeAngles = 0;
  const int kDontCheckRangeOfAngles = 0;

  // get recognized faces seen since last face update
  auto faceIds = GetBEI().GetFaceWorld().GetFaceIDs(_dVars.lastFaceCheckTime_ms, 
                                                    kRecognizableFacesOnly,
                                                    kDontCheckRelativeAngles,
                                                    kDontCheckRangeOfAngles);

  for(const auto& faceId : faceIds) {
    const auto face = GetBEI().GetFaceWorld().GetFace(faceId);
    std::string name = face->GetName();

    // greeting should only happen if the face is recognized and named
    if(!name.empty()) {
      // if face has not been seen for long enough, react strongly
      auto faceLastSeenTimeIter = _dVars.lastSeenTimesPtr->find(name);

      // if the face is newly recognizable, add it to the map with a greeting cooldown starting now
      const bool isFaceNewlySeen = faceLastSeenTimeIter == _dVars.lastSeenTimesPtr->end();
      if(isFaceNewlySeen) {
        _dVars.lastSeenTimesPtr->emplace(name, unixUtcTime_s);
      }
      else {
        time_t faceLastSeenTime_s = faceLastSeenTimeIter->second;

        // flag behavior for activation if we see the face after long enough
        _dVars.shouldActivateBehavior = (faceLastSeenTime_s + _iConfig.cooldownPeriod_s <= unixUtcTime_s);

        // regardless of a reaction, reset the cooldown
       _dVars.lastSeenTimesPtr->at(name) = unixUtcTime_s;
      }

    // keeps track of the last time a face was seen to avoid rechecking old faces
    // necessary because vision may add faces into the past after completing processing
    // note: times stored in lastFaceCheckTime_ms use BaseStationTime and are not compatible with WallTime
    _dVars.lastFaceCheckTime_ms = std::max(_dVars.lastFaceCheckTime_ms, face->GetTimeStamp());
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorGreetAfterLongTime::ShouldBehaviorUpdate(time_t& unixUtcTime_s)
{
  std::tm utcTimeObj;
  auto* wt = WallTime::getInstance();
  const bool gotTime = wt->GetUTCTime(utcTimeObj);
  unixUtcTime_s = std::mktime(&utcTimeObj);

  // only proceed if behavior is not activated and walltime is properly synced
  return !IsActivated() && gotTime;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorGreetAfterLongTime::PlayReactionAnimation()
{
  // value for number of loops argument for playing anim once
  const int kPlayAnimOnce = 1;
  // value for whether starting the animation can interrupt
  const bool kCanAnimationInterrupt = true;


  TriggerAnimationAction* action = new TriggerAnimationAction(AnimationTrigger::GreetAfterLongTime,
                                                              kPlayAnimOnce,
                                                              kCanAnimationInterrupt);
  DelegateNow(action);
}

}
}
